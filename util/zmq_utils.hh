#pragma once

#include <zmq.hpp>
#include <string>
#include <set>

namespace virtdb { namespace util {
  
  std::pair<std::string, unsigned short> parse_zmq_tcp_endpoint(const std::string & ep);
  
  class zmq_socket_wrapper final
  {
  public:
    typedef std::set<std::string> endpoint_set;
    typedef std::set<std::string> host_set;
    
  private:
    zmq::socket_t     socket_;
    endpoint_set      endpoints_;
    
  public:
    zmq_socket_wrapper(zmq::context_t &ctx, int type);
    ~zmq_socket_wrapper();
    
    zmq::socket_t & get();
    void bind (const char *addr);
    void batch_tcp_bind(const host_set & hosts);
    void connect (const char * addr);
    const endpoint_set & endpoints() const;
    
  private:
    zmq_socket_wrapper() = delete;
    zmq_socket_wrapper(const zmq_socket_wrapper &) = delete;
    zmq_socket_wrapper& operator=(const zmq_socket_wrapper &) = delete;
  };
  
}}
