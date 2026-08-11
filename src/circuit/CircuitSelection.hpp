#ifndef CIRCUIT_CIRCUIT_SELECTION_HPP
#define CIRCUIT_CIRCUIT_SELECTION_HPP

#include "ACAPinc.h"

namespace Circuit {

// Selects every wire (Spline) element carrying the given Circuit ID.
GSErrCode SelectCircuitWiring (const GS::UniString& circuitId);

// Selects every non-wire element (Objects, Lamps, ...) carrying the
// given Circuit ID — i.e. everything on the circuit except the wiring
// itself.
GSErrCode SelectCircuitObjects (const GS::UniString& circuitId);

// Convenience: reads the Circuit ID off whatever's currently selected
// (first selected element that has one) and returns it, for driving
// "select the rest of this circuit" from a single click.
GS::UniString CircuitIdOfSelection ();

} // namespace Circuit

#endif
