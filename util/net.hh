#pragma once

#include <vector>
#include <string>

namespace virtdb { namespace util {

  struct net
  {
    typedef std::vector<std::string> string_vector;
    typedef std::vector<unsigned short> port_vector;
    
    // gets own hostname and try to resolve its hostname to IPs
    static string_vector
    get_own_ips();
    
    // get own hostname
    static std::string
    get_own_hostname();
    
    // resolve a hostname and return the IP as string "123.123.123.123"
    static std::string
    resolve_hostname(const std::string & name);
    
    // find #count unused TCP port on the 'hostname' interface
    // - hostname has to be a local machine's name or IP
    // - or hostname can be a '*' wildcard, which means all local interfaces
    static port_vector
    find_unused_tcp_ports(unsigned short count,
                         const std::string & hostname="*");
    
    // find 1 unused TCP port like find_unused_ports(...)
    static unsigned short
    find_unused_tcp_port(const std::string & hostname="*");
  };
}}
