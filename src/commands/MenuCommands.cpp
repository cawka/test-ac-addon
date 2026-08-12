#include "MenuCommands.hpp"

#include "ResourceIds.hpp"
#include "GitVersion.hpp"
#include "Debug.hpp"
#include "wiring/WireElement.hpp"
#include "wiring/WireConnection.hpp"
#include "circuit/CircuitSelection.hpp"
#include "circuit/CircuitProperty.hpp"

namespace Commands {

GSErrCode MenuCommandDispatcher::Handle (const API_MenuParams* menuParams)
{
	// Circuit property setup + observer install are database work and
	// must run inside an undoable command.
	GSErrCode status = ACAPI_CallUndoableCommand ("Create Property Group", [&]() -> GSErrCode {
		GSErrCode err = Circuit::PropertyManager::EnsureDefinition ();
		if (err != NoError)
			return err;
		return Wiring::ConnectionManager::RestoreAllConnectionObservers ();
	});
	if (status != NoError)
		return status;

	// Each command has its own menu resource (see ResourceIds.hpp), so
	// menuResID identifies the command directly.
	switch (menuParams->menuItemRef.menuResID) {
		case ID_ADDON_MENU_CREATE_WIRE:					return CreateWire ();
		case ID_ADDON_MENU_CONNECT_WIRE_ENDPOINT:			return ConnectWireEndpoint ();
		case ID_ADDON_MENU_CREATE_WIRE_BETWEEN_OBJECTS:	return CreateWireBetweenObjects ();
		case ID_ADDON_MENU_SELECT_CIRCUIT_WIRING:			return SelectCircuitWiring ();
		case ID_ADDON_MENU_SELECT_CIRCUIT_OBJECTS:			return SelectCircuitObjects ();
		case ID_ADDON_MENU_ABOUT:							return About ();
	}

	return NoError;
}

GSErrCode MenuCommandDispatcher::CreateWire ()
{
	return APIERR_NOTSUPPORTED;
}

GSErrCode MenuCommandDispatcher::ConnectWireEndpoint ()
{
	return APIERR_NOTSUPPORTED;
}

GSErrCode MenuCommandDispatcher::CreateWireBetweenObjects ()
{
	API_Guid startHostGuid = APINULLGuid;
	API_Guid endHostGuid = APINULLGuid;

	// ACAPI_UserInput_GetPoint's clicked-element guid is pointInfo.neig.guid,
	// not the struct's top-level .guid field (that one isn't populated
	// by this function). enableQuickSelection turns on Archicad's Quick
	// Selection (magnet) filter: live-highlights the candidate element
	// and resolves overlaps sensibly.
	API_GetPointType pointInfo = {};
	pointInfo.enableQuickSelection = true;
	CHTruncate ("Click the start object", pointInfo.prompt, sizeof (pointInfo.prompt));
	GSErrCode err = ACAPI_UserInput_GetPoint (&pointInfo);
	if (err != NoError)
		return err;
	startHostGuid = pointInfo.neig.guid;
	if (startHostGuid == APINULLGuid)
		return APIERR_BADPARS; // clicked empty space, not an element

	pointInfo = {};
	pointInfo.enableQuickSelection = true;
	CHTruncate ("Click the end object", pointInfo.prompt, sizeof (pointInfo.prompt));
	err = ACAPI_UserInput_GetPoint (&pointInfo);
	if (err != NoError)
		return err;
	endHostGuid = pointInfo.neig.guid;
	if (endHostGuid == APINULLGuid)
		return APIERR_BADPARS;

	// TODO: layer index hardcoded to "use placement default" — swap in
	// a real layer selection policy (active layer, a dedicated
	// "Circuit Wiring" layer, etc).
	const short wireLayerIndex = 0;

	// Element creation/property writes are database work and must run
	// inside an undoable command; the two GetPoint calls above are
	// interactive UI and deliberately outside it.
	API_Guid wireGuid = APINULLGuid;
	ACAPI_CallUndoableCommand ("Create Circuit Wire", [&]() -> GSErrCode {
		wireGuid = Wiring::ConnectionManager::ConnectObjectsWithGdlWire (startHostGuid, endHostGuid, wireLayerIndex);
		return (wireGuid != APINULLGuid) ? NoError : APIERR_GENERAL;
	});

	if (wireGuid == APINULLGuid)
		return APIERR_GENERAL;

	// Deliberately outside the undoable command above and non-fatal: a
	// notification subscription isn't project content, and losing live
	// move-tracking is preferable to discarding an otherwise-successful
	// wire creation.
	Wiring::ConnectionManager::AttachHostObserver (startHostGuid);
	Wiring::ConnectionManager::AttachHostObserver (endHostGuid);

	return NoError;
}

GSErrCode MenuCommandDispatcher::SelectCircuitWiring ()
{
	GS::UniString circuitId = Circuit::Selection::CircuitIdOfSelection ();
	if (circuitId.IsEmpty ())
		return APIERR_NOTMINE;

	return Circuit::Selection::SelectWiring (circuitId);
}

GSErrCode MenuCommandDispatcher::SelectCircuitObjects ()
{
	GS::UniString circuitId = Circuit::Selection::CircuitIdOfSelection ();
	if (circuitId.IsEmpty ())
		return APIERR_NOTMINE;

	return Circuit::Selection::SelectObjects (circuitId);
}

GSErrCode MenuCommandDispatcher::About ()
{
	// TODO: shows the Report window (withDial=true), not a small alert
	// dialog — no separate always-visible "grayed out" info label exists
	// in the modern menu API, so this is a regular clickable item.
	ACAPI_WriteReport (GS::UniString ("A2 Electrical — build " A2E_GIT_VERSION), true);
	return NoError;
}

} // namespace Commands
