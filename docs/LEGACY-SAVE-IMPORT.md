# Original-version save import

The game always loads and writes its save in `saves/` beside the executable.

At startup, it checks that folder first. Any existing save bank, mod save, backup (`.bin.bak`), or pending save (`.bin.temp`) prevents importing. An empty folder or an unrelated file such as `README.txt` does not count as save data.

If there is no portable save data, the game checks the original project's per-user save folder for `sssv.n64.us.1.0.bin`. A supported save is 512 bytes. It copies that file into the portable folder, never overwrites an existing destination, and leaves the original unchanged. There is no import ledger or additional setting.

The legacy location uses the frontend's original platform helper:

- Windows: Local AppData / `SSSVRecompiled/saves`.
- Linux: `APP_FOLDER_PATH/saves` when that environment override is set; otherwise `~/.config/SSSVRecompiled/saves`, with the same passwd-directory fallback as the original frontend.
- macOS: `APP_FOLDER_PATH/saves` when set; otherwise the original Application Support helper / `SSSVRecompiled/saves`, with the original `.config` fallback.

Portable markers affect current configuration storage but do not hide the original per-user folder from this fallback check. Other filenames and files of the wrong size are not imported.
