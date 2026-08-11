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
// UNVERIFIED: the .grc's first STR# entry is now the top-level menu
// title ("A2", for MenuCode_UserDef — see AddOnMain.cpp), not itself a
// command. Graphisoft's own docs describe the real commands after a
// menu/submenu title as "corresponding to the menu items in the
// non-main-menu case" — read here as "still numbered 1..N, the title
// isn't counted" — but that's this repo's best-effort reading of an
// ambiguous doc sentence, not confirmed against a real running add-on.
// If items dispatch to the wrong command (or item 1 turns out
// unreachable), these values need to shift by one instead.
#define MENUITEM_CREATE_WIRE					1
#define MENUITEM_CONNECT_WIRE_ENDPOINT			2
#define MENUITEM_CREATE_WIRE_BETWEEN_OBJECTS	3
#define MENUITEM_SELECT_CIRCUIT_WIRING			4
#define MENUITEM_SELECT_CIRCUIT_OBJECTS		5
#define MENUITEM_ABOUT							6

#endif
