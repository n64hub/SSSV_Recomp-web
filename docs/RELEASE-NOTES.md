This release includes Windows x64, Linux x64, macOS Apple Silicon and macOS Intel packages, SHA-256 checksums, and the corresponding source archive.

The packages include this fork's widescreen/UI scaling fixes, intro and transition improvements, HD Text and HD Hud options, graphics stability changes, and smoother credits. See [the changelog](https://github.com/scottjco/SSSV_Recomp/blob/main/CHANGELOG.md) for the full list.

**No ROM or save data is included.** Supply your own supported USA 1.0 ROM through the first-launch file picker. Extract the complete package to a writable folder.

- Windows: run `SSSVRecompiled.exe` with its DLLs and assets alongside it.
- Linux: built on Ubuntu 24.04; requires SDL2, GTK3, FreeType, X11/XRandR and a Vulkan-capable graphics driver.
- macOS: separate Apple Silicon and Intel apps, targeting macOS 15+. Apps are ad-hoc signed, not Developer ID signed or notarized.

These packages are compiled by GitHub Actions. Linux and macOS gameplay/graphics still need testing on actual systems. The Windows version has been exercised locally; the GitHub-built packages may differ in toolchain details.

Based on Cellenseres/SSSV_Recomp, N64: Recompiled, RT64, and the work of their contributors. Third-party licenses and notices are included with each package.
