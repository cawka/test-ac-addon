#ifndef CIRCUIT_CIRCUIT_SELECTION_HPP
#define CIRCUIT_CIRCUIT_SELECTION_HPP

#include "ACAPinc.h"

namespace Circuit {

// Selection queries driven by Circuit ID membership (see PropertyManager).
class Selection {
public:
	Selection () = delete;

	// Selects every wire element carrying the given Circuit ID.
	static GSErrCode SelectWiring (const GS::UniString& circuitId);

	// Selects every non-wire element carrying the given Circuit ID —
	// i.e. everything on the circuit except the wiring itself.
	static GSErrCode SelectObjects (const GS::UniString& circuitId);

	// Reads the Circuit ID off whatever's currently selected (first
	// selected element that has one), for "select the rest of this
	// circuit" from a single click.
	static GS::UniString CircuitIdOfSelection ();

private:
	static GSErrCode SelectByCircuitId (const GS::UniString& circuitId, bool keepWires);
};

} // namespace Circuit

#endif
