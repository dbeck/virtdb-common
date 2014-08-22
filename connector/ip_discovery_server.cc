#include "ip_discovery_server.hh"
#include <util.hh>
#include <logger.hh>
#include <unistd.h>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace virtdb { namespace connector {
  
  namespace
  {
    int create_socket(int type, bool can_throw = false)
    {
      int ret = ::socket(type, SOCK_DGRAM, 0);
      if( ret < 0 )
      {
        if( can_throw )
        {
          THROW_("call to ::socket() failed");
        }
        else
        {
          return -1;
        }
      }
      
      int optval = 1;
      int rc = ::setsockopt(ret,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            (const void *)&optval,
                            sizeof(int));
      if( rc < 0 )
      {
        if( can_throw )
        {
          THROW_("call to ::setsockopt(..SO_REUSEADDR..) failed");
        }
      }
      return ret;
    }
  }
  
  ip_discovery_server::ip_discovery_server()
  : worker_(std::bind(&ip_discovery_server::handle_requests,this)),
    fd_ipv4_(-1),
    fd_ipv6_(-1)
  {
    // bind to *
    // return own_ips... in endpoint
    
    fd_ipv4_ = create_socket(AF_INET, true);
    
    struct sockaddr_in svr_addr_4;
    bzero((char *) &svr_addr_4, sizeof(svr_addr_4));
    
    svr_addr_4.sin_family       = AF_INET;
    svr_addr_4.sin_addr.s_addr  = htonl(INADDR_ANY);
    svr_addr_4.sin_port         = 0;
    
    if (bind(fd_ipv4_,
             (struct sockaddr *) &svr_addr_4,
             sizeof(svr_addr_4)) < 0)
    {
      THROW_("cannot bind IPV4 UDP address");
    }
    
    {
      socklen_t len = sizeof(svr_addr_4);
      int rc = ::getsockname(fd_ipv4_, (struct sockaddr *) &svr_addr_4, &len);
      if( rc < 0 )
      {
        THROW_("getcosckname() cannot determine socket address");
      }
      
      auto ips = util::net::get_own_ips(false);
      for( auto const & ip : ips )
      {
        std::ostringstream os;
        os << "raw_udp://" << ip << ':' << ntohs(svr_addr_4.sin_port);
        endpoints_.insert(os.str());
      }
    }
    
    fd_ipv6_ = create_socket(AF_INET6, false);
    
    if( fd_ipv6_ >= 0 )
    {
      struct sockaddr_in6 svr_addr_6;
      bzero((char *) &svr_addr_6, sizeof(svr_addr_6));
      
      svr_addr_6.sin6_family = AF_INET6;
      svr_addr_6.sin6_addr   = in6addr_any;
      svr_addr_6.sin6_port   = 0;
      
      if (bind(fd_ipv6_,
               (struct sockaddr *) &svr_addr_6,
               sizeof(svr_addr_6)) < 0 )
      {
        // ipv6 errors are treated as soft errors and we continue
        // with ipv4 only
        ::close(fd_ipv6_);
        fd_ipv6_ = -1;
      }
      else
      {
        socklen_t len = sizeof(svr_addr_6);
        int rc = ::getsockname(fd_ipv6_, (struct sockaddr *) &svr_addr_6, &len);
        if( rc < 0 )
        {
          ::close(fd_ipv6_);
          fd_ipv6_ = -1;
        }
        else
        {
          auto ips = util::net::get_own_ips(true);
          for( auto const & ip : ips )
          {
            // we only care about ipv6 here
            if( ip.find(":") != std::string::npos )
            {
              std::ostringstream os;
              os << "raw_udp://" << '[' << ip << "]:" << ntohs(svr_addr_6.sin6_port);
              endpoints_.insert(os.str());
            }
          }
        }
      }
    }
    
    // let worker go
    worker_.start();
  }
  
  ip_discovery_server::~ip_discovery_server()
  {
    worker_.stop();
    
    if( fd_ipv4_ >= 0 )
      ::close(fd_ipv4_);
    
    if( fd_ipv6_ >= 0 )
      ::close(fd_ipv6_);
    
    fd_ipv4_ = -1;
    fd_ipv6_ = -1;
  }
  
  const ip_discovery_server::endpoint_set &
  ip_discovery_server::endpoints() const
  {
    return endpoints_;
  }
  
  bool
  ip_discovery_server::handle_requests()
  {
    fd_set handles;
    
    FD_ZERO(&handles);
    FD_SET( fd_ipv4_, &handles);
    
    if( fd_ipv6_ >= 0) { FD_SET( fd_ipv6_, &handles ); }
    
    struct timeval tv { 1, 0 };
    int rc = ::select(std::max(fd_ipv6_+1, fd_ipv4_+1),
                      &handles,
                      NULL,
                      NULL,
                      &tv);
    
    if( rc == 0 )
    {
      // timed out
      return true;
    }
    else if( rc < 0 )
    {
      if( errno == EAGAIN )
      {
        // transient error
        return true;
      }
      else
      {
        // permanent error
        LOG_ERROR("select failed" << V_(errno) );
        return false;
      }
    }
    else
    {
      // we should have data
      char peer[2048];
      peer[0] = 0;
      
      if( FD_ISSET(fd_ipv4_, &handles) )
      {
        struct sockaddr_in addr_4;
        bzero((char *) &addr_4, sizeof(addr_4));
        socklen_t len = sizeof(addr_4);
        ssize_t recvret = ::recvfrom( fd_ipv4_,
                                     peer,
                                     sizeof(peer),
                                     0,
                                     (struct sockaddr *)&addr_4,
                                     &len );
        if( recvret > 0 )
        {
          if( inet_ntop( AF_INET,
                        &(addr_4.sin_addr),
                        peer,
                        sizeof(peer)) != NULL )
          {
            // make sure this is zero terminated
            peer[sizeof(peer)-1] = 0;
            
            ssize_t sendret = ::sendto(fd_ipv4_,
                                       peer,
                                       strlen(peer),
                                       0,
                                       (struct sockaddr *)&addr_4,
                                       len );
            if( sendret < 0 )
            {
              std::string peer_ip{peer};
              LOG_ERROR("sendto() failed" << V_(peer_ip));
            }
          }
        }
        else if( recvret < 0 )
        {
          LOG_ERROR("recvfrom failed" << V_(errno));
        }
      }
      
      if( fd_ipv6_ >=0 && FD_ISSET(fd_ipv6_, &handles) )
      {
        struct sockaddr_in6 addr_6;
        bzero((char *) &addr_6, sizeof(addr_6));
        
        socklen_t len = sizeof(addr_6);
        ssize_t recvret = ::recvfrom( fd_ipv6_,
                                     peer,
                                     sizeof(peer),
                                     0,
                                     (struct sockaddr *)&addr_6,
                                     &len );
        
        
        if( recvret > 0 )
        {
          if( inet_ntop( AF_INET6,
                        &(addr_6.sin6_addr),
                        peer,
                        sizeof(peer)) != NULL )
          {
            // make sure this is zero terminated
            peer[sizeof(peer)-1] = 0;
            
            ssize_t sendret = ::sendto( fd_ipv6_,
                                       peer,
                                       strlen(peer),
                                       0,
                                       (struct sockaddr *)&addr_6,
                                       len );
            
            if( sendret < 0 )
            {
              std::string peer_ip{peer};
              LOG_ERROR("sendto() failed" << V_(peer_ip));
            }
          }
        }
        else if( recvret < 0 )
        {
          LOG_ERROR("recvfrom failed" << V_(errno));
        }
      }
    }
    return true;
  }
  
}}