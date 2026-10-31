# Space Station Silicon Valley: Recompiled

## Purpose of Project

This is a fork of Cellenseres's currently abandonned Space Station Silicon Valley Recompilation Project. This was done to fix the scaling of the appropriate graphics and fix the intro bugs among other things. Credit to Cellenseres for the original project.

## AI Disclosure

I'm not going to pretend I didn't use AI, I used it. I used it a lot, if you've got a problem with that? I'm fine with it. I too am eagerly awaiting a true human-made version of Space Station Silicon Valley recomp and all credit goes to Wiseguy and Dario plus the rest of his team for even making this possible in the first place even though I know they frown upon projects like this.

## Changes

See [CHANGELOG.md](CHANGELOG.md) for the significant changes from the original project.

## Running

Supply your own supported Space Station Silicon Valley (USA) 1.0 big-endian `.z64` ROM. No ROM or personal saves are included in this repository.

Extract the Windows release and run `SSSVRecompiled.exe`. If `assets/data.z64` is missing, select your ROM in the file picker; the game validates it and installs it there. Saves are stored in `saves/` beside the executable.

- **Escape:** settings.
- **Alt+Enter:** switch fullscreen/windowed.
- **Start/Enter during the new-game intro:** skip with a transition into gameplay.
- **HD Text** and **HD Hud** are enabled by default and can be disabled independently.

## Building this version

1. Clone this repository and initialize its dependencies with `git submodule update --init --recursive`.
2. Run `python tools/apply_dependency_patches.py` to apply this fork's runtime and renderer changes. The script checks all patches before applying and supports already-applied patches.
3. Follow the toolchain/build instructions in [the upstream README](README.upstream.md) and `.github/workflows/validate.yml`. Supply the supported ROM locally as `sssv.us.z64`; it is required for recompilation and is never committed.

The dependency patch manifest records the exact upstream revisions. The patches include all local changes to N64ModernRuntime, RecompFrontend, RT64 and Plume, so a fresh clone does not depend on unpublished submodule commits.

## Upstream and credits

Original project: [Cellenseres/SSSV_Recomp](https://github.com/Cellenseres/SSSV_Recomp), based on commit `a4dbfe40e205bd804c76db62227e94f6d5858a9b`.

Built on [N64: Recompiled](https://github.com/N64Recomp/N64Recomp), [RT64](https://github.com/rt64/rt64), N64ModernRuntime, RecompFrontend, and the work of their contributors. Original upstream documentation and acknowledgements are preserved in [README.upstream.md](README.upstream.md). Dependencies retain their own copyright and license notices. Font licenses are included with their assets.

This repository is private for now. Because GitHub does not allow private forks of public repositories, it is maintained as an independent private repository preserving the original Git history.
