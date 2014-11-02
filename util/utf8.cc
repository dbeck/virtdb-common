
#include <util/utf8.hh>

namespace virtdb { namespace util {
  
  void
  utf8::sanitize(char * px, size_t len)
  {
    if( !px || !len ) { return; }
    
    unsigned char * p    = (unsigned char *)px;
    unsigned char * end  = p+len;
    
    int code_pos = 0;
    int code_len = 1;
    
    while( p != end )
    {
      // in any case, we don't allow 0x0 inside an UTF-8 string
      if( *p == 0 ) *p = ' ';
      
      // common bad case comes here. sanitize it here to simplify other cases
      // at a cost of an extra check. that is:
      //  - we think we are in a multibyte char
      //  - but the character is not a utf-8 continuation
      if( code_len > 1 && ((*p)>>6) != 2 )
      {
        ++code_pos;
        for( int i=1; i<code_pos; ++i )
          *(p-i) = ' ';
        // reset variables
        code_len = 1;
        code_pos = 0;
      }
      
      // one-byte length sequence is OK
      if( *p < 128 )
      {
        code_len = 1;
        code_pos = 0;
      }
      // start of a 4-byte sequence
      else if( ((*p)>>3) == 30 ) // 11110
      {
        code_len = 4;
        code_pos = 1;
      }
      // start of a 3-byte sequence
      else if( ((*p)>>4) == 14 ) // 1110
      {
        code_len = 3;
        code_pos = 1;
      }
      // start of a 2-byte sequence
      else if( ((*p)>>5) == 6 ) // 110
      {
        code_len = 2;
        code_pos = 1;
      }
      // continuation of a sequence
      else if( ((*p)>>6) == 2 )
      {
        ++code_pos;
        // we expected less bytes for the sequence than
        // what has arrived
        if( code_pos > code_len || code_len == 1 )
        {
          for( int i=0; i<code_pos; ++i )
            *(p-i) = ' ';
          // reset variables
          code_len = 1;
          code_pos = 0;
        }
        else if( code_pos == code_len )
        {
          code_len = 1;
        }
      }
      // random garbage
      else
      {
        *p = ' ';
        code_len = 1;
        code_pos = 0;
      }
      ++p;
    }
    
    // check for incomplete utf-8 sequences
    if( code_len > 1 && code_len > code_pos )
    {
      ++code_pos;
      for( int i=1; i<code_pos; ++i )
      {
        *(p-i) = ' ';
      }
    }
  }
  
}}