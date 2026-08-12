#include "MenuCommands.hpp"

#include "Debug.hpp"
#include "GitVersion.hpp"
#include "ResourceIds.hpp"
#include "circuit/CircuitProperty.hpp"
#include "circuit/CircuitSelection.hpp"
#include "wiring/WireConnection.hpp"
#include "wiring/WireElement.hpp"

namespace commands {

GSErrCode
MenuCommandDispatcher::handle(const API_MenuParams* menuParams)
{
  // Circuit property setup + observer install are database work and
  // must run inside an undoable command.
  GSErrCode status = ACAPI_CallUndoableCommand("Create Property Group", [&]() -> GSErrCode {
    GSErrCode err = circuit::PropertyManager::ensureDefinition();
    if (err != NoError)
      return err;
    return wiring::ConnectionManager::restoreAllConnectionObservers();
  });
  if (status != NoError)
    return status;

  // Each command has its own menu resource (see ResourceIds.hpp), so
  // menuResID identifies the command directly.
  switch (menuParams->menuItemRef.menuResID) {
    case ID_ADDON_MENU_CREATE_WIRE:
      return createWire();
    case ID_ADDON_MENU_CONNECT_WIRE_ENDPOINT:
      return connectWireEndpoint();
    case ID_ADDON_MENU_CREATE_WIRE_BETWEEN_OBJECTS:
      return createWireBetweenObjects();
    case ID_ADDON_MENU_SELECT_CIRCUIT_WIRING:
      return selectCircuitWiring();
    case ID_ADDON_MENU_SELECT_CIRCUIT_OBJECTS:
      return selectCircuitObjects();
    case ID_ADDON_MENU_ABOUT:
      return showAbout();
  }

  return NoError;
}

GSErrCode
MenuCommandDispatcher::createWire()
{
  return APIERR_NOTSUPPORTED;
}

GSErrCode
MenuCommandDispatcher::connectWireEndpoint()
{
  return APIERR_NOTSUPPORTED;
}

GSErrCode
MenuCommandDispatcher::createWireBetweenObjects()
{
  API_Guid startHostGuid = APINULLGuid;
  API_Guid endHostGuid = APINULLGuid;

  // ACAPI_UserInput_GetPoint's clicked-element guid is pointInfo.neig.guid,
  // not the struct's top-level .guid field (that one isn't populated
  // by this function). enableQuickSelection turns on Archicad's Quick
  // Selection (magnet) filter: live-highlights the candidate element
  // and resolves overlaps sensibly.
  API_GetPointType pointInfo = {};
  pointInfo.enableQuickSelection = true;
  CHTruncate("Click the start object", pointInfo.prompt, sizeof(pointInfo.prompt));
  GSErrCode err = ACAPI_UserInput_GetPoint(&pointInfo);
  if (err != NoError)
    return err;
  startHostGuid = pointInfo.neig.guid;
  if (startHostGuid == APINULLGuid)
    return APIERR_BADPARS; // clicked empty space, not an element

  pointInfo = {};
  pointInfo.enableQuickSelection = true;
  CHTruncate("Click the end object", pointInfo.prompt, sizeof(pointInfo.prompt));
  err = ACAPI_UserInput_GetPoint(&pointInfo);
  if (err != NoError)
    return err;
  endHostGuid = pointInfo.neig.guid;
  if (endHostGuid == APINULLGuid)
    return APIERR_BADPARS;

  // TODO: layer index hardcoded to "use placement default" — swap in
  // a real layer selection policy (active layer, a dedicated
  // "Circuit Wiring" layer, etc).
  const short wireLayerIndex = 0;

  // Element creation/property writes are database work and must run
  // inside an undoable command; the two getPoint calls above are
  // interactive UI and deliberately outside it.
  API_Guid wireGuid = APINULLGuid;
  ACAPI_CallUndoableCommand("Create Circuit Wire", [&]() -> GSErrCode {
    wireGuid = wiring::ConnectionManager::connectObjectsWithGdlWire(
      startHostGuid, endHostGuid, wireLayerIndex);
    return (wireGuid != APINULLGuid) ? NoError : APIERR_GENERAL;
  });

  if (wireGuid == APINULLGuid)
    return APIERR_GENERAL;

  // Deliberately outside the undoable command above and non-fatal: a
  // notification subscription isn't project content, and losing live
  // move-tracking is preferable to discarding an otherwise-successful
  // wire creation.
  wiring::ConnectionManager::attachHostObserver(startHostGuid);
  wiring::ConnectionManager::attachHostObserver(endHostGuid);

  return NoError;
}

GSErrCode
MenuCommandDispatcher::selectCircuitWiring()
{
  GS::UniString circuitId = circuit::Selection::getCircuitIdOfSelection();
  if (circuitId.IsEmpty())
    return APIERR_NOTMINE;

  return circuit::Selection::selectWiring(circuitId);
}

GSErrCode
MenuCommandDispatcher::selectCircuitObjects()
{
  GS::UniString circuitId = circuit::Selection::getCircuitIdOfSelection();
  if (circuitId.IsEmpty())
    return APIERR_NOTMINE;

  return circuit::Selection::selectObjects(circuitId);
}

GSErrCode
MenuCommandDispatcher::showAbout()
{
  // TODO: shows the Report window (withDial=true), not a small alert
  // dialog — no separate always-visible "grayed out" info label exists
  // in the modern menu API, so this is a regular clickable item.
  ACAPI_WriteReport(GS::UniString("A2 Electrical — build " A2E_GIT_VERSION), true);
  return NoError;
}

} // namespace commands
