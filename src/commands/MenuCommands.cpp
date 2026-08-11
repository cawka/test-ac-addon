#include "MenuCommands.hpp"

#include "ResourceIds.hpp"
#include "GitVersion.hpp"
#include "Debug.hpp"
#include "wiring/WireElement.hpp"
#include "wiring/WireConnection.hpp"
#include "circuit/CircuitSelection.hpp"
#include "circuit/CircuitProperty.hpp"

namespace Commands {

GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
	auto status = ACAPI_CallUndoableCommand ("Create Property Group", [&]() -> GSErrCode {
		// Our own setup, on top of the template:
		GSErrCode err = Circuit::EnsureCircuitPropertyDefinition ();
		A2E_TRACE ("A2E: Initialize - EnsureCircuitPropertyDefinition returned %d\n", (int) err);
		if (err != NoError)
			return err;

		err = Wiring::RestoreAllConnectionObservers ();
		A2E_TRACE ("A2E: Initialize - RestoreAllConnectionObservers returned %d\n", (int) err);
		return err;
	});
	A2E_TRACE ("A2E: MenuCommandHandler - CallUndoableCommand returned %d\n", (int) status);
	if (status != NoError) {
		return status;
	}

	A2E_TRACE ("A2E: MenuCommandHandler menuResID=%d itemIndex=%d\n",
		(int) menuParams->menuItemRef.menuResID, (int) menuParams->menuItemRef.itemIndex);

	// Each command has its own menu resource (see ResourceIds.hpp), so
	// menuResID identifies the command directly — itemIndex is always 2
	// (the one real item in that resource, after the "A2" title at [1]).
	switch (menuParams->menuItemRef.menuResID) {
		case ID_ADDON_MENU_CREATE_WIRE:					return CreateWireCommand ();
		case ID_ADDON_MENU_CONNECT_WIRE_ENDPOINT:			return ConnectWireEndpointCommand ();
		case ID_ADDON_MENU_CREATE_WIRE_BETWEEN_OBJECTS:	return CreateWireBetweenObjectsCommand ();
		case ID_ADDON_MENU_SELECT_CIRCUIT_WIRING:			return SelectCircuitWiringCommand ();
		case ID_ADDON_MENU_SELECT_CIRCUIT_OBJECTS:			return SelectCircuitObjectsCommand ();
		case ID_ADDON_MENU_ABOUT:							return AboutCommand ();
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
	// the real AC29 docs (Group: User Input) there isn't one. The
	// pattern is ACAPI_UserInput_GetPoint. CONFIRMED against the real
	// function doc (not just the struct's generic per-field doc, which
	// is what misled the previous version of this code): "the result is
	// returned in the pos and neig fields of pointInfo" — the top-level
	// pointInfo.guid is NOT what this function fills in, despite the
	// struct comment calling it "Guid of the clicked element" (that
	// comment describes the struct's use across other functions that
	// share it). The actual clicked-element guid is pointInfo.neig.guid
	// (API_Neig::guid, "Guid of the element"). This is why the start
	// click always fell through to "failed to get start object" even on
	// a dead-on click.
	API_GetPointType pointInfo = {};
	CHTruncate ("Click the start object", pointInfo.prompt, sizeof (pointInfo.prompt));
	GSErrCode err = ACAPI_UserInput_GetPoint (&pointInfo);
	if (err != NoError) {
		A2E_TRACE ("A2E: ACAPI_UserInput_GetPoint failed\n");
		return err;
	}
	startHostGuid = pointInfo.neig.guid;
	if (startHostGuid == APINULLGuid) {
		A2E_TRACE ("A2E: CreateWireBetweenObjectsCommand - failed to get start object\n");
		return APIERR_BADPARS; // clicked empty space, not an element
	}
	A2E_TRACE ("A2E: CreateWireBetweenObjectsCommand - got start object\n");

	pointInfo = {};
	CHTruncate ("Click the end object", pointInfo.prompt, sizeof (pointInfo.prompt));
	err = ACAPI_UserInput_GetPoint (&pointInfo);
	if (err != NoError) {
		A2E_TRACE ("A2E: ACAPI_UserInput_GetPoint failed\n");
		return err;
	}
	endHostGuid = pointInfo.neig.guid;
	if (endHostGuid == APINULLGuid) {
		A2E_TRACE ("A2E: CreateWireBetweenObjectsCommand - failed to get end object\n");
		return APIERR_BADPARS;
	}
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
