High-resolution in-game font

Latin letters, digits and common English punctuation are rendered from Comic
Neue Bold outlines into 256x256 glyph textures. Original text advances, wrapping,
colors, shadows, clipping, transitions and aspect-ratio placement are retained.
Every glyph uses a shared outline scale and baseline, preserving the font's
natural relative heights and proportions. The common size leaves padding within
the original character advances; glyphs are never stretched individually.

Music/speaker symbols, volume controls, Japanese glyphs, the digital HUD font and
lettering baked into pictures retain their original artwork.

Comic Neue: https://github.com/crozynski/comicneue
Copyright 2014 The Comic Neue Project Authors. Licensed under SIL OFL 1.1;
see OFL.txt. The replacement images are generated from this font, not ROM art.

The source font and generator are included in the source workspace. To regenerate
the pack, run tools/build_hd_font.py with a supported local ROM and rncu DLL.
No ROM data is included in this pack.
