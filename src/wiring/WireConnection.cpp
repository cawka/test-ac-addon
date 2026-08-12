#include "WireConnection.hpp"
#include "WireElement.hpp"
#include "GdlWireElement.hpp"
#include "ElementAnchor.hpp"
#include "../Debug.hpp"
#include "../circuit/CircuitProperty.hpp"

#include <algorithm>

namespace Wiring {

ConnectionInfo ConnectionManager::LoadConnection (const API_Guid& /*wireGuid*/, WireEnd /*end*/)
{
	// TODO: read back from wherever StoreConnection wrote to.
	return {};
}

GSErrCode ConnectionManager::StoreConnection (const API_Guid& /*wireGuid*/, WireEnd /*end*/, const ConnectionInfo& /*info*/)
{
	// TODO: persist into the wire element (memo extra segment, or a
	// private property) so connections survive save/reload.
	return NoError;
}

GSErrCode ConnectionManager::Connect (const API_Guid& wireGuid, WireEnd end, const ConnectionInfo& info)
{
	if (info.hostGuid == APINULLGuid)
		return APIERR_BADPARS;

	GSErrCode err = StoreConnection (wireGuid, end, info);
	if (err != NoError)
		return err;

	// Share (or create) the Circuit ID across wire + host.
	GS::UniString circuitId = Circuit::PropertyManager::GetId (info.hostGuid);
	if (circuitId.IsEmpty ())
		circuitId = Circuit::PropertyManager::GetId (wireGuid);
	if (circuitId.IsEmpty ())
		circuitId = Circuit::PropertyManager::GenerateId ();

	err = Circuit::PropertyManager::SetId (wireGuid, circuitId);
	if (err != NoError)
		return err;
	err = Circuit::PropertyManager::SetId (info.hostGuid, circuitId);
	if (err != NoError)
		return err;

	hostToWires[APIGuid2GSGuid (info.hostGuid)].push_back ({ wireGuid, end });
	return NoError;
}

GSErrCode ConnectionManager::AttachHostObserver (const API_Guid& hostGuid)
{
	// NOTE: called with notifyFlags left at its default (0); not
	// confirmed whether that's a valid mask for this element type.
	GSErrCode err = ACAPI_Element_AttachObserver (hostGuid);
	A2E_TRACE ("A2E: ConnectionManager::AttachHostObserver returned %d\n", (int) err);
	return err;
}

GSErrCode ConnectionManager::Disconnect (const API_Guid& wireGuid, WireEnd end)
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

API_Guid ConnectionManager::ConnectObjectsWithGdlWire (const API_Guid& startHostGuid, const API_Guid& endHostGuid, short layerIndex)
{
	if (startHostGuid == APINULLGuid || endHostGuid == APINULLGuid || startHostGuid == endHostGuid)
		return APINULLGuid;

	const API_Coord startPoint = ElementAnchor::GetPoint (startHostGuid);
	const API_Coord endPoint = ElementAnchor::GetPoint (endHostGuid);

	API_Guid wireGuid = GdlWireElement::Create (startPoint, endPoint, layerIndex);
	if (wireGuid == APINULLGuid)
		return APINULLGuid;

	GSErrCode err = Connect (wireGuid, WireEnd::Start, ConnectionInfo { startHostGuid });
	if (err == NoError)
		err = Connect (wireGuid, WireEnd::End, ConnectionInfo { endHostGuid });

	if (err != NoError) {
		// TODO: delete the half-connected wire (ACAPI_Element_Delete) so
		// a failed connect doesn't leave a stray element behind.
		return APINULLGuid;
	}

	return wireGuid;
}

GSErrCode ConnectionManager::RestoreAllConnectionObservers ()
{
	// TODO: enumerate all wire elements (both backends), load each
	// one's stored connections, and call AttachHostObserver for each
	// distinct host GUID found.
	return NoError;
}

GSErrCode ConnectionManager::OnHostElementChanged (const API_NotifyElementType* elemType)
{
	const API_Guid elemGuid = elemType->elemHead.guid;

	auto it = hostToWires.find (APIGuid2GSGuid (elemGuid));
	if (it == hostToWires.end ())
		return NoError;

	const API_Coord newAnchor = ElementAnchor::GetPoint (elemGuid);

	for (const auto& [wireGuid, end] : it->second) {
		if (GdlWireElement::IsInstance (wireGuid)) {
			GdlWireElement::SetEndpoint (wireGuid, end, newAnchor);
		} else if (SplineWireElement::IsInstance (wireGuid)) {
			SplineWireElement::SetEndpoint (wireGuid, end, newAnchor);
		}
	}

	return NoError;
}

} // namespace Wiring
