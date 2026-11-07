"""ROM-free generated-source snapshots and Windows/Linux/macOS release packaging."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = ROOT / 'generated/recompiled-source.tar.gz'
MANIFEST = ROOT / 'generated/manifest.json'
INPUTS = ['sssv.us.toml', 'aspMain.toml',
          'SSSVRecompSyms/sssv.us.syms.toml', 'SSSVRecompSyms/sssv.us.datasyms.toml']
BAD_SUFFIXES = {'.z64', '.n64', '.v64', '.rom', '.sav', '.srm', '.eep', '.fla'}
ROM_MAGIC = {b'\x80\x37\x12\x40', b'\x37\x80\x40\x12', b'\x40\x12\x37\x80'}
ASSET_FILES = ['InterVariable.ttf', 'NotoEmoji-Regular.ttf', 'recomp.rcss']
ASSET_DIRS = ['icons', 'promptfont']
TEXTURE_DIRS = ['hd-font', 'hd-hud', 'hd-title', 'hd-menu', 'hd-world']
TEXTURE_FILES = ['name.txt']


def digest(data):
    return hashlib.sha256(data).hexdigest()


def input_digest(path):
    # Git checkouts may normalize text line endings on Linux/macOS.
    return digest(path.read_bytes().replace(b"\r\n", b"\n"))


def audit_file(name, data):
    path = PurePosixPath(name)
    if path.suffix.lower() in BAD_SUFFIXES or {'saves', 'save-backups'} & set(path.parts):
        raise ValueError(f'Forbidden ROM/save file: {name}')
    # Also reject ROMs renamed to an innocent extension.
    if data[:4] in ROM_MAGIC:
        raise ValueError(f'N64 ROM header found: {name}')


def audit_tree(directory):
    for path in directory.rglob('*'):
        if path.is_file():
            audit_file(path.relative_to(directory).as_posix(), path.read_bytes())


def snapshot():
    """Run locally AFTER N64Recomp/RSPRecomp using the user's local ROM."""
    paths = sorted((ROOT / 'RecompiledFuncs').glob('*')) + [ROOT / 'rsp/aspMain.cpp']
    if not any(p.name.startswith('funcs_') and p.suffix == '.c' for p in paths):
        raise ValueError('Generate RecompiledFuncs locally first')
    ARCHIVE.parent.mkdir(exist_ok=True)
    files = {}
    with tarfile.open(ARCHIVE, 'w:gz') as archive:
        for path in paths:
            if not path.is_file() or path.suffix not in {'.c', '.h', '.inl', '.cpp'}:
                raise ValueError(f'Unexpected generated file: {path}')
            name = path.relative_to(ROOT).as_posix()
            data = path.read_bytes()
            audit_file(name, data)
            info = tarfile.TarInfo(name)
            info.size, info.mode, info.mtime = len(data), 0o644, 0
            archive.addfile(info, io.BytesIO(data))
            files[name] = digest(data)
    manifest = {'format': 1, 'tool_revision': '81213c1831fab2521a6a5459c67b63437d67e253',
                'archive_sha256': digest(ARCHIVE.read_bytes()), 'files': files,
                'inputs': {p: input_digest(ROOT / p) for p in INPUTS}}
    MANIFEST.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'Created {ARCHIVE.name}: {len(files)} generated source files; no ROM/data blobs')


def prepare():
    manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
    if digest(ARCHIVE.read_bytes()) != manifest['archive_sha256']:
        raise ValueError('Generated-source archive hash mismatch')
    for name, expected in manifest['inputs'].items():
        if input_digest(ROOT / name) != expected:
            raise ValueError(f'Generated code is stale for {name}; regenerate locally and snapshot')
    pending = []
    with tarfile.open(ARCHIVE, 'r:gz') as archive:
        seen = set()
        for member in archive.getmembers():
            name = member.name
            path = PurePosixPath(name)
            allowed = (len(path.parts) == 2 and path.parts[0] == 'RecompiledFuncs'
                       and path.suffix in {'.c', '.cpp', '.h', '.inl'}) or name == 'rsp/aspMain.cpp'
            if not member.isfile() or not allowed or '..' in path.parts or name in seen:
                raise ValueError(f'Unsafe/unexpected archive entry: {name}')
            data = archive.extractfile(member).read()
            audit_file(name, data)
            if digest(data) != manifest['files'].get(name):
                raise ValueError(f'Generated source hash mismatch: {name}')
            seen.add(name)
            pending.append((name, data))
    if seen != set(manifest['files']):
        raise ValueError('Generated source file list mismatch')
    for name, data in pending:
        path = ROOT / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    # A source checkout used by CI must not contain a ROM, even outside assets/.
    for folder in [ROOT, ROOT / 'assets']:
        for path in folder.glob('*'):
            if path.is_file():
                audit_file(path.name, path.read_bytes())
    print(f'Verified and restored {len(pending)} source files without a ROM')


def copy_assets(destination):
    destination.mkdir(parents=True, exist_ok=True)
    for name in ASSET_FILES:
        shutil.copy2(ROOT / 'assets' / name, destination / name)
    for name in ASSET_DIRS:
        shutil.copytree(ROOT / 'assets' / name, destination / name)


def copy_textures(destination):
    destination.mkdir(parents=True, exist_ok=True)
    for name in TEXTURE_FILES:
        shutil.copy2(ROOT / 'Textures' / name, destination / name)
    for name in TEXTURE_DIRS:
        shutil.copytree(ROOT / 'Textures' / name, destination / name)


def package(build, platform, arch):
    out = ROOT / 'dist'
    stage = out / f'SSSVRecompiled-{platform}-{arch}'
    if stage.exists():
        raise ValueError(f'Refusing to merge into old package: {stage}')
    stage.mkdir(parents=True)
    if platform == 'macOS':
        app = stage / 'SSSVRecompiled.app'
        shutil.copytree(build / app.name, app, symlinks=True)
        # Rebuild the app's resource folder from an explicit runtime allowlist.
        for location in [app / 'Contents/Resources/assets', app / 'Contents/MacOS/assets',
                         app / 'Contents/Resources/Textures', app / 'Contents/MacOS/Textures']:
            if location.exists():
                shutil.rmtree(location)
        copy_assets(app / 'Contents/Resources/assets')
        copy_textures(app / 'Contents/Resources/Textures')
        shutil.copy2(ROOT / 'recompcontrollerdb.txt', app / 'Contents/Resources/recompcontrollerdb.txt')
        subprocess.run(['codesign', '--force', '--deep', '--sign', '-', '--entitlements',
                        str(ROOT / '.github/macos/entitlements.plist'), str(app)], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(app)], check=True)
    elif platform == 'Windows':
        for name in ['SSSVRecompiled.exe', 'SDL2.dll', 'dxcompiler.dll', 'dxil.dll']:
            shutil.copy2(build / name, stage / name)
        redist = Path(os.environ['VCToolsRedistDir'])
        crt_dirs = sorted(redist.glob('x64/Microsoft.VC*.CRT'))
        if not crt_dirs:
            raise ValueError('Visual C++ redistributable directory not found')
        for name in ['msvcp140.dll', 'msvcp140_atomic_wait.dll', 'vcruntime140.dll', 'vcruntime140_1.dll']:
            shutil.copy2(crt_dirs[-1] / name, stage / name)
        notice = Path(os.environ['VSINSTALLDIR']) / 'Licenses/1033/Redist.txt'
        (stage / 'licenses').mkdir(exist_ok=True)
        shutil.copy2(notice, stage / 'licenses/Microsoft-Redist.txt')
        copy_assets(stage / 'assets')
        copy_textures(stage / 'Textures')
        shutil.copy2(ROOT / 'recompcontrollerdb.txt', stage / 'recompcontrollerdb.txt')
    else:
        # A matching desktop ID supplies the dog icon on Wayland, where window
        # icons cannot be set by SDL_SetWindowIcon.
        app_id = 'io.github.scottjco.SSSVRecompiled'
        app_dir = stage / 'share/applications'
        icon_dir = stage / 'share/icons/hicolor/512x512/apps'
        app_dir.mkdir(parents=True)
        icon_dir.mkdir(parents=True)
        shutil.copy2(ROOT / '.github/linux/SSSVRecompiled.desktop', app_dir / (app_id + '.desktop'))
        shutil.copy2(ROOT / 'icons/512.png', icon_dir / (app_id + '.png'))
        shutil.copy2(build / 'SSSVRecompiled', stage / 'SSSVRecompiled')
        (stage / 'SSSVRecompiled').chmod(0o755)
        copy_assets(stage / 'assets')
        copy_textures(stage / 'Textures')
        shutil.copy2(ROOT / 'recompcontrollerdb.txt', stage / 'recompcontrollerdb.txt')

    # Include the actual licenses from the checked-out dependency sources.
    for path in (ROOT / 'lib').rglob('*'):
        if not path.is_file() or '.git' in path.parts:
            continue
        if path.name.lower().startswith(('license', 'licence', 'copying', 'notice')) or path.name.lower() in {'ftl.txt', 'gplv2.txt'}:
            target = stage / 'licenses' / path.relative_to(ROOT)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
    shutil.copytree(ROOT / 'release-notices', stage / 'licenses/extra')
    if platform == 'macOS':
        formulae = {'sdl2', 'freetype', 'zlib'}
        formulae.update(subprocess.check_output(
            ['brew', 'deps', '--union', 'sdl2', 'freetype', 'zlib'], text=True).split())
        for formula in sorted(formulae):
            prefix = Path(subprocess.check_output(['brew', '--prefix', formula], text=True).strip())
            for path in prefix.rglob('*'):
                if path.is_file() and path.name.lower().startswith(('license', 'licence', 'copying', 'notice')):
                    target = stage / 'licenses/Homebrew' / formula / path.relative_to(prefix)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(path, target)
    mirror_destination = (app / 'Contents/Resources/Mirror.tex') if platform == 'macOS' else (stage / 'Mirror.tex')
    shutil.copy2(ROOT / 'Mirror.tex', mirror_destination)
    shutil.copy2(ROOT / 'lib/N64ModernRuntime/COPYING', stage / 'COPYING.txt')
    for name in ['README.md', 'CHANGELOG.md', 'README.upstream.md']:
        shutil.copy2(ROOT / name, stage / name)
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    (stage / 'BUILD-INFO.txt').write_text(f'Source: https://github.com/scottjco/SSSV_Recomp\nCommit: {revision}\nPlatform: {platform} {arch}\n', encoding='utf-8')
    (stage / 'RUNNING.txt').write_text(
        'Supply your own supported USA 1.0 .z64 ROM. No ROM or saves are included.\n'
        'Extract to a writable folder. Launch SSSVRecompiled.exe (Windows), SSSVRecompiled (Linux) or the .app (Mac).\n'
        'The first launch asks for your ROM and installs it in assets/data.z64.\n'
        'Keep the Textures folder beside the executable; it contains the optional HD artwork.\n'
        'HD Text, HD Textures and HD World Textures are enabled by default. Escape opens settings.\n'
        'On Mac, bundled assets, Textures and saves live inside Contents/Resources.\n'
        'The Mac app is ad-hoc signed, not Apple Developer ID signed/notarized.\n'
        'Linux requires SDL2, GTK3, FreeType, X11/XRandR and a Vulkan-capable driver.\n'
        'Built on Ubuntu 24.04; older distributions may need newer system libraries.\n'
        'Third-party licenses are in licenses/ and COPYING.txt.\n', encoding='utf-8')
    audit_tree(stage)
    archive = out / (stage.name + ('.tar.gz' if platform == 'Linux' else '.zip'))
    if platform == 'macOS':
        subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(stage), str(archive)], check=True)
        with zipfile.ZipFile(archive) as result:
            for item in result.infolist():
                if not item.is_dir():
                    audit_file(item.filename, result.read(item))
    elif platform == 'Windows':
        with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as result:
            for path in stage.rglob('*'):
                if path.is_file():
                    result.write(path, stage.name + '/' + path.relative_to(stage).as_posix())
        with zipfile.ZipFile(archive) as result:
            for item in result.infolist():
                if not item.is_dir():
                    audit_file(item.filename, result.read(item))
    else:
        with tarfile.open(archive, 'w:gz') as result:
            result.add(stage, arcname=stage.name)
        with tarfile.open(archive, 'r:gz') as result:
            for item in result.getmembers():
                if item.isfile():
                    audit_file(item.name, result.extractfile(item).read())
    archive.with_name(archive.name + '.sha256').write_text(f'{digest(archive.read_bytes())}  {archive.name}\n')
    print(f'Packaged and audited {archive}')


def source_package():
    out = ROOT / 'dist'
    out.mkdir(exist_ok=True)
    tracked = subprocess.check_output(['git', 'ls-files', '--recurse-submodules', '-z'], cwd=ROOT).decode().split('\0')
    paths = {ROOT / p for p in tracked if p and (ROOT / p).is_file()}
    for folder in ['RecompiledFuncs', 'RecompiledPatches', 'N64RecompSource',
                   'src', 'include', 'patches', 'tools', 'Textures', 'dependency-patches', 'generated']:
        paths.update(p for p in (ROOT / folder).rglob('*') if p.is_file() and '.git' not in p.parts)
    excluded = BAD_SUFFIXES | {'.bin', '.exe', '.dll', '.lib', '.pdb', '.obj', '.o', '.a', '.so', '.dylib', '.pyc', '.zip', '.7z'}
    archive = out / 'SSSVRecompiled-source.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as result:
        for path in sorted(paths):
            rel = path.relative_to(ROOT)
            if path.suffix.lower() in excluded or '.git' in rel.parts or '__pycache__' in rel.parts:
                continue
            data = path.read_bytes()
            audit_file(rel.as_posix(), data)
            result.writestr('SSSVRecompiled-source/' + rel.as_posix(), data)
        revisions = subprocess.check_output(['git', 'submodule', 'status', '--recursive'], cwd=ROOT, text=True)
        result.writestr('SSSVRecompiled-source/DEPENDENCY-REVISIONS.txt', revisions)
    with zipfile.ZipFile(archive) as result:
        for item in result.infolist():
            audit_file(item.filename, result.read(item))
    archive.with_name(archive.name + '.sha256').write_text(f'{digest(archive.read_bytes())}  {archive.name}\n')
    print('Audited source package:', archive)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['snapshot', 'prepare', 'package', 'source'])
    parser.add_argument('--build', type=Path, default=ROOT / 'build-release')
    parser.add_argument('--platform', choices=['Linux', 'macOS', 'Windows'])
    parser.add_argument('--arch', choices=['x64', 'arm64'])
    args = parser.parse_args()
    if args.command == 'snapshot':
        snapshot()
    elif args.command == 'prepare':
        prepare()
    elif args.command == 'source':
        source_package()
    else:
        if not args.platform or not args.arch:
            parser.error('package requires --platform and --arch')
        package(args.build.resolve(), args.platform, args.arch)
