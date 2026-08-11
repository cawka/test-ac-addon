// IDs follow the numbering convention from GRAPHISOFT/archicad-addon-cmake's
// Src/ResourceIds.hpp (32000/32500/... blocks), extended with our own
// menu resource ids.
#ifndef RESOURCEIDS_HPP
#define RESOURCEIDS_HPP

#define ID_ADDON_INFO		32000

// CONFIRMED (graphisoft.github.io/archicad-api-devkit,
// ACAPI_MenuItem_RegisterMenu docs): a MenuCode_UserDef top-level menu
// can only hold ONE thing directly after its title item in a given
// STR# — either one flat command ("Scenario 3": [title, item]) or one
// submenu whose own children follow in that same STR# ("Scenario 4":
// [title, submenuTitle, item, item...]). Our old single 7-item STR#
// ([title, cmd1..cmd6]) has exactly Scenario 4's shape, so item [2]
// silently became a submenu header and items [3]..[7] became its
// children instead of flat siblings under "A2" — that's why "Create
// Wire (Spline)" looked like it wasn't there at all.
//
// Workaround (Graphisoft community "Menu creation with API" thread —
// this part is community-reported convention, not in the AC29 doc text
// itself): register one 2-item STR# resource PER command — [1] "A2"
// (repeated), [2] the one command — and call
// ACAPI_MenuItem_RegisterMenu once per resource, all with
// MenuCode_UserDef. Archicad merges same-titled UserDef menus into one
// flat list on the menu bar instead of creating duplicate "A2" menus.
// See AddOnMain.cpp (RegisterInterface/Initialize) and RINT/AddOn.grc.
#define ID_ADDON_MENU_CREATE_WIRE					32500
#define ID_ADDON_MENU_CONNECT_WIRE_ENDPOINT		32501
#define ID_ADDON_MENU_CREATE_WIRE_BETWEEN_OBJECTS	32502
#define ID_ADDON_MENU_SELECT_CIRCUIT_WIRING		32503
#define ID_ADDON_MENU_SELECT_CIRCUIT_OBJECTS		32504
#define ID_ADDON_MENU_ABOUT						32505

#endif
