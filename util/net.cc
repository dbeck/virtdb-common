#include "net.hh"
#include "exception.hh"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

namespace virtdb { namespace util {

  namespace
  {
    struct auto_close_fd
    {
      int value_;
      auto_close_fd() : value_(-1) {}
      auto_close_fd(int fd) : value_(fd) {}
      ~auto_close_fd()
      {
        if( value_ >= 0 ) ::close( value_ );
        value_ = -1;
      }
    };
    
    struct net_helper
    {
      struct addrinfo *    addrinfo_;
      std::string          name_;
      net::port_vector     ports_;
      net::string_vector   ips_;
      
      net_helper() : addrinfo_(0) {}
    };
  }
  
  // gets own hostname and try to resolve its hostname to IPs
  net::string_vector
  net::get_own_ips()
  {
    return string_vector();
  }
  
  // get own hostname
  std::string
  net::get_own_hostname()
  {
    return std::string();
  }
  
  // resolve a hostname and return the IP as string "123.123.123.123"
  std::string
  resolve_hostname(const std::string & name)
  {
    return std::string();
  }
  
  // find #count unused TCP port on the 'hostname' interface
  // - hostname has to be a local machine's name or IP
  // - or hostname can be a '*' wildcard, which means all local interfaces
  net::port_vector
  net::find_unused_tcp_ports(unsigned short count,
                             const std::string & hostname)
  {
    return port_vector();
  }
  
  unsigned short
  net::find_unused_tcp_port(const std::string & hostname)
  {
    if( hostname.empty() )
    {
      THROW_("empty hostname parameter");
    }
    
    auto_close_fd fd{::socket(PF_INET, SOCK_STREAM, 0)};
    
    if( fd.value_ < 0 )
    {
      THROW_("socket() failed");
    }
    
    struct sockaddr_in sa;
    bzero(&sa, sizeof sa);

    // resolve hostname:
    // inet_aton
    // inet_addr
    // gethostbyname
    //  - wildcard host: htonl(INADDR_ANY)
    //  - wildcard port: 0
    // getsockname
    // no SO_REUSEADDR needed, because we don't want real connections
  }
  
}}
