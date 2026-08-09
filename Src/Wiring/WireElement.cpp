#include "WireElement.hpp"

namespace Wiring {

API_Guid CreateWire (const std::vector<WireNode>& nodes, short layerIndex)
{
	if (nodes.size () < 2)
		return APINULLGuid;

	API_Element element = {};
	element.header.type = API_SplineID;
	element.header.layer = layerIndex;

	// DEVKIT: API_SplineType's node/direction storage is a memo-backed
	// array (like polylines), not inline in API_Element — fill
	// element.spline for the fixed part, then build the matching
	// API_ElementMemo (coords/dirs arrays) below once the AC29 struct
	// layout is confirmed against Support/Inc/APIdefs_Elements.h.
	API_ElementMemo memo = {};
	// memo.coords = ...   (nodes[i].position, 1-based array per AC convention)
	// memo.vertexIDs = ...
	// memo.splineDirs = ... (nodes[i].dirIn / dirOut)

	GSErrCode err = ACAPI_Element_Create (&element, &memo);
	ACAPI_DisposeElemMemoHdls (&memo);

	if (err != NoError)
		return APINULLGuid;

	return element.header.guid;
}

GSErrCode SetWireNodes (const API_Guid& wireGuid, const std::vector<WireNode>& nodes)
{
	if (nodes.size () < 2)
		return APIERR_BADPARS;

	API_Element element = {};
	element.header.guid = wireGuid;

	GSErrCode err = ACAPI_Element_Get (&element);
	if (err != NoError)
		return err;

	if (element.header.type.typeID != API_SplineID)
		return APIERR_BADID;

	API_ElementMemo memo = {};
	err = ACAPI_Element_GetMemo (wireGuid, &memo);
	if (err != NoError)
		return err;

	// DEVKIT: overwrite memo.coords / memo.splineDirs from `nodes` here,
	// same struct-layout caveat as CreateWire above.

	API_Element mask = {};
	ACAPI_ELEMENT_MASK_CLEAR (mask);
	// DEVKIT: set the mask fields for the geometry actually changed
	// (coordinate/spline data) once the field names are confirmed.

	err = ACAPI_Element_Change (&element, &mask, &memo, 0, true);
	ACAPI_DisposeElemMemoHdls (&memo);

	return err;
}

bool IsWireElement (const API_Guid& elemGuid)
{
	API_Elem_Head head = {};
	head.guid = elemGuid;

	if (ACAPI_Element_GetHeader (&head) != NoError)
		return false;

	return head.type.typeID == API_SplineID;
}

} // namespace Wiring
