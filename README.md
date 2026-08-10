# test-ac-addon

Skeleton for an Archicad 29 C++ Add-On implementing "circuit wiring":
curved wires that stay attached to the objects they connect, plus
selection of a whole circuit (its wiring, or its objects) by a custom
property.

This is a **skeleton**, not a buildable add-on yet: the Archicad API
Development Kit (APIDevKit) is a proprietary Graphisoft download that
requires a developer-portal login, so it isn't vendored here. The code
compiles against `ACAPinc.h` and the `ACAPI_*` call families exactly as
Graphisoft's headers declare them; you'll need the real headers present
before `cmake --build` will succeed. See "Setup" below.

## What's here

```
CMakeLists.txt              top-level build, both platforms
cmake/FindArchicadAPIDevKit.cmake   locates the DevKit via env var / cache var
Src/AddOnMain.cpp            required Add-On lifecycle entry points
Src/AddOnIdentity.hpp        Add-On GUIDs / name — placeholders, regenerate before shipping
Src/Wiring/WireEnd.hpp       shared Start/End enum used by both wire backends below
Src/Wiring/WireElement.*     backend 1: curved wire as a native Spline element
Src/Wiring/GdlWireElement.*  backend 2: curved wire as a custom GDL object (see Library/CircuitWire)
Src/Wiring/ElementAnchor.*   reads an element's own placement point — what a wire endpoint tracks
Src/Wiring/WireConnection.*  attach a wire endpoint to a host element, track its moves, either backend
Src/Circuit/CircuitProperty.* custom "Circuit ID" property definition + get/set
Src/Circuit/CircuitSelection.* select-by-circuit-property commands
Src/Commands/MenuCommands.*  menu wiring that calls into the above
RFIX/RFIX.grc                menu strings / resource IDs (Windows .rc2 mirrors this)
Library/CircuitWire/         the GDL object's script + setup instructions
docs/ARCHITECTURE.md         how the three features map onto the API
```

## Setup

1. Get the **Archicad 29 API Development Kit** from Graphisoft's developer
   site (requires a free developer account): the DevKit ships
   `Support/Inc/ACAPinc.h`, the rest of the `Support/Inc` headers, and the
   import libraries for both platforms.
2. Point the build at it, either:
   - `export AC_API_DEVKIT_DIR=/path/to/API Development Kit 29` before
     running CMake, or
   - `cmake -S . -B build -DAC_API_DEVKIT_DIR="/path/to/API Development Kit 29"`
3. Configure and build:
   ```
   cmake -S . -B build
   cmake --build build --config RelWithDebInfo
   ```
   - Windows produces `TestACAddOn.apx` (a renamed DLL).
   - Mac produces `TestACAddOn.bundle`.
4. Regenerate the Add-On's identity GUIDs in `Src/AddOnIdentity.hpp`
   (any GUID generator works) before you ever load this into a real
   Archicad — two add-ons sharing a GUID will conflict.
5. If you want the GDL-object wire backend (the "click start object,
   click end object" flow), follow `Library/CircuitWire/README.md` to
   create that library part in Archicad and attach it to your test
   project. The native-Spline backend doesn't need this step.

## Architecture overview (see docs/ARCHITECTURE.md for detail)

Two interchangeable wire backends, both 2D-only for now:

- **Native Spline** (`WireElement.*`) — a real `API_SplineType`
  element, so Archicad's own edit/undo/snap machinery applies for free,
  and it can have any number of curve nodes. No placement UI is wired
  up yet (see `Commands::CreateWireCommand`) — this backend is the one
  to extend if/when you want interactive multi-node curve drawing.
- **GDL object** (`GdlWireElement.*`, library part in
  `Library/CircuitWire/`) — a small custom Object with `endX`/`endY`/
  `bulge` parameters; the 2D script draws a single curved (or straight)
  line from its origin to the far end. Always exactly two points, which
  is what makes the click-click placement flow below possible.
  `Commands::CreateWireBetweenObjectsCommand` is wired up end-to-end:
  click a start object, click an end object, and it places one of
  these directly between their anchor points.
- **Live connection to objects** — a wire endpoint is anchored to a
  host element's own placement origin (`ElementAnchor.cpp` —
  deliberately not a specific hotspot/edge, to keep "click an object"
  a complete connect gesture). `WireConnection.cpp` registers an
  element observer (`ACAPI_Notification_InstallElementObserver`) on
  the host; when it fires, `OnHostElementChanged` recomputes and pushes
  new geometry through whichever backend that wire uses
  (`WireElement::SetWireEndpoint` or `GdlWireElement::SetGdlWireEndpoint`).
- **Circuit selection** — a custom property definition ("Circuit ID")
  is created once via `ACAPI_Property_CreatePropertyDefinition` and
  written onto every wire/object as it's connected, by either backend.
  Selection commands in `CircuitSelection.cpp` filter elements by that
  property value and call `ACAPI_Selection_Select`.

## Verifying API call names against your DevKit

Graphisoft renamed most `ACAPI_*` entry points into namespaced groups
(`ACAPI_Element_*`, `ACAPI_MenuItem_*`, `ACAPI_Notification_*`,
`ACAPI_Property_*`, `ACAPI_Selection_*`, ...) starting around Archicad 26,
and individual signatures do shift slightly release to release. Every
call in this skeleton is flagged with a `// DEVKIT:` comment where you
should diff against the AC29 headers
(`Support/Inc/ACAPI_*Procedures.hpp`) once you have them, since I wrote
this without the actual AC29 header set in front of me.
