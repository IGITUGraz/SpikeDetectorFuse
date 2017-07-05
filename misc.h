#ifndef MISC_H
#define MISC_H

#include <sstream>
#include <string>

template <typename T>
std::string numberToString ( T Number )
{
  std::ostringstream ss;
  ss << Number;
  return ss.str();
}

#endif