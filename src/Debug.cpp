#include "Debug.hpp"

#include <cstdarg>
#include <cstdio>

void
A2E_LogToFile(const char* format, ...)
{
  FILE* f = std::fopen("/tmp/a2e_debug.log", "a");
  if (f == nullptr)
    return;

  va_list args;
  va_start(args, format);
  std::vfprintf(f, format, args);
  va_end(args);

  std::fclose(f);
}
