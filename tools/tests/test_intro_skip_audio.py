"""Run the intro-skip state-machine regression with native game-call stubs."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

class IntroSkipAudio(unittest.TestCase):
    def test_reload_cannot_leave_pending_music_or_restore_full_volume(self):
        compiler = shutil.which('cc') or shutil.which('clang')
        if not compiler:
            self.skipTest('A C compiler is required')
        with tempfile.TemporaryDirectory() as folder:
            executable = Path(folder) / 'intro-skip-test'
            subprocess.run([compiler, '-std=c17', str(Path(__file__).with_suffix('.c')),
                            '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

if __name__ == '__main__':
    unittest.main()
