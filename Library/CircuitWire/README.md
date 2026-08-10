# Circuit Wire (GDL library part)

The GDL-object alternative to the native-Spline wire (`Src/Wiring/WireElement.*`):
a small 2D-only symbol object with two custom parameters describing where
its far end is, plus a bulge amount so it can curve. `Src/Wiring/GdlWireElement.*`
on the C++ side is the only thing that ever writes these parameters —
everything else about the object is standard.

## Why this isn't checked in as a ready-to-load `.gsm`/library folder

Archicad's on-disk library-part container format (compressed `.gsm`, or
the text/HSF folder layout used for git-friendly libraries) has real,
Graphisoft-specific file/metadata structure that I don't have precise
enough recall of to fabricate here with confidence — getting it subtly
wrong would leave you debugging a "why won't this load" problem instead
of a "why won't this compile" one, which is worse than just saying so.

What's reliable, and what's here instead, is the actual **GDL script
content** — GDL itself is a stable, documented language independent of
the container format question.

## Setup (takes a few minutes)

1. In Archicad: **File > Libraries and Objects > Library Manager**,
   create a new empty embedded/local library (or use an existing one).
2. **File > Libraries and Objects > New... > Object**, name it
   `Circuit Wire` (must match `Wiring::kGdlWireLibPartName` in
   `Src/Wiring/GdlWireElement.hpp` — change one or the other if you
   rename it).
3. Open it in the Object Editor. On the **2D Script** page, paste in
   `2D_Script.gdl` from this folder.
4. On the **Parameters** page, add these three (see
   `Parameters.md` for the full table): `endX`, `endY`, `bulge` — all
   type Length, default `0`.
5. Save. Once it's in a library attached to your project, the
   `AC_API_DEVKIT_DIR`-built add-on will find it by name via
   `ACAPI_LibPart_Search` (see the `DEVKIT:` note in `GdlWireElement.cpp`
   for what to check there).
6. Once this round-trips cleanly, consider exporting it back out via
   **File > Libraries and Objects > Convert Library Part(s) to Text
   Format** — that gives you Graphisoft's actual, current on-disk
   layout for your Archicad version, which you can then check into this
   folder in place of (or alongside) this README-driven setup for a
   real git-friendly library workflow.

## What it intentionally doesn't do (yet)

- **No 3D geometry.** The 3D Script page can stay empty — you said 3D
  modeling can wait, and an object with only a 2D script is valid
  (it just won't render in 3D/section views yet).
- **No width/depth (A/B) parameters.** Most GDL objects have these for
  a rectangular footprint; a wire doesn't have one, so they're left out
  rather than added and ignored.
- **A single circular arc, not a multi-node spline.** See the caveats
  at the top of `2D_Script.gdl`. Good enough for "curve in 2D, endpoints
  have to match" — revisit if you later want S-curves or multiple bends
  in one wire.
