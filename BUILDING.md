# Building the web page

The desktop build is in the README. This is the page: the same game built with Emscripten into one web page that
draws through WebGPU, with [N64Recomp-browser](https://github.com/n64hub/N64Recomp-browser)'s web support. The game's
code is recompiled at build time from your own dump of Space Station Silicon Valley (USA 1.0, big-endian `.z64`); the
page holds none of it but the recompiled code, and takes the dump from its launcher's Load ROM.

## Tools

On Linux (or WSL): [emsdk](https://emscripten.org/docs/getting_started/downloads.html) active (`emsdk install
latest`), clang and ld.lld, CMake 3.20+, make, Ninja, Python 3, and
[naga](https://github.com/gfx-rs/wgpu/tree/trunk/naga) (`cargo install naga-cli`). N64Recomp-browser's README (Using
it in a recomp, What the build needs) has the rest: spirv-opt, and [ccache](https://ccache.dev) on `PATH`, which
N64Recomp-browser's `frontend/build_speed.cmake` puts in front of every compile.

## Build

```bash
git clone --recurse-submodules https://github.com/n64hub/SSSV_Recomp-web
cd SSSV_Recomp-web
bash tools/build_web.sh <your USA 1.0 dump>
```

`tools/build_web.sh` (its header says each step) patches the submodules, builds N64Recomp and RSPRecomp from the
patched `lib/N64ModernRuntime/N64Recomp`, recompiles the game (`sssv.us.toml`) and the audio microcode
(`aspMain.toml`) from the ROM in `build-web-gen`, and builds the page in `build-web`: `SSSVRecompiled.html`, `.js`,
`.wasm` and `.data`.

The patches are the game's own (`dependency-patches/`), then N64Recomp-browser's web support
(`lib/N64Recomp-browser/patches/`). For the files where the two change the same lines (`web/patches/files.txt`) the
web's change is `web/patches/<project>.patch`, merged by hand, and what N64Recomp-browser has changed in them since is
merged in by its `apply.cmake --since`. A change there that meets the game's own lines stops the build: merge it into
`web/patches` by hand then, as `files.txt` says. `bash tools/build_web.sh --patches` does only that step, without a
ROM; N64Recomp-browser's CI runs it on every change there.

The recompiled game calls the port's hooks (`include/sssv_hooks.h`, forced into every recompiled file on the page,
where an undeclared function is an error rather than a guess at its type).

Serve `build-web` with `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp`
(threads need SharedArrayBuffer), or with `coi_serviceworker.js` beside the page where headers can't be set, and open
it in a browser with WebGPU. The ROM, settings and saves are kept in the browser's storage.

## What the page leaves out

The launcher's 3D logo: `lib/CellenseresSDK/src/launcher_model3d.cpp` draws it through plume's Vulkan, D3D12 and
Metal backends, so on the page `launcher_model3d_web.cpp` stands in for it and the launcher shows its menu alone.
The HD text and textures: their options show only where the texture pack is, and the page has none. Haptics and the
file dialog: Emscripten's SDL2 has no haptics, and the launcher's Load ROM takes the dump. Everything else is the
desktop build's, and a page starts with Framerate at Display, RT64 interpolating to the display's rate, and Resolution
at Auto, the canvas's size.

## The shader list

The page specializes the shaders in `web/rt64_shaders.txt` at build time (`RT64_PREBUILT_SHADER_LIST`), so that it
never draws with RT64's slow general shader. The list is recorded by playing: N64Recomp-browser's
`tools/record_shaders.mjs` plays `web/shader_plan.json` in the page opened with `?record-shaders`, which prints every
shader the game asks for, a page load a section:

```bash
node lib/N64Recomp-browser/tools/record_shaders.mjs web/shader_plan.json build-web <your USA 1.0 dump> web/rt64_shaders.txt
```

then build the page again. A section reaches its level through `Module._sssv_web_warp(level)`
(`src/game/recomp_api.cpp`), the page's way into the test control the desktop build reads from
`SSSV_TEST_CONTROL`, and reads the level the game is in from its memory. The plan's header says what was played.

`web/play_plan.json` is the page's regular play, 2-3 minutes with every timeout counted, which N64Recomp-browser's
`tools/play_page.mjs` plays to measure the page: the title, Bank Select, a new game's intro skipped, and Smashing
Start.
