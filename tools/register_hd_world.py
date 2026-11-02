"""Register HD world art without changing the native geometry or animation.

Native hashes follow RT64 TMEMHasher v5, not Sprite2D's RDRAM hash.
The inventory holds big-endian N64 texture bytes. TMEM restores this byte
order, then swaps the two 32-bit halves of each 64-bit word on odd rows.
Authored PNG files are never resampled or modified by this tool.
"""
from pathlib import Path
import argparse
import json
import struct

from PIL import Image
import xxhash

ROOT = Path(__file__).resolve().parents[1]
FORMATS = {'RGBA16': (2, 0), 'I4': (0, 4), 'I8': (1, 4), 'IA8': (1, 3), 'IA16': (2, 3)}


def native_hash(raw, width, height, fmt):
    """Dense, nonpaletted native TMEM image (loadTile or equivalent loadBlock)."""
    siz, image_fmt = FORMATS[fmt]
    row_bytes = max((width << siz) >> 1, 1)
    line = (row_bytes + 7) // 8
    if len(raw) != row_bytes * height or row_bytes % 8:
        raise ValueError(f'Unsupported non-dense TMEM image {fmt} {width}x{height}: {len(raw)} bytes')
    tmem = bytearray(line * 8 * height)
    for y in range(height):
        for x in range(row_bytes):
            tmem[y * line * 8 + (x ^ (4 if y & 1 else 0))] = raw[y * row_bytes + x]
    return xxhash.xxh3_64_hexdigest(tmem + struct.pack('<HHIHBB', width, height, 0, line, siz, image_fmt))



def packed_mipmap_hash(raw, width, height, line, address):
    """RT64 v5 hash for an already-swizzled RGBA16 chain loaded with dxt=0.

    Small native mips use padded TMEM rows; their bytes cannot be hashed as a
    newly swizzled dense image. Match TMEMHasher's partial odd-row handling.
    """
    tmem = raw + bytes(max(4096 - len(raw), 0))
    row_bytes, stride, start = width * 2, line * 8, address * 8
    if stride > row_bytes:
        data = bytearray()
        for y in range(height):
            base = start + y * stride
            if y & 1:
                full = row_bytes // 8 * 8
                data.extend(tmem[base:base + full])
                remain = row_bytes - full
                base += full
                if remain > 4:
                    data.extend(tmem[base:base + remain - 4])
                    data.extend(tmem[base + 4:base + 8])
                elif remain:
                    data.extend(tmem[base + 4:base + 4 + remain])
            else:
                data.extend(tmem[base:base + row_bytes])
    else:
        data = tmem[start:start + stride * (height - 1) + row_bytes]
    return xxhash.xxh3_64_hexdigest(bytes(data) + struct.pack('<HHIHBB', width, height, 0, line, 2, 0))


def uv_transform(item, replacement):
    width, height = item['width'], item['height']
    original = Path(item.get('combined_png_path', item['png_path']))
    with Image.open(original) as image:
        old = image.convert('RGBA').getchannel('A').getbbox()
    with Image.open(replacement) as image:
        rw, rh = image.size
        new = image.convert('RGBA').getchannel('A').point(lambda a: 255 if a >= 32 else 0).getbbox()
    if not old or not new:
        raise ValueError(f'Empty RGBA world artwork: {item["name"]}')
    sx = ((new[2] - new[0]) / rw) / ((old[2] - old[0]) / width)
    sy = ((new[3] - new[1]) / rh) / ((old[3] - old[1]) / height)
    return sx, sy, new[0] / rw - old[0] / width * sx, new[1] / rh - old[1] / height * sy, rw, rh


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inventory', type=Path, default=ROOT / 'diagnostics/hd-world/inventory.json')
    parser.add_argument('--unique', type=Path, default=ROOT / 'diagnostics/hd-world/unique-assets.json')
    parser.add_argument('--pack', type=Path, default=ROOT / 'Textures/hd-world')
    args = parser.parse_args()
    items = json.loads(args.inventory.read_text())
    uniques = json.loads(args.unique.read_text()) if args.unique.exists() else items
    canonical = {alias: entry['name'] for entry in uniques for alias in entry.get('aliases', [entry['name']])}
    # Optional explicit author mapping: native/canonical name -> pack-relative PNG.
    mapping_path = args.pack / 'artworks.json'
    mapping = json.loads(mapping_path.read_text()) if mapping_path.exists() else {}
    paths = {p.stem: p.relative_to(args.pack).as_posix() for p in args.pack.rglob('*.png')}
    hashes, pairs, transforms, intensities, coverage, missing, retained = {}, {}, {}, {}, [], [], []
    for item in items:
        name = item['name']
        status = item.get('authoring_status', '')
        if status.startswith('out-of-scope:') or (status.startswith('retain-native:') and name not in mapping):
            retained.append({'source': name, 'reason': status})
            continue
        if item.get('usage_status') == 'addressable-but-not-observed-in-static-calls-or-level-data' and name not in mapping:
            retained.append({'source': name, 'reason': 'Unused addressable intensity view; no static renderer/level-data usage. Native fallback retained.'})
            continue
        if item['group'] in ('dynamic-billboard-alpha', 'dynamic-energy-alpha'):
            continue  # Paired alpha selects its color's RGBA replacement at render time.
        raw = Path(item['raw_path']).read_bytes()
        if (item.get('paired_alpha_path') and not any(Path(item['paired_alpha_path']).read_bytes())) or not any(raw):
            retained.append({'source': name, 'reason': 'Empty native alpha/image/frame; preserve transparent/zero-intensity fallback.'})
            continue
        key = canonical.get(name, name)
        authored = mapping.get(name, mapping.get(key, paths.get(key)))
        relative = authored.get('path') if isinstance(authored, dict) else authored
        if not relative or not (args.pack / relative).exists():
            missing.append(name)
            continue
        if item.get('native_tmem_layout') == 'preswizzled-mipmap':
            digest = packed_mipmap_hash(raw, item['width'], item['height'], item['tmem_line'], item['tmem_address'])
        else:
            digest = native_hash(raw, item['width'], item['height'], item['format'])
        color_digest = digest
        mask = None
        if item.get('paired_alpha_path'):
            mask_format = 'I8' if item['group'] == 'dynamic-energy' else 'I4'
            mask = native_hash(Path(item['paired_alpha_path']).read_bytes(), item['width'], item['height'], mask_format)
            # A native color can have several different alpha masks. Give the
            # complete pair its own key instead of silently conflating shapes.
            digest = xxhash.xxh3_64_hexdigest(b'SSSVHDWorldPair\0' + bytes.fromhex(color_digest + mask))
        prior = hashes.get(digest)
        if prior:
            # Identical native frames can recur in different animation banks.
            # Keep one deterministic authored frame for the identical hash,
            # including its UV rectangle; never mix one PNG with another rect.
            coverage.append({'source': name, 'canonical': key, 'path': prior, 'hash': digest,
                             'native_size': [item['width'], item['height']], 'identical_native_alias': True})
            continue
        hashes[digest] = relative
        replacement = args.pack / relative
        with Image.open(replacement) as image:
            rw, rh = image.size
        rect = authored.get('rect') if isinstance(authored, dict) else None
        if rect:
            if len(rect) != 4 or not (0 <= rect[0] < rect[2] <= 1 and 0 <= rect[1] < rect[3] <= 1):
                raise ValueError(f'Invalid normalized artwork rectangle: {name} {rect}')
            transforms[digest] = (rect[2] - rect[0], rect[3] - rect[1], rect[0], rect[1], rw, rh)
        elif item['format'] == 'RGBA16':
            transforms[digest] = uv_transform(item, replacement)
        if item['format'] in ('I4', 'I8'):
            intensities[digest] = (rw, rh)
        if mask:
            pairs[(color_digest, mask)] = (rw, rh, mask_format, digest)
        coverage.append({'source': name, 'canonical': key, 'path': relative, 'hash': digest,
                         'native_size': [item['width'], item['height']]})
    args.pack.mkdir(parents=True, exist_ok=True)
    config = {'configurationVersion': 3, 'hashVersion': 5, 'defaultOperation': 'preload', 'defaultShift': 'none'}
    entries = [{'path': path, 'hashes': {'rt64': digest}, 'operation': 'preload', 'shift': 'none'} for digest, path in sorted(hashes.items())]
    (args.pack / 'rt64.json').write_text(json.dumps({'configuration': config, 'textures': entries}, indent=2) + '\n')
    report = {'registered': coverage, 'missing': missing, 'retained_native': retained, 'hash_algorithm': 'RT64 TMEMHasher v5 dense nonpaletted native TMEM',
              'paired_masks': [{'color': color, 'mask': mask, 'size': list(v[:2]), 'format': v[2], 'replacement': v[3]} for (color, mask), v in sorted(pairs.items())]}
    (args.inventory.parent / 'registered.json').write_text(json.dumps(report, indent=2) + '\n')
    header = ['// Generated by tools/register_hd_world.py; artwork is never modified.', '#pragma once', '#include <cstdint>',
              'namespace RT64::SSSVHDWorld {',
              'inline uint64_t pairedTexture(uint64_t color, uint64_t mask) {']
    for (color, mask), (rw, rh, _, replacement) in sorted(pairs.items()):
        header.append(f'    if (color == 0x{color}ULL && mask == 0x{mask}ULL) return 0x{replacement}ULL;')
    header += ['    return 0;', '}', 'inline bool pairDimensions(uint64_t hash, uint32_t width, uint32_t height) {', '    switch (hash) {']
    for replacement, (rw, rh) in sorted({v[3]: v[:2] for v in pairs.values()}.items()):
        header.append(f'    case 0x{replacement}ULL: return width == {rw} && height == {rh};')
    header += ['    default: return false;', '    }', '}', 'inline bool intensity(uint64_t hash, uint32_t width, uint32_t height) {', '    switch (hash) {']
    for digest, (rw, rh) in sorted(intensities.items()):
        header.append(f'    case 0x{digest}ULL: return width == {rw} && height == {rh};')
    header += ['    default: return false;', '    }', '}', 'inline bool textureUV(uint64_t hash, float &sx, float &sy, float &tx, float &ty, uint32_t width, uint32_t height) {', '    switch (hash) {']
    for digest, values in sorted(transforms.items()):
        sx, sy, tx, ty, rw, rh = values
        assignments = ' '.join(f'{k} = {v:.9f}f;' for k, v in zip(('sx', 'sy', 'tx', 'ty'), (sx, sy, tx, ty)))
        header.append(f'    case 0x{digest}ULL: if (width != {rw} || height != {rh}) return false; {assignments} return true;')
    header += ['    default: return false;', '    }', '}', '}']
    (ROOT / 'lib/rt64/src/gbi/sssv_hd_world.h').write_text('\n'.join(header) + '\n')
    print(f'Registered {len(hashes)} unique world hashes covering {len(coverage)} inventory rows; {len(missing)} not authored.')


if __name__ == '__main__':
    main()
