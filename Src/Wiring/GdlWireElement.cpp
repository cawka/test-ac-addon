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

// GS::UniString has no "fill this fixed uchar_t buffer" convenience —
// ToUStr() returns a UStr wrapper that only offers an implicit
// conversion to `const GS::UniChar::Layout*` (confirmed from
// Support/Modules/GSRoot/UniString.hpp), so this is a manual bounded
// copy. destCapacity is the array's element count (i.e.
// sizeof(dest)/sizeof(dest[0])), not a byte count.
//
// DEVKIT: two assumptions here, not confirmed as certainly as the rest
// of this function: GS::UniString::GetLength() is the length accessor
// (very standard name, but unseen directly in what I grepped), and
// GS::uchar_t / GS::UniChar::Layout are the same underlying type (both
// are unsigned short per the compiler's own error output, which is
// suggestive but not a direct confirmation they're literally the same
// typedef rather than merely same-sized).
void FillFixedUniBuffer (GS::uchar_t* dest, USize destCapacity, const GS::UniString& name)
{
	const GS::UniChar::Layout* src = name.ToUStr ();
	USize len = name.GetLength ();
	if (len > destCapacity - 1)
		len = destCapacity - 1;
	for (USize i = 0; i < len; ++i)
		dest[i] = src[i];
	dest[len] = 0;
}

} // namespace

API_Guid CreateGdlWire (const API_Coord& startPoint, const API_Coord& endPoint, short layerIndex)
{
	API_LibPart libPart = {};
	FillFixedUniBuffer (libPart.docu_UName,
		sizeof (libPart.docu_UName) / sizeof (libPart.docu_UName[0]),
		kGdlWireLibPartName);

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

	// DEVKIT: same GS::uchar_t/GS::UniChar::Layout compatibility assumption
	// as FillFixedUniBuffer above — constructing a UniString straight
	// from the fixed array (confirmed constructor:
	// UniString(const GS::UniChar::Layout* uStr)).
	return GS::UniString (libPart.docu_UName) == kGdlWireLibPartName;
}

} // namespace Wiring
