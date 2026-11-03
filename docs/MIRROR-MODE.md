# Mirror Mode

Enable **Mirror Mode (Experimental)** in General settings. It is Off by default and does not
require the optional texture pack. The world is reflected horizontally; HUD,
on-screen text and menus retain their normal layout. Movement, D-pad left/right
and camera left/right are reversed while playing. Menu navigation is unchanged.

The pause preview captures the mirrored world, so returning to gameplay keeps
the same orientation. The mode uses raster rendering for the reflected world.

## Readable lettering on world textures

`Mirror.tex` belongs beside the executable (in the app's Resources on macOS).
It lists textures that need an extra horizontal flip so their lettering remains
readable within the mirrored scene. These rules work with original and HD art.
Normal mode never applies them. Restart after editing the file; removing it
disables these texture corrections while HUD and overlay text remain readable.

Each standalone rule uses this format:

```text
width height source_hash partner_hash # description
```

Repeat the same hexadecimal hash in both columns. Identifiers are **RT64 TMEM
v5 hashes**, including the native dimensions and format, rather than Banjo's
pixel-payload hashes. Each mip level has its own entry. This version supports
standalone corrections; it does not exchange separately tiled sign halves.

The supplied list covers the Earth/Silicon Valley signs, countdown digits,
sleeping Z particles, SV television cards, the Do Not Feed material, numbered
and GO race signs, and lettering/glyph panels found in the reviewed material
banks. Directional arrows are deliberately reflected with the world.
