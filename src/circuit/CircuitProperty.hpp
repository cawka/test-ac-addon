#ifndef CIRCUIT_CIRCUIT_PROPERTY_HPP
#define CIRCUIT_CIRCUIT_PROPERTY_HPP

#include "ACAPinc.h"

namespace Circuit {

// Owns the "Circuit ID" custom property definition and reads/writes its
// per-element values. Elements sharing a Circuit ID are considered part
// of the same circuit (see Selection).
class PropertyManager {
public:
	PropertyManager () = delete;

	// Creates the property definition (and its group) if missing.
	// Idempotent; safe to call on every command dispatch.
	static GSErrCode EnsureDefinition ();

	// Returns a fresh, unused circuit identifier.
	static GS::UniString GenerateId ();

	// Returns elemGuid's current Circuit ID, or an empty string if it
	// isn't part of a circuit.
	static GS::UniString GetId (const API_Guid& elemGuid);

	// Sets (or overwrites) elemGuid's Circuit ID.
	static GSErrCode SetId (const API_Guid& elemGuid, const GS::UniString& circuitId);

private:
	static GSErrCode EnsureGroup ();

	// Appends itemGuid and every descendant classification item's GUID
	// to outGuids, so the property can be scoped to all classifications.
	static void CollectClassificationItem (const API_Guid& itemGuid, GS::Array<API_Guid>& outGuids);

	static inline const GS::UniString kGroupName = GS::UniString ("A2 Electrical");
	static inline const GS::UniString kPropertyName = GS::UniString ("Circuit ID");

	static inline API_Guid groupGuid = APINULLGuid;
	static inline API_Guid propertyGuid = APINULLGuid;
};

} // namespace Circuit

#endif
