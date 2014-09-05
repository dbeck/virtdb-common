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
    
  private:
    typedef std::mutex              mtx;
    typedef std::unique_lock<mtx>   lock;

    zmq::socket_t                   socket_;
    endpoint_set                    endpoints_;
    int                             type_;
    bool                            stop_;
    size_t                          n_waiting_;
    bool                            valid_;
    std::condition_variable         cv_;
    mutable mtx                     mtx_;
    
    void set_valid();
    void set_invalid();
    
  public:
    zmq_socket_wrapper(zmq::context_t &ctx, int type);
    ~zmq_socket_wrapper();
    
    zmq::socket_t & get();
    void bind(const char *addr);
    void batch_tcp_bind(const host_set & hosts);
    void connect(const char * addr);
    void reconnect(const char * addr);
    void disconnect_all();
    bool valid() const;
    const endpoint_set & endpoints() const;
    
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
    
    bool poll_in(unsigned long ms);
    
  private:
    zmq_socket_wrapper() = delete;
    zmq_socket_wrapper(const zmq_socket_wrapper &) = delete;
    zmq_socket_wrapper& operator=(const zmq_socket_wrapper &) = delete;
  };
  
}}
