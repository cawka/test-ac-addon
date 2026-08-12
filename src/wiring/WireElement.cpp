#include "WireElement.hpp"

namespace Wiring {

API_Guid SplineWireElement::Create (const std::vector<WireNode>& nodes, short layerIndex)
{
	if (nodes.size () < 2)
		return APINULLGuid;

	API_Element element = {};
	element.header.type = API_SplineID;
	element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);

	// TODO: fill element.spline and the matching API_ElementMemo
	// (coords/dirs arrays) from `nodes` once the AC29 struct layout is
	// confirmed against Support/Inc/APIdefs_Elements.h.
	API_ElementMemo memo = {};

	GSErrCode err = ACAPI_Element_Create (&element, &memo);
	ACAPI_DisposeElemMemoHdls (&memo);

	return err == NoError ? element.header.guid : APINULLGuid;
}

GSErrCode SplineWireElement::SetNodes (const API_Guid& wireGuid, const std::vector<WireNode>& nodes)
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

	// TODO: overwrite memo.coords / memo.splineDirs from `nodes`, same
	// struct-layout caveat as Create above.

	API_Element mask = {};
	ACAPI_ELEMENT_MASK_CLEAR (mask);
	// TODO: set the mask bits for the changed geometry fields.

	err = ACAPI_Element_Change (&element, &mask, &memo, 0, true);
	ACAPI_DisposeElemMemoHdls (&memo);

	return err;
}

GSErrCode SplineWireElement::SetEndpoint (const API_Guid& wireGuid, WireEnd end, const API_Coord& newPoint)
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

	// TODO: read the existing node array out of memo, replace the
	// Start/End node's x/y (keep z) per `end`, then call SetNodes.
	std::vector<WireNode> nodes;
	(void) newPoint;

	ACAPI_DisposeElemMemoHdls (&memo);

	if (nodes.size () < 2)
		return APIERR_NOTSUPPORTED;

	return SetNodes (wireGuid, nodes);
}

bool SplineWireElement::IsInstance (const API_Guid& elemGuid)
{
	API_Elem_Head head = {};
	head.guid = elemGuid;

	if (ACAPI_Element_GetHeader (&head) != NoError)
		return false;

	return head.type.typeID == API_SplineID;
}

} // namespace Wiring
