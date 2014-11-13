#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <random>

namespace virtdb { namespace fault {

  class injector final
  {
    struct rule
    {
      std::string  name_;
      uint64_t     lead_period_ms_;       // no errors for this many initial ms
      uint64_t     min_period_ms_;        // if an error happened, wait at leat this
                                          // amount, before the next error
      double       chance_for_error_;     // when time allows, this is the chance
                                          // for a new error
      
      rule(const std::string & name,
           uint64_t lead_period,
           uint64_t min_period,
           double chance);
      
      typedef std::shared_ptr<rule> sptr;
    };

    struct fault_inst
    {
      std::string  name_;
      uint64_t     no_faults_till_;
      size_t       fault_count_;
      size_t       check_count_;
      
      fault_inst(const std::string & name,
                 uint64_t no_faults_till);
      
      typedef std::shared_ptr<fault_inst> sptr;
    };
    
    std::random_device                                 rnd_dev_;
    std::mt19937                                       rnd_generator_;
    std::uniform_real_distribution<double>             rnd_distribution_;
    uint64_t                                           started_at_;
    rule                                               default_rule_;
    std::unordered_map<std::string, rule::sptr>        rules_;
    std::unordered_map<std::string, fault_inst::sptr>  instances_;
    std::mutex                                         mtx_;
    
    rule & get_rule(const std::string & name);
    fault_inst & get_instance(const std::string & name,
                              const rule & r);
    
  public:
    injector(uint64_t default_lead_period_ms=5000,
             uint64_t default_min_period_ms=2000,
             double default_error_chance=0.1);
    
    ~injector();
    
    // support for global injector singleton
    static injector & instance();
    
    bool inject(const std::string & rule_name,      // selects the rule
                const std::string & instance_name); // check the fault
    
    void set_rule(const std::string & name,
                  uint64_t lead_period_ms,
                  uint64_t min_period_ms,
                  double error_chance);
    
  private:
    injector(const injector &) = delete;
    injector & operator=(const injector &) = delete;
  };
}}
