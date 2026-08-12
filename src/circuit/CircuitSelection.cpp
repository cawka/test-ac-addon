#include "CircuitSelection.hpp"

#include "../wiring/WireElement.hpp"
#include "CircuitProperty.hpp"

namespace circuit {

// TODO: replace this linear element scan with an indexed/criteria-based
// search once available (property-based find is far cheaper than a
// full element walk on large projects).
GSErrCode
Selection::selectByCircuitId(const GS::UniString& circuitId, bool keepWires)
{
  if (circuitId.IsEmpty())
    return APIERR_BADPARS;

  GS::Array<API_Guid> allElements;
  // TODO: populate allElements (e.g. ACAPI_Element_Filter over all
  // elements in the current database).

  GS::Array<API_Neig> toSelect;
  for (const API_Guid& guid : allElements) {
    bool isWire = wiring::SplineWireElement::isInstance(guid);
    if (isWire != keepWires)
      continue;

    if (PropertyManager::getId(guid) != circuitId)
      continue;

    API_Neig neig = {};
    neig.guid = guid;
    toSelect.Push(neig);
  }

  if (toSelect.IsEmpty())
    return NoError;

  return ACAPI_Selection_Select(toSelect, true);
}

GSErrCode
Selection::selectWiring(const GS::UniString& circuitId)
{
  return selectByCircuitId(circuitId, /*keepWires=*/true);
}

GSErrCode
Selection::selectObjects(const GS::UniString& circuitId)
{
  return selectByCircuitId(circuitId, /*keepWires=*/false);
}

GS::UniString
Selection::getCircuitIdOfSelection()
{
  API_SelectionInfo selectionInfo = {};
  GS::Array<API_Neig> selection;

  if (ACAPI_Selection_Get(&selectionInfo, &selection, true) != NoError)
    return GS::UniString();

  for (const API_Neig& neig : selection) {
    GS::UniString id = PropertyManager::getId(neig.guid);
    if (!id.IsEmpty())
      return id;
  }

  return GS::UniString();
}

} // namespace circuit
