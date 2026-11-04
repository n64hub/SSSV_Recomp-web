"""Right-size runtime PNGs; retain full-resolution masters outside the shipped pack.
Run from the repository with --apply. Subsequent runs always use the saved masters.
Afterwards regenerate the HD world/menu registration headers.
"""
from pathlib import Path
import argparse, json, shutil
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
PACK = ROOT / 'Textures'
MASTERS = ROOT / 'diagnostics/texture-masters'

def limit(relative):
    group, *parts = relative.parts
    name = relative.stem
    if group == 'hd-world':
        if name.startswith(('particle-', 'trail-', 'star-atlas')): return 1024
        if 'sign' in name: return 1024
        if name.startswith(('butterfly', 'energy-', 'shadow', 'ground-', 'effect-', 'footprint')): return 256
        return 512
    if group == 'hd-menu':
        if parts[0] == 'thumbnails': return 768
        if parts[0] == 'trophies': return 512
        if parts[0] == 'shared' and name not in ('header-frame',): return 512
        if parts[0] == 'transitions': return 1024
    return None

def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--apply', action='store_true'); args=parser.parse_args()
    report=[]
    for target in sorted(PACK.rglob('*.png')):
        rel=target.relative_to(PACK); cap=limit(rel)
        if cap is None: continue
        master=MASTERS/rel
        source=master if master.exists() else target
        with Image.open(source) as original:
            w,h=original.size
            if max(w,h)<=cap: continue
            size=(round(w*cap/max(w,h)), round(h*cap/max(w,h)))
            old_bytes=source.stat().st_size
            if args.apply:
                if not master.exists():
                    master.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(target,master)
                # Premultiplied alpha prevents dark fringes around transparent sprites.
                im=original.convert('RGBA').convert('RGBa').resize(size,Image.Resampling.LANCZOS).convert('RGBA')
                im.save(target,optimize=True,compress_level=9)
            report.append(dict(path=rel.as_posix(),original=[w,h],runtime=list(size),original_bytes=old_bytes,runtime_bytes=target.stat().st_size))
    out=ROOT/'diagnostics/texture-optimization.json'; out.write_text(json.dumps(report,indent=2))
    print(f'{len(report)} oversized textures; report: {out}')
if __name__=='__main__': main()
