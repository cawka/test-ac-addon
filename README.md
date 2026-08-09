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
Src/Wiring/WireElement.*     create/update the curved wire (Spline) element
Src/Wiring/WireConnection.*  attach a wire endpoint to a host element, track its moves
Src/Circuit/CircuitProperty.* custom "Circuit ID" property definition + get/set
Src/Circuit/CircuitSelection.* select-by-circuit-property commands
Src/Commands/MenuCommands.*  menu wiring that calls into the above
RFIX/RFIX.grc                menu strings / resource IDs (Windows .rc2 mirrors this)
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

## Architecture overview (see docs/ARCHITECTURE.md for detail)

- **Curved wires** — modeled as native Archicad `Spline` elements
  (`API_SplineType`), not GDL objects, so Archicad's own edit/undo/snap
  machinery applies to them for free. `WireElement.cpp` is where node/
  direction-vector editing lives.
- **Live connection to objects** — a wire endpoint stores the host
  element's GUID plus an attachment description (hotspot index or a
  parameter along its placement) in the wire's own memo/custom data.
  `WireConnection.cpp` registers an element observer
  (`ACAPI_Notification_InstallElementObserver`) on the host; when it
  fires, the wire's endpoint is recomputed and the element is updated
  via `ACAPI_Element_Change`.
- **Circuit selection** — a custom property definition ("Circuit ID")
  is created once via `ACAPI_Property_CreatePropertyDefinition` and
  written onto every wire/object as it's connected. Selection commands
  in `CircuitSelection.cpp` filter elements by that property value and
  call `ACAPI_Selection_Select`.

## Verifying API call names against your DevKit

Graphisoft renamed most `ACAPI_*` entry points into namespaced groups
(`ACAPI_Element_*`, `ACAPI_MenuItem_*`, `ACAPI_Notification_*`,
`ACAPI_Property_*`, `ACAPI_Selection_*`, ...) starting around Archicad 26,
and individual signatures do shift slightly release to release. Every
call in this skeleton is flagged with a `// DEVKIT:` comment where you
should diff against the AC29 headers
(`Support/Inc/ACAPI_*Procedures.hpp`) once you have them, since I wrote
this without the actual AC29 header set in front of me.
