#include "tcp.hh"
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
  }
  
  unsigned short
  tcp::find_unused_port(const std::string & hostname)
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
