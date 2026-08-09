#include "MenuCommands.hpp"
#include "../AddOnIdentity.hpp"
#include "../Wiring/WireElement.hpp"
#include "../Wiring/WireConnection.hpp"
#include "../Circuit/CircuitSelection.hpp"

namespace Commands {

// Menu item indices within RFIX/RFIX.grc's ADDON_MENU_STRINGS_RESOURCE_ID
// menu. Keep in sync with that file.
enum MenuItemIndex {
	MenuItem_CreateWire = 1,
	MenuItem_ConnectWireEndpoint = 2,
	MenuItem_SelectCircuitWiring = 3,
	MenuItem_SelectCircuitObjects = 4,
};

GSErrCode RegisterMenu ()
{
	// DEVKIT: ACAPI_MenuItem_RegisterMenu's AC29 parameter list
	// (resource IDs, MenuCode_UserDef placement, flags) — this call
	// shape is carried over from the pre-rename ACAPI_Register_Menu.
	return ACAPI_MenuItem_RegisterMenu (
		ADDON_MENU_STRINGS_RESOURCE_ID,
		ADDON_MENU_PROMPT_STRINGS_ID,
		MenuCode_UserDef,
		MenuFlag_Default);
}

GSErrCode InstallMenuHandler ()
{
	return ACAPI_MenuItem_InstallMenuHandler (ADDON_MENU_STRINGS_RESOURCE_ID, MenuCommandHandler);
}

GSErrCode __ACENV_CALL MenuCommandHandler (const API_MenuParams* menuParams)
{
	if (menuParams == nullptr)
		return APIERR_BADPARS;

	switch (menuParams->menuItemRef.itemIndex) {
		case MenuItem_CreateWire:				return CreateWireCommand ();
		case MenuItem_ConnectWireEndpoint:		return ConnectWireEndpointCommand ();
		case MenuItem_SelectCircuitWiring:		return SelectCircuitWiringCommand ();
		case MenuItem_SelectCircuitObjects:	return SelectCircuitObjectsCommand ();
		default:								return NoError;
	}
}

GSErrCode CreateWireCommand ()
{
	// TODO: interactive node-by-node curve input (click to place nodes,
	// drag to set Bezier direction handles, Enter/Esc to finish) —
	// modeled after Archicad's own polyline/spline input tool. For the
	// skeleton, wire this up to WireElement::CreateWire once user input
	// is implemented; there's no hardcoded geometry to fall back to.
	return APIERR_NOTSUPPORTED;
}

GSErrCode ConnectWireEndpointCommand ()
{
	// TODO: let the user click a wire endpoint, then click a host
	// element; resolve the click to a Wiring::ConnectionInfo (host GUID
	// + hotspot/anchor) and call Wiring::Connect.
	return APIERR_NOTSUPPORTED;
}

GSErrCode SelectCircuitWiringCommand ()
{
	GS::UniString circuitId = Circuit::CircuitIdOfSelection ();
	if (circuitId.IsEmpty ())
		return APIERR_NOTMINE;

	return Circuit::SelectCircuitWiring (circuitId);
}

GSErrCode SelectCircuitObjectsCommand ()
{
	GS::UniString circuitId = Circuit::CircuitIdOfSelection ();
	if (circuitId.IsEmpty ())
		return APIERR_NOTMINE;

	return Circuit::SelectCircuitObjects (circuitId);
}

} // namespace Commands
