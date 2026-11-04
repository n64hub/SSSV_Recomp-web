# Build 5

- Fixed energy pickups appearing through fences, including The Battery Farm. The fence’s solid pixels tested depth but did not write it, so sprites drawn later could appear in front of the fence. Cutout surfaces now write depth for their visible pixels while their transparent gaps stay open; glass and other translucent surfaces are unchanged.
- Reduced the texture pack from approximately 217 MB to 62 MB (72% smaller) by right-sizing oversized sprites, effects, trophies and thumbnails. Transparency is preserved; full-screen artwork, fonts and HUD artwork retain their resolution.
- Removed Herobrine.

Includes Windows x64, Linux x64, macOS Apple Silicon and macOS Intel packages, with the optional texture pack and licenses. No ROM or saves are included.

HD World Textures remain incomplete.
