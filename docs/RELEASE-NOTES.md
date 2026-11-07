# Build 6 — Linux support and intro-skip fixes

- Fixed Linux startup crashes and black output.
- Fixed interrupted gameplay audio when the Linux window is unfocused or minimized.
- Improved Linux NVIDIA frame pacing and reduced cutscene patch overhead.
- Restored the dog icon for Linux windows and desktop/taskbar launchers.
- **Windows and Linux:** skipping the new-game intro now keeps music quiet until the normal ship trigger and shows the mission briefing. Direct level entry keeps its usual behavior.
- Improved compatibility with newer Linux compilers and corrected startup error handling.
- Removed Herobrine.

Includes **Windows x64**, **Linux x64**, and corresponding source packages, with the optional texture pack, licenses, and SHA-256 checksums. No ROM or saves are included.

Linux gameplay was tested locally on CachyOS with KDE Wayland and an NVIDIA RTX 5090. Windows is built by GitHub Actions and still needs separate gameplay testing. macOS packages are not included in this release.

Supply your own supported USA 1.0 big-endian `.z64` ROM. Extract the package to a writable folder and launch `SSSVRecompiled.exe` (Windows) or `SSSVRecompiled` (Linux). Escape opens settings; Alt+Enter toggles fullscreen.
