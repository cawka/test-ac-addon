// Copied verbatim from GRAPHISOFT/archicad-addon-cmake's Src/APIEnvir.h
// (MIT licensed) — this is DevKit-required boilerplate, not something
// to hand-write.
#ifndef	_APIENVIR_H_
#define	_APIENVIR_H_

#if defined (_MSC_VER)
	#if !defined (WINDOWS)
		#define WINDOWS
	#endif
#endif

#if defined (WINDOWS)
	#include "Win32Interface.hpp"
#endif

#if defined (macintosh)
	#include <CoreServices/CoreServices.h>
#endif

#if !defined (ACExtension)
	#define ACExtension
#endif

#endif
