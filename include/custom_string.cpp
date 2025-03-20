#include "custom_string.h"
#include <cstring>
#include <iostream>

std::ostream &operator<<( std::ostream &os, const string &s )
   {
   os << s.cstr();
   return os;
   }
std::istream &operator>>( std::istream &is, string &s )
   {
   char buffer[1024];
   is >> buffer;
   s = string(buffer);
   return is;
   }