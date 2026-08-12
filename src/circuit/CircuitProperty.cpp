#include "CircuitProperty.hpp"
#include "../Debug.hpp"

namespace Circuit {

void PropertyManager::CollectClassificationItem (const API_Guid& itemGuid, GS::Array<API_Guid>& outGuids)
{
	outGuids.Push (itemGuid);

	GS::Array<API_ClassificationItem> children;
	if (ACAPI_Classification_GetClassificationItemChildren (itemGuid, children) != NoError)
		return;
	for (const API_ClassificationItem& child : children)
		CollectClassificationItem (child.guid, outGuids);
}

GSErrCode PropertyManager::EnsureGroup ()
{
	if (groupGuid != APINULLGuid)
		return NoError;

	GS::Array<API_PropertyGroup> allGroups;
	GSErrCode err = ACAPI_Property_GetPropertyGroups (allGroups);
	if (err != NoError)
		return err;

	for (const API_PropertyGroup& existing : allGroups) {
		if (existing.name == kGroupName) {
			groupGuid = existing.guid;
			return NoError;
		}
	}

	API_PropertyGroup group;
	group.groupType = API_PropertyCustomGroupType;
	group.name = kGroupName;

	err = ACAPI_Property_CreatePropertyGroup (group);
	if (err != NoError)
		return err;

	groupGuid = group.guid;
	return NoError;
}

GSErrCode PropertyManager::EnsureDefinition ()
{
	if (propertyGuid != APINULLGuid)
		return NoError;

	GSErrCode err = EnsureGroup ();
	if (err != NoError)
		return err;

	GS::Array<API_PropertyDefinition> allDefinitions;
	err = ACAPI_Property_GetPropertyDefinitions (groupGuid, allDefinitions);
	if (err != NoError)
		return err;

	for (const API_PropertyDefinition& existing : allDefinitions) {
		if (existing.name == kPropertyName) {
			propertyGuid = existing.guid;
			return NoError;
		}
	}

	API_PropertyDefinition definition;
	definition.groupGuid      = groupGuid;
	definition.name           = kPropertyName;
	definition.collectionType = API_PropertySingleCollectionType;
	definition.valueType      = API_PropertyStringValueType;
	definition.measureType    = API_PropertyDefaultMeasureType;
	definition.definitionType = API_PropertyCustomDefinitionType;
	definition.canValueBeEditable = true;
	definition.defaultValue.basicValue.singleVariant.variant.type = API_PropertyStringValueType;

	// Scope the property to every classification item (not just each
	// system's root) so it applies to any element, matching what the
	// Property Manager UI's "All" availability option does.
	GS::Array<API_ClassificationSystem> classificationSystems;
	ACAPI_Classification_GetClassificationSystems (classificationSystems);
	for (const API_ClassificationSystem& system : classificationSystems) {
		GS::Array<API_ClassificationItem> rootItems;
		if (ACAPI_Classification_GetClassificationSystemRootItems (system.guid, rootItems) != NoError)
			continue;
		for (const API_ClassificationItem& rootItem : rootItems)
			CollectClassificationItem (rootItem.guid, definition.availability);
	}

	err = ACAPI_Property_CreatePropertyDefinition (definition);
	A2E_TRACE ("A2E: PropertyManager::EnsureDefinition - CreatePropertyDefinition returned %d\n", (int) err);
	if (err != NoError)
		return err;

	propertyGuid = definition.guid;
	return NoError;
}

GS::UniString PropertyManager::GenerateId ()
{
	GS::Guid gsGuid;
	gsGuid.Generate ();
	return GS::UniString ("CKT-") + APIGuidToString (GSGuid2APIGuid (gsGuid));
}

GS::UniString PropertyManager::GetId (const API_Guid& elemGuid)
{
	if (EnsureDefinition () != NoError)
		return GS::UniString ();

	API_Property property = {};
	if (ACAPI_Element_GetPropertyValue (elemGuid, propertyGuid, property) != NoError)
		return GS::UniString ();

	if (property.isDefault || property.value.variantStatus != API_VariantStatusNormal)
		return GS::UniString ();

	return property.value.singleVariant.variant.uniStringValue;
}

GSErrCode PropertyManager::SetId (const API_Guid& elemGuid, const GS::UniString& circuitId)
{
	GSErrCode err = EnsureDefinition ();
	if (err != NoError)
		return err;

	API_Property property = {};
	property.definition.guid = propertyGuid;
	property.isDefault = false;
	property.value.variantStatus = API_VariantStatusNormal;
	// variant is a tagged union: .type must match the populated member.
	property.value.singleVariant.variant.type = API_PropertyStringValueType;
	property.value.singleVariant.variant.uniStringValue = circuitId;

	GS::Array<API_Guid> elemGuids;
	elemGuids.Push (elemGuid);
	err = ACAPI_Property_ModifyPropertyValue (property, elemGuids);
	A2E_TRACE ("A2E: PropertyManager::SetId - ACAPI_Property_ModifyPropertyValue returned %d\n", (int) err);
	return err;
}

} // namespace Circuit
