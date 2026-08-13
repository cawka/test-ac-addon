// Adapted from GRAPHISOFT/archicad-addon-cmake's
// Src/ExamplePrecompiledHeader.hpp (MIT licensed).
#ifndef CIRCUIT_WIRING_PRECOMPILED_HEADER_HPP
#define CIRCUIT_WIRING_PRECOMPILED_HEADER_HPP

#include <GSMalloc.hpp>
#include <GSNew.hpp>

#if defined(macintosh)
namespace std {
void*
GS_realloc(void* userData, size_t newSize);
}
#endif

#include <limits.h>
#include <math.h>
#include <memory>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(WINDOWS)
#include "Win32Interface.hpp"
#endif

#include "ACAPinc.h"
#include "APIEnvir.h"

#endif // CIRCUIT_WIRING_PRECOMPILED_HEADER_HPP
