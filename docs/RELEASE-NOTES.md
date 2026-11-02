This update fixes the black world in **Sting in the Tail**, protects dense scenes from model-matrix and opaque display-list overflows, and fixes plants and other queued scenery disappearing in crowded views.

**HD World Textures are incomplete.** This update was released urgently after discovering that Sting in the Tail was broken, so the level fix could reach players without waiting for the texture pack to be finished.

It also adds **HD World Textures** for level sprites and tutorial signs, an external `--rom "path/to/file.z64"` option, and automatic import of the original save only when the portable `saves/` folder contains no save data. Existing saves and the original source save are preserved.

The earlier widescreen/UI fixes, optional HD menus and HUD, intro improvements and smoother credits remain included. See [the changelog](https://github.com/scottjco/SSSV_Recomp/blob/main/CHANGELOG.md) and [comparison screenshots](https://github.com/scottjco/SSSV_Recomp#screenshot-comparisons).

**No ROM or save data is included.** Supply your own supported USA 1.0 ROM through the first-launch file picker, `assets/data.z64`, or the external ROM option. Extract the complete package to a writable folder.

Packages: Windows x64, Linux x64, macOS Apple Silicon and macOS Intel, with SHA-256 checksums and a corresponding source archive. Optional artwork lives in `Textures/`; removing that folder hides its HD settings.

- Windows: run `SSSVRecompiled.exe` with its DLLs and assets alongside it.
- Linux: built on Ubuntu 24.04; requires SDL2, GTK3, FreeType, X11/XRandR and a Vulkan-capable graphics driver.
- macOS: separate Apple Silicon and Intel apps, targeting macOS 15+. Apps are ad-hoc signed, not Developer ID signed or notarized.

The Windows candidate was exercised locally, including all 32 initial-level scenes, focused pickup tests and repeated same-angle plant approaches. This is not a claim of exhaustive objective testing. Linux and macOS gameplay/graphics still need testing on actual systems; these packages are compiled by GitHub Actions.

Based on Cellenseres/SSSV_Recomp, N64: Recompiled, RT64, and the work of their contributors. Third-party licenses and notices are included.
