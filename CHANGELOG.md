# Changelog

## Current version — 31 October 2026

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

### HD text and HUD

- Added **HD Text**, enabled by default, with an option to restore the original lettering.
- Replaced in-game bitmap lettering with sharper text and adjusted spacing, sizing and baselines.
- Replaced menu header lettering with live HD text over a textless frame.
- Added **HD Hud**, enabled by default.
- Created clearer health and energy bars with concentric lights and consistent silver shading.
- Added HD green LCD digits with visible inactive segments.
- Removed the separate "2D Sprite Upscaling" option and permanently selected Upscaled.

### Stability and credits

- Added detailed diagnostics for picture stalls.
- Expanded graphics command buffers to prevent widescreen subtitles from overwriting neighbouring graphics data.
- Fixed a graphics-frame counter underflow that could permanently freeze rendering during level transitions.
- Added smooth interpolation to the scrolling credits text.
