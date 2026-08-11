# Based directly on GRAPHISOFT/archicad-addon-cmake (MIT licensed) —
# see README.md for why. All the DevKit-discovery/linking/resource-
# compilation logic lives in Tools/CMakeCommon.cmake (the
# archicad-addon-cmake-tools submodule); this file just wires our own
# src/ into it, same as the upstream template's own CMakeLists.txt does
# for its Src/ (theirs is capitalized; ours isn't — matched to this
# repo's own naming convention, not a typo).

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
else ()
    # The folder existing isn't enough — Tools/CompileResources.py's
    # LibraryCompiler looks for the actual tool at one specific path
    # inside it (confirmed by reading that script directly, after
    # hitting this exact gap: LP_XML_CONVERTER_FOLDER was set, cmake
    # configured fine, the add-on built and loaded fine, but
    # ACAPI_LibraryPart_Search couldn't find "Circuit Wire" at runtime).
    # When the tool isn't found there, CompileResources.py does NOT fail
    # the build — it prints one yellow "no libpart compiler available"
    # warning during the build (easy to miss) and silently skips
    # compiling RINT/BuiltInLibParts.grc, so the add-on ships with no
    # GDL library part embedded at all. Catch that here instead, loudly,
    # at configure time.
    if (WIN32)
        set (_a2e_libpart_tool "${LP_XML_CONVERTER_FOLDER}/LP_XMLConverter.exe")
    elseif (EXISTS "${LP_XML_CONVERTER_FOLDER}/LP_XMLConverter.app")
        set (_a2e_libpart_tool "${LP_XML_CONVERTER_FOLDER}/LP_XMLConverter.app/Contents/MacOS/LP_XMLConverter")
    else ()
        set (_a2e_libpart_tool "${LP_XML_CONVERTER_FOLDER}/LP_XMLConverter")
    endif ()

    if (NOT EXISTS "${_a2e_libpart_tool}")
        string (APPEND _a2e_setup_errors "
  LP_XML_CONVERTER_FOLDER is set, but the actual tool isn't where the
  build expects it:
    Looked for: '${_a2e_libpart_tool}'
    LP_XML_CONVERTER_FOLDER should be the direct unzip of the
    LP_XMLConverter zip, with LP_XMLConverter (or LP_XMLConverter.app
    on Mac) directly at its top level — not a nested subfolder.
    If this isn't fixed the build will SILENTLY skip compiling
    RINT/ACLib/Src/Circuit Wire/ into the add-on: it still builds and
    loads normally, but ACAPI_LibraryPart_Search will never find
    \"Circuit Wire\" at runtime.
")
    endif ()
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

set (AddOnSourcesFolder src)
set (AddOnResourcesFolder .)

set (PCH_HEADER "src/PrecompiledHeader.hpp")

GenerateAddOnProject (CMakeTarget ${AC_VERSION} "${AC_API_DEVKIT_DIR}" ${AddOnSourcesFolder} ${AddOnResourcesFolder} ${AC_ADDON_LANGUAGE} ${addOnDefaultLanguage} ${PCH_HEADER})

# GitVersion.hpp (A2E_GIT_VERSION) — regenerated on every build (this is
# an `ALL`, no-OUTPUT-tracked custom target, so it always re-runs, unlike
# a plain configure-time `execute_process` which would go stale the
# moment you rebuild without reconfiguring) so the add-on always reports
# the commit it was actually built from. See src/AddOnMain.cpp's
# Initialize() for where it's surfaced.
set (A2E_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
add_custom_target (A2E_GenerateVersionHeader ALL
    COMMAND ${CMAKE_COMMAND}
        "-DSRC_DIR=${CMAKE_SOURCE_DIR}"
        "-DDEST=${A2E_GENERATED_DIR}/GitVersion.hpp"
        -P "${CMAKE_SOURCE_DIR}/cmake/GenerateGitVersion.cmake"
    BYPRODUCTS "${A2E_GENERATED_DIR}/GitVersion.hpp"
    COMMENT "Updating GitVersion.hpp"
)
add_dependencies (CMakeTarget A2E_GenerateVersionHeader)
target_include_directories (CMakeTarget PRIVATE "${A2E_GENERATED_DIR}")
