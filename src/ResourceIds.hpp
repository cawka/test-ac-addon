// IDs follow the numbering convention from GRAPHISOFT/archicad-addon-cmake's
// Src/ResourceIds.hpp (32000/32500/... blocks), extended with our own
// menu resource ids.
#ifndef RESOURCEIDS_HPP
#define RESOURCEIDS_HPP

#define ID_ADDON_INFO 32000

// One 2-item STR# resource per command ([1] "A2" title, [2] the
// command), each registered as its own MenuCode_UserDef menu (see
// AddOnMain.cpp) — Archicad merges same-titled UserDef menus into one
// flat list. A single [title, cmd, cmd, ...] STR# does NOT stay flat:
// item [2] becomes a submenu header and the rest become its children.
#define ID_ADDON_MENU_CREATE_WIRE 32500
#define ID_ADDON_MENU_CONNECT_WIRE_ENDPOINT 32501
#define ID_ADDON_MENU_CREATE_WIRE_BETWEEN_OBJECTS 32502
#define ID_ADDON_MENU_SELECT_CIRCUIT_WIRING 32503
#define ID_ADDON_MENU_SELECT_CIRCUIT_OBJECTS 32504
#define ID_ADDON_MENU_ABOUT 32505

#endif
