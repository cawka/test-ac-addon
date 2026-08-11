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
	if (err != NoError)
		return err;

	// Share (or create) the Circuit ID across wire + host.
	GS::UniString circuitId = Circuit::GetCircuitId (info.hostGuid);
	if (circuitId.IsEmpty ())
		circuitId = Circuit::GetCircuitId (wireGuid);
	if (circuitId.IsEmpty ())
		circuitId = Circuit::GenerateCircuitId ();

	err = Circuit::SetCircuitId (wireGuid, circuitId);
	if (err != NoError)
		return err;
	err = Circuit::SetCircuitId (info.hostGuid, circuitId);
	if (err != NoError)
		return err;

	// Per-element attach — pairs with the one-time global handler install
	// via ACAPI_Element_InstallElementObserver (see
	// AddOnMain.cpp/RestoreAllConnectionObservers; that side still needs
	// API_NotifyElementType's fields confirmed before it's wired up).
	err = ACAPI_Element_AttachObserver (info.hostGuid);
	if (err != NoError)
		return err;

	hostToWires[APIGuid2GSGuid (info.hostGuid)].push_back ({ wireGuid, end });

	return NoError;
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
	GSErrCode err = ACAPI_Element_InstallElementObserver (&OnHostElementChanged);
	if (err != NoError)
		return err;

	// TODO: enumerate all wire elements — both native Splines
	// (API_SplineID) and placed "Circuit Wire" objects
	// (GdlWireElement::IsGdlWireElement) — load each one's stored
	// connections, and call ACAPI_Element_AttachObserver for each
	// distinct host GUID found. Needed so connections survive project
	// reload, since attaching (unlike the install above, which only
	// needs to happen once ever) is per-element and runtime-only.
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
