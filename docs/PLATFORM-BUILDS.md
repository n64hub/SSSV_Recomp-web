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
All four dependency patches are applied before compilation, and current
HD Text/HD Hud assets are included in the packages. Runtime assets use an
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
