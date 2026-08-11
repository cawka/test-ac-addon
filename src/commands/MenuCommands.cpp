#include "MenuCommands.hpp"
#include "../ResourceIds.hpp"
#include "GitVersion.hpp"
#include "Debug.hpp"
#include "../wiring/WireElement.hpp"
#include "../wiring/WireConnection.hpp"
#include "../circuit/CircuitSelection.hpp"

namespace Commands {

GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
	A2E_TRACE ("A2E: MenuCommandHandler menuResID=%d itemIndex=%d\n",
		(int) menuParams->menuItemRef.menuResID, (int) menuParams->menuItemRef.itemIndex);

	switch (menuParams->menuItemRef.menuResID) {
		case ID_ADDON_MENU:
			switch (menuParams->menuItemRef.itemIndex) {
				case MENUITEM_CREATE_WIRE:						return CreateWireCommand ();
				case MENUITEM_CONNECT_WIRE_ENDPOINT:				return ConnectWireEndpointCommand ();
				case MENUITEM_CREATE_WIRE_BETWEEN_OBJECTS:			return CreateWireBetweenObjectsCommand ();
				case MENUITEM_SELECT_CIRCUIT_WIRING:				return SelectCircuitWiringCommand ();
				case MENUITEM_SELECT_CIRCUIT_OBJECTS:				return SelectCircuitObjectsCommand ();
				case MENUITEM_ABOUT:								return AboutCommand ();
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
	A2E_TRACE ("A2E: CreateWireBetweenObjectsCommand start\n");

	API_Guid startHostGuid = APINULLGuid;
	API_Guid endHostGuid = APINULLGuid;

	// There's no dedicated "click an element" call — confirmed against
	// the real AC29 docs (Group: User Input) there isn't one. The actual
	// pattern is ACAPI_UserInput_GetPoint: its output struct carries a
	// `guid` field ("Guid of the clicked element"), APINULLGuid if the
	// click didn't land on one.
	API_GetPointType pointInfo = {};
	CHTruncate ("Click the start object", pointInfo.prompt, sizeof (pointInfo.prompt));
	GSErrCode err = ACAPI_UserInput_GetPoint (&pointInfo);
	if (err != NoError)
		return err;
	startHostGuid = pointInfo.guid;
	if (startHostGuid == APINULLGuid)
		return APIERR_BADPARS; // clicked empty space, not an element
	A2E_TRACE ("A2E: CreateWireBetweenObjectsCommand - got start object\n");

	pointInfo = {};
	CHTruncate ("Click the end object", pointInfo.prompt, sizeof (pointInfo.prompt));
	err = ACAPI_UserInput_GetPoint (&pointInfo);
	if (err != NoError)
		return err;
	endHostGuid = pointInfo.guid;
	if (endHostGuid == APINULLGuid)
		return APIERR_BADPARS;
	A2E_TRACE ("A2E: CreateWireBetweenObjectsCommand - got end object, calling ConnectObjectsWithGdlWire\n");

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

GSErrCode AboutCommand ()
{
	// Stands in for a "grayed out, always-visible version label" —
	// ACAPI_MenuItem_InstallMenuHandler doesn't offer a separate
	// enable/disable callback in the modern API (checked; it's a
	// resource-level mechanism instead, meant for window/context gating
	// like "only in 3D", not a permanent info label), so this is a
	// regular clickable item instead: shows the same build identifier
	// Initialize() already writes to the Report window on load, as a
	// dialog too (withDial=true) so it's visible without hunting for
	// that window.
	ACAPI_WriteReport (GS::UniString ("A2 Electrical — build " A2E_GIT_VERSION), true);
	return NoError;
}

} // namespace Commands
