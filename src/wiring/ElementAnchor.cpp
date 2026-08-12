#include "ElementAnchor.hpp"

namespace Wiring {

API_Coord ElementAnchor::GetPoint (const API_Guid& elemGuid)
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
			return API_Coord { 0.0, 0.0 };
	}
}

} // namespace Wiring
