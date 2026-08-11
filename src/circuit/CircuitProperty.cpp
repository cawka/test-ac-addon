#include "CircuitProperty.hpp"

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

	// DEVKIT: look the definition up first (e.g.
	// ACAPI_Property_GetPropertyDefinition by name/group) so re-running
	// this across sessions doesn't create duplicate definitions; only
	// fall through to ACAPI_Property_CreatePropertyDefinition when the
	// lookup comes back empty. Left as a single create call here
	// pending that lookup API's exact AC29 signature.

	API_PropertyDefinition definition = {};
	definition.name = kCircuitIdPropertyName;
	definition.valueType = API_PropertyStringValueType;
	definition.collectionType = API_PropertySingleCollectionType;
	definition.measureType = API_PropertyDefaultMeasureType;

	GSErrCode err = ACAPI_Property_CreatePropertyDefinition (definition);
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
