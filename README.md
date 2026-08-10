# test-ac-addon — Circuit Wiring

Archicad 29 C++ Add-On: curved wires that stay attached to the objects
they connect, plus selection of a whole circuit (its wiring, or its
objects) by a custom property.

## This is built on top of GRAPHISOFT's own template, not invented from scratch

The build system, entry points, and resource/library-part layout here
are copied and adapted from
**[GRAPHISOFT/archicad-addon-cmake](https://github.com/GRAPHISOFT/archicad-addon-cmake)**
(MIT licensed) and its submodule,
**[GRAPHISOFT/archicad-addon-cmake-tools](https://github.com/GRAPHISOFT/archicad-addon-cmake-tools)**
(vendored here as `Tools/`) — Graphisoft's own, actively maintained,
public CMake template for Archicad Add-Ons. An earlier version of this
skeleton reconstructed all of this from memory instead, which got
several things wrong (see git history if curious); everything below is
checked against the real template's actual files, not reconstructed.

`Tools/CMakeCommon.cmake` does all DevKit discovery, compiler flags,
and resource compilation — this repo's own `CMakeLists.txt` just points
it at `Src/` and `config.json`, the same way the upstream template's
own `CMakeLists.txt` does for its own example source.

## What's here

```
config.json                  Add-On metadata (name/version/languages) — read by CMake
CMakeLists.txt               thin wrapper around Tools/CMakeCommon.cmake
Tools/                       archicad-addon-cmake-tools, git submodule (build logic, not ours)
Src/APIEnvir.h               DevKit-required boilerplate, copied from upstream
Src/PrecompiledHeader.hpp    PCH, adapted from upstream's example
Src/ResourceIds.hpp          resource + menu-item IDs
Src/AddOnMain.cpp            required Add-On lifecycle entry points
Src/Wiring/WireEnd.hpp       shared Start/End enum used by both wire backends below
Src/Wiring/WireElement.*     backend 1: curved wire as a native Spline element
Src/Wiring/GdlWireElement.*  backend 2: curved wire as a custom GDL object (see RINT/ACLib below)
Src/Wiring/ElementAnchor.*   reads an element's own placement point — what a wire endpoint tracks
Src/Wiring/WireConnection.*  attach a wire endpoint to a host element, track its moves, either backend
Src/Circuit/CircuitProperty.* custom "Circuit ID" property definition + get/set
Src/Circuit/CircuitSelection.* select-by-circuit-property commands
Src/Commands/MenuCommands.*  menu command implementations, dispatched from AddOnMain.cpp
RFIX/AddOnFix.grc            language-independent resources (MDID, icon)
RFIX/Images/                 toolbar icon
RINT/AddOn.grc               menu strings (English/International)
RINT/BuiltInLibParts.grc     registers the GDL object below as built into the add-on itself
RINT/ACLib/Src/Circuit Wire/ the "Circuit Wire" GDL object's real on-disk source form
docs/ARCHITECTURE.md         how the three features map onto the API
```

## Setup

1. Get the **Archicad 29 API Development Kit**. Same download either
   way, two places to get it:
   - [archicadapi.graphisoft.com/downloads](https://archicadapi.graphisoft.com/downloads/api-development-kit)
   - [GRAPHISOFT/archicad-api-devkit releases on GitHub](https://github.com/GRAPHISOFT/archicad-api-devkit/releases)
2. Point the build at its **`Support`** subfolder (not the DevKit root
   — `Tools/CMakeCommon.cmake`'s `verify_api_devkit_folder` checks for
   a folder literally named `Support`):
   ```
   export AC_API_DEVKIT_DIR="/path/to/API Development Kit 29/Support"
   ```
3. Clone submodules if you haven't (`Tools/` is one):
   ```
   git submodule update --init --recursive
   ```
4. Configure and build. Two ways:
   - The upstream build script (handles resource compilation, PDB/dSYM
     packaging, etc. for you):
     ```
     python Tools/BuildAddOn.py --configFile config.json --acVersion 29 \
         --devKitPath "$AC_API_DEVKIT_DIR" --buildNum 3100
     ```
   - Or plain CMake:
     ```
     cmake -B Build -DAC_API_DEVKIT_DIR="$AC_API_DEVKIT_DIR" -DAC_VERSION=29
     cmake --build Build --config RelWithDebInfo
     ```
     (On Windows, add `-G "Visual Studio 17 2022" -A x64 -T v143`; on
     Mac, `-G Xcode`. See the upstream README's version tables for
     which toolset/deployment-target pairs with which Archicad
     version.)
   - Windows produces `CircuitWiring.apx`; Mac produces
     `CircuitWiring.bundle` (name comes from `config.json`'s
     `addOnName`).
5. To use it: Archicad's Add-On Manager, add the built `.apx`/`.bundle`.
   The example/template note applies here too — some interactive-input
   calls may only work against a demo-mode Archicad
   (`Archicad.exe -DEMO` / `open Archicad\ 29.app --args -demo`).

## Architecture overview (see docs/ARCHITECTURE.md for detail)

Two interchangeable wire backends, both 2D-only for now:

- **Native Spline** (`WireElement.*`) — a real `API_SplineType`
  element, so Archicad's own edit/undo/snap machinery applies for free,
  and it can have any number of curve nodes. No placement UI is wired
  up yet (see `Commands::CreateWireCommand`) — this backend is the one
  to extend if/when you want interactive multi-node curve drawing.
- **GDL object** (`GdlWireElement.*`, library part in
  `RINT/ACLib/Src/Circuit Wire/`) — a small custom Object with
  `endX`/`endY`/`bulge` parameters; the 2D script draws a single
  curved (or straight) line from its origin to the far end. Always
  exactly two points, which is what makes the click-click placement
  flow below possible. Shipped as a **built-in** library part (baked
  into the add-on bundle, auto-registered at startup — see
  `HasBuiltInLibPart`/`RegisterInterface` in `AddOnMain.cpp`), not
  something you add to a project library by hand.
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

## What's still a guess, and why it's a much smaller list now

The build system, entry points (`CheckEnvironment`/`RegisterInterface`/
`Initialize`/`FreeData`), menu registration calls, and the GDL
library-part file format are now copied from real, working Graphisoft
source — not reconstructed. What's *not* covered by that public
template, because it's inside the proprietary DevKit headers I still
don't have direct access to, is the business logic that touches
element/property/notification internals: `API_SplineType`'s exact
field layout, `API_AddParType`'s field names for reading/writing GDL
object parameters, the exact `ACAPI_Notification_*`/
`ACAPI_UserInput_ClickAnElement` signatures, and similar. Every one of
those is still flagged with a `// DEVKIT:` comment at the point it's
used — that's the honest remaining gap, not the whole skeleton.
