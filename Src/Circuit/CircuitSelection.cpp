#include "CircuitSelection.hpp"
#include "CircuitProperty.hpp"
#include "../Wiring/WireElement.hpp"

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

	GS::Array<API_Neig> toSelect;
	for (const API_Guid& guid : allElements) {
		bool isWire = Wiring::IsWireElement (guid);
		if (isWire != keepWires)
			continue;

		if (GetCircuitId (guid) != circuitId)
			continue;

		API_Neig neig = {};
		neig.guid = guid;
		toSelect.Push (neig);
	}

	if (toSelect.IsEmpty ())
		return NoError;

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
	API_SelectionInfo selectionInfo = {};
	GS::Array<API_Neig> selection;

	GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selection, true);
	// DEVKIT: API_SelectionInfo may carry a handle (e.g. a marquee
	// coordinate list) that needs disposing — check its fields once
	// convenient; not used here since we only read `selection`.
	if (err != NoError)
		return GS::UniString ();

	for (const API_Neig& neig : selection) {
		GS::UniString id = GetCircuitId (neig.guid);
		if (!id.IsEmpty ())
			return id;
	}

	return GS::UniString ();
}

} // namespace Circuit
