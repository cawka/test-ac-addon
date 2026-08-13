#ifndef WIRING_WIRE_ELEMENT_HPP
#define WIRING_WIRE_ELEMENT_HPP

#include "ACAPinc.h"
#include "WireEnd.hpp"

#include <vector>

namespace wiring {

// A single curve node: position plus the incoming/outgoing Bezier
// direction vectors that give the wire its curvature at that node.
// Zero-length vectors mean a straight (non-curved) node.
struct WireNode
{
  API_Coord3D position;
  API_Vector3D dirIn;
  API_Vector3D dirOut;
};

// Native Spline-element wire backend, as an alternative to
// GdlWireElement's placed-object backend. Supports true multi-node
// curves; the GDL backend only ever has two endpoints.
//
// TODO: unimplemented stub. API_SplineType's node/direction storage is
// memo-backed (like polylines), not inline in API_Element -- fill in
// once the AC29 struct layout is confirmed.
class SplineWireElement
{
public:
  SplineWireElement() = delete;

  // Creates a wire through the given nodes. Returns the new element's
  // GUID, or APINULLGuid on failure.
  static API_Guid
  create(const std::vector<WireNode>& nodes, short layerIndex);

  // Replaces an existing wire's node geometry.
  static GSErrCode
  setNodes(const API_Guid& wireGuid, const std::vector<WireNode>& nodes);

  // Replaces just the Start or End node's position (interior nodes
  // untouched) -- the connection-tracking counterpart to
  // GdlWireElement::setEndpoint.
  static GSErrCode
  setEndpoint(const API_Guid& wireGuid, WireEnd end, const API_Coord& newPoint);

  // True if elemGuid is a Spline element created by this class.
  static bool
  isInstance(const API_Guid& elemGuid);
};

} // namespace wiring

#endif
