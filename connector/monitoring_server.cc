#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <connector/monitoring_server.hh>
#include <logger.hh>
#include <time.h>
#include <tuple>
#include <set>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  monitoring_server::on_reply_fwd(const rep_base_type::req_item & req,
                                         rep_base_type::rep_item_sptr rep)
  {
    on_reply(req, rep);
  }
  
  void
  monitoring_server::on_request_fwd(const rep_base_type::req_item & req,
                                           rep_base_type::send_rep_handler handler)
  {
    on_request(req, handler);
  }
  
  void
  monitoring_server::on_reply(const rep_base_type::req_item & req,
                                     rep_base_type::rep_item_sptr rep)
  {
  }
  
  std::pair<bool, uint64_t>
  monitoring_server::locked_get_state(const std::string & name) const
  {
    std::pair<bool, uint64_t> ret{true, 0};
    // check set_states
    {
      auto it = set_state_map_.find(name);
      if( it != set_state_map_.end() )
      {
        for( auto const & i : it->second )
        {
          uint64_t time_stamp = i.first;
          ret.second = time_stamp;
          auto const & obj = i.second;
          if( obj->type() != pb::MonitoringRequest::SetState::CLEAR )
          {
            ret.first = false;
            return ret;
          }
          break;
        }
      }
    }
    // check component errors
    {
      auto it = component_error_map_.find(name);
      if( it != component_error_map_.end() )
      {
        std::set<std::string> compos;
        
        for( auto const & i : it->second )
        {
          uint64_t time_stamp = i.first;
          if( ret.second < time_stamp ) ret.second = time_stamp;
          auto const & obj = i.second;
          if( compos.count(obj->reportedby()) == 0 )
          {
            if( obj->type() != pb::MonitoringRequest::ComponentError::CLEAR )
            {
              ret.first = false;
              return ret;
            }
            else
            {
              compos.insert(obj->reportedby());
            }
          }
        }
      }
    }
    if( !ret.second ) ret.second = ::time(NULL);
    return ret;
  }
  
  void
  monitoring_server::locked_add_events(pb::MonitoringReply::Status & s,
                                       const std::string & name) const
  {
    typedef std::pair<uint64_t, uint64_t> msg_id;

    std::map<msg_id, pb::MonitoringReply::Event> events;
    
    auto add_event = [&events](pb::MonitoringReply::Event & e)
    {
      msg_id id{e.epoch(),0};
      for( auto it=events.find(id); it != events.end(); ++it )
      {
        if( it->first.first == id.first )
        {
          if( it->first.second >= id.second )
            id.second = (it->first.second+1);
        }
        else
        {
          break;
        }
      }
      events.insert(std::make_pair(id, e));
    };
    
    // add report stats
    {
      auto it = report_stats_map_.find(name);
      if( it != report_stats_map_.end() )
      {
        for( auto const & i : it->second )
        {
          uint64_t time_stamp = i.first;
          auto const & obj = i.second;
          pb::MonitoringReply::Event ev;
          ev.set_epoch(time_stamp);
          auto req = ev.mutable_request();
          req->set_type(pb::MonitoringRequest::REPORT_STATS);
          auto inner = req->mutable_repstats();
          inner->MergeFrom(*obj);
          add_event(ev);
        }
      }
    }
    // add set_states
    {
      auto it = set_state_map_.find(name);
      if( it != set_state_map_.end() )
      {
        for( auto const & i : it->second )
        {
          uint64_t time_stamp = i.first;
          auto const & obj = i.second;
          pb::MonitoringReply::Event ev;
          ev.set_epoch(time_stamp);
          auto req = ev.mutable_request();
          req->set_type(pb::MonitoringRequest::SET_STATE);
          auto inner = req->mutable_setst();
          inner->MergeFrom(*obj);
          add_event(ev);
        }
      }
    }
    // add component errors
    {
      auto it = component_error_map_.find(name);
      if( it != component_error_map_.end() )
      {
        for( auto const & i : it->second )
        {
          uint64_t time_stamp = i.first;
          auto const & obj = i.second;
          pb::MonitoringReply::Event ev;
          ev.set_epoch(time_stamp);
          auto req = ev.mutable_request();
          req->set_type(pb::MonitoringRequest::COMPONENT_ERROR);
          auto inner = req->mutable_comperr();
          inner->MergeFrom(*obj);
          add_event(ev);
        }
      }
    }
    // add request errors
    {
      auto it = request_error_map_.find(name);
      if( it != request_error_map_.end() )
      {
        for( auto const & i : it->second )
        {
          uint64_t time_stamp = i.first;
          auto const & obj = i.second;
          pb::MonitoringReply::Event ev;
          ev.set_epoch(time_stamp);
          auto req = ev.mutable_request();
          req->set_type(pb::MonitoringRequest::REQUEST_ERROR);
          auto inner = req->mutable_reqerr();
          inner->MergeFrom(*obj);
          add_event(ev);
        }
      }
    }
    
    for( auto it = events.begin(); it != events.end(); ++it )
    {
      auto tmp_ev = s.add_events();
      tmp_ev->MergeFrom(it->second);
    }
  }
  
  void
  monitoring_server::on_request(const rep_base_type::req_item & req,
                                       rep_base_type::send_rep_handler handler)
  {
    rep_base_type::rep_item_sptr rep{new rep_base_type::rep_item};
    
    try
    {
      switch( req.type() )
      {
        case pb::MonitoringRequest::REPORT_STATS:
        {
          if( !req.has_repstats() )       { THROW_("missing ReportStats from MonitoringRequest"); }
          auto const & req_msg = req.repstats();
          if( req_msg.stats_size() == 0 ) { THROW_("empty Stats reported in ReportStats message"); }
          if( !req_msg.has_name() )       { THROW_("empty Name in ReportStats message"); }
          
          {
            lock l(mtx_);
            auto it = report_stats_map_.find(req_msg.name());
            if( it == report_stats_map_.end() )
            {
              auto res = report_stats_map_.insert(std::make_pair(req_msg.name(),report_stats_list()));
              it = res.first;
            }
            report_stats_sptr tmp{new pb::MonitoringRequest::ReportStats{req_msg}};
            {
              it->second.push_front(std::make_pair(::time(NULL), tmp));
              if( it->second.size() > 5 )
              {
                // only keep the last 5 entries
                auto erase_from = it->second.begin();
                for( int i=0; i<5; ++i ) ++erase_from;
                it->second.erase( erase_from, it->second.end() );
              }
            }
            components_.insert(req_msg.name());
          }
          
          rep->set_type(pb::MonitoringReply::REPORT_STATS);
          break;
        }
        case pb::MonitoringRequest::SET_STATE:
        {
          if( !req.has_setst() )          { THROW_("missing SetState from MonitoringRequest"); }
          auto const & req_msg = req.setst();
          if( !req_msg.has_name() )       { THROW_("empty Name in SetState message"); }
          if( !req_msg.has_type() )       { THROW_("empty Type in SetState message"); }
          
          {
            lock l(mtx_);
            auto it = set_state_map_.find(req_msg.name());
            if( it == set_state_map_.end() )
            {
              auto res = set_state_map_.insert(std::make_pair(req_msg.name(),set_state_list()));
              it = res.first;
            }
            set_state_sptr tmp{new pb::MonitoringRequest::SetState{req_msg}};
            {
              it->second.push_front(std::make_pair(::time(NULL), tmp));
              typedef std::set<pb::MonitoringRequest::SetState::Types> comp_state_set;
              comp_state_set comp_states;
              
              int len = it->second.size();
              for( auto ei=it->second.begin(); ei!=it->second.end() && len > 0; ++ei )
              {
                if( comp_states.count(req_msg.type()) > 0 )
                {
                  ei = it->second.erase(ei);
                }
                else
                {
                  comp_states.insert(req_msg.type());
                }
                --len;
              }
            }
            components_.insert(req_msg.name());
          }
          
          rep->set_type(pb::MonitoringReply::SET_STATE);
          break;
        }
        case pb::MonitoringRequest::COMPONENT_ERROR:
        {
          if( !req.has_comperr() )             { THROW_("missing ComponentError from MonitoringRequest"); }
          auto const & req_msg = req.comperr();
          if( !req_msg.has_reportedby() )      { THROW_("empty ReportedBy in ComponentError message"); }
          if( !req_msg.has_impactedpeer() )    { THROW_("empty ImpactedPeer in ComponentError message"); }
          if( !req_msg.has_type() )            { THROW_("empty Type in ComponentError message"); }

          {
            lock l(mtx_);
            auto it = component_error_map_.find(req_msg.impactedpeer());
            if( it == component_error_map_.end() )
            {
              auto res = component_error_map_.insert(std::make_pair(req_msg.impactedpeer(),component_error_list()));
              it = res.first;
            }
            component_error_sptr tmp{new pb::MonitoringRequest::ComponentError{req_msg}};
            
            {
              it->second.push_front(std::make_pair(::time(NULL), tmp));
              typedef std::pair<std::string, pb::MonitoringRequest::ComponentError::Types> reported_component_type;
              typedef std::set<reported_component_type> error_set;
              error_set  errors;
              
              int len = it->second.size();
              for( auto ei=it->second.begin(); ei!=it->second.end() && len > 0; ++ei )
              {
                reported_component_type typ = std::make_pair(ei->second->reportedby(), ei->second->type());
                if( errors.count(typ) > 0 )
                {
                  if( ei->second->reportedby() == req_msg.reportedby() )
                  {
                    LOG_TRACE("removing: " << V_(ei->first) << M_(*ei->second) << "after inserted" << M_(req_msg) << V_(len) << V_(errors.size()));
                    ei = it->second.erase(ei);
                  }
                }
                else
                {
                  errors.insert(typ);
                }
                --len;
              }
            }
            components_.insert(req_msg.reportedby());
            components_.insert(req_msg.impactedpeer());
          }
 
          rep->set_type(pb::MonitoringReply::COMPONENT_ERROR);
          break;
        }
        case pb::MonitoringRequest::REQUEST_ERROR:
        {
          if( !req.has_reqerr() )             { THROW_("missing RequestError from MonitoringRequest"); }
          auto const & req_msg = req.reqerr();
          
          if( !req_msg.has_reportedby() )      { THROW_("empty ReportedBy in RequestError message"); }
          if( !req_msg.has_impactedpeer() )    { THROW_("empty ImpactedPeer in RequestError message"); }
          if( !req_msg.has_type() )            { THROW_("empty Type in RequestError message"); }
          
          {
            lock l(mtx_);
            auto it = request_error_map_.find(req_msg.impactedpeer());
            if( it == request_error_map_.end() )
            {
              auto res = request_error_map_.insert(std::make_pair(req_msg.impactedpeer(),request_error_list()));
              it = res.first;
            }
            request_error_sptr tmp{new pb::MonitoringRequest::RequestError{req_msg}};
            {
              it->second.push_front(std::make_pair(::time(NULL), tmp));
              if( it->second.size() > 5 )
              {
                // only keep the last 5 entries
                auto erase_from = it->second.begin();
                for( int i=0; i<5; ++i ) ++erase_from;
                it->second.erase( erase_from, it->second.end() );
              }
            }
            components_.insert(req_msg.reportedby());
            components_.insert(req_msg.impactedpeer());
          }
          rep->set_type(pb::MonitoringReply::REQUEST_ERROR);
          break;
        }
        case pb::MonitoringRequest::GET_STATES:
        {
          std::string name;
          bool has_name = false;
          if( req.has_getsts() && req.getsts().has_name() )
          {
            has_name = true;
            name = req.getsts().name();
          }
          
          auto states = rep->mutable_states();
          {
            lock l(mtx_);
            for( auto const & i : components_ )
            {
              // filter by name if given
              if( has_name && i != name )
                continue;
              
              auto status = states->add_states();
              status->set_name(i);
              auto is_ok = locked_get_state(i);
              status->set_ok(is_ok.first);
              status->set_updatedepoch(is_ok.second);
              locked_add_events(*status, i);
              LOG_TRACE("reporintg state: " << V_(i) << V_(is_ok.first) << V_(is_ok.second));
            }
          }
          rep->set_type(pb::MonitoringReply::GET_STATES);
          break;
        }
        default:
        {
          THROW_("Unkown request type");
          break;
        }
      };
    }
    catch (const std::exception & e)
    {
      rep->Clear();
      rep->set_type(pb::MonitoringReply::ERROR_MSG);
      auto * err = rep->mutable_err();
      err->set_msg(e.what());
      LOG_ERROR(E_(e));
    }
    
    // send a single reply back
    bool has_more = false;
    handler(rep, has_more);
  }
  
  monitoring_server::monitoring_server(server_context::sptr ctx,
                                       config_client & cfg_client)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&monitoring_server::on_request_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&monitoring_server::on_reply_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::MONITORING)
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ctx->service_name());
      ep_data.set_svctype(pb::ServiceType::MONITORING);
      ep_data.set_cmd(pb::EndpointData::ADD);
      ep_data.set_validforms(DEFAULT_ENDPOINT_EXPIRY_MS);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
      ctx->add_endpoint(ep_data);
    }
  }
  
  monitoring_server::~monitoring_server()
  {
  }  
}}
