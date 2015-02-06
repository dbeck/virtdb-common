#include "hex_util.hh"

namespace virtdb { namespace util {
  
  namespace
  {
    const char hex_vals[16] = {
      '0', '1', '2', '3',
      '4', '5', '6', '7',
      '8', '9', 'a', 'b',
      'c', 'd', 'e', 'f'
    };
  }
  
  void hex_util(unsigned long long hashed, std::string & res)
  {
    char result[] = "0123456789abcdef";
    
    for( int i=0; i<16; ++i )
    {
      result[i] = hex_vals[(hashed >> ((15-i)*4)) & 0x0f];
    }
    res = result;
  }
  
}}
