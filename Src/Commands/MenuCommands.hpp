#ifndef COMMANDS_MENU_COMMANDS_HPP
#define COMMANDS_MENU_COMMANDS_HPP

#include "ACAPinc.h"

namespace Commands {

// Registers the add-on's menu (RFIX/RFIX.grc holds the menu item
// strings/IDs) — called from RegisterInterface.
GSErrCode RegisterMenu ();

// Installs the menu item click handler — called from Initialize.
GSErrCode InstallMenuHandler ();

// Dispatches a menu item click to the relevant command below.
//
// DEVKIT: confirm the AC29 menu handler callback signature
// (ACAPI_MenuItem_InstallMenuHandler's expected function pointer type)
// in ACAPI_MenuItemProcedures.hpp.
GSErrCode __ACENV_CALL MenuCommandHandler (const API_MenuParams* menuParams);

// Menu item implementations. Kept separate from MenuCommandHandler's
// dispatch so they're callable directly (tests, a future palette, ...).
GSErrCode CreateWireCommand ();
GSErrCode ConnectWireEndpointCommand ();
GSErrCode CreateWireBetweenObjectsCommand ();
GSErrCode SelectCircuitWiringCommand ();
GSErrCode SelectCircuitObjectsCommand ();

} // namespace Commands

#endif
