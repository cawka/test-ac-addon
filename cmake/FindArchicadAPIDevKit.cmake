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

if (NOT AC_API_DEVKIT_DIR AND DEFINED ENV{AC_API_DEVKIT_DIR})
    set (AC_API_DEVKIT_DIR "$ENV{AC_API_DEVKIT_DIR}")
endif ()

set (AC_API_DEVKIT_DIR "${AC_API_DEVKIT_DIR}" CACHE PATH
     "Path to the Archicad 29 API Development Kit root (contains Support/Inc/ACAPinc.h)")

set (_ac_devkit_header "${AC_API_DEVKIT_DIR}/Support/Inc/ACAPinc.h")

if (AC_API_DEVKIT_DIR AND EXISTS "${_ac_devkit_header}")
    set (ArchicadAPIDevKit_FOUND TRUE)

    set (ArchicadAPIDevKit_INCLUDE_DIRS
        "${AC_API_DEVKIT_DIR}/Support/Inc"
        "${AC_API_DEVKIT_DIR}/Support/GSRoot"
        "${AC_API_DEVKIT_DIR}/Support/Modules/GSRoot"
        "${AC_API_DEVKIT_DIR}/Support/Modules/GSUtils"
    )

    if (WIN32)
        file (GLOB ArchicadAPIDevKit_LIBRARIES
              "${AC_API_DEVKIT_DIR}/Support/Lib/Win/*.lib")
    elseif (APPLE)
        file (GLOB ArchicadAPIDevKit_LIBRARIES
              "${AC_API_DEVKIT_DIR}/Support/Lib/Mac/*.a")
    endif ()
else ()
    set (ArchicadAPIDevKit_FOUND FALSE)
    message (WARNING
        "Archicad API Development Kit not found at '${AC_API_DEVKIT_DIR}'.\n"
        "Set -DAC_API_DEVKIT_DIR=<path> or the AC_API_DEVKIT_DIR env var to "
        "the DevKit root (the directory containing Support/Inc/ACAPinc.h). "
        "Configure will continue but the add-on target will not build.")
endif ()
