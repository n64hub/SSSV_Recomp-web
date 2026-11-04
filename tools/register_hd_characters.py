"""Install authored character material tiles under HD World Textures."""
from pathlib import Path
import json,struct
import xxhash
from PIL import Image
from register_hd_world import native_hash
R=Path(__file__).resolve().parents[1];O=R/'diagnostics/hd-characters';P=R/'Textures/hd-world';A=P/'art/characters';A.mkdir(exist_ok=True)
rom=(R/'sssv.us.z64').read_bytes();rows=json.loads((O/'inventory.json').read_text());sheet=Image.open(O/'hd-atlas.png').convert('RGBA');manifest=json.loads((P/'rt64.json').read_text());manifest['textures']=[x for x in manifest['textures'] if not x['path'].startswith('art/characters/')];coverage=[];seen=set()
for i,row in enumerate(rows):
 w,h=row['width'],row['height'];off=row['offset'];size=w*h//2 if row['format']=='ci4' else w*h*2;raw=rom[off:off+size]
 if row['format']=='ci4':
  data=bytearray();stride=w//2
  for y in range(h):
   line=raw[y*stride:(y+1)*stride]
   for x in range(0,stride,8):
    word=line[x:x+8];data.extend(word[4:]+word[:4] if y&1 else word)
  used=sorted({v for b in raw for v in (b>>4,b&15)});pal=rom[row['palette']:row['palette']+32]
  for index in used:data.extend(pal[index*2:index*2+2]*4)
  digest=xxhash.xxh3_64_hexdigest(data+struct.pack('<HHIHBB',w,h,2<<14,w//16,0,2))
 else:digest=native_hash(raw,w,h,'RGBA16')
 col=i%6;rr=i//6;box=(round(col*sheet.width/6),round(rr*sheet.height/5),round((col+1)*sheet.width/6),round((rr+1)*sheet.height/5));box=(box[0]+3,box[1]+3,box[2]-3,box[3]-3);im=sheet.crop(box);name=row['name']+'.png';im.save(A/name);rel='art/characters/'+name
 
 if digest not in seen:manifest['textures'].append(dict(path=rel,hashes={'rt64':digest},operation='preload',shift='none'));seen.add(digest)
 coverage.append(dict(row,path=rel,hash=digest))
(P/'rt64.json').write_text(json.dumps(manifest,indent=2)+'\n');(P/'characters.json').write_text(json.dumps(coverage,indent=2)+'\n');print(len(coverage),'character materials registered')
