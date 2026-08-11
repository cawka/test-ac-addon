#include "GdlWireElement.hpp"
#include "../Debug.hpp"

namespace Wiring {

namespace {

// CONFIRMED (struct_a_p_i___element_memo.html / struct_a_p_i___add_par_type.html,
// and a matching real-world usage example): memo.params is a Mac-style
// handle, API_AddParType**, not a modern container. Element count comes
// from the handle's byte size, not a separate count field; *memo.params
// dereferences to the API_AddParType array itself; name is a plain
// char[API_NameLen] (not GS::UniString) so a plain strcmp is correct;
// the numeric value for a Length-type param (endX/endY, per
// paramlist.xml) lives in the value.real member of API_AddParType's
// value union.
API_AddParType* FindObjectParam (API_ElementMemo& memo, const char* paramName)
{
	if (memo.params == nullptr)
		return nullptr;

	UInt32 count = BMGetHandleSize ((GSConstHandle) memo.params) / sizeof (API_AddParType);
	API_AddParType* params = *memo.params;
	for (UInt32 i = 0; i < count; ++i) {
		if (strcmp (params[i].name, paramName) == 0)
			return &params[i];
	}
	return nullptr;
}

bool GetObjectParam (const API_ElementMemo& memo, const char* paramName, double& outValue)
{
	API_AddParType* param = FindObjectParam (const_cast<API_ElementMemo&> (memo), paramName);
	if (param == nullptr)
		return false;
	outValue = param->value.real;
	return true;
}

bool SetObjectParam (API_ElementMemo& memo, const char* paramName, double value)
{
	API_AddParType* param = FindObjectParam (memo, paramName);
	if (param == nullptr)
		return false;
	param->value.real = value;
	return true;
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
	auto src = name.ToUStr ();
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
	A2E_TRACE ("A2E: CreateGdlWire start\n");

	API_LibPart libPart = {};
	FillFixedUniBuffer (libPart.docu_UName,
		sizeof (libPart.docu_UName) / sizeof (libPart.docu_UName[0]),
		kGdlWireLibPartName);

	if (ACAPI_LibraryPart_Search (&libPart, false) != NoError) {
		A2E_TRACE ("A2E: CreateGdlWire - ACAPI_LibraryPart_Search failed (library part not found)\n");
		return APINULLGuid;
	}
	A2E_TRACE ("A2E: CreateGdlWire - found library part, index=%d\n", (int) libPart.index);

	API_Element element = {};
	element.header.type = API_ObjectID;
	element.object.libInd = libPart.index;

	A2E_TRACE ("A2E: CreateGdlWire - startPoint=(%.4f,%.4f) endPoint=(%.4f,%.4f) layerIndex(raw arg)=%d\n",
		startPoint.x, startPoint.y, endPoint.x, endPoint.y, (int) layerIndex);

	API_ElementMemo memo = {};
	// Was previously calling ACAPI_Element_Create with a memo that had
	// never been populated (SetObjectParam below was, and still is, a
	// no-op stub) — creating an Object with a completely empty
	// parameter memo is a known crash pattern (Graphisoft's own
	// community threads confirm this exact workflow). GetDefaults gives
	// a validly-shaped default memo to build on instead of an empty one.
	GSErrCode defaultsErr = ACAPI_Element_GetDefaults (&element, &memo);
	A2E_TRACE ("A2E: CreateGdlWire - GetDefaults returned %d, post-defaults: typeID=%d libInd=%d floorInd=%d pos=(%.4f,%.4f) memo.params=%s\n",
		(int) defaultsErr, (int) element.header.type.typeID, (int) element.object.libInd,
		(int) element.header.floorInd, element.object.pos.x, element.object.pos.y,
		(memo.params != nullptr) ? "non-null" : "null");
	if (defaultsErr != NoError) {
		A2E_TRACE ("A2E: CreateGdlWire - ACAPI_Element_GetDefaults failed\n");
		return APINULLGuid;
	}

	// GetDefaults may have overwritten these with generic Object
	// defaults — reassert what this specific wire needs.
	element.header.type = API_ObjectID;
	element.object.libInd = libPart.index;
	// layerIndex 0 is the caller's "use the default" placeholder (see
	// MenuCommands.cpp), not a real layer — ACAPI_CreateAttributeIndex(0)
	// is very likely not a valid layer attribute index, and clobbering
	// GetDefaults' own valid default layer with it is the prime suspect
	// for ACAPI_Element_Create's APIERR_BADINDEX-shaped failure. Only
	// override when a real, specific layer was actually requested.
	bool layerOverridden = (layerIndex != 0);
	if (layerOverridden)
		element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
	element.object.pos = startPoint;

	// DEVKIT: still a no-op stub (see above) — the wire will be created
	// at the library part's default endX/endY (likely 0,0, i.e.
	// zero-length) until this is implemented, not a crash but wrong
	// geometry. Needs API_ElementMemo::params' real field name/type and
	// API_AddParType's value-field name confirmed against the AC29
	// headers.
	bool setEndX = SetObjectParam (memo, "endX", endPoint.x - startPoint.x);
	bool setEndY = SetObjectParam (memo, "endY", endPoint.y - startPoint.y);

	A2E_TRACE ("A2E: CreateGdlWire - pre-Create: typeID=%d libInd=%d floorInd=%d pos=(%.4f,%.4f) layerOverridden=%d setEndX=%d setEndY=%d guid-before=%s memo.params=%s\n",
		(int) element.header.type.typeID, (int) element.object.libInd, (int) element.header.floorInd,
		element.object.pos.x, element.object.pos.y, (int) layerOverridden, (int) setEndX, (int) setEndY,
		(element.header.guid == APINULLGuid) ? "NULL" : "non-null", (memo.params != nullptr) ? "non-null" : "null");

	GSErrCode err = ACAPI_Element_Create (&element, &memo);
	ACAPI_DisposeElemMemoHdls (&memo);

	A2E_TRACE ("A2E: CreateGdlWire - ACAPI_Element_Create returned %d\n", (int) err);

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
