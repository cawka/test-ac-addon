#ifndef COMMANDS_MENU_COMMANDS_HPP
#define COMMANDS_MENU_COMMANDS_HPP

#include "ACAPinc.h"

namespace Commands {

// Registered against each of the ID_ADDON_MENU_* resources (see
// ResourceIds.hpp) via ACAPI_MenuItem_InstallMenuHandler in
// AddOnMain.cpp's Initialize() (menu registration itself happens in
// RegisterInterface(), also there) — mirroring where
// GRAPHISOFT/archicad-addon-cmake's own template does both, rather than
// wrapping them here.
GSErrCode MenuCommandHandler (const API_MenuParams* menuParams);

// Menu item implementations. Kept separate from MenuCommandHandler's
// dispatch so they're callable directly (tests, a future palette, ...).
GSErrCode CreateWireCommand ();
GSErrCode ConnectWireEndpointCommand ();
GSErrCode CreateWireBetweenObjectsCommand ();
GSErrCode SelectCircuitWiringCommand ();
GSErrCode SelectCircuitObjectsCommand ();
GSErrCode AboutCommand ();

} // namespace Commands

#endif
