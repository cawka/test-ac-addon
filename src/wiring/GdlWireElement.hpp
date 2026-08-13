#ifndef WIRING_GDL_WIRE_ELEMENT_HPP
#define WIRING_GDL_WIRE_ELEMENT_HPP

#include "ACAPinc.h"
#include "WireEnd.hpp"

namespace wiring {

// Places and maintains instances of the built-in "Circuit Wire" GDL
// object (see RINT/ACLib/Src/Circuit Wire/, registered as a built-in
// library part in RINT/BuiltInLibParts.grc). Geometry is encoded as
// (placement origin, rotation angle, length): the object's own X axis
// points from its origin toward the far end, so endX is the segment
// length and endY is always 0 in the object's local frame.
class GdlWireElement
{
public:
  GdlWireElement() = delete;

  // Places a new wire from startPoint to endPoint. Returns the new
  // element's GUID, or APINULLGuid on failure.
  static API_Guid
  create(const API_Coord& startPoint, const API_Coord& endPoint, short layerIndex);

  // Repositions an existing wire after movedEnd moved to newPoint. The
  // other endpoint is recovered from the element's current placement
  // (origin + angle + length) and kept fixed.
  static GSErrCode
  setEndpoint(const API_Guid& wireGuid, WireEnd movedEnd, const API_Coord& newPoint);

  // True if elemGuid is a placed instance of the "Circuit Wire" library part.
  static bool
  isInstance(const API_Guid& elemGuid);

private:
  static API_AddParType*
  findParam(API_ElementMemo& memo, const char* paramName);
  static bool
  getParam(const API_ElementMemo& memo, const char* paramName, double& outValue);
  static bool
  setParam(API_ElementMemo& memo, const char* paramName, double value);

  // GS::UniString has no "fill this fixed uchar_t buffer" convenience;
  // destCapacity is the destination array's element count, not bytes.
  static void
  fillFixedUniBuffer(GS::uchar_t* dest, USize destCapacity, const GS::UniString& name);

  static inline const GS::UniString LIB_PART_NAME = GS::UniString("Circuit Wire");
};

} // namespace wiring

#endif
