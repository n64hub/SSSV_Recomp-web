"""Apply this fork's dependency changes after initializing Git submodules."""
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
patches = root / 'dependency-patches'
pending = []
for item in json.loads((patches / 'manifest.json').read_text()):
    dependency = root / item['path']
    patch = patches / item['patch']
    if not (dependency / '.git').exists():
        raise SystemExit('Initialize dependencies first: git submodule update --init --recursive')
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=dependency, text=True).strip()
    if head != item['revision']:
        raise SystemExit(f"Unexpected revision in {item['path']}: {head}")
    def check(*flags):
        return subprocess.run(['git', 'apply', '--check', *flags, str(patch)],
                              cwd=dependency, capture_output=True).returncode == 0
    if check('--reverse'):
        print('Already applied:', item['path'])
    elif check():
        pending.append((dependency, patch))
    else:
        raise SystemExit(f"Patch conflicts in {item['path']}; no patches were applied.")
for dependency, patch in pending:
    subprocess.run(['git', 'apply', str(patch)], cwd=dependency, check=True)
    print('Applied:', dependency.relative_to(root))
