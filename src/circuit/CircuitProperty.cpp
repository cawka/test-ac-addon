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

	API_PropertyGroup group;
	group.groupType = API_PropertyCustomGroupType;
	group.name = kCircuitPropertyGroupName;

	err = ACAPI_Property_CreatePropertyGroup (group);
	A2E_TRACE ("A2E: EnsureCircuitPropertyGroup - CreatePropertyGroup returned %d\n", (int) err);
	if (err != NoError) {

		return err;
	}

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

	API_PropertyDefinition definition;
	definition.groupGuid      = circuitPropertyGroupGuid;
	definition.name           = kCircuitIdPropertyName;
	definition.collectionType = API_PropertySingleCollectionType;
	definition.valueType      = API_PropertyStringValueType;
	definition.measureType    = API_PropertyDefaultMeasureType;
	definition.definitionType = API_PropertyCustomDefinitionType;

	definition.canValueBeEditable = true;
	definition.defaultValue.basicValue.singleVariant.variant.type = API_PropertyStringValueType;

	// DEVKIT: availability ("The list of classification GUIDs this
	// property definition is available for") is left empty here.
	// A prior version of this code tried to replicate the Property
	// Manager UI's "All" option by enumerating every classification
	// system's root item (ACAPI_Classification_GetClassificationSystems
	// + ACAPI_Classification_GetClassificationSystemRootItems) -- that
	// was WRONG, confirmed directly by the user: it produced a narrow,
	// incomplete subset, nowhere close to what the UI's "All" actually
	// sets. The UI's "All" is a single toggle, not visibly an
	// enumeration, and the real underlying mechanism isn't confirmed
	// yet -- reverted rather than ship code known to do the wrong thing.
	// Until this is actually confirmed (real header/doc showing what
	// "All" writes into availability, or a working code example), set
	// classification availability by hand in Property Manager after
	// this creates the definition, same as the user is already doing.

	err = ACAPI_Property_CreatePropertyDefinition (definition);
	A2E_TRACE ("A2E: EnsureCircuitPropertyDefinition - CreatePropertyDefinition returned %d\n", (int) err);
	if (err != NoError)
		return err;

	circuitPropertyGuid = definition.guid;
	return NoError;
}

GS::UniString GenerateCircuitId ()
{
	// Was previously always APINULLGuid -- newGuid was declared and
	// never actually assigned a generated value, so every "new" circuit
	// ID was the same constant string. Confirmed real pattern (Graphisoft
	// community example): GS::Guid::Generate() is an instance method, not
	// a static factory.
	GS::Guid gsGuid;
	gsGuid.Generate ();
	API_Guid newGuid = GSGuid2APIGuid (gsGuid);
	return GS::UniString ("CKT-") + APIGuidToString (newGuid);
}

GS::UniString GetCircuitId (const API_Guid& elemGuid)
{
	if (EnsureCircuitPropertyDefinition () != NoError)
		return GS::UniString ();

	API_Property property = {};
	GSErrCode err = ACAPI_Element_GetPropertyValue (elemGuid, circuitPropertyGuid, property);
	A2E_TRACE ("A2E: GetCircuitId - ACAPI_Element_GetPropertyValue returned %d, isDefault=%d, variantStatus=%d\n",
		(int) err, (int) property.isDefault, (int) property.value.variantStatus);
	if (err != NoError)
		return GS::UniString ();

	if (property.isDefault || property.value.variantStatus != API_VariantStatusNormal)
		return GS::UniString ();

	return property.value.singleVariant.variant.uniStringValue;
}

GSErrCode SetCircuitId (const API_Guid& elemGuid, const GS::UniString& circuitId)
{
	GSErrCode err = EnsureCircuitPropertyDefinition ();
	A2E_TRACE ("A2E: SetCircuitId - EnsureCircuitPropertyDefinition returned %d\n", (int) err);
	if (err != NoError)
		return err;

	API_Property property = {};
	property.definition.guid = circuitPropertyGuid;
	property.isDefault = false;
	property.value.variantStatus = API_VariantStatusNormal;
	// variant is a tagged union -- .type must match which member is set
	// (mirrors EnsureCircuitPropertyDefinition's defaultValue, which sets
	// this correctly; this call didn't, leaving it zero-initialized).
	property.value.singleVariant.variant.type = API_PropertyStringValueType;
	property.value.singleVariant.variant.uniStringValue = circuitId;

	GS::Array<API_Guid> elemGuids;
	elemGuids.Push (elemGuid);
	err = ACAPI_Property_ModifyPropertyValue (property, elemGuids);
	A2E_TRACE ("A2E: SetCircuitId - ACAPI_Property_ModifyPropertyValue returned %d\n", (int) err);
	return err;
}

} // namespace Circuit
