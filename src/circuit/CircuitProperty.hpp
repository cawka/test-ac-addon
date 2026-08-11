#ifndef CIRCUIT_CIRCUIT_PROPERTY_HPP
#define CIRCUIT_CIRCUIT_PROPERTY_HPP

#include "ACAPinc.h"

namespace Circuit {

// Ensures the "Circuit ID" custom property definition exists in the
// current project (creates it, as a single-line-text user property, if
// missing). Idempotent — safe to call from Initialize() every launch.
//
// DEVKIT: confirm ACAPI_Property_CreatePropertyDefinition's parameter
// struct for AC29 (property group placement, default value handling)
// against ACAPI_PropertyProcedures.hpp.
GSErrCode EnsureCircuitPropertyDefinition ();

// Returns a fresh, unused circuit identifier (e.g. a short GUID-derived
// string) — used when connecting two elements that don't already
// belong to a circuit.
GS::UniString GenerateCircuitId ();

// Returns the element's current Circuit ID, or an empty string if it
// isn't part of a circuit.
GS::UniString GetCircuitId (const API_Guid& elemGuid);

// Sets (or overwrites) the element's Circuit ID.
GSErrCode SetCircuitId (const API_Guid& elemGuid, const GS::UniString& circuitId);

} // namespace Circuit

#endif
