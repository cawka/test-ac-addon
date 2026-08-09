#include "CircuitSelection.hpp"
#include "CircuitProperty.hpp"
#include "../Wiring/WireElement.hpp"

#include <vector>

namespace Circuit {

namespace {

// Shared scan: walk every element in the database, keep the ones whose
// Circuit ID matches and whose type matches `keepWires` (true: only
// Spline wires, false: everything except Spline wires), then select
// them.
//
// DEVKIT: ACAPI_Element_Filter / the AC29 equivalent for "iterate all
// elements in the current database" — replace this linear scan with
// whatever indexed criteria search AC29 offers (property-based find is
// usually far cheaper than a full element walk on large projects).
GSErrCode SelectByCircuitId (const GS::UniString& circuitId, bool keepWires)
{
	if (circuitId.IsEmpty ())
		return APIERR_BADPARS;

	GS::Array<API_Guid> allElements;
	// TODO: populate allElements, e.g. via ACAPI_Element_Filter over
	// API_ZombieElemID with an appropriate filter mask, or a project-wide
	// element list call — name to be confirmed against AC29 headers.

	std::vector<API_Neig> toSelect;
	for (const API_Guid& guid : allElements) {
		bool isWire = Wiring::IsWireElement (guid);
		if (isWire != keepWires)
			continue;

		if (GetCircuitId (guid) != circuitId)
			continue;

		API_Neig neig = {};
		neig.guid = guid;
		toSelect.push_back (neig);
	}

	if (toSelect.empty ())
		return NoError;

	// DEVKIT: ACAPI_Selection_Select's signature (wrap/replace flag,
	// API_Neig vs. API_Guid array) for AC29.
	return ACAPI_Selection_Select (toSelect, true);
}

} // namespace

GSErrCode SelectCircuitWiring (const GS::UniString& circuitId)
{
	return SelectByCircuitId (circuitId, /*keepWires=*/ true);
}

GSErrCode SelectCircuitObjects (const GS::UniString& circuitId)
{
	return SelectByCircuitId (circuitId, /*keepWires=*/ false);
}

GS::UniString CircuitIdOfSelection ()
{
	GS::Array<API_Neig> selection;
	// DEVKIT: ACAPI_Selection_Get's AC29 signature.
	if (ACAPI_Selection_Get (selection, nullptr, true) != NoError)
		return GS::UniString ();

	for (const API_Neig& neig : selection) {
		GS::UniString id = GetCircuitId (neig.guid);
		if (!id.IsEmpty ())
			return id;
	}

	return GS::UniString ();
}

} // namespace Circuit
