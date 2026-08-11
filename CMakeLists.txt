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

# Fail fast with the full checklist, not one cryptic error per variable
# per re-run. Both env vars are read once above; if you're setting them
# for the first time on a new machine, `export` both before running
# cmake (or pass the equivalent -D flags shown below) and re-configure.
set (_a2e_setup_errors "")

if (NOT AC_API_DEVKIT_DIR OR NOT EXISTS "${AC_API_DEVKIT_DIR}/Inc/ACAPinc.h")
    string (APPEND _a2e_setup_errors "
  AC_API_DEVKIT_DIR is not set to a valid Archicad API DevKit 'Support' folder.
    Current value: '${AC_API_DEVKIT_DIR}'
    Expected to find: <that path>/Inc/ACAPinc.h
    Get it from: https://github.com/GRAPHISOFT/archicad-api-devkit/releases
      (or https://archicadapi.graphisoft.com/downloads/api-development-kit)
    Point at the DevKit's *Support* subfolder, not its root, e.g.:
      export AC_API_DEVKIT_DIR=\"/path/to/API Development Kit 29/Support\"
    Or pass it directly to cmake instead of exporting it:
      cmake -DAC_API_DEVKIT_DIR=\"/path/to/API Development Kit 29/Support\" ...
")
endif ()

if (NOT LP_XML_CONVERTER_FOLDER OR NOT EXISTS "${LP_XML_CONVERTER_FOLDER}")
    string (APPEND _a2e_setup_errors "
  LP_XML_CONVERTER_FOLDER is not set to a valid LP_XMLConverter folder.
    Current value: '${LP_XML_CONVERTER_FOLDER}'
    Needed to compile the built-in \"Circuit Wire\" GDL library part
    (RINT/ACLib/Src/Circuit Wire/) into the add-on.
    Get it from the same releases page as the DevKit above, but the
    'LP_XMLConverter' zip instead of 'API.Development.Kit' (matching
    platform + Archicad version, e.g. LP_XMLConverter.MAC.29.3000.zip),
    then point at the unzipped folder:
      export LP_XML_CONVERTER_FOLDER=\"/path/to/unzipped/LP_XMLConverter\"
    Or pass it directly to cmake instead of exporting it:
      cmake -DLP_XML_CONVERTER_FOLDER=\"/path/to/LP_XMLConverter\" ...
")
endif ()

if (_a2e_setup_errors)
    message (FATAL_ERROR "Missing required setup before this project can be configured:\n${_a2e_setup_errors}")
endif ()

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
