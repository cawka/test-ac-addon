#include "MenuCommands.hpp"
#include "../ResourceIds.hpp"
#include "../Wiring/WireElement.hpp"
#include "../Wiring/WireConnection.hpp"
#include "../Circuit/CircuitSelection.hpp"

namespace Commands {

GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
	switch (menuParams->menuItemRef.menuResID) {
		case ID_ADDON_MENU:
			switch (menuParams->menuItemRef.itemIndex) {
				case MENUITEM_CREATE_WIRE:						return CreateWireCommand ();
				case MENUITEM_CONNECT_WIRE_ENDPOINT:				return ConnectWireEndpointCommand ();
				case MENUITEM_CREATE_WIRE_BETWEEN_OBJECTS:			return CreateWireBetweenObjectsCommand ();
				case MENUITEM_SELECT_CIRCUIT_WIRING:				return SelectCircuitWiringCommand ();
				case MENUITEM_SELECT_CIRCUIT_OBJECTS:				return SelectCircuitObjectsCommand ();
			}
			break;
	}
	return NoError;
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
	// need checking against the real header (this is one I still
	// couldn't verify against the DevKit itself — only the build
	// template was public, not the API headers).
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
