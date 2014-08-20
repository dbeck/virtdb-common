#include "ip_discovery_client.hh"
#include <util.hh>
#include <unistd.h>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace virtdb { namespace connector {
  
  namespace
  {
    struct close_fd
    {
      int fd_;
      close_fd() : fd_(-1) {}
      ~close_fd()
      {
        if ( fd_ != -1 ) { ::close(fd_); }
      }
    };
  }
  
  std::string
  ip_discovery_client::get_ip(const endpoint_vector & srv_endpoints)
  {
    std::string ret;
    char tmp_ret[2048];
    for( auto const & ep : srv_endpoints )
    {
      if( ep.size() > 10 && ::strncasecmp("raw_udp://", ep.c_str(), 10) == 0 )
      {
        const char * end_str = ep.c_str() + ep.size();
        const char * p = ep.c_str() + 10;
        if( *p == '[' )
        {
          // ipv6
          ++p;
          const char * p_end = p;
          while( *p_end && *p_end != ']' ) ++p_end;
          std::string ip(p,p_end);
          if( (p_end+3) < end_str )
          {
            close_fd fd;
            fd.fd_ = socket(AF_INET6,SOCK_DGRAM,0);
            unsigned short port = atoi(p_end+2);
            struct sockaddr_in6 peer_addr;
            bzero(&peer_addr, sizeof(peer_addr));
            peer_addr.sin6_family = AF_INET6;
            peer_addr.sin6_port=htons(port);
            
            if(fd.fd_ != -1 &&
               inet_pton(AF_INET6, ip.c_str(), &peer_addr.sin6_addr) == 1 )
            {
              auto rc = ::sendto(fd.fd_,
                                 "?",
                                 1,
                                 0,
                                 (const struct sockaddr *)&peer_addr,
                                 sizeof(peer_addr)
                                 );
              if( rc != -1 )
              {
                fd_set handles;
                FD_ZERO(&handles);
                FD_SET(fd.fd_, &handles);
                
                struct timeval tv { 10, 0 };
                int rc_sel = ::select(fd.fd_+1,
                                      &handles,
                                      NULL,
                                      NULL,
                                      &tv );
                
                if( rc_sel > 0 )
                {
                  socklen_t len = sizeof(peer_addr);
                  tmp_ret[0] = 0;
                  rc = ::recvfrom(fd.fd_,
                                  tmp_ret,
                                  sizeof(tmp_ret),
                                  0,
                                  (struct sockaddr *)&peer_addr,
                                  &len);
                  if( rc > 0 )
                  {
                    tmp_ret[sizeof(tmp_ret)-1] = 0;
                    tmp_ret[rc] = 0;
                    ret = "[";
                    ret += tmp_ret;
                    ret += "]";
                    return ret;
                  }
                }
              }
            }
          }
        }
        else
        {
          // ipv4
          const char * p_end = p;
          while( *p_end && *p_end != ':' ) ++p_end;
          std::string ip(p,p_end);
          if( (p_end+2) < end_str )
          {
            close_fd fd;
            fd.fd_ = socket(AF_INET,SOCK_DGRAM,0);
            unsigned short port = atoi(p_end+1);
            in_addr_t addr = inet_addr(ip.c_str());
            struct sockaddr_in peer_addr;
            bzero(&peer_addr, sizeof(peer_addr));
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_port=htons(port);
            peer_addr.sin_addr.s_addr=addr;
            
            if( addr == INADDR_NONE )
            {
              // TODO : try DNS
              // net::resolve(ip.c_str())
              // ...
            }
            
            if( fd.fd_ != -1 && addr != INADDR_NONE )
            {
              
              auto rc = ::sendto(fd.fd_,
                                 "?",
                                 1,
                                 0,
                                 (const struct sockaddr *)&peer_addr,
                                 sizeof(peer_addr));
              if( rc != -1 )
              {
                fd_set handles;
                FD_ZERO(&handles);
                FD_SET(fd.fd_, &handles);
                
                struct timeval tv { 10, 0 };
                int rc_sel = ::select(fd.fd_+1,
                                      &handles,
                                      NULL,
                                      NULL,
                                      &tv );
                if( rc_sel > 0 )
                {
                  socklen_t len = sizeof(peer_addr);
                  tmp_ret[0] = 0;
                  rc = ::recvfrom(fd.fd_,
                                  tmp_ret,
                                  sizeof(tmp_ret),
                                  0,
                                  (struct sockaddr *)&peer_addr,
                                  &len);
                  if( rc > 0 )
                  {
                    tmp_ret[sizeof(tmp_ret)-1] = 0;
                    tmp_ret[rc] = 0;
                    ret = tmp_ret;
                    return ret;
                  }
                }
              }
            }
          }
        }
      }
    }
    return ret;
  }
}}

