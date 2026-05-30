#include "Core/Assert.hpp"
#include <cstdio>
#include <string>

namespace core
{

void assertion(const char* fileName, int line, const char* funcName, const char* message)
{
  const std::string msg = std::string(fileName) + ":" + std::to_string(line) +
                          ": internal check failed in '" + funcName + "': '" + message + "'\n";
  std::fputs(msg.c_str(), stderr);
}

} // namespace core
