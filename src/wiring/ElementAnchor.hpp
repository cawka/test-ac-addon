#ifndef WIRING_ELEMENT_ANCHOR_HPP
#define WIRING_ELEMENT_ANCHOR_HPP

#include "ACAPinc.h"

namespace Wiring {

// Returns elemGuid's own placement origin in global 2D coordinates.
// This is deliberately the *whole* anchoring model for now: a wire
// endpoint connects to "this object", full stop, tracking wherever its
// origin goes — not to a specific hotspot/edge on it. That matches
// "click an object" as the connect gesture, and it's enough for lines
// that only need to keep their endpoints attached, not hug a specific
// point on the host's outline.
//
// DEVKIT: API_Element's placement-origin field lives in a different
// union member per element type (element.object.pos for Objects and
// Lamps; other placed-library-part types such as windows/doors-in-wall
// don't have a standalone origin the same way and would need different
// handling). Only API_ObjectID/API_LampID are implemented below —
// extend the switch as you connect wires to other element types.
API_Coord GetElementAnchorPoint (const API_Guid& elemGuid);

} // namespace Wiring

#endif
