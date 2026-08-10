# Based directly on GRAPHISOFT/archicad-addon-cmake (MIT licensed) —
# see README.md for why. All the DevKit-discovery/linking/resource-
# compilation logic lives in Tools/CMakeCommon.cmake (the
# archicad-addon-cmake-tools submodule); this file just wires our own
# Src/ into it, same as the upstream template's own CMakeLists.txt does
# for its Src/.

cmake_minimum_required (VERSION 3.19)

include (Tools/CMakeCommon.cmake)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)
set (AC_API_DEVKIT_DIR "$ENV{AC_API_DEVKIT_DIR}" CACHE PATH "API DevKit directory (the DevKit's Support folder).")

# Tools/CMakeCommon.cmake reads the LP_XML_CONVERTER_FOLDER *CMake*
# variable (used for compiling this add-on's built-in GDL library
# part), but never reads the environment variable of the same name —
# unlike AC_API_DEVKIT_DIR just above, nothing bridges the two on its
# own. Without this line, exporting LP_XML_CONVERTER_FOLDER in your
# shell silently does nothing; it has to be passed explicitly.
set (LP_XML_CONVERTER_FOLDER "$ENV{LP_XML_CONVERTER_FOLDER}" CACHE PATH "LP_XMLConverter directory (for compiling built-in GDL library parts).")

if (NOT DEFINED AC_VERSION)
    set (AC_VERSION 29)
endif ()

message (STATUS "Archicad Version: ${AC_VERSION}")
SetGlobalCompilerDefinitions (${AC_VERSION})

ReadConfigJson ()
set (AC_ADDON_LANGUAGE "${addOnDefaultLanguage}" CACHE STRING "Add-On language code.")

project (${addOnName}-${AC_VERSION})

set (AC_ADDON_FOR_DISTRIBUTION OFF CACHE BOOL "")

set (AddOnSourcesFolder Src)
set (AddOnResourcesFolder .)

set (PCH_HEADER "Src/PrecompiledHeader.hpp")

GenerateAddOnProject (CMakeTarget ${AC_VERSION} "${AC_API_DEVKIT_DIR}" ${AddOnSourcesFolder} ${AddOnResourcesFolder} ${AC_ADDON_LANGUAGE} ${addOnDefaultLanguage} ${PCH_HEADER})
