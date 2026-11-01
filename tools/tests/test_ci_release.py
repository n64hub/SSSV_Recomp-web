"""Guard against ROM leakage and unsafe generated-source archives."""
import importlib.util
import io
import json
from pathlib import Path
import tarfile
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('ci_release', Path(__file__).parents[1] / 'ci_release.py')
ci = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ci)


class ReleaseSafety(unittest.TestCase):
    def test_reject_rom_extensions_and_renamed_rom_headers(self):
        for name in ['assets/data.z64', 'game.N64', 'game.v64', 'game.rom', 'saves/game.eep']:
            with self.assertRaises(ValueError):
                ci.audit_file(name, b'example')
        for magic in ci.ROM_MAGIC:
            with self.assertRaises(ValueError):
                ci.audit_file('assets/innocent.png', magic + b'example')
        ci.audit_file('assets/hd-hud/frame.png', b'\x89PNGexample')

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
