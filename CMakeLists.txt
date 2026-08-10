cmake_minimum_required (VERSION 3.20)

project (TestACAddOn CXX)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include (FindArchicadAPIDevKit)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (ADDON_SOURCES
    Src/AddOnMain.cpp
    Src/Wiring/WireElement.cpp
    Src/Wiring/WireConnection.cpp
    Src/Wiring/GdlWireElement.cpp
    Src/Wiring/ElementAnchor.cpp
    Src/Circuit/CircuitProperty.cpp
    Src/Circuit/CircuitSelection.cpp
    Src/Commands/MenuCommands.cpp
)

set (ADDON_HEADERS
    Src/AddOnIdentity.hpp
    Src/Wiring/WireEnd.hpp
    Src/Wiring/WireElement.hpp
    Src/Wiring/WireConnection.hpp
    Src/Wiring/GdlWireElement.hpp
    Src/Wiring/ElementAnchor.hpp
    Src/Circuit/CircuitProperty.hpp
    Src/Circuit/CircuitSelection.hpp
    Src/Commands/MenuCommands.hpp
)

add_library (TestACAddOn MODULE ${ADDON_SOURCES} ${ADDON_HEADERS})

target_include_directories (TestACAddOn PRIVATE
    Src
    Src/Circuit
    Src/Wiring
    Src/Commands
)

if (ArchicadAPIDevKit_FOUND)
    target_include_directories (TestACAddOn PRIVATE ${ArchicadAPIDevKit_INCLUDE_DIRS})
    target_link_libraries (TestACAddOn PRIVATE ${ArchicadAPIDevKit_LIBRARIES})
else ()
    message (WARNING
        "Building TestACAddOn without the Archicad API Development Kit — "
        "this will fail at the '#include \"ACAPinc.h\"' step. See README.md.")
endif ()

if (WIN32)
    set_target_properties (TestACAddOn PROPERTIES
        SUFFIX ".apx"
        PREFIX ""
    )
    target_compile_definitions (TestACAddOn PRIVATE WINDOWS _WINDOWS)
elseif (APPLE)
    set_target_properties (TestACAddOn PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "bundle"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/RFIX/Info.plist.in"
    )
    target_compile_definitions (TestACAddOn PRIVATE macintosh __APPLE__)
endif ()
