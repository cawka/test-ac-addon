#include "WireConnection.hpp"
#include "WireElement.hpp"
#include "GdlWireElement.hpp"
#include "ElementAnchor.hpp"
#include "../Debug.hpp"
#include "../circuit/CircuitProperty.hpp"

#include <algorithm>
#include <map>
#include <vector>

namespace Wiring {

namespace {

// In-memory index: host GUID -> wires anchored to it. Rebuilt from the
// persisted per-wire connection data (see StoreConnection/LoadConnections
// below) at add-on init, so this cache is never itself the source of
// truth.
//
// DEVKIT: decide the persistence mechanism before relying on this in
// production. Two options, either fine for a skeleton:
//   (a) store ConnectionInfo in the wire element's own memo "extra"
//       segment (ACAPI_Element_SetMemo with API_ElemMemoMask_Extra), or
//   (b) a private/hidden property on the wire holding host GUID text.
// (a) is more robust against copy/paste duplicating stale GUIDs; (b) is
// easier to inspect/debug. Not decided here — see TODOs below.
std::map<GS::Guid, std::vector<std::pair<API_Guid, WireEnd>>> hostToWires;

ConnectionInfo LoadConnection (const API_Guid& /*wireGuid*/, WireEnd /*end*/)
{
	// TODO: read back from wherever StoreConnection wrote to.
	return {};
}

GSErrCode StoreConnection (const API_Guid& /*wireGuid*/, WireEnd /*end*/, const ConnectionInfo& /*info*/)
{
	// TODO: persist into the wire element (memo extra segment or a
	// private property — see note on hostToWires above).
	return NoError;
}

} // namespace

GSErrCode Connect (const API_Guid& wireGuid, WireEnd end, const ConnectionInfo& info)
{
	if (info.hostGuid == APINULLGuid)
		return APIERR_BADPARS;

	GSErrCode err = StoreConnection (wireGuid, end, info);
	A2E_TRACE ("A2E: Connect - StoreConnection returned %d\n", (int) err);
	if (err != NoError)
		return err;

	// Share (or create) the Circuit ID across wire + host.
	GS::UniString circuitId = Circuit::GetCircuitId (info.hostGuid);
	A2E_TRACE ("A2E: Connect - GetCircuitId(host) len=%d\n", (int) circuitId.GetLength ());
	if (circuitId.IsEmpty ())
		circuitId = Circuit::GetCircuitId (wireGuid);
	A2E_TRACE ("A2E: Connect - GetCircuitId(wire) len=%d\n", (int) circuitId.GetLength ());
	if (circuitId.IsEmpty ())
		circuitId = Circuit::GenerateCircuitId ();
	A2E_TRACE ("A2E: Connect - circuitId len=%d\n", (int) circuitId.GetLength ());

	err = Circuit::SetCircuitId (wireGuid, circuitId);
	A2E_TRACE ("A2E: Connect - SetCircuitId(wire) returned %d\n", (int) err);
	if (err != NoError)
		return err;
	err = Circuit::SetCircuitId (info.hostGuid, circuitId);
	A2E_TRACE ("A2E: Connect - SetCircuitId(host) returned %d\n", (int) err);
	if (err != NoError)
		return err;

	// Observer attach moved out to AttachHostObserver (see header) —
	// call it separately, after whatever undoable command wraps this
	// Connect() call has committed.
	hostToWires[APIGuid2GSGuid (info.hostGuid)].push_back ({ wireGuid, end });

	return NoError;
}

GSErrCode AttachHostObserver (const API_Guid& hostGuid)
{
	// Was a KNOWN GAP: this consistently failed (real error -2130312994
	// / 0x810600de -- confirmed via the actual DevKit header to be
	// neither of the two hypotheses tried at the time: not an
	// undo-context issue, since it still failed after moving this call
	// outside ACAPI_CallUndoableCommand entirely; and not APIERR_BADID
	// despite being the only return code the header documents for this
	// function, since APIERR_BADID's real value (APIErrorStart + 101 =
	// 0x81060065) doesn't match 0x810600de).
	//
	// Leading fix, not yet confirmed by a real test: the global handler
	// (ACAPI_Element_InstallElementObserver) was only ever being
	// installed from RestoreAllConnectionObservers, itself only called
	// from inside MenuCommandHandler's ACAPI_CallUndoableCommand on a
	// menu click -- never from a clean lifecycle hook. Moved to
	// AddOnMain.cpp's Initialize() instead, which is architecturally
	// correct for a notification registration (not a database write, no
	// undo context needed) and is the standard place real add-ons do
	// this. If AttachObserver still fails after this, notifyFlags being
	// left at its default (GSFlags notifyFlags = 0) is the next suspect
	// -- the real valid flag constants for that parameter still aren't
	// confirmed.
	GSErrCode err = ACAPI_Element_AttachObserver (hostGuid);
	A2E_TRACE ("A2E: AttachHostObserver - AttachObserver returned %d\n", (int) err);
	return err;
}

GSErrCode Disconnect (const API_Guid& wireGuid, WireEnd end)
{
	ConnectionInfo info = LoadConnection (wireGuid, end);
	if (info.hostGuid == APINULLGuid)
		return NoError;

	auto& wires = hostToWires[APIGuid2GSGuid (info.hostGuid)];
	wires.erase (std::remove_if (wires.begin (), wires.end (),
		[&] (const auto& entry) { return entry.first == wireGuid && entry.second == end; }),
		wires.end ());

	if (wires.empty ()) {
		hostToWires.erase (APIGuid2GSGuid (info.hostGuid));
		ACAPI_Element_DetachObserver (info.hostGuid);
	}

	return StoreConnection (wireGuid, end, ConnectionInfo {});
}

API_Guid ConnectObjectsWithGdlWire (const API_Guid& startHostGuid, const API_Guid& endHostGuid, short layerIndex)
{
	if (startHostGuid == APINULLGuid || endHostGuid == APINULLGuid || startHostGuid == endHostGuid)
		return APINULLGuid;

	const API_Coord startPoint = GetElementAnchorPoint (startHostGuid);
	const API_Coord endPoint = GetElementAnchorPoint (endHostGuid);

	API_Guid wireGuid = CreateGdlWire (startPoint, endPoint, layerIndex);
	if (wireGuid == APINULLGuid) {
		A2E_TRACE ("A2E: ConnectObjectsWithGdlWire - CreateGdlWire failed\n");
		return APINULLGuid;
	}
	A2E_TRACE ("A2E: ConnectObjectsWithGdlWire - wire created, connecting both ends\n");

	GSErrCode err = Connect (wireGuid, WireEnd::Start, ConnectionInfo { startHostGuid });
	if (err == NoError)
		err = Connect (wireGuid, WireEnd::End, ConnectionInfo { endHostGuid });
	A2E_TRACE ("A2E: ConnectObjectsWithGdlWire - Connect calls returned %d\n", (int) err);

	if (err != NoError) {
		// DEVKIT: delete the half-connected wire (ACAPI_Element_Delete)
		// so a failed connect doesn't leave a stray element behind.
		return APINULLGuid;
	}

	return wireGuid;
}

GSErrCode RestoreAllConnectionObservers ()
{
	// The global handler install used to happen here (guarded, since
	// this only ever ran from inside MenuCommandHandler's
	// ACAPI_CallUndoableCommand on a menu click). Moved to
	// AddOnMain.cpp's Initialize() instead — a notification registration
	// isn't a database write and doesn't belong inside undo context, and
	// running it from the wrong place was the leading suspect for
	// ACAPI_Element_AttachObserver's persistent failure (see the KNOWN
	// GAP note on AttachHostObserver below).
	//
	// TODO: enumerate all wire elements — both native Splines
	// (API_SplineID) and placed "Circuit Wire" objects
	// (GdlWireElement::IsGdlWireElement) — load each one's stored
	// connections, and call ACAPI_Element_AttachObserver for each
	// distinct host GUID found. Needed so connections survive project
	// reload, since per-element attaching is runtime-only (unlike the
	// global install, which only needs to happen once ever, now handled
	// in Initialize()).
	return NoError;
}

GSErrCode OnHostElementChanged (const API_NotifyElementType* elemType)
{
	const API_Guid elemGuid = elemType->elemHead.guid;

	auto it = hostToWires.find (APIGuid2GSGuid (elemGuid));
	if (it == hostToWires.end ())
		return NoError;

	const API_Coord newAnchor = GetElementAnchorPoint (elemGuid);

	for (const auto& [wireGuid, end] : it->second) {
		if (IsGdlWireElement (wireGuid)) {
			SetGdlWireEndpoint (wireGuid, end, newAnchor);
		} else if (IsWireElement (wireGuid)) {
			SetWireEndpoint (wireGuid, end, newAnchor);
		}
	}

	return NoError;
}

} // namespace Wiring
