#include "WireConnection.hpp"
#include "WireElement.hpp"
#include "../Circuit/CircuitProperty.hpp"

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

	// DEVKIT: ACAPI_Notification_InstallElementObserver — confirm
	// signature (callback type, whether it takes a context pointer) in
	// ACAPI_NotificationProcedures.hpp for AC29.
	err = ACAPI_Notification_InstallElementObserver (&info.hostGuid);
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
		// DEVKIT: only uninstall the observer once no wire references
		// this host anymore — ACAPI_Notification_UninstallElementObserver.
		ACAPI_Notification_UninstallElementObserver (&info.hostGuid);
	}

	return StoreConnection (wireGuid, end, ConnectionInfo {});
}

GSErrCode RestoreAllConnectionObservers ()
{
	// TODO: enumerate all wire elements (ACAPI_Element_Filter over
	// API_SplineID), load each one's stored connections, and call
	// ACAPI_Notification_InstallElementObserver for each distinct host
	// GUID found. Needed so connections survive project reload, since
	// observers are a runtime-only registration.
	return NoError;
}

GSErrCode __ACENV_CALL OnHostElementChanged (const API_Guid& elemGuid, API_NotifyElementType /*notifType*/)
{
	auto it = hostToWires.find (APIGuid2GSGuid (elemGuid));
	if (it == hostToWires.end ())
		return NoError;

	for (const auto& [wireGuid, end] : it->second) {
		ConnectionInfo info = LoadConnection (wireGuid, end);

		// TODO: resolve info.hotspotIndex against the host's current
		// placement/transformation to get the new anchor point, then
		// fetch the wire's current nodes, replace the affected
		// endpoint, and call WireElement::SetWireNodes.
		(void) info;
	}

	return NoError;
}

} // namespace Wiring
