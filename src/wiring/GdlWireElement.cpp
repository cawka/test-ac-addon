#include "GdlWireElement.hpp"

#include "../Debug.hpp"

namespace wiring {

// memo.params is a Mac-style handle (API_AddParType**): element count
// comes from the handle's byte size, *memo.params is the array itself,
// name is a plain char[] (not GS::UniString).
API_AddParType*
GdlWireElement::findParam(API_ElementMemo& memo, const char* paramName)
{
  if (memo.params == nullptr)
    return nullptr;

  UInt32 count = BMGetHandleSize((GSConstHandle) memo.params) / sizeof(API_AddParType);
  API_AddParType* params = *memo.params;
  for (UInt32 i = 0; i < count; ++i) {
    if (strcmp(params[i].name, paramName) == 0)
      return &params[i];
  }
  return nullptr;
}

bool
GdlWireElement::getParam(const API_ElementMemo& memo, const char* paramName, double& outValue)
{
  API_AddParType* param = findParam(const_cast<API_ElementMemo&>(memo), paramName);
  if (param == nullptr)
    return false;
  outValue = param->value.real;
  return true;
}

bool
GdlWireElement::setParam(API_ElementMemo& memo, const char* paramName, double value)
{
  API_AddParType* param = findParam(memo, paramName);
  if (param == nullptr)
    return false;
  param->value.real = value;
  return true;
}

void
GdlWireElement::fillFixedUniBuffer(GS::uchar_t* dest, USize destCapacity, const GS::UniString& name)
{
  auto src = name.ToUStr();
  USize len = name.GetLength();
  if (len > destCapacity - 1)
    len = destCapacity - 1;
  for (USize i = 0; i < len; ++i)
    dest[i] = src[i];
  dest[len] = 0;
}

API_Guid
GdlWireElement::create(const API_Coord& startPoint, const API_Coord& endPoint, short layerIndex)
{
  API_LibPart libPart = {};
  fillFixedUniBuffer(
    libPart.docu_UName, sizeof(libPart.docu_UName) / sizeof(libPart.docu_UName[0]), LIB_PART_NAME);

  if (ACAPI_LibraryPart_Search(&libPart, false) != NoError) {
    A2E_TRACE("A2E: GdlWireElement::create - library part not found\n");
    return APINULLGuid;
  }

  API_Element element = {};
  element.header.type = API_ObjectID;
  element.object.libInd = libPart.index;

  // ACAPI_Element_GetDefaults ignores the libInd set above (it reflects
  // whatever object is "currently selected in the settings dialog",
  // not this library part) -- used only for generic element defaults
  // (floor, etc); libInd, params, and geometry are all reasserted below.
  API_ElementMemo memo = {};
  if (ACAPI_Element_GetDefaults(&element, &memo) != NoError) {
    A2E_TRACE("A2E: GdlWireElement::create - GetDefaults failed\n");
    return APINULLGuid;
  }

  element.header.type = API_ObjectID;
  element.object.libInd = libPart.index;
  // layerIndex 0 means "use the placement default" -- ACAPI_CreateAttributeIndex(0)
  // is not a valid layer, so only override when a real layer was requested.
  if (layerIndex != 0)
    element.header.layer = ACAPI_CreateAttributeIndex(layerIndex);
  element.object.pos = startPoint;

  const double dx = endPoint.x - startPoint.x;
  const double dy = endPoint.y - startPoint.y;
  const double length = sqrt(dx * dx + dy * dy);
  element.object.angle = (length > 0.0) ? atan2(dy, dx) : 0.0;

  // Fetch this library part's own default params (GetDefaults' memo
  // belongs to a different object) and swap them in before setting
  // endX/endY, so the strings/handles setParam looks for actually exist.
  double libA = 0.0, libB = 0.0;
  Int32 addParNum = 0;
  API_AddParType** addPars = nullptr;
  if (ACAPI_LibraryPart_GetParams(libPart.index, &libA, &libB, &addParNum, &addPars) == NoError &&
      addPars != nullptr) {
    if (memo.params != nullptr)
      ACAPI_DisposeAddParHdl(&memo.params);
    memo.params = addPars;
  }

  // Local frame points along the object's own (rotated) X axis by
  // construction, so the far end is the full length out along local X.
  setParam(memo, "endX", length);
  setParam(memo, "endY", 0.0);

  GSErrCode err = ACAPI_Element_Create(&element, &memo);
  ACAPI_DisposeElemMemoHdls(&memo);

  A2E_TRACE("A2E: GdlWireElement::create - ACAPI_Element_Create returned %d\n", (int) err);
  return err == NoError ? element.header.guid : APINULLGuid;
}

GSErrCode
GdlWireElement::setEndpoint(const API_Guid& wireGuid, WireEnd movedEnd, const API_Coord& newPoint)
{
  API_Element element = {};
  element.header.guid = wireGuid;

  GSErrCode err = ACAPI_Element_Get(&element);
  if (err != NoError)
    return err;

  API_ElementMemo memo = {};
  err = ACAPI_Element_GetMemo(wireGuid, &memo);
  if (err != NoError)
    return err;

  double endX = 0.0, endY = 0.0;
  getParam(memo, "endX", endX);
  getParam(memo, "endY", endY);

  // endX/endY are in the object's own rotated local frame, so
  // recovering the far end's world position needs the same rotation:
  // world = origin + R(angle) * local.
  const API_Coord currentOrigin = element.object.pos;
  const double cosA = cos(element.object.angle);
  const double sinA = sin(element.object.angle);
  const API_Coord otherEnd{currentOrigin.x + endX * cosA - endY * sinA,
                           currentOrigin.y + endX * sinA + endY * cosA};

  const API_Coord newStart = (movedEnd == WireEnd::Start) ? newPoint : currentOrigin;
  const API_Coord newEnd = (movedEnd == WireEnd::End) ? newPoint : otherEnd;

  const double newDx = newEnd.x - newStart.x;
  const double newDy = newEnd.y - newStart.y;
  const double newLength = sqrt(newDx * newDx + newDy * newDy);

  element.object.pos = newStart;
  element.object.angle = (newLength > 0.0) ? atan2(newDy, newDx) : 0.0;
  setParam(memo, "endX", newLength);
  setParam(memo, "endY", 0.0);

  API_Element mask = {};
  ACAPI_ELEMENT_MASK_CLEAR(mask);
  // TODO: set the mask bits for position/angle/params once the AC29
  // mask field names are confirmed.

  err = ACAPI_Element_Change(&element, &mask, &memo, 0, true);
  ACAPI_DisposeElemMemoHdls(&memo);

  return err;
}

bool
GdlWireElement::isInstance(const API_Guid& elemGuid)
{
  API_Element element = {};
  element.header.guid = elemGuid;

  if (ACAPI_Element_Get(&element) != NoError)
    return false;
  if (element.header.type.typeID != API_ObjectID)
    return false;

  API_LibPart libPart = {};
  libPart.index = element.object.libInd;
  if (ACAPI_LibraryPart_Get(&libPart) != NoError)
    return false;

  return GS::UniString(libPart.docu_UName) == LIB_PART_NAME;
}

} // namespace wiring
