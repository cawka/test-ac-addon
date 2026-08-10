#ifndef WIRING_WIRE_CONNECTION_HPP
#define WIRING_WIRE_CONNECTION_HPP

#include "ACAPinc.h"
#include "WireEnd.hpp"

namespace Wiring {

// Describes how a wire endpoint is anchored to a host element. v1
// anchoring is deliberately just "this object" — the endpoint tracks
// the host's own placement origin (see ElementAnchor.hpp), not a
// specific hotspot/edge on it. That's what makes "click the start
// object, click the end object" a complete connect gesture with no
// further picking needed.
struct ConnectionInfo {
	API_Guid	hostGuid = APINULLGuid;
};

// Attaches wireGuid's given endpoint to hostGuid, persists the
// connection (so it survives save/reload — see .cpp for where it's
// stored), stamps both elements with a shared Circuit ID (see
// Circuit/CircuitProperty.hpp), and installs the observer that keeps
// the wire glued to the host from then on.
GSErrCode Connect (const API_Guid& wireGuid, WireEnd end, const ConnectionInfo& info);

// Removes the connection and the observer. Does not touch the
// element's Circuit ID — that's a circuit-membership marker, not a
// per-endpoint attachment, and other connections may still rely on it.
GSErrCode Disconnect (const API_Guid& wireGuid, WireEnd end);

// The "click start object, click end object" gesture in one call:
// places a new GDL "Circuit Wire" (see GdlWireElement.hpp) between
// their current anchor points, then connects both of its ends via
// Connect() above so it tracks either host from then on. This is what
// Commands::CreateWireBetweenObjectsCommand calls once it has both
// clicked GUIDs. Returns the new wire's GUID, or APINULLGuid on
// failure (nothing is left half-created — see .cpp).
API_Guid ConnectObjectsWithGdlWire (const API_Guid& startHostGuid, const API_Guid& endHostGuid, short layerIndex);

// Re-installs observers for every stored connection. Call from
// Initialize() and after undo/redo — see docs/ARCHITECTURE.md, section 2.
GSErrCode RestoreAllConnectionObservers ();

// The single global handler installed once via
// ACAPI_Element_InstallElementObserver (see RestoreAllConnectionObservers)
// — signature matches APIElementEventHandlerProc. Archicad calls this
// for every element that's been ACAPI_Element_AttachObserver'd,
// regardless of which one; elemType->elemHead.guid says which. Recomputes
// and pushes new geometry for every wire endpoint anchored to it —
// dispatches to WireElement::SetWireEndpoint or
// GdlWireElement::SetGdlWireEndpoint depending on which backend that
// particular wire is.
//
// DEVKIT: API_ElementDBEventID (elemType->notifID) isn't filtered on
// yet — this reacts to every notification for an attached element
// (move, delete, ...) alike. Worth narrowing once the enum's members
// are confirmed, at least to skip redundant recomputes.
GSErrCode OnHostElementChanged (const API_NotifyElementType* elemType);

} // namespace Wiring

#endif
