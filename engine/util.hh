#pragma once

#include <string>
#include <murmur3/murmur3.h>
#include <cstdlib>

namespace virtdb { namespace engine {

inline std::string generate_hash(const std::string& source)
{
    std::string ret = "";
    char out_buffer[16];
    static const char hexchars[] ="0123456789abcdef";

    MurmurHash3_x64_128(source.c_str(), source.size(), 0xdeadbeef, &out_buffer);
    for (auto c : out_buffer)
    {
        ret += hexchars[c & 0x0f];
        ret += hexchars[(c >> 4) & 0x0f];
    }
    return ret;
}

// Source: http://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
inline std::string gen_random(const int len) {
    char * s = new char[len];
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[std::rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
    std::string ret = s;
    delete [] s;
    return ret;
}

}}