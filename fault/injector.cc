#include "injector.hh"
#include <util/relative_time.hh>


namespace virtdb { namespace fault {
  
  injector::rule::rule(const std::string & name,
                       uint64_t lead_period,
                       uint64_t min_period,
                       double probability)
  : name_{name},
    lead_period_ms_{lead_period},
    min_period_ms_{min_period},
    probability_{probability}
  {
  }

  injector::fault_inst::fault_inst(const std::string & name,
                                   uint64_t no_faults_till)
  : name_{name},
    no_faults_till_{no_faults_till},
    fault_count_{0},
    check_count_{0}
  {    
  }
  
  injector::injector(uint64_t default_lead_period_ms,
                     uint64_t default_min_period_ms,
                     double default_probability)
  : rnd_generator_{rnd_dev_()},
    rnd_distribution_{0.0, 1.0},
    started_at_{util::relative_time::instance().get_msec()},
    default_rule_{std::string(), default_lead_period_ms,
                  default_min_period_ms, default_probability}
  {
  }

  injector::rule &
  injector::get_rule(const std::string & name)
  {
    auto it = rules_.find(name);
    if( it == rules_.end() )
      return default_rule_;
    else
      return *(it->second);
  }
  
  injector::fault_inst &
  injector::get_instance(const std::string & name,
                         const rule & r)
  {
    auto it = instances_.find(name);
    if( it == instances_.end() )
    {
      uint64_t now = util::relative_time::instance().get_msec();
      uint64_t no_faults_till = now + r.lead_period_ms_;
      fault_inst::sptr tmp_inst{new fault_inst{name, no_faults_till}};
      instances_.insert(std::make_pair(name,tmp_inst));
      return *tmp_inst;
    }
    else
      return *(it->second);
  }
  
  injector::~injector()
  {
  }
  
  injector &
  injector::instance()
  {
    static injector instance_;
    return instance_;
  }
  
  bool
  injector::inject(const std::string & rule_name,
                   const std::string & instance_name)
  {
    std::unique_lock<std::mutex> l(mtx_);
    rule & r = get_rule(rule_name);
    fault_inst & i = get_instance(instance_name, r);
    uint64_t now = util::relative_time::instance().get_msec();
    
    // we are in a grace period, we don't inject faults
    if( i.no_faults_till_ > now )
      return false;
    
    // generate a random value between 0.0 and 1.0
    double rnd_val = rnd_distribution_(rnd_generator_);

    // we record the number of checks
    ++(i.check_count_);
    
    // make sure fault frequency doesn't exceed min_period_ms
    i.no_faults_till_ = now + r.min_period_ms_;
    
    if( rnd_val < r.probability_ )
    {
      ++(i.fault_count_);
      return true;
    }
    else
    {
      return false;
    }
  }
  
  void
  injector::set_rule(const std::string & name,
                     uint64_t lead_period_ms,
                     uint64_t min_period_ms,
                     double probability)
  {
    std::unique_lock<std::mutex> l(mtx_);
    rule * r = nullptr;
    if( name.empty() )
    {
      r = &default_rule_;
    }
    else
    {
      auto it = rules_.find(name);
      if( it == rules_.end() )
      {
        rule::sptr tmp_rule{new rule{name, lead_period_ms,
                                     min_period_ms, probability}};
        rules_.insert(std::make_pair(name, tmp_rule));
        r = tmp_rule.get();
      }
      else
      {
        r = it->second.get();
      }
    }
    r->lead_period_ms_  = lead_period_ms;
    r->min_period_ms_   = min_period_ms;
    r->probability_     = probability;
  }
  
}}
