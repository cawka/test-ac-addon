#ifndef WIRING_WIRE_END_HPP
#define WIRING_WIRE_END_HPP

namespace Wiring {

// Which end of a two-endpoint connection this is. Both wire backends
// (native Spline in WireElement.*, GDL object in GdlWireElement.*) only
// ever track their first and last point this way — a Spline's interior
// nodes stay free-floating/manually edited, and a GdlWire only ever has
// two points at all.
enum class WireEnd { Start, End };

} // namespace Wiring

#endif
