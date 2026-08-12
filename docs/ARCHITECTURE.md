# Architecture

Three requirements drive the module layout:

1. Create custom curved plines/splines.
2. Connect a wire's endpoint(s) to an object so the wire follows when
   the object moves.
3. Select all wires in a circuit, and all objects in a circuit, based
   on a custom property set at connect-time.

## 1. Curved wires — two backends, pick per use case

### 1a. Native Spline — `wiring::SplineWireElement` (`src/wiring/WireElement.*`)

`API_SplineType`, not a GDL library part:

- Has Bezier-style direction control vectors per node
  (`API_SplineDirs`), so "curved pline" is a first-class shape with any
  number of nodes, not something approximated with arcs.
- Participates in native undo, snap, pen/layer/floor plan display, and
  3D without reimplementing any of that.
- Less visual customization than a GDL object (no custom 2D script),
  and there's no placement UI wired up yet — see the TODO on
  `commands::MenuCommandDispatcher::createWire`.

TODO: unimplemented — `API_SplineType`'s node/direction storage is
memo-backed; needs the AC29 struct layout confirmed before `create`/
`setNodes`/`setEndpoint` can be filled in.

If curvature needs go beyond Bezier (e.g. constant-radius bends), the
fallback is `API_PolyLineType` with `curveType = APICurve_Bezier` per
segment — same module, swap the element type, same call sites.

### 1b. GDL object — `wiring::GdlWireElement` (`src/wiring/GdlWireElement.*` + `RINT/ACLib/Src/Circuit Wire/`)

A custom Object library part ("Circuit Wire") with parameters `endX`,
`endY`, `bulge`, `linePen`, `lineType`, and a 2D script
(`scripts/2d.gdl`) that draws one curved (or straight, if `bulge` is 0)
line from the placement origin to the far end. Shipped as a
**built-in** library part: `RFIX/BuiltInLibParts.grc` bundles
`RINT/ACLib/Src/Circuit Wire/` into the add-on itself, and
`AddOnMain.cpp`'s `RegisterInterface` calls
`ACAPI_AddOnIntegration_RegisterBuiltInLibrary()` unconditionally at
startup — no manual "add this to your library" step needed.

Why this exists alongside the Spline backend: it's always exactly two
points (start, end), which is what makes "click the start object,
click the end object" a complete placement gesture with nothing else
to specify — see `commands::MenuCommandDispatcher::createWireBetweenObjects`
and `wiring::ConnectionManager::connectObjectsWithGdlWire`, the fully
wired-up path in this skeleton. The tradeoff: it's genuinely just two
points and a bulge, not a true multi-node curve.

Geometry is encoded as (placement origin, rotation angle, length): the
object's own X axis points from its origin toward the far end, so
`endX` is the segment length and `endY` is always 0 in the object's
local frame.

## 2. Live connection to an object — `wiring::ConnectionManager` (`src/wiring/WireConnection.*`)

Archicad has no built-in "attach line to object" relationship for
generic elements, so this skeleton builds it from three primitives,
shared by both wire backends:

- **What a wire endpoint is anchored to.** `wiring::ElementAnchor::getPoint`
  reads the host's own placement origin (`element.object.pos` /
  `.lamp.pos` today; extend the switch for other element types as
  needed) — deliberately not a specific hotspot or edge point, which is
  what keeps "click an object" a complete connect gesture.
- **Where the wire remembers what it's attached to.**
  `ConnectionManager::loadConnection`/`storeConnection` are TODO stubs
  — need to persist the host `API_Guid` per wire endpoint (wire
  element's memo "extra" segment, or a private property) so connections
  survive save/reload.
- **How the wire finds out the host moved.**
  `ACAPI_Element_InstallElementObserver` (installed once, in
  `AddOnMain.cpp`'s `Initialize()` — a notification registration isn't
  a database write and must not run inside an
  `ACAPI_CallUndoableCommand`) plus a per-host
  `ConnectionManager::attachHostObserver` call make
  `ConnectionManager::onHostElementChanged` fire when a host moves,
  dispatching to whichever backend that wire actually is.
  NOTE: `attachHostObserver` is called with `notifyFlags` left at its
  default (0); not confirmed whether that's a valid mask.
- **Reconnecting after undo/redo/copy.** TODO:
  `ConnectionManager::restoreAllConnectionObservers` needs to enumerate
  existing wires (both backends) and re-attach their host observers —
  currently a no-op.

## 3. Circuit property + selection — `src/circuit/*`

- `circuit::PropertyManager` owns one custom property definition
  ("Circuit ID", string), created idempotently via
  `ensureDefinition()`. `setId`/`getId` read and write it per element.
- `ConnectionManager::connect` is where the property actually gets
  written: connecting a wire to an object stamps both elements with the
  same Circuit ID (generating a new one if neither side has one yet, or
  reusing the existing one if extending a circuit).
- `circuit::Selection` implements the later-phase commands:
  `selectWiring(circuitId)` and `selectObjects(circuitId)`, both
  backed by one parameterized `selectByCircuitId(circuitId, keepWires)`
  since the query logic is identical and only the element-type filter
  differs.
  TODO: `selectByCircuitId` needs a real element enumeration
  (`ACAPI_Element_Filter` or similar) — currently scans an empty list.

## Menu commands — `commands::MenuCommandDispatcher` (`src/commands/MenuCommands.*`)

Thin layer: `handle` (registered against each `ID_ADDON_MENU_*`
resource in `AddOnMain.cpp` — one 2-item `STR#` per command, see
`src/ResourceIds.hpp`) switches on `menuResID` and calls the private
per-command methods, which call the modules above.
