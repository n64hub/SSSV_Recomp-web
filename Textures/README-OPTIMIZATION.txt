Runtime artwork uses appropriately sized, losslessly compressed RGBA PNG files.
World sprites: up to 512 pixels; butterflies and small effects: up to 256;
signs and particle atlases: up to 1024; thumbnails: 768x576; trophies: 512.
Full-screen artwork, fonts and HUD assets retain their authored resolution.
Transparency is preserved; sprite resizing uses premultiplied alpha.

Developer workflow: tools/optimize_texture_pack.py --apply, then run
tools/register_hd_world.py and tools/register_hd_menu.py before building.
Full-resolution masters are retained locally in diagnostics/texture-masters
and are not included in the runtime pack.
