#ifndef WIRING_WIRE_ELEMENT_HPP
#define WIRING_WIRE_ELEMENT_HPP

#include "ACAPinc.h"
#include "WireEnd.hpp"
#include <vector>

namespace Wiring {

// A single curve node: position plus the incoming/outgoing Bezier
// direction vectors that give the wire its curvature at that node.
// Leave both vectors zero-length for a straight (non-curved) node.
struct WireNode {
	API_Coord3D		position;
	API_Vector3D	dirIn;
	API_Vector3D	dirOut;
};

// Creates a curved wire (native Spline element) through the given
// nodes on the given layer/floor. Returns the new element's GUID, or
// APINULLGuid on failure — check ACAPI_Element_Create's return code
// for the reason.
//
// DEVKIT: verify API_SplineType's exact field names/layout against the
// AC29 headers — this has been reshaped across releases (node array
// vs. polygon-style sub-element storage).
API_Guid CreateWire (const std::vector<WireNode>& nodes, short layerIndex);

// Replaces an existing wire's node geometry (endpoints moved because a
// connected host object moved) and pushes the change to the database.
GSErrCode SetWireNodes (const API_Guid& wireGuid, const std::vector<WireNode>& nodes);

// Convenience for the connection-tracking path (see WireConnection.*):
// reads the wire's current nodes, replaces just its first (Start) or
// last (End) node's position with newPoint (2D — z taken from the
// existing node, since connection-tracking only moves things in plan
// for now), and writes the result back via SetWireNodes. Interior
// nodes are left untouched.
GSErrCode SetWireEndpoint (const API_Guid& wireGuid, WireEnd end, const API_Coord& newPoint);

// True if elemGuid refers to an element this module created (checked
// via element type, not just existence).
bool IsWireElement (const API_Guid& elemGuid);

} // namespace Wiring

#endif
