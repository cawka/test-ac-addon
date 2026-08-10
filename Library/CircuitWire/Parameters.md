# Circuit Wire — parameters

Add these on the Object Editor's Parameters page. All three are written
by `Src/Wiring/GdlWireElement.cpp` at create/update time — nothing else
should need to touch them.

| Name    | Type   | Default | Meaning                                                                 |
|---------|--------|---------|--------------------------------------------------------------------------|
| `endX`  | Length | `0`     | Far endpoint's X offset from the placement origin (global-axis delta — the object is never rotated, so no trig is needed on the C++ side). |
| `endY`  | Length | `0`     | Far endpoint's Y offset from the placement origin.                      |
| `bulge` | Length | `0`     | Perpendicular offset of the curve's midpoint from the straight start→end chord. `0` = straight line. Not written by the add-on yet (`GdlWireElement::CreateGdlWire` leaves it at its default) — wire up a way to set it (menu command, or the object's own parameter dialog) once you want curved-by-default wires rather than editing it by hand per instance. |
