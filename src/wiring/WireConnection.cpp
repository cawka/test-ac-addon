#include "WireConnection.hpp"

#include "../Debug.hpp"
#include "../circuit/CircuitProperty.hpp"
#include "ElementAnchor.hpp"
#include "GdlWireElement.hpp"
#include "WireElement.hpp"

#include <algorithm>

namespace wiring {

ConnectionInfo
ConnectionManager::loadConnection(const API_Guid& /*wireGuid*/, WireEnd /*end*/)
{
  // TODO: read back from wherever storeConnection wrote to.
  return {};
}

GSErrCode
ConnectionManager::storeConnection(const API_Guid& /*wireGuid*/,
                                   WireEnd /*end*/,
                                   const ConnectionInfo& /*info*/)
{
  // TODO: persist into the wire element (memo extra segment, or a
  // private property) so connections survive save/reload.
  return NoError;
}

GSErrCode
ConnectionManager::connect(const API_Guid& wireGuid, WireEnd end, const ConnectionInfo& info)
{
  if (info.hostGuid == APINULLGuid)
    return APIERR_BADPARS;

  GSErrCode err = storeConnection(wireGuid, end, info);
  if (err != NoError)
    return err;

  // Share (or create) the Circuit ID across wire + host.
  GS::UniString circuitId = circuit::PropertyManager::getId(info.hostGuid);
  if (circuitId.IsEmpty())
    circuitId = circuit::PropertyManager::getId(wireGuid);
  if (circuitId.IsEmpty())
    circuitId = circuit::PropertyManager::generateId();

  err = circuit::PropertyManager::setId(wireGuid, circuitId);
  if (err != NoError)
    return err;
  err = circuit::PropertyManager::setId(info.hostGuid, circuitId);
  if (err != NoError)
    return err;

  s_hostToWires[APIGuid2GSGuid(info.hostGuid)].push_back({wireGuid, end});
  return NoError;
}

GSErrCode
ConnectionManager::attachHostObserver(const API_Guid& hostGuid)
{
  // NOTE: called with notifyFlags left at its default (0); not
  // confirmed whether that's a valid mask for this element type.
  GSErrCode err = ACAPI_Element_AttachObserver(hostGuid);
  A2E_TRACE("A2E: ConnectionManager::attachHostObserver returned %d\n", (int) err);
  return err;
}

GSErrCode
ConnectionManager::disconnect(const API_Guid& wireGuid, WireEnd end)
{
  ConnectionInfo info = loadConnection(wireGuid, end);
  if (info.hostGuid == APINULLGuid)
    return NoError;

  auto& wires = s_hostToWires[APIGuid2GSGuid(info.hostGuid)];
  wires.erase(std::remove_if(
                wires.begin(),
                wires.end(),
                [&](const auto& entry) { return entry.first == wireGuid && entry.second == end; }),
              wires.end());

  if (wires.empty()) {
    s_hostToWires.erase(APIGuid2GSGuid(info.hostGuid));
    ACAPI_Element_DetachObserver(info.hostGuid);
  }

  return storeConnection(wireGuid, end, ConnectionInfo{});
}

API_Guid
ConnectionManager::connectObjectsWithGdlWire(const API_Guid& startHostGuid,
                                             const API_Guid& endHostGuid,
                                             short layerIndex)
{
  if (startHostGuid == APINULLGuid || endHostGuid == APINULLGuid || startHostGuid == endHostGuid)
    return APINULLGuid;

  const API_Coord startPoint = ElementAnchor::getPoint(startHostGuid);
  const API_Coord endPoint = ElementAnchor::getPoint(endHostGuid);

  API_Guid wireGuid = GdlWireElement::create(startPoint, endPoint, layerIndex);
  if (wireGuid == APINULLGuid)
    return APINULLGuid;

  GSErrCode err = connect(wireGuid, WireEnd::Start, ConnectionInfo{startHostGuid});
  if (err == NoError)
    err = connect(wireGuid, WireEnd::End, ConnectionInfo{endHostGuid});

  if (err != NoError) {
    // TODO: delete the half-connected wire (ACAPI_Element_Delete) so
    // a failed connect doesn't leave a stray element behind.
    return APINULLGuid;
  }

  return wireGuid;
}

GSErrCode
ConnectionManager::restoreAllConnectionObservers()
{
  // TODO: enumerate all wire elements (both backends), load each
  // one's stored connections, and call attachHostObserver for each
  // distinct host GUID found.
  return NoError;
}

GSErrCode
ConnectionManager::onHostElementChanged(const API_NotifyElementType* elemType)
{
  const API_Guid elemGuid = elemType->elemHead.guid;

  auto it = s_hostToWires.find(APIGuid2GSGuid(elemGuid));
  if (it == s_hostToWires.end())
    return NoError;

  const API_Coord newAnchor = ElementAnchor::getPoint(elemGuid);

  for (const auto& [wireGuid, end] : it->second) {
    if (GdlWireElement::isInstance(wireGuid)) {
      GdlWireElement::setEndpoint(wireGuid, end, newAnchor);
    }
    else if (SplineWireElement::isInstance(wireGuid)) {
      SplineWireElement::setEndpoint(wireGuid, end, newAnchor);
    }
  }

  return NoError;
}

} // namespace wiring
