# Locates the Archicad API Development Kit for AC29.
#
# The DevKit is a proprietary Graphisoft download (developer-portal
# account required) and is intentionally not vendored in this repo.
#
# Resolution order:
#   1. -DAC_API_DEVKIT_DIR=... passed to cmake
#   2. AC_API_DEVKIT_DIR environment variable
#
# Sets, on success:
#   ArchicadAPIDevKit_FOUND
#   ArchicadAPIDevKit_INCLUDE_DIRS
#   ArchicadAPIDevKit_LIBRARIES
#
# Confirmed against a real Mac DevKit download (29.3100):
#   Support/Inc/            flat headers (ACAPinc.h etc, no subfolders)
#   Support/Lib/             exactly one file: libACAP_STAT.a
#   Support/Frameworks/      ~40 .framework bundles (ArchicadAPI, GSRoot,
#                            Geometry, MEPAPI, IFCInOutAPI, ...)
#   Support/Tools/           CompileResources.py + an OSX/ subfolder
#
# NOT confirmed: which of the ~40 frameworks an add-on actually needs to
# link against. Defaulting to just ArchicadAPI.framework (its name is
# the closest match to "this is the add-on API surface") plus
# libACAP_STAT.a. If the linker reports undefined symbols, that error
# names the framework providing them — add it to
# AC_API_DEVKIT_EXTRA_FRAMEWORKS (a semicolon-separated list of
# framework names, no ".framework" suffix, e.g.
# `-DAC_API_DEVKIT_EXTRA_FRAMEWORKS="GSRoot;GSUtils"`) rather than
# editing this file. That's the reliable way to find the real minimal
# set — guessing it up front got the Windows-style ".a directory" shape
# wrong once already, not repeating that here.
#
# Windows Lib layout is UNVERIFIED (no Windows DevKit inspected yet) —
# guessing it mirrors Mac's flat Support/Lib/*.lib rather than the
# nested Support/Lib/Win/ this file assumed before. Correct once you
# have a Windows DevKit to check `ls Support\Lib` against.

if (NOT AC_API_DEVKIT_DIR AND DEFINED ENV{AC_API_DEVKIT_DIR})
    set (AC_API_DEVKIT_DIR "$ENV{AC_API_DEVKIT_DIR}")
endif ()

set (AC_API_DEVKIT_DIR "${AC_API_DEVKIT_DIR}" CACHE PATH
     "Path to the Archicad 29 API Development Kit root (contains Support/Inc/ACAPinc.h)")

set (AC_API_DEVKIT_EXTRA_FRAMEWORKS "" CACHE STRING
     "Extra framework names (no .framework suffix) from Support/Frameworks to link on Mac, beyond ArchicadAPI. Fill in as the linker reports missing symbols.")

set (_ac_devkit_header "${AC_API_DEVKIT_DIR}/Support/Inc/ACAPinc.h")

if (AC_API_DEVKIT_DIR AND EXISTS "${_ac_devkit_header}")
    set (ArchicadAPIDevKit_FOUND TRUE)

    set (ArchicadAPIDevKit_INCLUDE_DIRS
        "${AC_API_DEVKIT_DIR}/Support/Inc"
    )

    if (WIN32)
        # UNVERIFIED — see file header comment above.
        file (GLOB ArchicadAPIDevKit_LIBRARIES
              "${AC_API_DEVKIT_DIR}/Support/Lib/*.lib")
    elseif (APPLE)
        set (ArchicadAPIDevKit_LIBRARIES
            "${AC_API_DEVKIT_DIR}/Support/Lib/libACAP_STAT.a"
            "${AC_API_DEVKIT_DIR}/Support/Frameworks/ArchicadAPI.framework"
        )
        foreach (_fw ${AC_API_DEVKIT_EXTRA_FRAMEWORKS})
            list (APPEND ArchicadAPIDevKit_LIBRARIES
                  "${AC_API_DEVKIT_DIR}/Support/Frameworks/${_fw}.framework")
        endforeach ()
    endif ()
else ()
    set (ArchicadAPIDevKit_FOUND FALSE)
    message (WARNING
        "Archicad API Development Kit not found at '${AC_API_DEVKIT_DIR}'.\n"
        "Set -DAC_API_DEVKIT_DIR=<path> or the AC_API_DEVKIT_DIR env var to "
        "the DevKit root (the directory containing Support/Inc/ACAPinc.h). "
        "Configure will continue but the add-on target will not build.")
endif ()
