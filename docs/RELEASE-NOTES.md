## 4 November 2026 — v2026.11.04

- Added **Fast Forward** with Disabled / 2X / 4X / 8X / 16X speeds, defaulting to 4X. Tab is the default keyboard binding; the controller binding is initially empty.
- Added Held (default) and Toggle activation, with safeguards to turn toggle mode off after a two-second hold or excessive repeated presses. Fast forward works in gameplay, menus and cutscenes, keeps accelerated audio playing, and displays a white FF indicator at the top left. Higher speeds depend on system performance.
- Corrected television static to use clearer, spaced white dots and native-rate animation, with more consistent visibility at different viewing distances.
- Restored the glass dome's reflection visibility checks and limited animal reflections to nearby animals. Reflections fade between four and five terrain tiles, reaching half visibility at 4.5 tiles.
- Extended the distance at which animals retain their proper models, reducing the coloured stand-ins seen at a distance.
- Fixed the temporary black rectangle over the title background when skipping startup screens quickly.
- Stabilised flower and other scenery sprite interpolation as objects enter view, and corrected the clipped edge of HD flowers.
- Extended the introductory glass pattern across widescreen and ultrawide views in both glass-breaking sequences, preserving the original breakable panes and shattering effects.

**HD World Textures remain incomplete.** Character eye and body replacements are deferred and excluded. The optional texture pack is bundled with each platform package.

**No ROM or save data is included.** Supply your supported USA 1.0 ROM through the first-launch picker, `assets/data.z64`, or `--rom "path/to/file.z64"`. Extract the complete package to a writable folder.

- Windows x64: run `SSSVRecompiled.exe` with the supplied assets and DLLs.
- Linux x64: built on Ubuntu 24.04; requires SDL2, GTK3, FreeType, X11/XRandR and a Vulkan-capable driver.
- macOS: separate Apple Silicon and Intel apps targeting macOS 15+. Apps are ad-hoc signed, not notarized.

The Windows build was checked locally for the changes above. Linux and macOS packages are compiled by GitHub Actions; native gameplay testing remains outstanding. Packages include licenses and SHA-256 checksums, plus a corresponding source archive.

See the [full changelog](https://github.com/scottjco/SSSV_Recomp/blob/main/CHANGELOG.md) and [screenshots](https://github.com/scottjco/SSSV_Recomp#screenshot-comparisons). Based on Cellenseres/SSSV_Recomp, N64: Recompiled, RT64 and their contributors' work.

Removed Herobrine.
