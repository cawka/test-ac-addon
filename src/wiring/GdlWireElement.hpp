#ifndef WIRING_GDL_WIRE_ELEMENT_HPP
#define WIRING_GDL_WIRE_ELEMENT_HPP

#include "ACAPinc.h"
#include "WireEnd.hpp"

namespace Wiring {

// Name of the built-in library part shipped in
// RINT/ACLib/Src/Circuit Wire/ (see RINT/BuiltInLibParts.grc, which is
// what makes it "built-in" — baked into the add-on bundle and
// auto-registered in AddOnMain.cpp's RegisterInterface, rather than
// needing to be added to a project library by hand). Must match the
// "Circuit Wire.gsm" name used there. Kept as one named constant so a
// rename only has to happen here.
inline const GS::UniString kGdlWireLibPartName ("Circuit Wire");

// Places a new "Circuit Wire" object with its origin at startPoint,
// its endX/endY parameters set to reach endPoint, and bulge left at
// the library part's default (straight — see
// RINT/ACLib/Src/Circuit Wire/scripts/2d.gdl). Returns the new
// element's GUID, or APINULLGuid on failure.
//
// DEVKIT: confirm the AC29 call shape for placing an Object by library
// part name — historically: ACAPI_LibPart_Search to resolve the part's
// index from its name, then ACAPI_Element_GetDefaults(API_ObjectID) to
// get a full valid parameter set to start from (an Object can't be
// created with only 2 of its parameters populated — the rest need
// their library-part defaults), override endX/endY on top, then
// ACAPI_Element_Create.
API_Guid CreateGdlWire (const API_Coord& startPoint, const API_Coord& endPoint, short layerIndex);

// Repositions an existing "Circuit Wire" element after one of its
// endpoints moved. Only `movedEnd` is passed in as a new point — the
// other endpoint is recovered from the element's current placement
// origin + endX/endY, since the two are joined (moving the origin
// without adjusting endX/endY would also silently move the far end,
// which is exactly what must NOT happen when only one side moved).
GSErrCode SetGdlWireEndpoint (const API_Guid& wireGuid, WireEnd movedEnd, const API_Coord& newPoint);

// True if elemGuid is a placed instance of the "Circuit Wire" library part.
bool IsGdlWireElement (const API_Guid& elemGuid);

} // namespace Wiring

#endif
