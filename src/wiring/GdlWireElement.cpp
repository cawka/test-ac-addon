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
	A2E_TRACE ("A2E: CreateGdlWire - GetDefaults returned %d, post-defaults: typeID=%d libInd=%d floorInd=%d pos=(%.4f,%.4f) angle=%.6f memo.params=%s\n",
		(int) defaultsErr, (int) element.header.type.typeID, (int) element.object.libInd,
		(int) element.header.floorInd, element.object.pos.x, element.object.pos.y,
		element.object.angle, (memo.params != nullptr) ? "non-null" : "null");
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

	// Encode the wire as (origin, angle, length) instead of (origin,
	// angle=0, world-axis-aligned endX/endY delta). Both encodings are
	// mathematically equivalent in world coordinates -- Archicad's
	// project coordinates are independent of the current view's on-
	// screen rotation, same as in any CAD system, so a rotated view is
	// not expected to make one encoding correct and the other wrong.
	// Switching anyway because it removes the "is angle really supposed
	// to be exactly 0" assumption entirely (GetDefaults leaves
	// element.object.angle, API_ObjectType::angle in radians, at
	// whatever the unrelated default object it picked was last rotated
	// to -- see the libInd/layer note above for the same GetDefaults
	// leakage pattern) in favor of a directly computed value.
	const double dx = endPoint.x - startPoint.x;
	const double dy = endPoint.y - startPoint.y;
	const double localLength = sqrt (dx * dx + dy * dy);
	element.object.angle = (localLength > 0.0) ? atan2 (dy, dx) : 0.0;

	// CONFIRMED (real-world usage report, matches what we just saw
	// directly: post-defaults libInd came back 6970, not our library
	// part's index): ACAPI_Element_GetDefaults only looks at the current
	// tool/variation and ignores the libInd we set beforehand -- the
	// memo.params it returns belongs to whatever object is "currently
	// selected in the settings dialog", not Circuit Wire. That's why
	// SetObjectParam below could never find endX/endY: they were never
	// in this memo to begin with. Fetch Circuit Wire's own default
	// params via ACAPI_LibraryPart_GetParams and swap them in.
	double libA = 0.0, libB = 0.0;
	Int32 addParNum = 0;
	API_AddParType** addPars = nullptr;
	GSErrCode paramsErr = ACAPI_LibraryPart_GetParams (libPart.index, &libA, &libB, &addParNum, &addPars);
	A2E_TRACE ("A2E: CreateGdlWire - ACAPI_LibraryPart_GetParams returned %d, addParNum=%d, addPars=%s\n",
		(int) paramsErr, (int) addParNum, (addPars != nullptr) ? "non-null" : "null");
	if (paramsErr == NoError && addPars != nullptr) {
		if (memo.params != nullptr)
			ACAPI_DisposeAddParHdl (&memo.params);
		memo.params = addPars;
	}

	// Local frame now points along the object's own (rotated) X axis by
	// construction, so the wire's far end is the full length out along
	// local X, with zero local Y.
	bool setEndX = SetObjectParam (memo, "endX", localLength);
	bool setEndY = SetObjectParam (memo, "endY", 0.0);

	A2E_TRACE ("A2E: CreateGdlWire - pre-Create: typeID=%d libInd=%d floorInd=%d pos=(%.4f,%.4f) angle=%.6f layerOverridden=%d setEndX=%d setEndY=%d localLength=%.4f guid-before=%s memo.params=%s\n",
		(int) element.header.type.typeID, (int) element.object.libInd, (int) element.header.floorInd,
		element.object.pos.x, element.object.pos.y, element.object.angle, (int) layerOverridden, (int) setEndX, (int) setEndY,
		localLength,
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

	// endX/endY are in the object's own rotated local frame (see
	// CreateGdlWire), so recovering the far end's world position needs
	// the same rotation applied: world = origin + R(angle) * local.
	const API_Coord currentOrigin = element.object.pos;
	const double cosA = cos (element.object.angle);
	const double sinA = sin (element.object.angle);
	const API_Coord otherEnd {
		currentOrigin.x + endX * cosA - endY * sinA,
		currentOrigin.y + endX * sinA + endY * cosA
	};

	const API_Coord newStart = (movedEnd == WireEnd::Start) ? newPoint : currentOrigin;
	const API_Coord newEnd   = (movedEnd == WireEnd::End)   ? newPoint : otherEnd;

	const double newDx = newEnd.x - newStart.x;
	const double newDy = newEnd.y - newStart.y;
	const double newLocalLength = sqrt (newDx * newDx + newDy * newDy);

	element.object.pos = newStart;
	element.object.angle = (newLocalLength > 0.0) ? atan2 (newDy, newDx) : 0.0;
	SetObjectParam (memo, "endX", newLocalLength);
	SetObjectParam (memo, "endY", 0.0);

	API_Element mask = {};
	ACAPI_ELEMENT_MASK_CLEAR (mask);
	// DEVKIT: set the mask bits for "position", "angle", and "parameters
	// changed" once the AC29 mask field names are confirmed -- angle is
	// now also modified here (it wasn't before this change), so it needs
	// to be in that set too, not just position.

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
