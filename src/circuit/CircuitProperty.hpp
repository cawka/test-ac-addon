#ifndef CIRCUIT_CIRCUIT_PROPERTY_HPP
#define CIRCUIT_CIRCUIT_PROPERTY_HPP

#include "ACAPinc.h"

namespace circuit {

// Owns the "Circuit ID" custom property definition and reads/writes its
// per-element values. Elements sharing a Circuit ID are considered part
// of the same circuit (see Selection).
class PropertyManager
{
public:
  PropertyManager() = delete;

  // Creates the property definition (and its group) if missing.
  // Idempotent; safe to call on every command dispatch.
  static GSErrCode ensureDefinition();

  // Returns a fresh, unused circuit identifier.
  static GS::UniString generateId();

  // Returns elemGuid's current Circuit ID, or an empty string if it
  // isn't part of a circuit.
  static GS::UniString getId(const API_Guid& elemGuid);

  // Sets (or overwrites) elemGuid's Circuit ID.
  static GSErrCode setId(const API_Guid& elemGuid, const GS::UniString& circuitId);

private:
  static GSErrCode ensureGroup();

  // Appends itemGuid and every descendant classification item's GUID
  // to outGuids, so the property can be scoped to all classifications.
  static void collectClassificationItem(const API_Guid& itemGuid, GS::Array<API_Guid>& outGuids);

  static inline const GS::UniString GROUP_NAME = GS::UniString("A2 Electrical");
  static inline const GS::UniString PROPERTY_NAME = GS::UniString("Circuit ID");

  static inline API_Guid s_groupGuid = APINULLGuid;
  static inline API_Guid s_propertyGuid = APINULLGuid;
};

} // namespace circuit

#endif
