// IDs follow the numbering convention from GRAPHISOFT/archicad-addon-cmake's
// Src/ResourceIds.hpp (32000/32500/... blocks), extended with our own
// menu item indices.
#ifndef RESOURCEIDS_HPP
#define RESOURCEIDS_HPP

#define ID_ADDON_INFO		32000
#define ID_ADDON_MENU		32500

// Item indices within the ID_ADDON_MENU string list (RINT/AddOn.grc) —
// keep in sync with that file and with the switch in
// Commands::MenuCommandHandler.
//
// CONFIRMED (graphisoft.github.io/archicad-api-devkit, Group Menu Item /
// ACAPI_MenuItem_RegisterMenu docs): for MenuCode_UserDef, STR# item
// [1] is the top-level menu's own title ("A2" — see AddOnMain.cpp), not
// a command, and API_MenuItemRef::itemIndex is literally the STR# item
// number as written in the .grc — it is NOT renumbered starting from 1
// for the first real command. So the first actual menu item ("Create
// Wire (Spline)", STR# [2]) arrives as itemIndex == 2, not 1. Previous
// values here started at 1 and were off by one against every real
// click.
#define MENUITEM_CREATE_WIRE					2
#define MENUITEM_CONNECT_WIRE_ENDPOINT			3
#define MENUITEM_CREATE_WIRE_BETWEEN_OBJECTS	4
#define MENUITEM_SELECT_CIRCUIT_WIRING			5
#define MENUITEM_SELECT_CIRCUIT_OBJECTS		6
#define MENUITEM_ABOUT							7

#endif
