#include "CircuitProperty.hpp"
#include "../Debug.hpp"

namespace Circuit {

namespace {

constexpr char kCircuitIdPropertyName[] = "Circuit ID";

// Cached once EnsureCircuitPropertyDefinition has run.
API_Guid circuitPropertyGuid = APINULLGuid;

} // namespace

GSErrCode EnsureCircuitPropertyDefinition ()
{
	if (circuitPropertyGuid != APINULLGuid)
		return NoError;

	// Confirmed real bug, not a guess: ACAPI_Property_CreatePropertyDefinition
	// fails with APIERR_NAMEALREADYUSED ("The name of the definition is
	// already used in the given property group") on every load after the
	// first, because this used to call Create unconditionally with no
	// lookup — that error was aborting the rest of Initialize() on every
	// subsequent add-on load. ACAPI_Property_GetPropertyDefinition only
	// looks up by guid (useless here, we don't have one yet), so the
	// real lookup is ACAPI_Property_GetPropertyDefinitions(APINULLGuid,
	// ...) — "all property definitions" — filtered by name.
	GS::Array<API_PropertyDefinition> allDefinitions;
	GSErrCode err = ACAPI_Property_GetPropertyDefinitions (APINULLGuid, allDefinitions);
	A2E_TRACE ("A2E: EnsureCircuitPropertyDefinition - GetPropertyDefinitions returned %d, count=%d\n",
		(int) err, (int) allDefinitions.GetSize ());
	if (err != NoError)
		return err;

	for (const API_PropertyDefinition& existing : allDefinitions) {
		bool isMatch = (existing.name == kCircuitIdPropertyName);
		A2E_TRACE ("A2E: EnsureCircuitPropertyDefinition - existing def, nameLen=%d, isMatch=%d\n",
			(int) existing.name.GetLength (), (int) isMatch);
		if (isMatch) {
			circuitPropertyGuid = existing.guid;
			A2E_TRACE ("A2E: EnsureCircuitPropertyDefinition - found existing definition\n");
			return NoError;
		}
	}

	A2E_TRACE ("A2E: EnsureCircuitPropertyDefinition - not found, creating\n");

	API_PropertyDefinition definition = {};
	definition.name = kCircuitIdPropertyName;
	definition.valueType = API_PropertyStringValueType;
	definition.collectionType = API_PropertySingleCollectionType;
	definition.measureType = API_PropertyDefaultMeasureType;

	err = ACAPI_Property_CreatePropertyDefinition (definition);
	if (err != NoError)
		return err;

	circuitPropertyGuid = definition.guid;
	return NoError;
}

GS::UniString GenerateCircuitId ()
{
	API_Guid newGuid = APINULLGuid;
	// DEVKIT: confirm the AC29 GUID-generation helper name
	// (GSGuid2APIGuid (GS::Guid::Generate ()) has been the historical
	// pattern; APIGuidFromString equivalents also exist).
	return GS::UniString ("CKT-") + APIGuidToString (newGuid);
}

GS::UniString GetCircuitId (const API_Guid& elemGuid)
{
	if (EnsureCircuitPropertyDefinition () != NoError)
		return GS::UniString ();

	API_Property property = {};
	if (ACAPI_Element_GetPropertyValue (elemGuid, circuitPropertyGuid, property) != NoError)
		return GS::UniString ();

	if (property.isDefault || property.value.variantStatus != API_VariantStatusNormal)
		return GS::UniString ();

	return property.value.singleVariant.variant.uniStringValue;
}

GSErrCode SetCircuitId (const API_Guid& elemGuid, const GS::UniString& circuitId)
{
	GSErrCode err = EnsureCircuitPropertyDefinition ();
	if (err != NoError)
		return err;

	API_Property property = {};
	property.definition.guid = circuitPropertyGuid;
	property.isDefault = false;
	property.value.variantStatus = API_VariantStatusNormal;
	property.value.singleVariant.variant.uniStringValue = circuitId;

	GS::Array<API_Guid> elemGuids;
	elemGuids.Push (elemGuid);
	return ACAPI_Property_ModifyPropertyValue (property, elemGuids);
}

} // namespace Circuit
