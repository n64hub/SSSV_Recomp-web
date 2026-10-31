"""Draw the HUD frame and lamps at 16x resolution; retain native texture hashes.

The frame is reconstructed from circles and straight edges rather than enlarging
its pixel steps. Native draw calls still control fills, flashing and placement.
"""
import argparse
import json
import math
from pathlib import Path
import struct

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import xxhash

root = Path(__file__).resolve().parents[1]
rom = (root / 'sssv.us.z64').read_bytes()
assert xxhash.xxh3_64_intdigest(rom) == 0x912A068AADB0D0C5
parser = argparse.ArgumentParser()
parser.add_argument('--output', type=Path, default=root/'assets/hd-hud')
args = parser.parse_args()
out = args.output
out.mkdir(parents=True,exist_ok=True)
S = 16
size = (96*S, 32*S)

def mask(inset=0):
    im = Image.new('L', size)
    d = ImageDraw.Draw(im)
    def box(coords):
        d.rounded_rectangle(tuple(round(c*S) for c in coords), radius=max(0,round((1.5-inset/2)*S)), fill=255)
    def circle(x, y, r):
        yy,xx=np.mgrid[0:size[1],0:size[0]]; disc=(((xx+.5)/S-x)**2+((yy+.5)/S-y)**2)<=r*r; im.paste(255,(0,0),Image.fromarray((disc*255).astype('uint8')))
    # Trace the original outline, including the larger lower ring and the
    # narrow metal necks. Coordinates share the native fill/indicator grid.
    box((11+inset,4+inset,90-inset,19-inset))
    box((17+inset,17+inset,63-inset,27-inset))
    circle(11,9.5,9.5-inset)
    circle(17,22.5,9.5-inset)
    return im

frame = Image.new('RGBA',size)
frame.paste((8,8,8,255),(0,0),mask())
frame.paste((143,163,177,255),(0,0),mask(.8))
d = ImageDraw.Draw(frame)
def rect(b,c): d.rectangle(tuple(round(v*S) for v in b),fill=c)
def ellipse(b,c):
    yy,xx=np.mgrid[0:size[1],0:size[0]]; cx=(b[0]+b[2])/2;cy=(b[1]+b[3])/2;r=(b[2]-b[0])/2;disc=(((xx+.5)/S-cx)**2+((yy+.5)/S-cy)**2)<=r*r;frame.paste(c,(0,0),Image.fromarray((disc*255).astype('uint8')))
# The recesses are measured from the source image; never move the live fills.
rect((20,8,86,15),(8,8,8,255))
rect((21,9,85,14),(0,74,82,255))
rect((25,18.2,59,23),(8,8,8,255))
rect((26,19,58,22),(41,66,16,255))
ellipse((4,2.5,18,16.5),(8,8,8,255))
ellipse((5.5,4,16.5,15),(0,74,82,255))
ellipse((10,15.5,24,29.5),(8,8,8,255))
ellipse((11.5,17,22.5,28),(41,66,16,255))
# One upper-left light shades all rails and rings from the same height field.
a = np.array(frame)
metal = a[:, :, 3] > 0
for colour in [(8,8,8,255), (0,74,82,255), (41,66,16,255)]:
    metal &= ~np.all(a == colour, axis=-1)
height = np.asarray(Image.fromarray((metal*255).astype('uint8')).filter(ImageFilter.GaussianBlur(.8*S)), dtype=float)/255*1.65
gy,gx = np.gradient(height, 1/S, 1/S)
n = np.stack((-gx,-gy,np.ones_like(gx)),axis=-1)
n /= np.linalg.norm(n,axis=-1,keepdims=True)
light = np.array([-.55,-.65,.65]); light /= np.linalg.norm(light)
halfway = light + np.array([0,0,1]); halfway /= np.linalg.norm(halfway)
diffuse = np.clip(n@light,0,1)
specular = np.clip(n@halfway,0,1)**28
rgb = np.array([161,181,195])*(.34+.79*diffuse[...,None])+specular[...,None]*80
a[metal,:3] = np.clip(rgb,0,255).astype('uint8')[metal]
frame = Image.fromarray(a)
entries=[]

def register(name,image,offset,w,h,fmt=0,siz=2):
    image.save(out/name)
    count=w*h*(2 if siz==2 else .5)
    raw=rom[offset:offset+int(count)]
    row=w*(2 if siz==2 else .5)
    tmem=bytearray(raw)
    for y in range(1,h,2):
        start=int(y*row)
        for x in range(start,start+int(row),8):
            tmem[x:x+8]=raw[x+4:x+8]+raw[x:x+4]
    # TMEMHasher v5: tile dimensions, TLUT, line stride, size and format.
    digest=xxhash.xxh3_64_hexdigest(tmem+struct.pack('<HHIHBB',w,h,0,math.ceil(row/8),siz,fmt))
    entries.append({'path':name,'hashes':{'rt64':digest},'operation':'preload','shift':'none'})

register('frame-left.png',frame.crop((0,0,48*S,32*S)),0x753f0,48,32)
register('frame-right.png',frame.crop((48*S,0,96*S,32*S)),0x75ff0,48,32)
for name,offset,colour in [('green',0x76bf0,(12,235,0)),('blue',0x76df0,(0,55,255))]:
    y,x=np.mgrid[0:16*S,0:16*S].astype(float)
    x=(x+.5)/S; y=(y+.5)/S
    r=np.sqrt(((x-6)/5.5)**2+((y-5.5)/5.5)**2)
    shade=np.clip(.25+.75*np.sqrt(np.clip(1-r*r,0,1)),0,1)
    glow=np.exp(-((x-4)**2+(y-3.5)**2)/3)*.75
    rgb=np.stack([np.clip(c*shade+glow*170,0,255) for c in colour]+[np.clip((1-r)*5.5*S,0,1)*255],axis=-1).astype('uint8')
    register(f'power-{name}.png',Image.fromarray(rgb),offset,16,16)
# The fill is an intensity ramp, tinted by the game's dynamic red/green colours.
raw=rom[0x636a0:0x636e0]
alpha=Image.new('L',(16,8))
alpha.putdata([v*17 for b in raw for v in (b>>4,b&15)])
alpha=alpha.resize((256,128),Image.Resampling.BICUBIC)
fill=Image.new('RGBA',alpha.size,(255,255,255,255))
# I4 contributes intensity and alpha, matching the original combiner.
fill=Image.merge('RGBA',(alpha,alpha,alpha,alpha))
register('fill-ramp.png',fill,0x636a0,16,8,4,0)
# Rebuild the green LCD digits as continuous seven-segment outlines. Keep the
# 16x16 native canvas and advances, including the opaque dark display backing.
segments = {
    'a': [(4,2),(5,1.1),(11,1.1),(12,2),(11,2.9),(5,2.9)],
    'g': [(4,8),(5,7.1),(11,7.1),(12,8),(11,8.9),(5,8.9)],
    'd': [(4,14),(5,13.1),(11,13.1),(12,14),(11,14.9),(5,14.9)],
}
for key,cx,cy in [('f',4,5),('b',12,5),('e',4,11),('c',12,11)]:
    segments[key] = [(cx,cy-2.7),(cx+.9,cy-1.8),(cx+.9,cy+1.8),
                     (cx,cy+2.7),(cx-.9,cy+1.8),(cx-.9,cy-1.8)]
digit_segments = ['abcdef','bc','abged','abgcd','fgbc',
                  'afgcd','afgecd','abc','abcdefg','abfgcd']
yy,xx=np.mgrid[0:16*S,0:16*S].astype(float)
xx=(xx+.5)/S; yy=(yy+.5)/S
# Keep the backing near black; dormant segments form a very faint green 8.
panel=5+4*np.exp(-((xx-8)**2+(yy-8)**2)/60)
for digit,lit in enumerate(digit_segments):
    active=Image.new('L',(16*S,16*S));dormant=Image.new('L',active.size)
    for name,polygon in segments.items():
        ImageDraw.Draw(active if name in lit else dormant).polygon(
            [(round(x*S),round(y*S)) for x,y in polygon],fill=255)
    coverage=np.asarray(active,dtype=float)/255
    unlit=np.asarray(dormant,dtype=float)/255
    glow=np.asarray(active.filter(ImageFilter.GaussianBlur(.35*S)),dtype=float)/255
    # A small bloom retains the luminous green style without soft pixel edges.
    rgb=np.zeros((16*S,16*S,4),dtype='uint8')
    brightness=.83+.17*np.exp(-((yy-3)/6)**2)
    rgb[:,:,0]=np.clip(coverage*22*brightness,0,255)
    rgb[:,:,1]=np.clip(panel+unlit*((panel+29)*1.25*1.20-panel)+glow*31+coverage*211*brightness,0,255)
    rgb[:,:,2]=np.clip(coverage*8*brightness,0,255)
    rgb[:,:,3]=255
    for base in [0x33c50,0x76ff0]:
        register(f'lcd-{digit}.png',Image.fromarray(rgb),base+digit*512,16,16)
# Some revisions contain byte-identical LCD tiles in both source sets.
entries=list({entry['hashes']['rt64']:entry for entry in entries}.values())
(out/'rt64.json').write_text(json.dumps({'configuration':{'configurationVersion':3,'hashVersion':5,'defaultOperation':'preload','defaultShift':'none'},'textures':entries},indent=2)+'\n')
frame.save(out/'frame.png')
print(f'Generated {len(entries)} HUD textures at 16x resolution.')
