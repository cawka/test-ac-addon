#ifndef COMMANDS_MENU_COMMANDS_HPP
#define COMMANDS_MENU_COMMANDS_HPP

#include "ACAPinc.h"

namespace Commands {

// Registered directly against ID_ADDON_MENU via
// ACAPI_MenuItem_InstallMenuHandler in AddOnMain.cpp's Initialize()
// (menu registration itself happens in RegisterInterface(), also
// there) — mirroring exactly where GRAPHISOFT/archicad-addon-cmake's
// own template does both, rather than wrapping them here.
GSErrCode MenuCommandHandler (const API_MenuParams* menuParams);

// Menu item implementations. Kept separate from MenuCommandHandler's
// dispatch so they're callable directly (tests, a future palette, ...).
GSErrCode CreateWireCommand ();
GSErrCode ConnectWireEndpointCommand ();
GSErrCode CreateWireBetweenObjectsCommand ();
GSErrCode SelectCircuitWiringCommand ();
GSErrCode SelectCircuitObjectsCommand ();

} // namespace Commands

#endif
