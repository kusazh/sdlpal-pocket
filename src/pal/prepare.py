#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare a private Pocket installation from the user's DOS PAL files."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import struct
import subprocess
import tempfile
import urllib.request

NAMES = 'abc.mkf ball.mkf data.mkf f.mkf fbp.mkf fire.mkf gop.mkf map.mkf mgo.mkf midi.mkf mus.mkf pat.mkf rgm.mkf rng.mkf sss.mkf voc.mkf m.msg word.dat wor16.asc wor16.fon'.split()
SDK_REV = 'a408ddc12aed0dfaa4aa22c06af82f829db77126'
RUNTIME = {
    'pocket/os25.rbf_r': ('637c8e0f2656f5ac7344e43f98262617', 'Cores/Local.PAL/os25.rbf_r'),
    'pocket/loader.bin': ('48c625e82daf68d14680893ac8cb27af', 'Cores/Local.PAL/loader.bin'),
    'pocket/os.bin': ('cdc237dc18192844beff9375ebe4e7b1', 'Assets/pal/common/os.bin'),
}


def game_files(directory):
    files = {}
    for p in directory.iterdir():
        if not p.is_file():
            continue
        name = p.name.lower()
        if name in files and name in NAMES:
            raise ValueError('Duplicate filename ignoring case: ' + name)
        files[name] = p
    missing = [n for n in NAMES if n not in files]
    if missing:
        raise ValueError('Missing DOS game files: ' + ', '.join(missing))
    for name in NAMES:
        if name.endswith('.mkf'):
            data = files[name].read_bytes()
            if len(data) < 8:
                raise ValueError('Invalid MKF: ' + name)
            first, = struct.unpack_from('<I', data)
            if first < 8 or first % 4 or first > len(data):
                raise ValueError('Invalid MKF index: ' + name)
            offsets = struct.unpack_from('<%dI' % (first // 4), data)
            if any(a > b for a, b in zip(offsets, offsets[1:])) or offsets[-1] > len(data):
                raise ValueError('Invalid MKF bounds: ' + name)
    fbp = files['fbp.mkf'].read_bytes()
    offset, = struct.unpack_from('<I', fbp)
    if fbp[offset:offset + 4] != b'YJ_1':
        raise ValueError('This release requires DOS/YJ_1 data, not Windows 95/98 data.')
    return files


def pack(files, config, target):
    entries = [(n, files[n].read_bytes()) for n in NAMES]
    entries.append(('sdlpal.cfg', config.read_bytes()))
    offset = 12 + 32 * len(entries)
    table = bytearray()
    for name, data in entries:
        table += name.encode().ljust(24, b'\0') + struct.pack('<II', offset, len(data))
        offset += len(data)
    if offset > 32 * 1024 * 1024:
        raise ValueError('Game data exceeds the current 32 MiB archive limit.')
    with target.open('wb') as f:
        f.write(b'PALPAK1\0' + struct.pack('<I', len(entries)) + table)
        for _, data in entries:
            f.write(data)


def renderer(port, release, work):
    binary = release / 'tools/render-rix-macos'
    if platform.system() == 'Darwin' and binary.is_file():
        if not os.access(binary, os.X_OK):
            binary.chmod(binary.stat().st_mode | 0o111)
        return binary
    output = work / ('render-rix.exe' if os.name == 'nt' else 'render-rix')
    compiler = shlex.split(os.environ.get('CXX', 'c++'))
    sources = ['tools/render_rix.cpp'] + ['engine/adplug/' + n + '.cpp' for n in ('rix', 'player', 'fprovide', 'binio', 'binfile', 'dosbox_opls')]
    subprocess.run(compiler + ['-std=c++11', '-O2', '-Iengine', '-include', 'tools/rix_compat.h'] + sources + ['-o', str(output)], cwd=port, check=True)
    return output


def prepare(args):
    port = Path(__file__).resolve().parent
    release = port.parents[2]  # release/source/src/pal/prepare.py
    if not (release / 'sdcard').is_dir():
        raise ValueError('Run prepare.py from the extracted public release directory.')
    files = game_files(args.game.resolve())
    output = args.output.resolve()
    if output.exists():
        raise ValueError('Output already exists; choose a new directory (existing saves are never touched).')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.pal-prepare-', dir=output.parent) as tmp:
        work = Path(tmp)
        ready = work / 'ready'
        shutil.copytree(release / 'sdcard', ready)
        assets = ready / 'Assets/pal/common'
        pack(files, port / 'sdlpal.cfg', assets / 'pal.pak')
        print('Rendering DOS FM music (about 686 MiB; this can take a few minutes)...', flush=True)
        exe = renderer(port, release, work)
        subprocess.run([str(exe), str(files['mus.mkf']), str(assets / 'pal-fm.pcm')], check=True)
        for name, (expected, dest) in RUNTIME.items():
            if args.runtime:
                data = (args.runtime / name).read_bytes()
            else:
                url = f'https://raw.githubusercontent.com/openfpgaOS/openfpgaSDK/{SDK_REV}/runtime/{name}'
                print('Fetching upstream runtime: ' + name, flush=True)
                with urllib.request.urlopen(url, timeout=120) as response:
                    data = response.read()
            if hashlib.md5(data).hexdigest() != expected:
                raise ValueError('Runtime manifest mismatch: ' + name)
            (ready / dest).write_bytes(data)
        (ready / 'PAL-INSTALL.json').write_text(json.dumps({
            'version': '0.2.0', 'runtime_sdk_revision': SDK_REV,
            'game_files_sha256': {n: hashlib.sha256(files[n].read_bytes()).hexdigest() for n in NAMES},
            'private_installation': True,
        }, indent=2) + '\n')
        ready.rename(output)
    print('Ready: ' + str(output))
    print('Merge Assets, Cores and Platforms into your SD card. Do not redistribute generated game data.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, required=True, help='Your legally obtained DOS PAL directory (case insensitive filenames)')
    parser.add_argument('--output', type=Path, required=True, help='New private installation directory')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--runtime', type=Path, help='Existing pinned SDK runtime/ directory (offline)')
    group.add_argument('--fetch-runtime', action='store_true', help='Fetch three pinned runtime files directly from upstream')
    args = parser.parse_args()
    try:
        prepare(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as e:
        parser.exit(1, 'Error: ' + str(e) + '\n')


if __name__ == '__main__':
    main()
