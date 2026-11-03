# Changelog

## 3 November 2026 — v2026.11.03

- Added Herobrine.
- Added **Mirror Mode (Experimental)** in the **General** tab, with readable HUD/text and automatic left/right movement and camera reversal.
- Added `Mirror.tex` rules for world textures that need readable lettering in Mirror Mode.
- Restored animated static on animal-preview television monitors.
- Restored animal reflections on the glass dome.
- Restored motion-blur trails in the animal stats screen, including trails following the HD text.
- Restored intermittent television interference in the menus.
- Kept the animal stats foreground at 4:3 proportions, expanding the background up to 16:9 with black sides on wider displays.
- Renamed the optional artwork pack to **Scotts Somewhat Ok Texture Pack**.
- Removed Herobrine.

## Current version — 2 November 2026



- Fixed the blank world in **Sting in the Tail**: dynamic tails and chains now use a separate matrix buffer instead of overwriting the camera and projection matrices.

- Expanded the opaque display-list buffer to prevent dense scenes from overwriting neighbouring graphics data, including the overflow found in level 16.

- Fixed plants and other scenery disappearing in crowded scenes when animal rendering exhausted the shared matrix pool.

- Added **HD World Textures**, enabled by default and controlled separately from HD Text and HD Textures.

- Added HD world sprites, including plants, flowers, butterflies, collectibles and effects, plus HD sheep, jumping-sheep and dog tutorial signs with their mipmapped variants.

- Preserved native sprite placement, animation frames and paired colour/opacity textures.

- Added `--rom "path/to/file.z64"` to load a validated ROM from another location without installing a copy into `assets/`.

- Import the original version's save into `saves/` only when no portable save data exists. Keep the original file and preserve existing saves.



## Previous version — 1 November 2026



Significant changes from the original Cellenseres repository.



### Launching, files and window behaviour



- Removed the launcher front screen so the game starts directly.

- ROM loads from `assets/data.z64` beside the executable.

- If the ROM is missing, a file picker opens. Valid ROMs are copied into place; invalid files produce an error and close the game.

- Save data always lives in `saves/` beside the executable.

- Windowed mode starts at approximately 75% of the screen.

- Added live FPS to the window/taskbar title.

- Enabled Alt+Enter fullscreen switching.

- Kept settings accessible through Escape.

- Changed the settings GUI font to Inter.



### Graphics and aspect ratios



- Fixed Dan's floating hair and Evo's detached visor.

- Extended the DMA scene's white background to cover widescreen.

- Made the TV report consistently 4:3 with black side bars.

- Fixed its final background slide so it enters at widescreen size without snapping wider afterward.

- Kept the title logo, spaceship and "Press Start" at their original proportions while allowing the background to stretch.

- Matched the demo's "Press Start" size and position to the title screen.

- Corrected copyright, bank select, zone select and pause-menu foreground scaling.

- Preserved the shrinking-tile title transition with consistent foreground scaling.

- Corrected stretched world sprites, including flowers and collectibles.

- Fixed rendering problems at extreme window widths.

- Added adaptive HUD placement for ultrawide screens.

- Made expanded layouts respect the selected aspect-ratio mode.



### Pause screen and cutscenes



- Fixed the corrupted pause-menu gameplay preview.

- Preserved the preview's aspect ratio with black bars where necessary.

- Adjusted the pause/unpause animation to reduce aspect-ratio jumps.

- Added Start/Enter skipping to the new-game intro, with a transition that holds black until the dog is visible.

- Allowed startup logos and the TV report to be skipped even with a fresh save.



### HD text and textures



- Added **HD Text**, enabled by default, with an option to restore the original lettering.

- Replaced in-game bitmap lettering with sharper text and adjusted spacing, sizing and baselines.

- Replaced menu header lettering with live HD text over a textless frame.

- Added **HD Textures** (previously HD Hud), enabled by default.

- Created clearer health and energy bars with concentric lights and consistent silver shading.

- Added HD green LCD digits with visible inactive segments.

- Removed the separate "2D Sprite Upscaling" option and permanently selected Upscaled.



### Optional texture pack



- Added an HD title logo and an expanded title background: show the centre at 16:9, reveal more of the panorama up to 32:9, and stretch beyond that width.

- Rebuilt the copyright screen with sharp lettering over a separate HD background, using the same ultrawide panorama rules.

- Added HD bank and zone select artwork, including circular progress rings, spiral centres, collectible icons, trophies and all 32 level thumbnails.

- Added an HD SV television test card while preserving its transition animation.

- Put the optional assets in `Textures/` beside the executable. The settings heading reads `Textures/name.txt`, falling back to “Textures” when the name is absent.

- Hide the texture-pack heading and HD settings when the texture folder is absent.

- Keep the menu's top caption sharp when HD Textures is enabled, even if HD Text is disabled.

- Preserve original artwork when HD settings are off, including after changing settings during transitions.



### Stability and credits



- Added detailed diagnostics for picture stalls.

- Expanded graphics command buffers to prevent widescreen subtitles from overwriting neighbouring graphics data.

- Fixed a graphics-frame counter underflow that could permanently freeze rendering during level transitions.

- Added smooth interpolation to the scrolling credits text.

