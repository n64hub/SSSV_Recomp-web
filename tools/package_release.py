"""Package the current Windows Release build without ROMs or user data."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'dist'
STAGE = OUT / 'SSSVRecompiled-Windows-x64-2026-10-31'
assert not STAGE.exists(), 'Use a new staging directory; never merge old packages.'
STAGE.mkdir(parents=True)

def copy(src, dest):
    src = ROOT / src
    dest = STAGE / dest
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dest)

for name in ['SSSVRecompiled.exe', 'SDL2.dll', 'dxcompiler.dll', 'dxil.dll']:
    copy(Path('cmake-build-dist') / name, name)
crt = Path('C:/Program Files/Microsoft Visual Studio/18/Community/VC/Redist/MSVC/14.50.35710/x64/Microsoft.VC145.CRT')
for name in ['msvcp140.dll', 'msvcp140_atomic_wait.dll', 'vcruntime140.dll', 'vcruntime140_1.dll']:
    copy(crt / name, name)
copy('recompcontrollerdb.txt', 'recompcontrollerdb.txt')
for name in ['InterVariable.ttf', 'NotoEmoji-Regular.ttf', 'recomp.rcss']:
    copy(Path('assets') / name, Path('assets') / name)
for name in ['icons', 'promptfont']:
    for src in (ROOT / 'assets' / name).rglob('*'):
        if src.is_file():
            copy(src, src.relative_to(ROOT))

# Texture packs live beside the executable, separate from ROM/runtime assets.
for src in (ROOT / 'Textures').rglob('*'):
    if src.is_file():
        copy(src, src.relative_to(ROOT))

# Preserve actual upstream notices rather than paraphrasing their terms.
license_files = []
for src in (ROOT / 'lib').rglob('*'):
    if not src.is_file() or '.git' in src.parts:
        continue
    n = src.name.lower()
    if n.startswith(('license', 'licence', 'copying', 'notice')) or n in ['ftl.txt', 'gplv2.txt', 'gpl-license']:
        if any(x in src.parts for x in ['Tests', 'Samples', 'tests', 'examples', 'nasm-2.16.01', 'gawk-3.1.6-1']):
            continue
        dest = Path('licenses') / src.relative_to(ROOT)
        copy(src, dest)
        license_files.append(str(dest))
for src in (ROOT / 'diagnostics/release-licenses').iterdir():
    copy(src, Path('licenses') / src.name)
copy('C:/Program Files/Microsoft Visual Studio/18/Community/Licenses/1033/Redist.txt', 'licenses/Microsoft-Redist.txt')
copy('lib/N64ModernRuntime/COPYING', 'COPYING.txt')

(STAGE / 'README.txt').write_text('''Space Station Silicon Valley: Recompiled - Windows x64
Release build: 31 October 2026

Extract the entire folder before running SSSVRecompiled.exe.
Keep the DLLs, assets and Textures folders beside the executable.

You must supply your own supported Space Station Silicon Valley (USA) 1.0
big-endian .z64 ROM. No ROM or save data is included. On first launch, select
the ROM in the file picker. It is validated and copied to assets/data.z64.
An unsupported file produces an error and the application closes.

Saves are created in saves/ beside the executable. Install to a writable folder.
Escape opens settings. Alt+Enter toggles fullscreen.
HD Text, HD Textures and HD World Textures are enabled by default and can be switched off independently in settings.
Start/Enter can skip the new-game intro, with a fade into gameplay.

Windows 10/11 x64 and a graphics driver supporting the selected D3D12 or Vulkan
backend are required. The necessary app-local Visual C++ runtime DLLs are included.

Credits and third-party notices: THIRD-PARTY-NOTICES.txt and licenses/.
This software uses the FreeType Project (https://freetype.org).
This is an unofficial project, unaffiliated with the original game's owners.
Game names, artwork and other game rights remain with their respective owners.

Source companion: SSSVRecompiled-source-2026-10-31.zip, supplied alongside this
binary package. Keep that companion available when redistributing this build.
''', encoding='utf-8')
(STAGE / 'THIRD-PARTY-NOTICES.txt').write_text('''Based on SSSV_Recomp by Cellenseres and contributors:
https://github.com/Cellenseres/SSSV_Recomp

Includes N64ModernRuntime/librecomp/ultramodern, RT64 and its dependencies,
RecompFrontend, CellenseresSDK, SDL2, FreeType, RmlUi, LunaSVG/PlutoVG,
GamepadMotionHelpers, nativefiledialog-extended, and supporting libraries.
Original copyright and license texts are preserved in licenses/.
N64ModernRuntime's GPL version 3 text is also supplied as COPYING.txt.

Fonts: Inter, Noto Emoji, Comic Neue and PromptFont. Their applicable license
texts are in licenses/ and Textures/hd-font/OFL.txt or assets/promptfont/LICENSE.txt.
This software uses the FreeType Project (https://freetype.org).

Microsoft DirectX Shader Compiler and third-party notices are in
licenses/DXC-LICENSE.txt and licenses/DXC-ThirdPartyNotices.txt.
The Microsoft Visual C++ runtime DLLs are Microsoft redistributable components;
they retain their own terms and are not relicensed by this package.

The corresponding source companion includes the current modified source files,
generated C source, build scripts, and dependency revision information. ROMs,
personal saves, diagnostics and private configuration are excluded.
''', encoding='utf-8')

source_zip = OUT / 'SSSVRecompiled-source-2026-10-31.zip'
tracked = subprocess.check_output(['git', 'ls-files', '--recurse-submodules', '-z'], cwd=ROOT).decode().split('\0')
sources = {ROOT / x for x in tracked if x and (ROOT / x).is_file()}
for directory in ['RecompiledFuncs', 'RecompiledPatches', 'src', 'include', 'patches', 'tools', 'Textures', 'dependency-patches', 'generated']:
    sources.update(x for x in (ROOT / directory).rglob('*') if x.is_file())
sources.update([ROOT / x for x in ['patches/intro_skip_patches.c', 'patches/pause_preview_patches.c', 'src/game/display_list_buffers.cpp', 'src/main/input_movie.h']])
forbidden = {'.z64', '.n64', '.v64', '.rom', '.exe', '.dll', '.lib', '.pdb', '.obj', '.o', '.a', '.so', '.dylib', '.pyc', '.zip', '.7z'}
with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for src in sorted(sources):
        rel = src.relative_to(ROOT)
        if src.suffix.lower() in forbidden or '.git' in rel.parts or '__pycache__' in rel.parts:
            continue
        # Native ROM/patch binary blobs are regenerated locally, never shipped.
        if src.suffix.lower() == '.bin':
            continue
        z.write(src, 'SSSVRecompiled-source/' + rel.as_posix())
    revisions = subprocess.check_output(['git', 'submodule', 'status', '--recursive'], cwd=ROOT, text=True)
    z.writestr('SSSVRecompiled-source/DEPENDENCY-REVISIONS.txt', revisions)
    z.writestr('SSSVRecompiled-source/SOURCE-README.txt', '''Current source snapshot, including local modifications, for the accompanying
31 October 2026 Windows release. Dependency sources are included at the recorded
revisions; precompiled dependency/tool binaries are excluded. Existing upstream
copyright and license notices apply to their respective files.

Build using the Windows workflow in .github/workflows/validate.yml and the root
CMakeLists.txt. Install Visual Studio C++ tools, clang-cl, CMake, Ninja, Python,
the MIPS patch compiler, N64Recomp and RSPRecomp. Supply the supported ROM locally
as sssv.us.z64. Generated C is included; ROM and binary patch blobs are not.
DXC and other precompiled dependency binaries can be obtained from the upstream
submodule URLs/revisions in .gitmodules and DEPENDENCY-REVISIONS.txt.
''')

manifest = {str(p.relative_to(STAGE)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(STAGE.rglob('*')) if p.is_file()}
(STAGE / 'SHA256SUMS.txt').write_text(''.join(f'{digest}  {name}\n' for name, digest in manifest.items()))
archive = OUT / 'SSSVRecompiled-Windows-x64-2026-10-31.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for p in sorted(STAGE.rglob('*')):
        if p.is_file():
            z.write(p, STAGE.name + '/' + p.relative_to(STAGE).as_posix())
for path in [archive, source_zip]:
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        assert not any(Path(n).suffix.lower() in {'.z64', '.n64', '.v64', '.rom'} for n in z.namelist())
    print(path, path.stat().st_size, hashlib.sha256(path.read_bytes()).hexdigest())
