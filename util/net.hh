#pragma once

#include <vector>
#include <string>
#include <utility>

namespace virtdb { namespace util {

  struct net
  {
    typedef std::vector<std::string> string_vector;
    typedef std::vector<unsigned short> port_vector;
    
    // gets own hostname and try to resolve its hostname to IPs
    static string_vector
    get_own_ips(bool ipv6_support=false);
    
    // get own hostname
    static std::string
    get_own_hostname();
    
    // resolve a hostname and return the IP as vector of IP strings
    // ["123.123.123.123", ...]
    static string_vector
    resolve_hostname(const std::string & name,
                     bool ipv6_support=false);
    
    // get peer IP:Port of a given socket
    static std::pair<std::string, unsigned short>
    get_peer_ip(int fd);
  };
}}
