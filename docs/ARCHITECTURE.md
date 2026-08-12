# Architecture

Three requirements drive the module layout:

1. Create custom curved plines/splines.
2. Connect a wire's endpoint(s) to an object so the wire follows when
   the object moves.
3. Later: select all wires in a circuit, and all objects in a circuit,
   based on a custom property set at connect-time.

## 1. Curved wires — two backends, pick per use case

### 1a. Native Spline — `src/wiring/WireElement.*`

`API_SplineType`, not a GDL library part:

- It already has Bezier-style direction control vectors per node
  (`API_SplineDirs`), so "curved pline" is a first-class shape with any
  number of nodes, not something you approximate with arcs.
- It participates in native undo, snap, pen/layer/floor plan display,
  and 3D without any of that being reimplemented.
- Downside: less visual customization than a GDL object (no custom 2D
  script), and there's no placement UI wired up yet — see the TODO in
  `Commands::CreateWireCommand`.

`WireElement.cpp` owns:
- `CreateWire(nodes, layer) -> API_Guid`
- `SetWireNodes(guid, nodes)` / `SetWireEndpoint(guid, end, point)` —
  the latter is what connection-tracking calls after a host moves.

If curvature needs go beyond what `API_SplineType` gives you (e.g.
constant-radius bends, not just Bezier), the fallback is `API_PolyLineType`
with `curveType = APICurve_Bezier` per segment — same module, swap the
element type, same call sites.

### 1b. GDL object — `src/wiring/GdlWireElement.*` + `RINT/ACLib/Src/Circuit Wire/`

A small custom Object library part ("Circuit Wire") with three
parameters — `endX`, `endY`, `bulge` — and a 2D script
(`scripts/2d.gdl`) that draws one curved (or straight, if `bulge` is 0)
line from the placement origin to the far end. Shipped as a **built-in**
library part: `RINT/BuiltInLibParts.grc` bundles
`RINT/ACLib/Src/Circuit Wire/` into the add-on itself (compiled in by
`Tools/CompileResources.py`), and `AddOnMain.cpp`'s
`HasBuiltInLibPart`/`RegisterInterface` detect that and call
`ACAPI_AddOnIntegration_RegisterBuiltInLibrary ()` at startup — so
there's no manual "add this to your library" step for whoever loads the
add-on. That on-disk folder format (`libpartdata.xml`, `libpartdocs.xml`,
`ancestry.xml`, `calledmacros.xml`, `paramlist.xml`, `scripts/2d.gdl`,
`scripts/3d.gdl`) is copied from GRAPHISOFT/archicad-addon-cmake's own
`SampleObject` built-in part, with its parameters swapped for ours —
not reconstructed from memory, unlike an earlier version of this file.

Why this exists alongside the Spline backend, not instead of it: it's
always exactly two points (start, end), which is what makes a
"click the start object, click the end object" placement gesture
complete with nothing else to specify — see
`Commands::CreateWireBetweenObjectsCommand` and
`Wiring::ConnectObjectsWithGdlWire`, which is the fully wired-up path
in this skeleton (unlike the Spline backend's placement command, which
is still a TODO). The tradeoff for that simplicity: it's genuinely just
two points and a bulge, not a true multi-node curve — fine per the
brief ("don't have to match exact spline, just endpoints"), but revisit
if a circuit ever needs a wire to route around something.

`GdlWireElement.cpp` owns:
- `CreateGdlWire(startPoint, endPoint, layer) -> API_Guid`
- `SetGdlWireEndpoint(guid, end, point)` — same role as
  `WireElement::SetWireEndpoint`, called from the same
  `OnHostElementChanged` dispatch (section 2 below).

## 2. Live connection to an object — `src/wiring/WireConnection.*`

Archicad has no built-in "attach line to object" relationship for
generic elements (that exists for MEP systems, not arbitrary
wire+Object pairs), so this skeleton builds it from three primitives,
shared by both wire backends:

- **What a wire endpoint is anchored to.** Deliberately just "this
  object" — `ElementAnchor.cpp`'s `GetElementAnchorPoint` reads the
  host's own placement origin (`element.object.pos` / `.lamp.pos`
  today; extend the switch for other element types as needed). Not a
  specific hotspot or edge point. That's what makes "click the start
  object, click the end object" a complete connect gesture with
  nothing further to pick — it's also the piece most worth revisiting
  if a host's *shape*, not just its origin, needs to matter later (e.g.
  anchoring to a specific corner of a panel rather than its insertion
  point).
- **Where the wire remembers what it's attached to.** Store, per wire
  endpoint, the host element's `API_Guid` (`ConnectionInfo`) in the wire
  element's own memo (`ACAPI_Element_GetMemo` / `ACAPI_Element_SetMemo`,
  using the free-form "extra" segment, or a private property if the
  memo route turns out to be too fragile across copy/paste — decide
  once you're against real headers; `LoadConnection`/`StoreConnection`
  in `WireConnection.cpp` are the TODO stubs for this).
- **How the wire finds out the host moved, and which backend to push
  the update through.** `ACAPI_Element_InstallElementObserver` (once,
  globally) plus `ACAPI_Element_AttachObserver` (per host) is meant to
  make `OnHostElementChanged` fire when a host moves, reading its new
  anchor point and dispatching to whichever backend that wire actually
  is — `GdlWireElement::SetGdlWireEndpoint` if `IsGdlWireElement` says
  so, `WireElement::SetWireEndpoint` otherwise.
  **KNOWN GAP, not working yet:** `AttachHostObserver`
  (`WireConnection.cpp`) consistently fails at runtime with a real,
  confirmed-against-the-actual-header error
  (`ACAPI_Element_AttachObserver`'s only documented return code,
  `APIERR_BADID`, is ruled out numerically — see the comment on
  `AttachHostObserver` for the full trail). Wires are created and
  connected correctly (position, geometry, Circuit ID) but do **not**
  currently move when their host moves. Leading unconfirmed suspect:
  `notifyFlags` is left at its default (`GSFlags notifyFlags = 0`),
  which may not be a valid flags value — the real valid constants for
  this parameter aren't confirmed yet.
- **Reconnecting after undo/redo/copy.** Observers don't survive across
  undo boundaries or document reload by themselves — re-install them
  from `Initialize()` by scanning existing wires (both backends) for
  their stored host GUIDs, and again after any undo notification the
  DevKit exposes (`APINotify_UndoRedo` or similar).

This is the module most worth re-checking against the real AC29 headers
first: the exact notification reason enum and the memo layout are the
two things most likely to have shifted.

## 3. Circuit property + selection — `src/circuit/*`

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

## Menu commands — `src/commands/MenuCommands.*`

Thin layer: `MenuCommandHandler` (registered against each
`ID_ADDON_MENU_*` resource in `AddOnMain.cpp` — one 2-item `STR#` per
command, not one shared list; see the note in `src/ResourceIds.hpp`
about why a single `[title, cmd, cmd, ...]` list folds every command
after the first into a submenu instead of staying flat under "A2")
switches on `menuResID` (`src/ResourceIds.hpp`, kept in sync with
`RINT/AddOn.grc`) and calls functions here, which call the modules
above. Keeping command dispatch separate from the wiring/property logic
means the logic is testable/reusable if you later add a palette or
toolbar instead of (or in addition to) menu items.
