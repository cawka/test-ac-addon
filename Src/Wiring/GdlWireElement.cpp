#include "GdlWireElement.hpp"

namespace Wiring {

namespace {

// DEVKIT: API_AddParType's field names (paramName vs. name, real vs.
// value.real for a Length/RealNum parameter) and how memo.params is
// typed (GS::Array<API_AddParType>* vs. a raw handle) both need
// confirming against the AC29 headers — this is the same kind of
// struct-layout gap flagged in WireElement.cpp for the Spline backend.

bool GetObjectParam (const API_ElementMemo& memo, const char* paramName, double& outValue)
{
	(void) memo; (void) paramName; (void) outValue;
	return false; // TODO
}

bool SetObjectParam (API_ElementMemo& memo, const char* paramName, double value)
{
	(void) memo; (void) paramName; (void) value;
	return false; // TODO
}

} // namespace

API_Guid CreateGdlWire (const API_Coord& startPoint, const API_Coord& endPoint, short layerIndex)
{
	API_LibPart libPart = {};
	// DEVKIT: confirm the field ACAPI_LibPart_Search matches the name
	// against for AC29 (docu_UName has been the historical field for a
	// library part's document/display name).
	CHTruncate (kGdlWireLibPartName, libPart.docu_UName, sizeof (libPart.docu_UName));

	if (ACAPI_LibraryPart_Search (&libPart, false) != NoError)
		return APINULLGuid;

	API_Element element = {};
	element.header.type = API_ObjectID;
	element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
	element.object.pos = startPoint;
	element.object.libInd = libPart.index;

	API_ElementMemo memo = {};
	// DEVKIT: fetch the library part's default parameter set first
	// (ACAPI_Element_GetDefaults or ACAPI_LibPart_GetParams) and copy it
	// into memo.params before overriding endX/endY below — an Object
	// can't be created with only these two parameters populated, it
	// needs the rest of the part's declared parameter list too.
	SetObjectParam (memo, "endX", endPoint.x - startPoint.x);
	SetObjectParam (memo, "endY", endPoint.y - startPoint.y);

	GSErrCode err = ACAPI_Element_Create (&element, &memo);
	ACAPI_DisposeElemMemoHdls (&memo);

	return err == NoError ? element.header.guid : APINULLGuid;
}

GSErrCode SetGdlWireEndpoint (const API_Guid& wireGuid, WireEnd movedEnd, const API_Coord& newPoint)
{
	API_Element element = {};
	element.header.guid = wireGuid;

	GSErrCode err = ACAPI_Element_Get (&element);
	if (err != NoError)
		return err;

	API_ElementMemo memo = {};
	err = ACAPI_Element_GetMemo (wireGuid, &memo);
	if (err != NoError)
		return err;

	double endX = 0.0, endY = 0.0;
	GetObjectParam (memo, "endX", endX);
	GetObjectParam (memo, "endY", endY);

	const API_Coord currentOrigin = element.object.pos;
	const API_Coord otherEnd { currentOrigin.x + endX, currentOrigin.y + endY };

	const API_Coord newStart = (movedEnd == WireEnd::Start) ? newPoint : currentOrigin;
	const API_Coord newEnd   = (movedEnd == WireEnd::End)   ? newPoint : otherEnd;

	element.object.pos = newStart;
	SetObjectParam (memo, "endX", newEnd.x - newStart.x);
	SetObjectParam (memo, "endY", newEnd.y - newStart.y);

	API_Element mask = {};
	ACAPI_ELEMENT_MASK_CLEAR (mask);
	// DEVKIT: set the mask bits for "position" and "parameters changed"
	// once the AC29 mask field names are confirmed.

	err = ACAPI_Element_Change (&element, &mask, &memo, 0, true);
	ACAPI_DisposeElemMemoHdls (&memo);

	return err;
}

bool IsGdlWireElement (const API_Guid& elemGuid)
{
	API_Element element = {};
	element.header.guid = elemGuid;

	if (ACAPI_Element_Get (&element) != NoError)
		return false;
	if (element.header.type.typeID != API_ObjectID)
		return false;

	API_LibPart libPart = {};
	libPart.index = element.object.libInd;
	if (ACAPI_LibraryPart_Get (&libPart) != NoError)
		return false;

	// DEVKIT: compare libPart.docu_UName (or whatever the confirmed
	// field turns out to be) against kGdlWireLibPartName.
	return true; // TODO
}

} // namespace Wiring
