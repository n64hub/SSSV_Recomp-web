"""Build crisp font replacements while retaining the game's glyph rectangles.

Requires Pillow, xxhash and the existing local rncu DLL. Builds replacement font
artwork and an upscaled textless menu frame from the user's local ROM.
"""
import argparse
import ctypes
import json
import pathlib
import struct
import string

from PIL import Image, ImageDraw, ImageFont
import xxhash

root = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--rom', type=pathlib.Path, default=root / 'sssv.us.z64')
parser.add_argument('--rncu', type=pathlib.Path, default=root / 'diagnostics/full-game-test/rncu.dll')
args = parser.parse_args()
rom = args.rom.read_bytes()
assert xxhash.xxh3_64_intdigest(rom) == 0x912A068AADB0D0C5, 'Requires the supported US 1.0 ROM'
compressed = rom[0x12A390:0x12EDC0]
assert compressed[:4] == b'RNC\x01'
size = struct.unpack_from('>I', compressed, 4)[0]
lib = ctypes.CDLL(str(args.rncu))
lib.unpack.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32]
source = ctypes.create_string_buffer(compressed[18:] + b'\0' * 16)
target = ctypes.create_string_buffer(size + 16)
lib.unpack(source, target, size)
glyphs = target.raw[:size]
out = root / 'assets/hd-font'
out.mkdir(exist_ok=True)
font = ImageFont.truetype(str(out / 'ComicNeue-Bold.ttf'), 256)
entries = []
scale = 16
width_table = 0x1000 + 0x80154370 - 0x80125900
# Several apparent ASCII punctuation slots actually contain Japanese glyphs or
# UI artwork (music/speaker icons, volume segments, checkboxes). Preserve them.
characters = string.ascii_letters + string.digits + '!"\',-.:?'
outlines = {}
for character in characters:
    mask = Image.new('L', (512, 512))
    ImageDraw.Draw(mask).text((64, 320), character, font=font, anchor='ls', fill=255)
    outlines[character] = (mask, mask.getbbox())
# Use one uniform outline scale for every character. In particular, do not fit
# each capital's height to its old pixel-art ink box: that makes letters bounce.
# Limit the common size by the narrowest native letter advance, retaining room
# between glyphs without distorting individual letters or changing line breaks.
cap_height = 320 - outlines['H'][1][1]
outline_scale = min(11 * scale / cap_height, *(
    max(1, rom[width_table + ord(ch) + 0x110] - 0.75) * scale /
    (outlines[ch][1][2] - outlines[ch][1][0])
    for ch in string.ascii_letters + string.digits
))
for obsolete in out.glob('glyph-*.png'):
    obsolete.unlink()
for code in sorted(map(ord, characters)):
    tile = code + 0x110
    raw = glyphs[tile * 128:(tile + 1) * 128]
    assert len(raw) == 128
    mask, bbox = outlines[chr(code)]
    crop = mask.crop(bbox)
    advance = rom[width_table + tile]
    crop = crop.resize((round((bbox[2] - bbox[0]) * outline_scale),
                        round((bbox[3] - bbox[1]) * outline_scale)), Image.Resampling.LANCZOS)
    alpha = Image.new('L', (16 * scale, 16 * scale))
    left = round((advance * scale - crop.width) / 2)
    top = round(12 * scale + (bbox[1] - 320) * outline_scale)
    assert left >= 0 and top >= 0 and top + crop.height <= alpha.height, chr(code)
    alpha.paste(crop, (left, top))
    # I4 uses intensity as both RGB and alpha; the replacement's white RGB
    # avoids dark fringes when filtered while its alpha supplies coverage.
    image = Image.new('RGBA', alpha.size, (255, 255, 255, 0))
    image.putalpha(alpha)
    name = f'glyph-{code:03}.png'
    image.save(out / name)
    # N64 loadBlock: one 64-bit word per row, DXT=2048 swaps odd-row halves.
    tmem = bytearray(raw)
    for row in range(1, 16, 2):
        start = row * 8
        tmem[start:start + 8] = raw[start + 4:start + 8] + raw[start:start + 4]
    metadata = struct.pack('<HHIHBB', 16, 16, 0, 1, 0, 4)
    digest = xxhash.xxh3_64_hexdigest(tmem + metadata)
    entries.append({'path': name, 'hashes': {'rt64': digest}, 'operation': 'preload', 'shift': 'none'})
# Upscale the original menu frame, removing only the six-pixel-high baked label.
# The game draws the new caption as live glyphs, not as part of this PNG.
compressed_header = rom[0x11EB70:]
assert compressed_header[:4] == b'RNC\x01'
header_size = struct.unpack_from('>I', compressed_header, 4)[0]
header_source = ctypes.create_string_buffer(compressed_header[18:] + b'\0' * 16)
header_target = ctypes.create_string_buffer(header_size + 16)
lib.unpack(header_source, header_target, header_size)
header_raw = header_target.raw[:header_size]
assert header_size == 320 * 36 * 2
rgba = [((v >> 11) * 255 // 31, ((v >> 6) & 31) * 255 // 31,
         ((v >> 1) & 31) * 255 // 31, (v & 1) * 255)
        for (v,) in struct.iter_unpack('>H', header_raw)]
header = Image.new('RGBA', (320, 36))
header.putdata(rgba)
ImageDraw.Draw(header).rectangle((23, 7, 233, 14), fill=(0, 0, 0, 0))
# guSprite2DInit renders 35 of the source image's 36 rows.
header = header.crop((0, 0, 320, 35)).resize((2560, 280), Image.Resampling.LANCZOS)
header.save(out / 'header-frame.png')
# Sprite2D full-image hashes use the emulator's word-swapped RDRAM bytes.
header_bytes = header_raw[:320 * 35 * 2]
swapped = b''.join(header_bytes[i:i + 4][::-1] for i in range(0, len(header_bytes), 4))
header_hash = xxhash.xxh3_64_hexdigest(swapped + struct.pack('<BBHH', 2, 0, 0, 0))
entries.append({'path': 'header-frame.png', 'hashes': {'rt64': header_hash},
                'operation': 'preload', 'shift': 'none'})
database = {'configuration': {'configurationVersion': 3, 'hashVersion': 5,
                              'defaultOperation': 'preload', 'defaultShift': 'none'},
            'textures': entries}
(out / 'rt64.json').write_text(json.dumps(database, indent=2) + '\n')
print(f'Generated {len(entries) - 1} outline-rendered 256px glyphs and a textless menu frame.')
print(f'Common scale {outline_scale:.4f}, baseline {12 * scale}px, capital height {cap_height * outline_scale / scale:.2f} native pixels.')
