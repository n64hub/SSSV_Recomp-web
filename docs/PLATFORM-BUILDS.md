# Linux and macOS release builds

In GitHub, open **Actions > Platform release builds > Run workflow**.
The same workflow runs for tags beginning with `v`. It builds Windows x64, Linux x64,
macOS Apple Silicon, and macOS Intel separately. Successful jobs provide
downloadable packages and SHA-256 checksums in the run's Artifacts section.
Enter a new release tag to publish all four packages and corresponding source together after every build succeeds. Leave the tag blank to produce artifacts only. Tag-triggered builds publish under that tag.

The Mac builds currently target macOS 15 and later. They are ad-hoc signed,
not Developer ID signed or notarized. Linux is built on Ubuntu 24.04 and
requires SDL2, GTK3, FreeType, X11/XRandR and a working Vulkan graphics driver.
Build success is not a substitute for graphics/gameplay testing on each OS.

## How the ROM stays local

`generated/recompiled-source.tar.gz` contains only generated C/C++ headers
and code, including the audio microcode translation. It contains no ROM or
extracted game data blobs. `generated/manifest.json` checks every member's
hash and the generating configuration/symbol hashes before extraction.

GitHub builds the pinned N64Recomp tool to regenerate **our patch code** from
the current patch sources. It does not run N64Recomp against a game ROM.
All dependency patches are applied before compilation, and current
HD Text/HD Textures/HD World Textures assets from `Textures/` are included in the packages. Runtime assets use an
explicit allowlist. Packaging checks both filenames and N64 ROM headers,
including inside the finished archive, and rejects ROMs and saves.

## Updating the generated source

After changing `sssv.us.toml`, `aspMain.toml`, or the symbol definitions,
regenerate locally with your own supported ROM and the pinned tools:

```text
N64Recomp sssv.us.toml
RSPRecomp aspMain.toml
python tools/ci_release.py snapshot
```

On Windows, use the `.exe` tool names. Commit both files in `generated/`.
Never commit `sssv.us.z64`, `assets/data.z64`, saves, or build directories.
If these inputs change without refreshing the snapshot, CI deliberately
fails instead of shipping stale hooks. Ordinary changes in `patches/*.c`
are recompiled on each build and do not require a new snapshot.

Licenses/notices are copied from initialized dependency sources and
`release-notices/`. Archives identify the exact source commit. The old
private-ROM checkout and its access token are no longer used. Source checks
run on pushes and PRs; full platform builds run only manually or on `v*` tags. The release publishing job alone has write access; build jobs have read-only access.

## Linux startup fixes (7 November 2026)

The Clang patch compiler now targets MIPS II with the O32 ABI. Using MIPS III
with Clang can put arguments in 64-bit registers instead of the stack slots
expected by the original game. This caused black output and crashes when
patches called original game functions. The Windows GCC/VR4300 settings remain
the same; the MIPS II instruction subset is compatible with the game CPU.

Generated patch calls to `osVirtualToPhysical` are routed through
`osVirtualToPhysical_recomp`, which reads and writes the emulated registers.
The native C function has a different calling convention; calling it directly
caused framebuffer readback to overwrite display-list pointers on Linux.

A pinned RmlUi patch adds a missing `<cstdint>` include for current compilers.
The SDL initialization error check now correctly detects negative return values.

Locally verified on CachyOS x64, KDE Wayland, NVIDIA RTX 5090. Windows runtime
testing is still required; no Windows-specific source or workflow was changed.

Linux also embeds the dog icon and sets a matching desktop application ID.
Packages include the desktop entry and icon under `share/`. On Wayland,
installing those resources is required for the taskbar icon; on X11 the window
icon is also set directly.

Background Linux presentation skips extra interpolation frames when the window
loses focus, and skips presentation completely when SDL reports minimization.
This avoids tying game/audio timing to the compositor's background frame rate.
Windows presentation is unchanged. Level testing at a 240 Hz target confirmed
approximately 30 game display lists and 60 screen updates per second while
background presentation was throttled to about 30 FPS.

The intro-skip path now cancels sequence requests again after level reload,
clears the target music track, and restores the original script's zero music
volume. The normal ship script remains responsible for starting/fading music.
A native test exercises the actual skip state machine with a level reload that
reintroduces a pending sequence, and verifies cleanup and preserved save flags.

Clang builds also optimize MIPS patch code before recompilation, reducing
emulated inline-helper calls and stack traffic. GCC settings (including the
Windows workflow) are byte-for-byte unchanged. Tail-call optimization is disabled
for the Clang patch step to retain ordinary call boundaries.

Skipping a fresh-game intro now requests the original mission briefing once,
after the dog is visible and the reveal finishes. Direct level entry does not
request an extra briefing. This behavior and the skip-audio cleanup are shared
by Windows and Linux, and are covered by the state-machine regression test.

On Linux NVIDIA Vulkan devices, the optional present-wait extension is disabled
to avoid the driver stalls already documented upstream. Standard Vulkan
swapchain/fence/semaphore synchronization and RT64 frame pacing remain in use.
Other Linux GPUs and Windows retain their existing presentation policy.
