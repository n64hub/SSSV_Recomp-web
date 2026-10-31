"""Build a consistently lit, concentric-circle HUD candidate without installing it."""
from pathlib import Path
import math

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

root = Path(__file__).resolve().parents[1]
out = root / 'diagnostics/hd-hud-directional-preview'
out.mkdir(exist_ok=True)
import subprocess,sys
subprocess.run([sys.executable,str(root/'tools/build_hd_hud.py'),'--output',str(out)],check=True)

S = 16
frame = Image.open(out/'frame.png')
# Numerical check: the alpha silhouette of both lamps is a circle centred on
# the corresponding 11,9.5 / 17,22.5 outer ring in the native HUD coordinates.
for name,x,y,cx,cy in [('green',5,4,11,9.5),('blue',11,17,17,22.5)]:
    lamp = Image.open(out/f'power-{name}.png')
    alpha = np.asarray(lamp)[:,:,3]
    yy,xx = np.mgrid[0:alpha.shape[0],0:alpha.shape[1]]
    assert abs(((xx+.5)/S*alpha).sum()/alpha.sum()+x-cx)<1e-10
    assert abs(((yy+.5)/S*alpha).sum()/alpha.sum()+y-cy)<1e-10
    ys,xs = np.nonzero(alpha)
    assert xs.max()-xs.min() == ys.max()-ys.min()
    frame.alpha_composite(lamp.crop((0,0,12*S,11*S)),(x*S,y*S))
d = ImageDraw.Draw(frame)
for y in range(5*S):
    brightness=.4+.6*math.exp(-((y/S-1.5)/1.6)**2)
    d.line((21*S,9*S+y,71*S,9*S+y),fill=(int(15*brightness),int(218*brightness),0,255))
canvas=Image.new('RGBA',frame.size,(32,34,37,255));canvas.alpha_composite(frame)
canvas.resize((960,320),Image.Resampling.LANCZOS).save(out/'preview.png')
print('Verified both lamps are round and exactly concentric with their outer rings. Preview only.')
