#include "CircuitProperty.hpp"
#include "../Debug.hpp"

namespace Circuit {

namespace {

// Real GS::UniStrings, constructed once — not raw char[]s relying on an
// implicit, freshly-repeated conversion every time they're assigned
// (create) or compared against (lookup). Same fix already applied to
// kGdlWireLibPartName in GdlWireElement.hpp for the same reason.
const GS::UniString kCircuitPropertyGroupName ("A2 Electrical");
const GS::UniString kCircuitIdPropertyName ("Circuit ID");

// Cached once Ensure*() has run.
API_Guid circuitPropertyGroupGuid = APINULLGuid;
API_Guid circuitPropertyGuid = APINULLGuid;

// ACAPI_Property_CreatePropertyDefinition requires definition.groupGuid
// to already refer to a valid property group (confirmed real failure
// mode: APIERR_BADID, "The groupGuid of the parameter did not refer to
// a valid property group") — a zero-initialized guid never does. This
// was missing entirely; find-or-create it first, same pattern as the
// definition lookup below.
// DEVKIT: API_PropertyGroup's .name/.guid fields are inferred by
// analogy with API_PropertyDefinition's confirmed ones (and
// ACAPI_Property_GetPropertyGroup's doc: "[in/out] The guid field
// specifies the property group to retrieve") — not directly confirmed
// against the struct definition itself.
GSErrCode EnsureCircuitPropertyGroup ()
{
	if (circuitPropertyGroupGuid != APINULLGuid)
		return NoError;

	GS::Array<API_PropertyGroup> allGroups;
	GSErrCode err = ACAPI_Property_GetPropertyGroups (allGroups);
	A2E_TRACE ("A2E: EnsureCircuitPropertyGroup - GetPropertyGroups returned %d, count=%d\n",
		(int) err, (int) allGroups.GetSize ());
	if (err != NoError)
		return err;

	for (const API_PropertyGroup& existing : allGroups) {
		if (existing.name == kCircuitPropertyGroupName) {
			circuitPropertyGroupGuid = existing.guid;
			A2E_TRACE ("A2E: EnsureCircuitPropertyGroup - found existing group\n");
			return NoError;
		}
	}

	A2E_TRACE ("A2E: EnsureCircuitPropertyGroup - not found, creating\n");

	API_PropertyGroup group = {};
	group.name = kCircuitPropertyGroupName;

	err = ACAPI_Property_CreatePropertyGroup (group);
	A2E_TRACE ("A2E: EnsureCircuitPropertyGroup - CreatePropertyGroup returned %d\n", (int) err);
	if (err != NoError)
		return err;

	circuitPropertyGroupGuid = group.guid;
	return NoError;
}

} // namespace

GSErrCode EnsureCircuitPropertyDefinition ()
{
	if (circuitPropertyGuid != APINULLGuid)
		return NoError;

	GSErrCode err = EnsureCircuitPropertyGroup ();
	if (err != NoError)
		return err;

	// ACAPI_Property_GetPropertyDefinition only looks up by guid
	// (useless here, we don't have one yet), so the real lookup is
	// ACAPI_Property_GetPropertyDefinitions(groupGuid, ...) scoped to
	// our own group — filtered by name, only falling through to Create
	// when genuinely not found, so re-running this across sessions
	// doesn't hit APIERR_NAMEALREADYUSED.
	GS::Array<API_PropertyDefinition> allDefinitions;
	err = ACAPI_Property_GetPropertyDefinitions (circuitPropertyGroupGuid, allDefinitions);
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
	definition.groupGuid = circuitPropertyGroupGuid;
	definition.name = kCircuitIdPropertyName;
	definition.valueType = API_PropertyStringValueType;
	definition.collectionType = API_PropertySingleCollectionType;
	definition.measureType = API_PropertyDefaultMeasureType;

	err = ACAPI_Property_CreatePropertyDefinition (definition);
	A2E_TRACE ("A2E: EnsureCircuitPropertyDefinition - CreatePropertyDefinition returned %d\n", (int) err);
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
