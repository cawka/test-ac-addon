# test-ac-addon — A2 Electrical

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
public CMake template for Archicad Add-Ons.

`Tools/CMakeCommon.cmake` does all DevKit discovery, compiler flags,
and resource compilation — this repo's own `CMakeLists.txt` just points
it at `src/` and `config.json`, the same way the upstream template's
own `CMakeLists.txt` does for its own example source.

## What's here

```
config.json                  Add-On metadata (name/version/languages) — read by CMake
CMakeLists.txt               thin wrapper around Tools/CMakeCommon.cmake
Tools/                       archicad-addon-cmake-tools, git submodule (build logic, not ours)
src/APIEnvir.h               DevKit-required boilerplate, copied from upstream
src/PrecompiledHeader.hpp    PCH, adapted from upstream's example
src/ResourceIds.hpp          resource + menu-item IDs
src/AddOnMain.cpp            required Add-On lifecycle entry points
src/wiring/WireEnd.hpp       shared Start/End enum used by both wire backends below
src/wiring/WireElement.*     backend 1: curved wire as a native Spline element
src/wiring/GdlWireElement.*  backend 2: curved wire as a custom GDL object (see RINT/ACLib below)
src/wiring/ElementAnchor.*   reads an element's own placement point — what a wire endpoint tracks
src/wiring/WireConnection.*  attach a wire endpoint to a host element, track its moves, either backend
src/circuit/CircuitProperty.* custom "Circuit ID" property definition + get/set
src/circuit/CircuitSelection.* select-by-circuit-property commands
src/commands/MenuCommands.*  menu command implementations, dispatched from AddOnMain.cpp
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
   - Windows produces `A2 Electrical.apx`; Mac produces
     `A2 Electrical.bundle` (name comes from `config.json`'s
     `addOnName`).
5. To use it: Archicad's Add-On Manager, add the built `.apx`/`.bundle`.
   The example/template note applies here too — some interactive-input
   calls may only work against a demo-mode Archicad
   (`Archicad.exe -DEMO` / `open Archicad\ 29.app --args -demo`).

## Debug output, and confirming which build is loaded

- **`ACAPI_WriteReport(message, withDial)`** — writes to Archicad's own
  Report window (`withDial=true` also pops a dialog). `Initialize()` in
  `AddOnMain.cpp` calls this once at load time with a build identifier
  (see below), and the "About A2 Electrical..." menu item calls it
  on demand. No debugger needed — this is the first thing to check.
- **`DBPrintf`** — lower-level debug-console output; only useful if
  Archicad itself is running under a debugger (Xcode: Debug > Attach to
  Process, pick the running Archicad process, once this add-on is
  loaded you can set breakpoints in this repo's source directly and see
  console output / step through code).
- **Build identifier**: `A2E_GIT_VERSION` (from `git describe --always
  --dirty --long`) is written into a generated header,
  `build/generated/GitVersion.hpp`, regenerated on every build (not
  just every `cmake` reconfigure — see `cmake/GenerateGitVersion.cmake`)
  so it always reflects the actual commit an add-on binary was built
  from. Surfaced via the Report-window message above.

## Architecture overview (see docs/ARCHITECTURE.md for detail)

Two interchangeable wire backends, both 2D-only for now:

- **Native Spline** (`Wiring::SplineWireElement`) — a real
  `API_SplineType` element, so Archicad's own edit/undo/snap machinery
  applies for free, and it can have any number of curve nodes.
  Unimplemented — see the TODOs on the class.
- **GDL object** (`Wiring::GdlWireElement`, library part in
  `RINT/ACLib/Src/Circuit Wire/`) — a custom Object with
  `endX`/`endY`/`bulge`/`linePen`/`lineType` parameters; the 2D script
  draws a single curved (or straight) line from its origin to the far
  end. Always exactly two points, which is what makes the click-click
  placement flow below possible. Shipped as a **built-in** library part
  (baked into the add-on bundle, auto-registered at startup — see
  `RegisterInterface` in `AddOnMain.cpp`), not something added to a
  project library by hand.
  `Commands::MenuCommandDispatcher::CreateWireBetweenObjects` is wired
  up end-to-end: click a start object, click an end object, and it
  places one of these directly between their anchor points.
- **Live connection to objects** — a wire endpoint is anchored to a
  host element's own placement origin (`Wiring::ElementAnchor` —
  deliberately not a specific hotspot/edge, to keep "click an object"
  a complete connect gesture). `Wiring::ConnectionManager` installs a
  global element observer (`ACAPI_Element_InstallElementObserver`, from
  `AddOnMain.cpp`'s `Initialize()`) plus a per-host
  `AttachHostObserver`; when a host moves, `OnHostElementChanged`
  recomputes and pushes new geometry through whichever backend that
  wire uses.
- **Circuit selection** — `Circuit::PropertyManager` owns a custom
  property definition ("Circuit ID"), written onto every wire/object as
  it's connected. `Circuit::Selection` filters elements by that
  property value and calls `ACAPI_Selection_Select`.

## Known gaps

See `docs/ARCHITECTURE.md` and the `// TODO:`/`// NOTE:` comments at
each point they apply — notably: the Spline backend's geometry code,
connection persistence across save/reload, observer re-attachment
after undo/redo, and element enumeration for circuit selection.
