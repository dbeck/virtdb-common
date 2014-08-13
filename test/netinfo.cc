
#include "util/net.hh"
#include <iostream>

using namespace virtdb::util;

int main()
{
  std::string name(net::get_own_hostname());
  
  std::cout << "My Hostname is: " << name << "\n\n";
  
  net::string_vector ips{net::get_own_ips()};
  for( auto ip : ips )
  {
    std::cout << "IP: - " << ip << "\n";
  }
  
  return 0;
}

