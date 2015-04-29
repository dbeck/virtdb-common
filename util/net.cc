#include "net.hh"
#include "exception.hh"
#include <mutex>
#include <atomic>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

namespace virtdb { namespace util {
  
  std::pair<std::string, unsigned short>
  convert(const struct sockaddr *sa)
  {
    char result[2048];
    size_t maxlen = sizeof(result);
    std::pair<std::string, unsigned short> ret;
    result[0] = 0;
    result[sizeof(result)-1] = 0;
    
    switch(sa->sa_family) {
      case AF_INET:
      {
        struct sockaddr_in *s = (struct sockaddr_in *)sa;
        inet_ntop(AF_INET,
                  &(s->sin_addr),
                  result,
                  maxlen);
        ret.first = result;
        ret.second = ntohs(s->sin_port);
        break;
      }
        
      case AF_INET6:
      {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)sa;
        inet_ntop(AF_INET6,
                  &(s->sin6_addr),
                  result,
                  maxlen);
        ret.first = result;
        ret.second = ntohs(s->sin6_port);
        break;
      }
        
      default:
        return ret;
    }

    return ret;
  }
  
  
  net::string_vector
  net::get_own_ips(bool ipv6_support)
  {
#ifdef ENABLE_OWN_IP_CACHE
    static std::mutex mtx;
    static net::string_vector ipv6_ips;
    static net::string_vector non_ipv6_ips;
    static std::atomic<bool> ipv6_ips_resolved{false};
    static std::atomic<bool> non_ipv6_ips_resolved{false};
    
    net::string_vector ret;
    
    if( ipv6_support )
    {
      if( !ipv6_ips_resolved )
      {
        ret = resolve_hostname(get_own_hostname(), ipv6_support);
        if( ret.empty() )
          ret.push_back("127.0.0.1");
        
        {
          std::unique_lock<std::mutex> l(mtx);
          ipv6_ips = ret;
        }
        ipv6_ips_resolved = true;
      }
      else
      {
        std::unique_lock<std::mutex> l(mtx);
        ret = ipv6_ips;
      }
    }
    else
    {
      if( !non_ipv6_ips_resolved )
      {
        ret = resolve_hostname(get_own_hostname(), ipv6_support);
        {
          std::unique_lock<std::mutex> l(mtx);
          non_ipv6_ips = ret;
        }
        non_ipv6_ips_resolved = true;
      }
      else
      {
        std::unique_lock<std::mutex> l(mtx);
        ret = non_ipv6_ips;
      }
    }
    return ret;
#else
    ret = resolve_hostname(get_own_hostname(),
                           ipv6_support);
    if( ret.empty() )
      ret.push_back("127.0.0.1");
    
    return ret;
#endif
    
    
  }
  
  std::string
  net::get_own_hostname()
  {
    char name[2048];
    name[0] = 0;
    gethostname(name, sizeof(name));
    name[2047] = 0;
    return std::string(name);
  }
  
  net::string_vector
  net::resolve_hostname(const std::string & name,
                        bool ipv6_support)
  {
    string_vector ret;
    
    if( name.empty() )
      return ret;
    
    // check if this is a '123.123.123.123'
    // if it is, then we don't resolve
    in_addr_t dotted_addr = inet_addr(name.c_str());
    if( dotted_addr != INADDR_NONE )
      return string_vector{ name };
    
    {
      struct addrinfo hints, *servinfo;
      
      memset(&hints, 0, sizeof hints);
      hints.ai_socktype  = SOCK_STREAM;
      if( ipv6_support )  hints.ai_family    = AF_UNSPEC;
      else                hints.ai_family    = AF_INET;
      
      if ( !getaddrinfo(name.c_str(), "65432", &hints, &servinfo) )
      {
        for(struct addrinfo *p = servinfo; p != NULL; p = p->ai_next)
        {
          ret.push_back(convert(p->ai_addr).first);
        }
        freeaddrinfo(servinfo); // all done with this structure
      }
    }
    return ret;
  }
  
  std::pair<std::string, unsigned short>
  net::get_peer_ip(int fd)
  {
    std::pair<std::string, unsigned short> ret{"",0};
    if( fd < 0 ) return ret;
    
    socklen_t len;
    struct sockaddr_storage addr;
    
    len = sizeof (addr);
    if( !getpeername(fd, (struct sockaddr*)&addr, &len) )
    {
      ret = convert((struct sockaddr*)&addr);
    }
    return ret;
  }

}}
