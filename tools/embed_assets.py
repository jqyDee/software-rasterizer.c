#!/usr/bin/env python3
"""
Generate embedded_assets.c from asset directories.
Usage: embed_assets.py <obj_dir> <tex_dir> <out_c> <out_h_unused>
The .h is a stable source file; this script only writes the .c.
"""
import sys
import os


def c_ident(path):
    name = os.path.basename(path)
    return '_' + ''.join(c if c.isalnum() else '_' for c in name)


def embed_file(f, path):
    ident = c_ident(path)
    with open(path, 'rb') as fp:
        data = fp.read()
    f.write(f'static const unsigned char {ident}[] = {{\n')
    for i, b in enumerate(data):
        if i % 16 == 0:
            f.write('    ')
        f.write(f'0x{b:02x},')
        if i % 16 == 15:
            f.write('\n')
    if len(data) % 16 != 0:
        f.write('\n')
    f.write('};\n\n')
    return ident, len(data)


def collect(d, exts):
    files = []
    if os.path.isdir(d):
        for name in sorted(os.listdir(d)):
            ext = os.path.splitext(name)[1].lower()
            if ext in exts:
                files.append(os.path.join(d, name))
    return files


def main():
    if len(sys.argv) != 5:
        print(f'Usage: {sys.argv[0]} <obj_dir> <tex_dir> <out_c> <out_h_unused>')
        sys.exit(1)

    obj_dir, tex_dir, out_c = sys.argv[1], sys.argv[2], sys.argv[3]

    obj_files = collect(obj_dir, {'.obj'})
    tex_files = collect(tex_dir, {'.png', '.jpg', '.jpeg'})
    all_files = obj_files + tex_files

    with open(out_c, 'w') as f:
        f.write('#include <string.h>\n')
        f.write('#include "embedded_assets.h"\n\n')

        entries = []
        for path in all_files:
            ident, size = embed_file(f, path)
            rel = path.replace('\\', '/')
            entries.append((rel, ident, size))

        f.write(f'const int embedded_asset_count = {len(entries)};\n\n')
        f.write('const embedded_asset_t embedded_assets[] = {\n')
        for rel, ident, size in entries:
            f.write(f'    {{ "{rel}", {ident}, {size} }},\n')
        f.write('    { 0, 0, 0 }\n')
        f.write('};\n\n')

        f.write('const embedded_asset_t *find_embedded_asset(const char *path) {\n')
        f.write('    for (int i = 0; i < embedded_asset_count; i++) {\n')
        f.write('        if (strcmp(embedded_assets[i].path, path) == 0)\n')
        f.write('            return &embedded_assets[i];\n')
        f.write('    }\n')
        f.write('    return 0;\n')
        f.write('}\n')


if __name__ == '__main__':
    main()
