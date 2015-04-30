#pragma once

#include <zmq.hpp>
#include <string>
#include <set>
#include <future>
#include <mutex>
#include <condition_variable>

namespace virtdb { namespace util {
  
  std::pair<std::string, unsigned short> parse_zmq_tcp_endpoint(const std::string & ep);
  
  class zmq_socket_wrapper final
  {
  public:
    typedef std::set<std::string>   endpoint_set;
    typedef std::set<std::string>   host_set;
    
    struct endpoint_info
    {
      std::string endpoint_;
      std::string host_;
      int         port_;
      
      endpoint_info() : port_(-1) {}
    };
    
  private:
    typedef std::mutex              mtx;
    typedef std::unique_lock<mtx>   lock;

    zmq::socket_t                   socket_;
    endpoint_set                    endpoints_;
    int                             type_;
    bool                            stop_;
    bool                            closed_;
    size_t                          n_waiting_;
    bool                            valid_;
    std::condition_variable         cv_;
    mutable mtx                     mtx_;
    
    void set_valid();
    void set_invalid();
    
  public:
    zmq_socket_wrapper(zmq::context_t &ctx, int type);
    ~zmq_socket_wrapper();
    
    void close();
    
    zmq::socket_t & get();
    endpoint_info bind(const char *addr);
    void batch_tcp_bind(const host_set & hosts);
    bool batch_ep_rebind(const endpoint_set & eps,
                         bool same_host_once=false);
    void connect(const char * addr);
    void reconnect(const char * addr);
    void disconnect_all();
    bool valid() const;
    const endpoint_set & endpoints() const;
    
    size_t send(const void *buf_, size_t len_, int flags_ = 0);
    bool send(zmq::message_t &msg_, int flags_ = 0);
    
    template <typename ITER>
    bool connected_to(const ITER & begin, const ITER & end) const
    {
      ITER it{begin};
      for( ; it != end ; ++it )
      {
        if( endpoints_.count(*it) > 0 )
          return true;
      }
      return false;
    }
    
    bool wait_valid(unsigned long ms);
    void wait_valid();
    
    bool poll_in(unsigned long ms,
                 unsigned long check_interval_ms=100);

    static void valid_subscription(const char * sub_data,
                                   size_t sub_len,
                                   std::string & result);

    static void valid_subscription(const zmq::message_t & msg,
                                   std::string & result);
    
  private:
    zmq_socket_wrapper() = delete;
    zmq_socket_wrapper(const zmq_socket_wrapper &) = delete;
    zmq_socket_wrapper& operator=(const zmq_socket_wrapper &) = delete;
  };
  
}}
