#include "CircuitSelection.hpp"
#include "CircuitProperty.hpp"
#include "../wiring/WireElement.hpp"

namespace Circuit {

// TODO: replace this linear element scan with an indexed/criteria-based
// search once available (property-based find is far cheaper than a
// full element walk on large projects).
GSErrCode Selection::SelectByCircuitId (const GS::UniString& circuitId, bool keepWires)
{
	if (circuitId.IsEmpty ())
		return APIERR_BADPARS;

	GS::Array<API_Guid> allElements;
	// TODO: populate allElements (e.g. ACAPI_Element_Filter over all
	// elements in the current database).

	GS::Array<API_Neig> toSelect;
	for (const API_Guid& guid : allElements) {
		bool isWire = Wiring::SplineWireElement::IsInstance (guid);
		if (isWire != keepWires)
			continue;

		if (PropertyManager::GetId (guid) != circuitId)
			continue;

		API_Neig neig = {};
		neig.guid = guid;
		toSelect.Push (neig);
	}

	if (toSelect.IsEmpty ())
		return NoError;

	return ACAPI_Selection_Select (toSelect, true);
}

GSErrCode Selection::SelectWiring (const GS::UniString& circuitId)
{
	return SelectByCircuitId (circuitId, /*keepWires=*/ true);
}

GSErrCode Selection::SelectObjects (const GS::UniString& circuitId)
{
	return SelectByCircuitId (circuitId, /*keepWires=*/ false);
}

GS::UniString Selection::CircuitIdOfSelection ()
{
	API_SelectionInfo selectionInfo = {};
	GS::Array<API_Neig> selection;

	if (ACAPI_Selection_Get (&selectionInfo, &selection, true) != NoError)
		return GS::UniString ();

	for (const API_Neig& neig : selection) {
		GS::UniString id = PropertyManager::GetId (neig.guid);
		if (!id.IsEmpty ())
			return id;
	}

	return GS::UniString ();
}

} // namespace Circuit
