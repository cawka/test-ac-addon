#include "ElementAnchor.hpp"

namespace Wiring {

API_Coord GetElementAnchorPoint (const API_Guid& elemGuid)
{
	API_Element element = {};
	element.header.guid = elemGuid;

	if (ACAPI_Element_Get (&element) != NoError)
		return API_Coord { 0.0, 0.0 };

	switch (element.header.type.typeID) {
		case API_ObjectID:
			return element.object.pos;
		case API_LampID:
			return element.lamp.pos;
		default:
			// TODO: add cases as wires start connecting to other
			// element types (see DEVKIT note in the header).
			return API_Coord { 0.0, 0.0 };
	}
}

} // namespace Wiring
