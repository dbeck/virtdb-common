#ifdef RELEASE
#undef LOG_TRACE_IS_ENABLED
#define LOG_TRACE_IS_ENABLED false
#undef LOG_SCOPED_IS_ENABLED
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <engine/feeder.hh>
#include <logger.hh>
#include <functional>

namespace virtdb { namespace engine {

  static const uint16_t ST_FETCH_FIRST_    = 1;
  static const uint16_t ST_FIRST_TIMEOUT_  = 2;
  static const uint16_t ST_COMPLETE_       = 3;
  static const uint16_t ST_IN_PROGRESS_    = 4;
  static const uint16_t ST_FETCH_NEXT_     = 5;
  static const uint16_t ST_TIMED_OUT_      = 6;
  static const uint16_t ST_RESEND_         = 7;
  static const uint16_t ST_ERROR_          = 8;
  static const uint16_t ST_NO_MORE_DATA_   = 9;
  
  static const uint16_t EV_NEED_DATA_      = 1;
  static const uint16_t EV_TIME_OUT_       = 2;
  static const uint16_t EV_OK_             = 3;
  static const uint16_t EV_DONE_           = 4;

  feeder::feeder(collector::sptr cll)
  : collector_(cll),
    act_block_{-1},
    next_block_timeout_ms_{30000},
    state_machine_{std::to_string((uint64_t)this),
      [this](uint16_t seqno,
         const std::string & desc,
         const fsm::transition & trans,
         const fsm::state_machine & sm)
      {
        trace(seqno, desc, trans, sm);
      }
    },
    last_state_{ST_FETCH_FIRST_}
  {
    std::unique_ptr<char[]> buffer;
    for( size_t i=0; i<collector_->n_columns(); ++i)
    {
      // add empty readers
      readers_.push_back(collector::reader_sptr());
    }

    // state names
    state_machine_.state_name(ST_COMPLETE_,       "COMPLETE");
    state_machine_.state_name(ST_TIMED_OUT_,      "TIMED_OUT");
    state_machine_.state_name(ST_IN_PROGRESS_,    "IN PROGRESS");
    state_machine_.state_name(ST_FETCH_NEXT_,     "FETCH NEXT");
    state_machine_.state_name(ST_FETCH_FIRST_,    "FETCH FIRST");
    state_machine_.state_name(ST_RESEND_,         "RESEND");
    state_machine_.state_name(ST_FIRST_TIMEOUT_,  "FIRST TIME OUT");
    state_machine_.state_name(ST_ERROR_,          "ERROR");
    state_machine_.state_name(ST_NO_MORE_DATA_,   "NO MORE DATA");
    
    // event names
    state_machine_.event_name(EV_NEED_DATA_,   "NEED DATA");
    state_machine_.event_name(EV_TIME_OUT_,    "TIME OUT");
    state_machine_.event_name(EV_OK_,          "OK");
    state_machine_.event_name(EV_DONE_,        "DONE");
    
    using namespace virtdb::fsm;
    
    // ST_FETCH_FIRST_ + EV_NEED_DATA_
    {
      transition::sptr tr{new transition{ST_FETCH_FIRST_, EV_NEED_DATA_ , ST_IN_PROGRESS_ ,"Need first block" }};
      tr->on_timeout_state(ST_FIRST_TIMEOUT_);
      tr->on_error_state(ST_ERROR_);
      tr->default_state(ST_IN_PROGRESS_);
      
      timer::sptr tm(new fsm::timer{[](uint16_t seqno,
                                       transition & trans,
                                       state_machine & sm,
                                       const timer::clock_type::time_point & started_at)
        {
          auto now = timer::clock_type::now();
          bool ret = now < started_at+std::chrono::seconds(30);
          if( !ret )
          {
            sm.enqueue_unique(EV_TIME_OUT_);
          }
          return ret;
        }, "FIRST DATA TIMER" });

      loop::sptr lp(new loop{[this](uint16_t seqno,
                                    transition & trans,
                                    state_machine & sm,
                                    uint64_t iteration) {
        
        auto n_columns    = collector_->n_columns();
        auto got_columns  = collector_->get(0,
                                            ((iteration*3)+1)*50,
                                            1000,
                                            readers_);
        
        return (got_columns != n_columns);
        
      }, "FIRST DATA LOOP"});
      
      action::sptr act(new action{[this](uint16_t seqno,
                                         transition & trans,
                                         state_machine & sm){

        auto last         = collector_->last_block_id();
        auto n_columns    = collector_->n_columns();
        auto got_columns  = collector_->get(0,
                                            1,
                                            10,
                                            readers_);
        if( n_columns == got_columns )
        {
          if( last == 0 )
          {
            // this is the last block
            trans.default_state(ST_COMPLETE_);
            sm.enqueue_if_empty(EV_DONE_);
          }
          else
          {
            // default state is ST_IN_PROGRESS_
            sm.enqueue_if_empty(EV_OK_);
          }
        }
      },"EVALUATE FIRST FETCH"});
      
      tr->set_timer(1, tm);
      tr->set_loop(2, lp);
      tr->set_action(3, act);
      state_machine_.add_transition(tr);
      // can go to:
      // ST_FIRST_TIMEOUT_ + EV_TIME_OUT_
      // ST_IN_PROGRESS_ + EV_OK_
      // ST_COMPLETE_ + EV_DONE_
    }
    
    // ST_FETCH_NEXT_ + NEED DATA
    {
      transition::sptr tr{new transition{ST_FETCH_NEXT_, EV_NEED_DATA_ , ST_IN_PROGRESS_ ,"Need data block" }};
      tr->on_timeout_state(ST_RESEND_);
      tr->on_error_state(ST_ERROR_);
      tr->default_state(ST_IN_PROGRESS_);
      
      timer::sptr tm(new fsm::timer{[](uint16_t seqno,
                                       transition & trans,
                                       state_machine & sm,
                                       const timer::clock_type::time_point & started_at)
        {
          auto now = timer::clock_type::now();
          bool ret = now < started_at+std::chrono::seconds(30);
          if( !ret )
          {
            // default timeout state is: ST_RESEND_
            sm.enqueue_unique(EV_TIME_OUT_);
          }
          return ret;
        }, "DATA TIMER" });
      
      loop::sptr lp(new loop{[this](uint16_t seqno,
                                    transition & trans,
                                    state_machine & sm,
                                    uint64_t iteration) {
        
        auto n_columns    = collector_->n_columns();
        auto got_columns  = collector_->get(act_block_+1,
                                            ((iteration*3)+1)*50,
                                            1000,
                                            readers_);
        
        return (got_columns != n_columns);
        
      }, "DATA LOOP"});
      
      action::sptr act(new action{[this](uint16_t seqno,
                                         transition & trans,
                                         state_machine & sm){
        
        auto last         = collector_->last_block_id();
        auto n_columns    = collector_->n_columns();
        auto got_columns  = collector_->get(act_block_+1,
                                            1,
                                            10,
                                            readers_);
        if( n_columns == got_columns )
        {
          if( last == (act_block_+1) )
          {
            // this is the last block
            trans.default_state(ST_COMPLETE_);
            sm.enqueue_if_empty(EV_DONE_);
          }
          else
          {
            // default state is ST_IN_PROGRESS_
            sm.enqueue_if_empty(EV_OK_);
          }
        }
      }, "EVALUATE FETCH"});
      
      tr->set_timer(1, tm);
      tr->set_loop(2, lp);
      tr->set_action(3, act);
      state_machine_.add_transition(tr);
      // can go to:
      // ST_RESEND_ + EV_TIME_OUT_
      // ST_IN_PROGRESS_ + EV_OK_
      // ST_COMPLETE_ + EV_DONE_
    }
    
    // ST_RESEND_ + EV_TIME_OUT_
    {
      transition::sptr tr{new transition{ST_RESEND_, EV_TIME_OUT_ , ST_IN_PROGRESS_ , "Asking provider to resend" }};
      tr->on_timeout_state(ST_TIMED_OUT_);
      tr->on_error_state(ST_ERROR_);
      tr->default_state(ST_IN_PROGRESS_);
      
      timer::sptr tm(new fsm::timer{[](uint16_t seqno,
                                       transition & trans,
                                       state_machine & sm,
                                       const timer::clock_type::time_point & started_at)
        {
          auto now = timer::clock_type::now();
          return now < started_at+std::chrono::seconds(300);
        }, "RESEND TIMER" });
      
      loop::sptr lp(new loop{[this](uint16_t seqno,
                                    transition & trans,
                                    state_machine & sm,
                                    uint64_t iteration) {
        
        auto n_columns    = collector_->n_columns();
        auto got_columns  = collector_->get(act_block_+1,
                                            (iteration+1)*1000,
                                            1000,
                                            readers_);
        
        if( n_columns == got_columns )
        {
          // no more resend needed
          return false;
        }
        else
        {
          // do a resend if still not enough columns has arrived
          std::vector<size_t> cols;
          for( size_t i=0; i<readers_.size(); ++i )
          {
            if( readers_[i].get() == nullptr )
            {
              cols.push_back(i);
            }
          }
          
          if( cols.size() )
          {
            LOG_INFO("sending resend request" <<  V_(act_block_+1) << V_(cols.size()) << V_(iteration) << V_(sm.description()));
            collector_->resend(act_block_+1, cols);
          }
          return true;
        }
      }, "RESEND LOOP"});
      
      action::sptr act(new action{[this](uint16_t seqno,
                                         transition & trans,
                                         state_machine & sm){
        
        auto last         = collector_->last_block_id();
        auto n_columns    = collector_->n_columns();
        auto got_columns  = collector_->get(act_block_+1,
                                            1,
                                            10,
                                            readers_);
        if( n_columns == got_columns )
        {
          if( last == (act_block_+1) )
          {
            // this is the last block
            trans.default_state(ST_COMPLETE_);
            sm.enqueue_if_empty(EV_DONE_);
          }
          else
          {
            // default state is ST_IN_PROGRESS_
            sm.enqueue_if_empty(EV_OK_);
          }
        }
        
      }, "EVALUATE RESEND"});
      
      tr->set_timer(1, tm);
      tr->set_loop(2, lp);
      tr->set_action(3, act);
      state_machine_.add_transition(tr);
      // can go to:
      // ST_TIMED_OUT_ (terminate)
      // ST_IN_PROGRESS_ + EV_OK_
      // ST_COMPLETE_ + EV_DONE_
    }
    
    // ST_FIRST_TIMEOUT_ + EV_TIME_OUT_
    {
      transition::sptr tr{new transition{ST_FIRST_TIMEOUT_, EV_TIME_OUT_ , ST_TIMED_OUT_ , "Need first block" }};
      
      action::sptr act(new action{[this](uint16_t seqno,
                                         transition & trans,
                                         state_machine & sm){
        
        auto got_columns  = collector_->get(0,
                                            1,
                                            10,
                                            readers_);
        if( got_columns == 0 )
        {
          // nothing has arrived, default is: ST_TIMED_OUT_
        }
        else
        {
          // we received some data, let ST_RESEND_ decide what to do
          trans.default_state(ST_RESEND_);
          sm.enqueue_if_empty(EV_TIME_OUT_);
        }
      },"EVALUATE FIRST FETCH"});
      
      tr->set_action(1, act);
      state_machine_.add_transition(tr);
      // can go to:
      // ST_TIMED_OUT_ (terminate)
      // ST_RESEND_ + EV_TIME_OUT_
    }
    
    // ST_IN_PROGRESS_ + EV_NEED_DATA_
    {
      transition::sptr tr{new transition{ST_IN_PROGRESS_, EV_NEED_DATA_ , ST_FETCH_NEXT_ , "Need next block" }};
      action::sptr act(new action{[this](uint16_t seqno,
                                         transition & trans,
                                         state_machine & sm) {
        sm.enqueue_if_empty(EV_NEED_DATA_);
      },"FIRE FETCH NEXT"});
      
      tr->set_action(1, act);
      state_machine_.add_transition(tr);
      // goes to:
      // ST_FETCH_NEXT_ + EV_NEED_DATA_
    }
    
    // ST_TIMED_OUT_ + EV_NEED_DATA_
    {
      transition::sptr tr{new transition{ST_TIMED_OUT_, EV_NEED_DATA_ , ST_TIMED_OUT_ , "Timed out" }};
      state_machine_.add_transition(tr);
      // terminate at: ST_TIMED_OUT_
    }
    
    // ST_COMPLETE_ + EV_DONE_
    {
      transition::sptr tr{new transition{ST_COMPLETE_, EV_DONE_ , ST_COMPLETE_ , "Last block arrived" }};
      action::sptr act(new action{[this](uint16_t seqno,
                                         transition & trans,
                                         state_machine & sm){ ++act_block_; },"STEP ACT BLOCK"});
      tr->set_action(1, act);
      state_machine_.add_transition(tr);
      // terminate at: ST_COMPLETE_
    }
    
    // ST_IN_PROGRESS_ + EV_OK_
    {
      transition::sptr tr{new transition{ST_IN_PROGRESS_, EV_OK_ , ST_IN_PROGRESS_ , "All columns arrived" }};
      action::sptr act(new action{[this](uint16_t seqno,
                                         transition & trans,
                                         state_machine & sm){ 
        ++act_block_; 
      },"STEP ACT BLOCK"});
      action::sptr sched(new action{[this](uint16_t seqno,
                                          transition & trans,
                                          state_machine & sm){ 
        collector_->background_process(act_block_+1);
      },"SCHEDULE BACKGROUND DECOMPRESS"});
      action::sptr erase(new action{[this](uint16_t seqno,
                                           transition & trans,
                                           state_machine & sm){ 
        if( act_block_ > 1 )
          collector_->erase(act_block_-2);
      },"DROP OLD DATA"});

      tr->set_action(1, act);
      tr->set_action(2, sched);
      tr->set_action(3, erase);
      state_machine_.add_transition(tr);
      // terminate at: ST_IN_PROGRESS_
    }
    
    // ST_COMPLETE_ + EV_NEED_DATA_
    {
      transition::sptr tr{new transition{ST_COMPLETE_, EV_NEED_DATA_ , ST_NO_MORE_DATA_ , "No more data" }};
      state_machine_.add_transition(tr);
      // terminate at: ST_NO_MORE_DATA_
    }
    
    // ST_NO_MORE_DATA_ + EV_NEED_DATA_
    {
      transition::sptr tr{new transition{ST_NO_MORE_DATA_, EV_NEED_DATA_ , ST_NO_MORE_DATA_ , "No more data" }};
      state_machine_.add_transition(tr);
      // terminate at: ST_NO_MORE_DATA_
    }
  }
  
  void
  feeder::trace(uint16_t seqno,
                const std::string & desc,
                const fsm::transition & t,
                const fsm::state_machine & sm)
  {
    if( LOG_TRACE_IS_ENABLED )
    {
      std::string fsm   = sm.description();
      std::string state = sm.state_name(t.state());
      std::string event = sm.event_name(t.event());
      std::string trans = t.description();
      
      LOG_TRACE(V_(fsm) << V_(trans) <<
                V_((int)t.state()) << V_(state) <<
                V_((int)t.event()) << V_(event) <<
                V_((int)seqno) << V_(desc));
    }
  }
  
  feeder::~feeder() {}
  
  const util::relative_time &
  feeder::timer() const
  {
    return timer_;
  }
  
  bool
  feeder::fetch_next()
  {
    auto prev_state = last_state_;
    state_machine_.enqueue(EV_NEED_DATA_);
    last_state_ = state_machine_.run(prev_state);
    
    switch( last_state_ )
    {
      case ST_COMPLETE_:
      case ST_IN_PROGRESS_:
        return true;
        
      case ST_TIMED_OUT_:
        LOG_ERROR("Query timed out");
        return false;
        
      default:
        LOG_ERROR("Unexpected state" <<
                  V_(state_machine_.state_name(last_state_)) <<
                  V_(state_machine_.state_name(prev_state)) );
        return false;
    };
  }
    
}}
