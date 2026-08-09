# Architecture

Three requirements drive the module layout:

1. Create custom curved plines/splines.
2. Connect a wire's endpoint(s) to an object so the wire follows when
   the object moves.
3. Later: select all wires in a circuit, and all objects in a circuit,
   based on a custom property set at connect-time.

## 1. Curved wires — `Src/Wiring/WireElement.*`

Use Archicad's native **Spline** element type (`API_SplineType`) rather
than a GDL library part:

- It already has Bezier-style direction control vectors per node
  (`API_SplineDirs`), so "curved pline" is a first-class shape, not
  something you approximate with arcs.
- It participates in native undo, snap, pen/layer/floor plan display,
  and 3D without any of that being reimplemented.
- Downside: less visual customization than a GDL object (no custom 2D
  script) — acceptable for wiring, which is really just geometry plus a
  couple of properties.

`WireElement.cpp` owns:
- `CreateWire(nodes, directions, layer) -> API_Guid`
- `SetWireNodes(guid, nodes, directions)` — used after a connected
  endpoint moves.

If curvature needs go beyond what `API_SplineType` gives you (e.g.
constant-radius bends, not just Bezier), the fallback is `API_PolyLineType`
with `curveType = APICurve_Bezier` per segment — same module, swap the
element type, same call sites.

## 2. Live connection to an object — `Src/Wiring/WireConnection.*`

Archicad has no built-in "attach line to object" relationship for
generic elements (that exists for MEP systems, not arbitrary
Spline+Object pairs), so this skeleton builds it from two primitives:

- **Where the wire remembers what it's attached to.** Store, per wire
  endpoint, the host element's `API_Guid` and an attachment descriptor
  (which hotspot index, or an anchor-point-relative-to-origin offset)
  in the wire element's own memo (`ACAPI_Element_GetMemo` /
  `ACAPI_Element_SetMemo`, using the free-form "extra" segment, or a
  private property if the memo route turns out to be too fragile across
  copy/paste — decide once you're against real headers).
- **How the wire finds out the host moved.** Register an element
  observer on the host GUID via
  `ACAPI_Notification_InstallElementObserver`. On
  `APINotify_ChangeType` (or whatever AC29 names the "geometry changed"
  reason), read the host's new anchor point/transformation and call
  `WireElement::SetWireNodes` on every wire that references it, then
  `ACAPI_Element_Change`.
- **Reconnecting after undo/redo/copy.** Observers don't survive across
  undo boundaries or document reload by themselves — re-install them
  from `Initialize()` by scanning existing wires' stored host GUIDs, and
  again after any undo notification the DevKit exposes
  (`APINotify_UndoRedo` or similar).

This is the module most worth re-checking against the real AC29 headers
first: the exact notification reason enum and the memo layout are the
two things most likely to have shifted.

## 3. Circuit property + selection — `Src/Circuit/*`

- `CircuitProperty.cpp` creates one custom property definition
  ("Circuit ID", string or integer, your choice) once per project via
  `ACAPI_Property_CreatePropertyDefinition`, idempotently (look it up
  first, create only if missing). `SetCircuitId(elemGuid, id)` /
  `GetCircuitId(elemGuid)` wrap `ACAPI_Property_SetPropertyValue` /
  `GetPropertyValue`.
- The **connect** command (in `MenuCommands.cpp`, calling into
  `WireConnection::Connect`) is where the property actually gets
  written: connecting a wire to an object stamps both elements with the
  same Circuit ID (generating a new one if neither side has one yet, or
  reusing the existing one if you're extending a circuit).
- `CircuitSelection.cpp` implements the two later-phase commands:
  - `SelectCircuitWiring(circuitId)` — filter all Spline/PolyLine
    elements by Circuit ID property value, `ACAPI_Selection_Select`.
  - `SelectCircuitObjects(circuitId)` — same filter over Object
    elements.
  Both are one function today (`SelectByCircuitProperty(elemTypeFilter,
  circuitId)`) parameterized by element type, since the query logic is
  identical — only which element types you scan differs.

## Menu commands — `Src/Commands/MenuCommands.*`

Thin layer: menu item IDs (`RFIX/RFIX.grc`) map to functions here, which
call the modules above. Keeping command dispatch separate from the
wiring/property logic means the logic is testable/reusable if you later
add a palette or toolbar instead of (or in addition to) menu items.
