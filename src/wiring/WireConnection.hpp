#ifndef WIRING_WIRE_CONNECTION_HPP
#define WIRING_WIRE_CONNECTION_HPP

#include "ACAPinc.h"
#include "WireEnd.hpp"

#include <map>
#include <utility>
#include <vector>

namespace wiring {

// Describes how a wire endpoint is anchored to a host element. v1
// anchoring is "this object" — the endpoint tracks the host's own
// placement origin (see ElementAnchor), not a specific hotspot/edge.
struct ConnectionInfo
{
  API_Guid hostGuid = APINULLGuid;
};

// Tracks which wires are anchored to which host elements, and keeps
// wire endpoints glued to their hosts via the Notification Manager
// (ACAPI_Element_InstallElementObserver, installed once from
// AddOnMain.cpp's Initialize(), plus a per-host attachHostObserver
// call).
class ConnectionManager
{
public:
  ConnectionManager() = delete;

  // Attaches wireGuid's given endpoint to hostGuid: persists the
  // connection and stamps both elements with a shared Circuit ID (see
  // circuit::PropertyManager). Does not attach the live-move observer
  // — see attachHostObserver.
  static GSErrCode
  connect(const API_Guid& wireGuid, WireEnd end, const ConnectionInfo& info);

  // Attaches the per-element observer that makes onHostElementChanged
  // fire when hostGuid moves. Deliberately callable on its own,
  // outside any ACAPI_CallUndoableCommand — a notification
  // subscription isn't a database write and doesn't need undo context.
  static GSErrCode
  attachHostObserver(const API_Guid& hostGuid);

  // Removes the connection and detaches the observer if hostGuid has
  // no other wires left. Does not touch the element's Circuit ID —
  // that's circuit membership, not a per-endpoint attachment.
  static GSErrCode
  disconnect(const API_Guid& wireGuid, WireEnd end);

  // The "click start object, click end object" gesture in one call:
  // places a new GDL wire (see GdlWireElement) between their current
  // anchor points and connects both ends. Returns the new wire's GUID,
  // or APINULLGuid on failure.
  static API_Guid
  connectObjectsWithGdlWire(const API_Guid& startHostGuid,
                            const API_Guid& endHostGuid,
                            short layerIndex);

  // Re-installs observers for every stored connection. Call after
  // undo/redo — see docs/ARCHITECTURE.md.
  // TODO: unimplemented — needs to enumerate existing wire elements
  // (both backends) and re-attach their host observers.
  static GSErrCode
  restoreAllConnectionObservers();

  // The global handler passed to ACAPI_Element_InstallElementObserver.
  // Archicad calls this for every element that's been
  // attachHostObserver'd; recomputes and pushes new geometry for every
  // wire endpoint anchored to whichever element changed.
  // TODO: elemType->notifID isn't filtered on yet — this reacts to
  // every notification (move, delete, ...) alike.
  static GSErrCode
  onHostElementChanged(const API_NotifyElementType* elemType);

private:
  static ConnectionInfo
  loadConnection(const API_Guid& wireGuid, WireEnd end);
  static GSErrCode
  storeConnection(const API_Guid& wireGuid, WireEnd end, const ConnectionInfo& info);

  // In-memory index: host GUID -> wires anchored to it. Rebuilt from
  // persisted connection data at startup (see storeConnection), so
  // this cache is never itself the source of truth.
  static inline std::map<GS::Guid, std::vector<std::pair<API_Guid, WireEnd>>> s_hostToWires;
};

} // namespace wiring

#endif
