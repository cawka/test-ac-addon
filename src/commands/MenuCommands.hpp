#ifndef COMMANDS_MENU_COMMANDS_HPP
#define COMMANDS_MENU_COMMANDS_HPP

#include "ACAPinc.h"

namespace commands {

// Dispatches Archicad menu clicks to this add-on's commands. One
// handler is registered per ID_ADDON_MENU_* resource (see
// ResourceIds.hpp) via ACAPI_MenuItem_InstallMenuHandler in
// AddOnMain.cpp's Initialize().
class MenuCommandDispatcher
{
public:
  MenuCommandDispatcher() = delete;

  static GSErrCode
  handle(const API_MenuParams* menuParams);

private:
  // TODO: unimplemented — interactive node-by-node curve input
  // (click to place nodes, drag Bezier handles, Enter/Esc to finish).
  static GSErrCode
  createWire();

  // TODO: unimplemented — for the native-Spline backend: click a wire
  // endpoint, then a host element, and call
  // wiring::ConnectionManager::connect. The GDL backend doesn't need
  // this (see createWireBetweenObjects).
  static GSErrCode
  connectWireEndpoint();

  static GSErrCode
  createWireBetweenObjects();
  static GSErrCode
  selectCircuitWiring();
  static GSErrCode
  selectCircuitObjects();
  static GSErrCode
  showAbout();
};

} // namespace commands

#endif
