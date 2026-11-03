"""Guard against ROM leakage and unsafe generated-source archives."""
import importlib.util
import io
import json
from pathlib import Path
import tarfile
import tempfile
import unittest
import zipfile
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('ci_release', Path(__file__).parents[1] / 'ci_release.py')
ci = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ci)


class ReleaseSafety(unittest.TestCase):
    def test_linux_package_includes_hd_assets_and_licenses_but_not_rom(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            build = root / 'build-release'
            for name in ['Mirror.tex', 'build-release/SSSVRecompiled', 'recompcontrollerdb.txt',
                         'README.md', 'README.upstream.md', 'CHANGELOG.md',
                         'lib/N64ModernRuntime/COPYING', 'release-notices/Inter-OFL.txt']:
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text('fixture')
            for name in ci.ASSET_FILES:
                (root / 'assets').mkdir(exist_ok=True)
                (root / 'assets' / name).write_text('asset')
            for name in ci.ASSET_DIRS:
                (root / 'assets' / name).mkdir(parents=True)
                (root / 'assets' / name / 'example.png').write_bytes(b'\x89PNGfixture')
            for name in ci.TEXTURE_DIRS:
                (root / 'Textures' / name).mkdir(parents=True)
                (root / 'Textures' / name / 'example.png').write_bytes(b'\x89PNGfixture')
            for name in ci.TEXTURE_FILES:
                (root / 'Textures' / name).write_text("Scott's Terrible AI Texture Pack\n")
            (root / 'assets/data.z64').write_bytes(next(iter(ci.ROM_MAGIC)) + b'private ROM')
            with patch.object(ci, 'ROOT', root), patch.object(ci.subprocess, 'check_output', return_value='abc123\n'):
                ci.package(build, 'Linux', 'x64')
            with tarfile.open(root / 'dist/SSSVRecompiled-Linux-x64.tar.gz') as archive:
                names = archive.getnames()
                self.assertTrue(any(n.endswith('Textures/hd-hud/example.png') for n in names))
                self.assertTrue(any(n.endswith('Textures/hd-font/example.png') for n in names))
                self.assertTrue(any(n.endswith('Textures/hd-world/example.png') for n in names))
                self.assertTrue(any(n.endswith('Textures/name.txt') for n in names))
                self.assertFalse(any('/assets/hd-' in n for n in names))
                self.assertTrue(any(n.endswith('licenses/extra/Inter-OFL.txt') for n in names))
                self.assertFalse(any(n.endswith('.z64') for n in names))
                for member in archive.getmembers():
                    if member.isfile():
                        ci.audit_file(member.name, archive.extractfile(member).read())

    def test_source_package_includes_new_local_source_and_hd_world_without_rom(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            files = ['README.md', 'src/main/rom_file.h', 'Textures/hd-world/art/flower.png',
                     'dependency-patches/lib-rt64.patch', 'generated/manifest.json']
            for name in files:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'fixture')
            (root / 'Textures/hd-world/private.z64').write_bytes(next(iter(ci.ROM_MAGIC)))
            def git_output(*args, **kwargs):
                return 'pinned revisions\n' if kwargs.get('text') else b'README.md\0'
            with patch.object(ci, 'ROOT', root), patch.object(ci.subprocess, 'check_output', side_effect=git_output):
                ci.source_package()
            with zipfile.ZipFile(root / 'dist/SSSVRecompiled-source.zip') as archive:
                for name in files:
                    self.assertEqual(archive.read('SSSVRecompiled-source/' + name), b'fixture')
                self.assertFalse(any(name.endswith('.z64') for name in archive.namelist()))

    def test_reject_rom_extensions_and_renamed_rom_headers(self):
        for name in ['assets/data.z64', 'game.N64', 'game.v64', 'game.rom', 'saves/game.eep']:
            with self.assertRaises(ValueError):
                ci.audit_file(name, b'example')
        for magic in ci.ROM_MAGIC:
            with self.assertRaises(ValueError):
                ci.audit_file('assets/innocent.png', magic + b'example')
        ci.audit_file('Textures/hd-hud/frame.png', b'\x89PNGexample')

    def test_snapshot_roundtrip_stale_inputs_and_path_traversal(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for name in ci.INPUTS + ['RecompiledFuncs/funcs_0.c', 'rsp/aspMain.cpp']:
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text('source\n')
            with patch.multiple(ci, ROOT=root, ARCHIVE=root / 'generated/recompiled-source.tar.gz',
                                MANIFEST=root / 'generated/manifest.json'):
                ci.snapshot()
                (root / 'RecompiledFuncs/funcs_0.c').unlink()
                ci.prepare()
                self.assertEqual((root / 'RecompiledFuncs/funcs_0.c').read_text(), 'source\n')
                (root / ci.INPUTS[0]).write_text('changed')
                with self.assertRaisesRegex(ValueError, 'stale'):
                    ci.prepare()
                (root / ci.INPUTS[0]).write_text('source\n')
                with tarfile.open(ci.ARCHIVE, 'w:gz') as archive:
                    info = tarfile.TarInfo('../escaped.c')
                    info.size = 1
                    archive.addfile(info, io.BytesIO(b'x'))
                manifest = json.loads(ci.MANIFEST.read_text())
                manifest['archive_sha256'] = ci.digest(ci.ARCHIVE.read_bytes())
                ci.MANIFEST.write_text(json.dumps(manifest))
                with self.assertRaisesRegex(ValueError, 'Unsafe'):
                    ci.prepare()
                self.assertFalse((root.parent / 'escaped.c').exists())


if __name__ == '__main__':
    unittest.main()
