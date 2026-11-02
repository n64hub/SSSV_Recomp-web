from pathlib import Path
import re,struct,ctypes,json,hashlib
from PIL import Image,ImageDraw
R=Path('C:/recomp/sssv');D=Path('C:/recomp/sssv-decomp-reference');O=R/'diagnostics/hd-world';P=O/'references';P.mkdir(exist_ok=True,parents=True);rom=(R/'sssv.us.z64').read_bytes();rows=[]
lib=ctypes.CDLL(str(R/'diagnostics/full-game-test/rncu.dll'));lib.unpack.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint32]
def dec(data):
 n=struct.unpack_from('>I',data,4)[0];src=ctypes.create_string_buffer(data[18:]+b'\0'*16);dst=ctypes.create_string_buffer(n+16);lib.unpack(src,dst,n);return dst.raw[:n]
def rgba(raw,w,h):
 vals=struct.unpack('>'+str(w*h)+'H',raw);return Image.frombytes('RGBA',(w,h),bytes(c for v in vals for c in (((v>>11)&31)*255//31,((v>>6)&31)*255//31,((v>>1)&31)*255//31,(v&1)*255)))
def add(name,raw,w,h,fmt,**meta):
 (P/(name+'.bin')).write_bytes(raw)
 if fmt=='RGBA16':im=rgba(raw,w,h)
 else:
  vs=list(raw) if fmt=='I8' else [v for b in raw for v in ((b>>4)*17,(b&15)*17)]
  im=Image.frombytes('RGBA',(w,h),bytes(c for v in vs[:w*h] for c in (255,255,255,v)))
 im.save(P/(name+'.png'))
 rows.append(dict(name=name,width=w,height=h,format=fmt,raw_path=str(P/(name+'.bin')),png_path=str(P/(name+'.png')),sha256=hashlib.sha256(raw).hexdigest(),**meta));return im
for off,name in re.findall(r'\[0x([0-9A-Fa-f]+), rnc, ([^,]+), regular\]',(D/'sssv.us.yaml').read_text()):
 off=int(off,16);data=dec(rom[off:]);base=Path(name.replace('.rgba16','')).name;biome='.rgba16' in name;n=8 if biome else 4
 for i in range(n):
  if (i+1)*2048>len(data):continue
  raw=data[i*2048:(i+1)*2048];alpha=data[n*2048+i*512:n*2048+(i+1)*512];nameid=('biome-' if biome else 'level-')+base+f'-{i:02d}'
  im=add(nameid,raw,32,32,'RGBA16',group='dynamic-billboard',container_rom_offset=hex(off),container_offset=i*2048,category=i if biome else i+8,source='overlay2_6AE5A0.c:331',paired_alpha_path=str(P/(nameid+'-alpha.bin')))
  if len(alpha)==512:
   add(nameid+'-alpha',alpha,32,32,'I4',group='dynamic-billboard-alpha',container_rom_offset=hex(off),container_offset=n*2048+i*512,parent=nameid)
   al=Image.frombytes('L',(32,32),bytes(v for b in alpha for v in ((b>>4)*17,(b&15)*17)));im.putalpha(al);im.save(P/(nameid+'-combined.png'));rows[-2]['combined_png_path']=str(P/(nameid+'-combined.png'))
for i in range(7):add(f'butterfly-{i:02d}',rom[0x7c8b80+i*2048:0x7c8b80+(i+1)*2048],32,32,'RGBA16',group='butterfly',rom_offset=hex(0x7c8b80+i*2048),frame=i)
for name,off in [('sign-silicon-valley',0x8d470),('sign-earth',0x8dc70)]:add(name,rom[off:off+2048],32,32,'RGBA16',group='sign',rom_offset=hex(off))
(O/'inventory.json').write_text(json.dumps(rows,indent=2))
for group in ['biome','level']:
 fs=[p for p in P.glob(group+'*-combined.png')];sheet=Image.new('RGB',(1024,((len(fs)+7)//8)*150),(45,45,45));dr=ImageDraw.Draw(sheet)
 for i,p in enumerate(fs):
  im=Image.open(p).resize((96,96),Image.Resampling.NEAREST);x=i%8*128;y=i//8*150;sheet.paste(im,(x,y),im);dr.text((x,y+98),p.stem[:20],fill='white');dr.text((x,y+112),p.stem[20:40],fill='white')
 sheet.save(O/(group+'-contact.png'))
print(len(rows),'inventory rows')
# Global particle/effect banks referenced by the world renderers.
for off in [0x5de20,0x5f020,0x60220,0x61420]:
 for i in range(9):add(f'particle-{off:06x}-{i:02d}',rom[off+i*512:off+(i+1)*512],32,32,'I4',group='particle-frame',rom_offset=hex(off+i*512),frame=i,bank=hex(off),source='overlay2_6DCA10.c:693-931')
for off in [0x65ae0,0x662e0,0x5bfa0,0x5c7a0,0x5a680,0x5ae80,0x5ce20,0x5d620,0x62620,0x62e20]:
 add(f'particle-atlas-{off:06x}',rom[off:off+2048],16,256,'I4',group='particle-atlas',rom_offset=hex(off),source='overlay2_6DCA10.c:367-539',note='Loaded as a complete 16x256 I4 atlas; retain full image layout.')
for i in range(16):
 off=0x636e0+i*1024;add(f'effect-i8-{i:02d}',rom[off:off+1024],32,32,'I8',group='dynamic-intensity',rom_offset=hex(off),category=32+i,source='overlay2_6AE5A0.c:237')
for i in range(8):
 off=0x7a580+i*2048;name=f'energy-animated-{i:02d}';im=add(name,rom[off:off+2048],32,32,'RGBA16',group='dynamic-energy',rom_offset=hex(off),frame=i,paired_alpha_path=str(P/(name+'-alpha.bin')),source='overlay2_6AE5A0.c:248')
 off=0x78580+i*1024;alpha=rom[off:off+1024];add(name+'-alpha',alpha,32,32,'I8',group='dynamic-energy-alpha',rom_offset=hex(off),parent=name)
 im.putalpha(Image.frombytes('L',(32,32),alpha));im.save(P/(name+'-combined.png'));rows[-2]['combined_png_path']=str(P/(name+'-combined.png'))
for off,sz,fmt,name in [(0x63ae0,32*128,'I8','star-atlas'),(0x652e0,2048,'I4','effect-large-652e0'),(0x71020,512,'I4','footprint'),(0x712f0,2048,'I4','ground-ring')]+[(off,2048,'I4',f'ground-shadow-{i}') for i,off in enumerate(range(0x71af0,0x742f0,0x800))]:
 w=32 if name in ['star-atlas','footprint'] else 64;h=sz*(2 if fmt=='I4' else 1)//w
 add(name,rom[off:off+sz],w,h,fmt,group='world-effect',rom_offset=hex(off))
# Exact-pair deduplication: different masks can change the same color image.
unique={};groups={}
for row in rows:
 raw=Path(row['raw_path']).read_bytes();mask=Path(row['paired_alpha_path']).read_bytes() if row.get('paired_alpha_path') and Path(row['paired_alpha_path']).exists() else b''
 sig=hashlib.sha256(row['format'].encode()+raw+mask).hexdigest();row['content_signature']=sig
 if sig not in unique:unique[sig]=dict(row,aliases=[])
 unique[sig]['aliases'].append(row['name']);groups.setdefault(row['group'],0);groups[row['group']]+=1
(O/'inventory.json').write_text(json.dumps(rows,indent=2));(O/'unique-assets.json').write_text(json.dumps(list(unique.values()),indent=2))
selected=[r for r in unique.values() if r['group'] in ['dynamic-billboard','butterfly','sign','dynamic-energy'] and Image.open(r.get('combined_png_path',r['png_path'])).getchannel('A').getextrema()!=(0,0)]
sheet=Image.new('RGB',(1024,((len(selected)+7)//8)*150),(45,45,45));dr=ImageDraw.Draw(sheet)
for i,row in enumerate(selected):
 im=Image.open(row.get('combined_png_path',row['png_path'])).resize((96,96),Image.Resampling.NEAREST);x=i%8*128;y=i//8*150;sheet.paste(im,(x,y),im);dr.text((x,y+98),row['name'][:20],fill='white');dr.text((x,y+112),row['name'][20:40],fill='white')
sheet.save(O/'unique-color-contact.png')
(O/'coverage.json').write_text(json.dumps({'scope':'World/level billboards, butterfly animation, world signs and global particle/effect banks. Excludes terrain, 3D skins, menus.','regular_banks_scanned':45,'groups':groups,'total_rows':len(rows),'unique_content_and_mask':len(unique),'source_renderers':['overlay2_6AE5A0.c:331 paired world billboards','overlay2_72C680.c:106-164 all biome/level regular banks','overlay2_6DCA10.c:367-539 particle atlases;693-931 animated particles;1855 butterflies','overlay2_785D10.c:86 footprints and ground ring','overlay2_6C5570.c:76 star atlas'],'hash_status':'Reference raw SHA256 and exact content+mask signatures included; RT64 hashes require TMEM layout and are handled by registration agent.'},indent=2))
print('Final',len(rows),'rows;',len(unique),'unique content',groups)
# 16x16 animation cells packed into each 16x256 particle I4 strip.
for off in [0x65ae0,0x662e0,0x5bfa0,0x5c7a0,0x5a680,0x5ae80,0x5ce20,0x5d620,0x62620,0x62e20]:
 for i in range(16):
  add(f'particle-atlas-{off:06x}-frame-{i:02d}',rom[off+i*128:off+(i+1)*128],16,16,'I4',group='particle-atlas-frame',rom_offset=hex(off+i*128),parent=f'particle-atlas-{off:06x}',frame=i,hash_profiles=[dict(siz=0,fmt=4,line=1,tmem=0,load_width=16,load_siz=0,load_method='block4b',render_width=16,height=16,tlut=0)],note='Native atlas cell; replacement can reference an authored4x4grid with normalizedrect.')
(O/'inventory.json').write_text(json.dumps(rows,indent=2))
print('Final with atlas cells:',len(rows))
# Explicit native render profiles, for TMEM-based RT64 registration.
for row in rows:
 fmt=row['format'];w=row['width'];h=row['height'];siz={'RGBA16':2,'I8':1,'I4':0}[fmt];gfmt=0 if fmt=='RGBA16' else 4;line=(w*(16 if fmt=='RGBA16' else 8 if fmt=='I8' else 4)+63)//64
 row.setdefault('hash_profiles',[dict(siz=siz,fmt=gfmt,line=line,tmem=0,render_width=w,height=h,tlut=0)])
 if row['group'] in ['dynamic-billboard-alpha','dynamic-energy-alpha']:row['hash_profiles'][0]['tmem']=256
unique={}
for row in rows:
 raw=Path(row['raw_path']).read_bytes();mask=Path(row['paired_alpha_path']).read_bytes() if row.get('paired_alpha_path') and Path(row['paired_alpha_path']).exists() else b''
 sig=hashlib.sha256(row['format'].encode()+raw+mask).hexdigest();row['content_signature']=sig
 if sig not in unique:unique[sig]=dict(row,aliases=[])
 unique[sig]['aliases'].append(row['name'])
(O/'inventory.json').write_text(json.dumps(rows,indent=2));(O/'unique-assets.json').write_text(json.dumps(list(unique.values()),indent=2))
authoring=[]
for row in unique.values():
 if row['group'] not in ['dynamic-billboard','butterfly','sign','dynamic-energy']:continue
 im=Image.open(row.get('combined_png_path',row['png_path']))
 if im.getchannel('A').getextrema()==(0,0) or len(set(im.get_flattened_data()))==1:continue
 authoring.append(dict(name=row['name'],reference=row.get('combined_png_path',row['png_path']),group=row['group'],aliases=row['aliases'],raw_path=row['raw_path'],mask_path=row.get('paired_alpha_path'),width=row['width'],height=row['height'],content_signature=row['content_signature']))
(O/'authoring-list.json').write_text(json.dumps(authoring,indent=2))
coverage=json.loads((O/'coverage.json').read_text());coverage.update(regular_banks_scanned=49,total_rows=len(rows),unique_content_and_mask=len(unique),particle_atlas_cells=160,sign_coverage='Both textured signs explicitly named in YAML included; generic signpost is untextured mesh.');(O/'coverage.json').write_text(json.dumps(coverage,indent=2))
# Native star texture loads sample a32x32 cell, not necessarily the entirebank.
for i in range(4):
 off=0x63ae0+i*1024;add(f'star-frame-{i:02d}',rom[off:off+1024],32,32,'I8',group='star-frame',rom_offset=hex(off),frame=i,parent='star-atlas',hash_profiles=[dict(siz=1,fmt=4,line=4,tmem=0,render_width=32,height=32,tlut=0)],source='overlay2_6C5570.c:76(firstframe); overlay2_6DCA10.c547(fullbank)')
usage={}
for level in (R/'diagnostics/full-game-test/levels').glob('*.json'):
 try:lev=json.loads(level.read_text())
 except:continue
 for obj in lev.get('cob',[]):
  if obj['id'] in [124,166,170,243] and obj['unkC']>=32:usage.setdefault(str(obj['unkC']),[]).append(dict(level=level.stem,object_id=obj['id']))
for row in rows:
 if row['group']=='dynamic-intensity':
  cat=row['category'];row['usage_status']='confirmed-constant-call' if cat in [32,33] else 'confirmed-level-parameter' if str(cat) in usage else 'addressable-but-not-observed-in-static-calls-or-level-data';row['usage_evidence']=usage.get(str(cat),[])
  if cat==39:row['authoring_status']='retain-native: thisaddressoverlapsI4chainmaskandI8viewisnotrecognizableart'
 if row['name']=='effect-large-652e0':row['authoring_status']='out-of-scope: lavenderchain3Dmaterial, dynamic_tail.c751'
unique={}
for row in rows:
 raw=Path(row['raw_path']).read_bytes();mask=Path(row['paired_alpha_path']).read_bytes() if row.get('paired_alpha_path') and Path(row['paired_alpha_path']).exists() else b''
 sig=hashlib.sha256(row['format'].encode()+raw+mask).hexdigest();row['content_signature']=sig
 if sig not in unique:unique[sig]=dict(row,aliases=[])
 unique[sig]['aliases'].append(row['name'])
(O/'inventory.json').write_text(json.dumps(rows,indent=2));(O/'unique-assets.json').write_text(json.dumps(list(unique.values()),indent=2));(O/'dynamic-intensity-usage.json').write_text(json.dumps(usage,indent=2))
coverage=json.loads((O/'coverage.json').read_text());coverage.update(total_rows=len(rows),unique_content_and_mask=len(unique),intensity_usage_caveat='Onlycategories32,33,35,39staticallyobserved;cat39overlapsI4chainmaskandstaysnative;otheraddressablecategoriesnotconfirmedused.');(O/'coverage.json').write_text(json.dumps(coverage,indent=2))
# Generic projected ground shadows and two trail intensity atlases.
for name,off in [('shadow-generic',0x6ca60),('shadow-object-0',0x6da60),('shadow-object-2',0x6e260)]:
 add(name,rom[off:off+2048],64,64,'I4',group='projected-shadow',rom_offset=hex(off),hash_profiles=[dict(siz=0,fmt=4,line=4,tmem=0,render_width=64,height=64,tlut=0)],source='sssv/init.c:16-20; overlay2_6AE5A0.c1004; animals/* func_8034BD20_75D3D0')
for bank,off in enumerate([0x7e580,0x7ed80]):
 name=f'trail-atlas-{bank}';add(name,rom[off:off+2048],32,128,'I4',group='trail-atlas',rom_offset=hex(off),hash_profiles=[dict(siz=0,fmt=4,line=2,tmem=bank*256,render_width=32,height=128,tlut=0)],source='sssv/trails.c393-394')
 for i in range(4):
  of=off+i*512;add(f'{name}-frame-{i}',rom[of:of+512],32,32,'I4',group='trail-frame',parent=name,frame=i,rom_offset=hex(of),hash_profiles=[dict(siz=0,fmt=4,line=2,tmem=bank*256,render_width=32,height=32,tlut=0)],source='data/D_01004A70_3E340.c12 firstcell;trails.c393 fullatlas')
unique={}
for row in rows:
 raw=Path(row['raw_path']).read_bytes();mask=Path(row['paired_alpha_path']).read_bytes() if row.get('paired_alpha_path') and Path(row['paired_alpha_path']).exists() else b''
 sig=hashlib.sha256(row['format'].encode()+raw+mask).hexdigest();row['content_signature']=sig
 if sig not in unique:unique[sig]=dict(row,aliases=[])
 unique[sig]['aliases'].append(row['name'])
(O/'inventory.json').write_text(json.dumps(rows,indent=2));(O/'unique-assets.json').write_text(json.dumps(list(unique.values()),indent=2))
coverage=json.loads((O/'coverage.json').read_text());coverage.update(total_rows=len(rows),unique_content_and_mask=len(unique),projected_shadows=3,trail_atlases=2);(O/'coverage.json').write_text(json.dumps(coverage,indent=2))
# Final coverage totals include native subframes, trails and projected shadows.
from collections import Counter
coverage=json.loads((O/'coverage.json').read_text());coverage['groups']=dict(Counter(x['group'] for x in rows));(O/'coverage.json').write_text(json.dumps(coverage,indent=2))
# Retain native utility whites: these are shader masks, not missing artwork.
for row in rows:
 if row['group']=='dynamic-billboard' and row.get('paired_alpha_path'):
  color=Path(row['raw_path']).read_bytes();alpha=Path(row['paired_alpha_path']).read_bytes()
  if color and all(v==255 for v in color) and alpha and all(v==255 for v in alpha):
   row['authoring_status']='retain-native: uniform white utility color+mask; not a visible authored object'
# Walrus/native trail tile starts at TMEM0x20, 256bytes into the first atlas,
# then straddles the secondatlas loaded atTMEM0x100.
trail_cross=rom[0x7e580+256:0x7ed80]+rom[0x7ed80:0x7ed80+256]
add('trail-native-cross',trail_cross,32,128,'I4',group='trail-native-cross',hash_profiles=[dict(siz=0,fmt=4,line=2,tmem=32,render_width=32,height=128,tlut=0)],source='sssv/trails.c; TMEM0x20 cross-bank window',rom_ranges=[['0x7e680','0x7ed80'],['0x7ed80','0x7ee80']])
unique={}
for row in rows:
 raw=Path(row['raw_path']).read_bytes();mask=Path(row['paired_alpha_path']).read_bytes() if row.get('paired_alpha_path') and Path(row['paired_alpha_path']).exists() else b''
 sig=hashlib.sha256(row['format'].encode()+raw+mask).hexdigest();row['content_signature']=sig
 if sig not in unique:unique[sig]=dict(row,aliases=[])
 unique[sig]['aliases'].append(row['name'])
(O/'inventory.json').write_text(json.dumps(rows,indent=2));(O/'unique-assets.json').write_text(json.dumps(list(unique.values()),indent=2))
coverage=json.loads((O/'coverage.json').read_text());coverage.update(total_rows=len(rows),unique_content_and_mask=len(unique));coverage['groups']=dict(Counter(x['group'] for x in rows));(O/'coverage.json').write_text(json.dumps(coverage,indent=2))


# Illustrated tutorial sign mip chains: Have A Nice Day uses level bank index1,
# distinct from the Smashing Start artwork bank despite appearing near the start.
# These native RGBA16 materials are pre-swizzled and loaded as one dxt=0 block.
sign_refs = O / 'tutorial-sign-references'
sign_refs.mkdir(exist_ok=True)
sign_bank = dec(rom[0x461A20:])
sign_names = {0: 'tutorial-sign-sheep', 1: 'tutorial-sign-dog', 4: 'tutorial-sign-sheep-jump'}
for index, name in sign_names.items():
    packed = sign_bank[index * 0xAB8:(index + 1) * 0xAB8]
    raw_path = sign_refs / f'tutorial-{index}-tmem.bin'
    png_path = sign_refs / f'tutorial-{index}-native.png'
    raw_path.write_bytes(packed)
    pixels = bytearray()
    for y in range(32):
        for x in range(32):
            offset = y * 64 + ((x * 2) ^ (4 if y & 1 else 0))
            value = int.from_bytes(packed[offset:offset + 2], 'big')
            pixels.extend((((value >> 11) & 31) * 255 // 31,
                           ((value >> 6) & 31) * 255 // 31,
                           ((value >> 1) & 31) * 255 // 31, (value & 1) * 255))
    Image.frombytes('RGBA', (32, 32), bytes(pixels)).save(png_path)
    for mip, (width, line, address) in enumerate(zip([32,16,8,4,2,1], [8,4,2,1,1,1], [0,256,320,336,340,342])):
        rows.append(dict(name=f'{name}-mip-{mip}', width=width, height=width, format='RGBA16',
                         raw_path=str(raw_path), png_path=str(png_path),
                         sha256=hashlib.sha256(packed).hexdigest(), group='tutorial-sign-mipmap',
                         native_tmem_layout='preswizzled-mipmap', tmem_line=line, tmem_address=address,
                         container_rom_offset='0x461a20', container_offset=index * 0xAB8,
                         texture_index=20 + index, mip=mip,
                         source='overlay2_6AE5A0.c:843; data/D_01004510_3DDE0.c',
                         material='RGBA16 mipmapped signpost face',
                         hash_profiles=[dict(siz=2,fmt=0,line=line,tmem=address,render_width=width,
                                             height=width,tlut=0,load_method='block',load_dxt=0,
                                             packed_chain_size=0xAB8)]))
unique = {}
for row in rows:
    raw = Path(row['raw_path']).read_bytes()
    mask = Path(row['paired_alpha_path']).read_bytes() if row.get('paired_alpha_path') and Path(row['paired_alpha_path']).exists() else b''
    profile = str((row['width'], row['height'], row.get('tmem_line'), row.get('tmem_address'))).encode() if row.get('native_tmem_layout') else b''
    signature = hashlib.sha256(row['format'].encode() + raw + mask + profile).hexdigest()
    row['content_signature'] = signature
    if signature not in unique:
        unique[signature] = dict(row, aliases=[])
    unique[signature]['aliases'].append(row['name'])
(O/'inventory.json').write_text(json.dumps(rows, indent=2))
(O/'unique-assets.json').write_text(json.dumps(list(unique.values()), indent=2))
coverage = json.loads((O/'coverage.json').read_text())
coverage.update(total_rows=len(rows), unique_content_and_mask=len(unique), groups=dict(Counter(x['group'] for x in rows)),
                sign_coverage='Earth/Silicon Valley flat textures plus HAVE_A_NICE_DAY illustrated sheep, dog, jumping-sheep mipmapped faces; all six native mip levels. Other mipmapped sign materials not yet audited.')
(O/'coverage.json').write_text(json.dumps(coverage, indent=2))
