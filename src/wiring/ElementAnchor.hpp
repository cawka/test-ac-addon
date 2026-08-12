#ifndef WIRING_ELEMENT_ANCHOR_HPP
#define WIRING_ELEMENT_ANCHOR_HPP

#include "ACAPinc.h"

namespace wiring {

// Resolves the point a wire endpoint tracks on a host element. v1
// anchoring is "the host's own placement origin" — a wire endpoint
// connects to the whole object, not a specific hotspot/edge on it.
class ElementAnchor
{
public:
  ElementAnchor() = delete;

  // Returns elemGuid's placement origin in global 2D coordinates.
  // TODO: only API_ObjectID/API_LampID are handled — extend as wires
  // start connecting to other element types (walls, etc).
  static API_Coord getPoint(const API_Guid& elemGuid);
};

} // namespace wiring

#endif
