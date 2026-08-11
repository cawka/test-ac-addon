#include "GdlWireElement.hpp"
#include "../Debug.hpp"

namespace Wiring {

namespace {

// DEVKIT: API_AddParType's field names (paramName vs. name, real vs.
// value.real for a Length/RealNum parameter) still need confirming.
// memo.params itself is likely NOT GS::Array<API_AddParType> as
// originally guessed here — ACAPI_LibPart_GetParams (confirmed real
// signature: `GSErrCode ACAPI_LibPart_GetParams (Int32 libInd, double*
// a, double* b, Int32* addParNum, API_AddParType*** addPars)`) returns
// the equivalent data as an old-style handle
// (API_AddParType***, disposed via ACAPI_DisposeAddParHdl), which
// strongly suggests memo.params is a handle (API_AddParType**) too,
// not a modern container — needs the real struct definition to
// implement correctly rather than guess a second time. This is the
// same kind of struct-layout gap flagged in WireElement.cpp for the
// Spline backend.

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

	API_ElementMemo memo = {};
	// Was previously calling ACAPI_Element_Create with a memo that had
	// never been populated (SetObjectParam below was, and still is, a
	// no-op stub) — creating an Object with a completely empty
	// parameter memo is a known crash pattern (Graphisoft's own
	// community threads confirm this exact workflow). GetDefaults gives
	// a validly-shaped default memo to build on instead of an empty one.
	if (ACAPI_Element_GetDefaults (&element, &memo) != NoError) {
		A2E_TRACE ("A2E: CreateGdlWire - ACAPI_Element_GetDefaults failed\n");
		return APINULLGuid;
	}

	// GetDefaults may have overwritten these with generic Object
	// defaults — reassert what this specific wire needs.
	element.header.type = API_ObjectID;
	element.object.libInd = libPart.index;
	element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
	element.object.pos = startPoint;

	// DEVKIT: still a no-op stub (see above) — the wire will be created
	// at the library part's default endX/endY (likely 0,0, i.e.
	// zero-length) until this is implemented, not a crash but wrong
	// geometry. Needs API_ElementMemo::params' real field name/type and
	// API_AddParType's value-field name confirmed against the AC29
	// headers.
	SetObjectParam (memo, "endX", endPoint.x - startPoint.x);
	SetObjectParam (memo, "endY", endPoint.y - startPoint.y);

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
