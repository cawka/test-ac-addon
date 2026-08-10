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
	MenuItem_CreateWireBetweenObjects = 3,
	MenuItem_SelectCircuitWiring = 4,
	MenuItem_SelectCircuitObjects = 5,
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
		case MenuItem_CreateWire:					return CreateWireCommand ();
		case MenuItem_ConnectWireEndpoint:			return ConnectWireEndpointCommand ();
		case MenuItem_CreateWireBetweenObjects:	return CreateWireBetweenObjectsCommand ();
		case MenuItem_SelectCircuitWiring:			return SelectCircuitWiringCommand ();
		case MenuItem_SelectCircuitObjects:		return SelectCircuitObjectsCommand ();
		default:									return NoError;
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
	// TODO: for the native-Spline backend (WireElement.*), where a wire
	// can have more than two nodes and only its Start/End are
	// connectable: let the user click a wire endpoint, then click a
	// host element, and call Wiring::Connect(wireGuid, end, { hostGuid }).
	// The GDL backend doesn't need this — see
	// CreateWireBetweenObjectsCommand below, which creates and connects
	// both ends in one click-click gesture.
	return APIERR_NOTSUPPORTED;
}

GSErrCode CreateWireBetweenObjectsCommand ()
{
	API_Guid startHostGuid = APINULLGuid;
	API_Guid endHostGuid = APINULLGuid;

	// DEVKIT: confirm AC29's interactive "let the user click an element"
	// call (historically ACAPI_UserInput_ClickAnElement /
	// ACAPI_Interface_ClickAnElement in APIdefs_UserInput.h) — signature,
	// prompt-string parameter, and how a cancelled click is reported all
	// need checking against the real header.
	GSErrCode err = ACAPI_UserInput_ClickAnElement ("Click the start object", nullptr, &startHostGuid);
	if (err != NoError || startHostGuid == APINULLGuid)
		return err;

	err = ACAPI_UserInput_ClickAnElement ("Click the end object", nullptr, &endHostGuid);
	if (err != NoError || endHostGuid == APINULLGuid)
		return err;

	// DEVKIT: layer index hardcoded to 0 (current/active layer) for the
	// skeleton — swap in whatever layer selection policy you want
	// (active layer via ACAPI_Environment, a dedicated "Circuit Wiring"
	// layer looked up/created once, etc).
	const short wireLayerIndex = 0;

	API_Guid wireGuid = Wiring::ConnectObjectsWithGdlWire (startHostGuid, endHostGuid, wireLayerIndex);
	return (wireGuid != APINULLGuid) ? NoError : APIERR_GENERAL;
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
