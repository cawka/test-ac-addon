#ifndef COMMANDS_MENU_COMMANDS_HPP
#define COMMANDS_MENU_COMMANDS_HPP

#include "ACAPinc.h"

namespace Commands {

// Dispatches Archicad menu clicks to this add-on's commands. One
// handler is registered per ID_ADDON_MENU_* resource (see
// ResourceIds.hpp) via ACAPI_MenuItem_InstallMenuHandler in
// AddOnMain.cpp's Initialize().
class MenuCommandDispatcher {
public:
	MenuCommandDispatcher () = delete;

	static GSErrCode Handle (const API_MenuParams* menuParams);

private:
	// TODO: unimplemented — interactive node-by-node curve input
	// (click to place nodes, drag Bezier handles, Enter/Esc to finish).
	static GSErrCode CreateWire ();

	// TODO: unimplemented — for the native-Spline backend: click a wire
	// endpoint, then a host element, and call
	// Wiring::ConnectionManager::Connect. The GDL backend doesn't need
	// this (see CreateWireBetweenObjects).
	static GSErrCode ConnectWireEndpoint ();

	static GSErrCode CreateWireBetweenObjects ();
	static GSErrCode SelectCircuitWiring ();
	static GSErrCode SelectCircuitObjects ();
	static GSErrCode About ();
};

} // namespace Commands

#endif
