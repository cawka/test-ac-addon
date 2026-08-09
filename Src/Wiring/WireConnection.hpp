#ifndef WIRING_WIRE_CONNECTION_HPP
#define WIRING_WIRE_CONNECTION_HPP

#include "ACAPinc.h"

namespace Wiring {

// Which end of the wire is attached.
enum class WireEnd { Start, End };

// Describes how a wire endpoint is anchored to a host element.
// hotspotIndex is host-type-specific (e.g. a GDL object's hotspot
// number); use whichever addressing makes sense once you know which
// element types wires connect to (Objects, Lamps, MEP elements, ...).
struct ConnectionInfo {
	API_Guid	hostGuid = APINULLGuid;
	Int32		hotspotIndex = 0;
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

// Re-installs observers for every stored connection. Call from
// Initialize() and after undo/redo — see docs/ARCHITECTURE.md, section 2.
GSErrCode RestoreAllConnectionObservers ();

// Notification callback registered per host element. Recomputes and
// pushes new geometry for every wire endpoint anchored to elemGuid.
//
// DEVKIT: confirm APINotifyElementID's "geometry/position changed"
// member name for AC29 (has been APINotify_ChangeType historically,
// but check ACAPI_NotificationProcedures.hpp) and the exact
// ACAPI_Notification_InstallElementObserver signature.
GSErrCode __ACENV_CALL OnHostElementChanged (const API_Guid& elemGuid, API_NotifyElementType notifType);

} // namespace Wiring

#endif
