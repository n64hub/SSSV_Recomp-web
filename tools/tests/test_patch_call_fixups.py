"""Check generated runtime calls use the emulated-register ABI."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

class PatchCallFixups(unittest.TestCase):
    def test_runtime_wrapper_and_idempotence(self):
        cmake = os.environ.get("CMAKE", shutil.which("cmake"))
        if not cmake:
            self.skipTest("CMake is required")
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "patches.c"
            source.write_text("osVirtualToPhysical(rdram, ctx);\n"
                              "memset(rdram, ctx);\n"
                              "osVirtualToPhysical_recomp(rdram, ctx);\n"
                              "game_function(rdram, ctx);\n")
            command = [cmake, "-DINPUT_FILE=" + str(source), "-P",
                       str(ROOT / "patches/fix_generated_patch_calls.cmake")]
            subprocess.run(command, check=True)
            expected = ("osVirtualToPhysical_recomp(rdram, ctx);\n"
                        "memset_recomp(rdram, ctx);\n"
                        "osVirtualToPhysical_recomp(rdram, ctx);\n"
                        "game_function(rdram, ctx);\n")
            self.assertEqual(source.read_text(), expected)
            subprocess.run(command, check=True)
            self.assertEqual(source.read_text(), expected)

if __name__ == "__main__":
    unittest.main()
