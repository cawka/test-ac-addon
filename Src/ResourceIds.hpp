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
#define MENUITEM_CREATE_WIRE					1
#define MENUITEM_CONNECT_WIRE_ENDPOINT			2
#define MENUITEM_CREATE_WIRE_BETWEEN_OBJECTS	3
#define MENUITEM_SELECT_CIRCUIT_WIRING			4
#define MENUITEM_SELECT_CIRCUIT_OBJECTS		5

#endif
