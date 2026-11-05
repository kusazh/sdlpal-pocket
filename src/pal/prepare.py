#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare a private Pocket installation from DOS PAL or PAL98 resources."""
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

from convert_avi import convert_movies

NAMES = 'abc.mkf ball.mkf data.mkf f.mkf fbp.mkf fire.mkf gop.mkf map.mkf mgo.mkf midi.mkf mus.mkf pat.mkf rgm.mkf rng.mkf sss.mkf voc.mkf m.msg word.dat wor16.asc wor16.fon'.split()
WIN_NAMES = [n for n in NAMES if n not in {'midi.mkf', 'voc.mkf', 'm.msg', 'word.dat', 'wor16.asc', 'wor16.fon'}]
WIN_NAMES += ['sounds.mkf', 'word_cht.txt', 'msg_cht.txt']
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
        if name in files and name in NAMES + WIN_NAMES:
            raise ValueError('Duplicate filename ignoring case: ' + name)
        files[name] = p
    is_windows = 'sounds.mkf' in files and 'voc.mkf' not in files
    names = WIN_NAMES if is_windows else NAMES
    missing = [n for n in names if n not in files]
    if missing:
        raise ValueError('Missing game files: ' + ', '.join(missing))
    for name in names:
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
    if (fbp[offset:offset + 4] != b'YJ_1') != is_windows:
        raise ValueError('Mixed DOS and Windows resources are not supported.')
    return files


def indexed_text(path: Path) -> dict[int, str]:
    """保留 PAL98 UTF-16 文本的原始编号和控制字符。"""
    values = {}
    for line in path.read_text(encoding='utf-16').splitlines():
        if not line or line.startswith(';'):
            continue
        key, separator, value = line.partition(':')
        if not separator or not key.isdecimal() or int(key) in values:
            raise ValueError('Invalid or duplicate text index in ' + path.name)
        if '\0' in value or value.startswith(('[END MESSAGE]', '[CLEAR MESSAGE]')):
            raise ValueError('Unsupported text control line in ' + path.name)
        values[int(key)] = value
    if not values or set(values) != set(range(max(values) + 1)):
        raise ValueError('Non-contiguous text indices in ' + path.name)
    return values


def windows_text(files: dict[str, Path]) -> bytes:
    """根据脚本对话块生成引擎原生的 UTF-8 语言文件，避免丢失繁体字。"""
    words = indexed_text(files['word_cht.txt'])
    messages = indexed_text(files['msg_cht.txt'])
    sss = files['sss.mkf'].read_bytes()
    start, end = struct.unpack_from('<II', sss, 4 * 4)
    if (end - start) % 8:
        raise ValueError('Invalid script table in sss.mkf')
    scripts = list(struct.iter_unpack('<4H', sss[start:end]))
    lines = ['[BEGIN SETTING]', 'FontFlavor=TradChin', '[END SETTING]', '[BEGIN WORDS]']
    lines += [f'{index}={value}' for index, value in words.items() if index]
    lines.append('[END WORDS]')
    blocks = {}
    # 包含每个可能的脚本入口，包括从连续对话块中间跳入的入口。
    for index, (operation, first, _, _) in enumerate(scripts):
        if operation != 0xffff:
            continue
        last = first
        content = []
        cursor = index
        while cursor < len(scripts):
            op, number, _, _ = scripts[cursor]
            if op == 0xffff and (cursor == index or number == last + 1):
                if number not in messages:
                    raise ValueError('Missing message index: ' + str(number))
                content.append(messages[number])
                last = number
            elif op == 0x008e:
                content.append('[CLEAR MESSAGE]')
            else:
                break
            cursor += 1
        key = (first, last)
        if key in blocks and blocks[key] != content:
            raise ValueError('Conflicting dialogue blocks: ' + str(key))
        blocks[key] = content
    # 未被脚本引用的文本也保留；确保引擎的消息编号边界检查有效。
    for index, value in messages.items():
        blocks.setdefault((index, index), [value])
    for (first, last), content in sorted(blocks.items()):
        lines += [f'[BEGIN MESSAGE] {first}', *content, f'[END MESSAGE] {last}']
    return ('\n'.join(lines) + '\n').encode('utf-8')


def pack(files, config, target, with_avi=False):
    is_windows = 'voc.mkf' not in files
    if is_windows:
        entries = [(n, files[n].read_bytes()) for n in WIN_NAMES if n.endswith('.mkf') and n != 'sounds.mkf']
        entries.append(('pal98.txt', windows_text(files)))
        config_data = config.read_bytes() + b'\nMessageFileName=pal98.txt\n'
        if with_avi:
            config_data = config_data.replace(b'EnableAviPlay=0', b'EnableAviPlay=1')
    else:
        entries = [(n, files[n].read_bytes()) for n in NAMES]
        config_data = config.read_bytes()
    entries.append(('sdlpal.cfg', config_data))
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


def windows_layout(ready: Path) -> None:
    """为 PAL98 分配独立实例和物理文件名，保留 DOS 安装。"""
    assets = ready / 'Assets/pal/common'
    aliases = {
        'pal.ini': 'pal98.ini', 'pal.elf': 'pal98.elf',
        'pal.pak': 'pal98.pak', 'pal-fm.pcm': 'pal98-fm.pcm',
        'pal-sfx.mkf': 'pal98-sfx.mkf',
    }
    if (assets / 'pal-avi.pav').is_file():
        aliases['pal-avi.pav'] = 'pal98-avi.pav'
    config = assets / 'pal.ini'
    config.write_text(config.read_text().replace('ELF=pal.elf', 'ELF=pal98.elf'))
    for old, new in aliases.items():
        (assets / old).rename(assets / new)
    path = ready / 'Assets/pal/Local.PAL/PAL.json'
    instance = json.loads(path.read_text())
    for slot in instance['instance']['data_slots']:
        slot['filename'] = aliases.get(slot['filename'], slot['filename'])
        if 10 <= slot['id'] <= 14:
            slot['filename'] = f"pal98-{slot['id'] - 9}.sav"
    (path.with_name('PAL98.json')).write_text(json.dumps(instance, indent=2) + '\n')
    path.unlink()


def prepare(args):
    port = Path(__file__).resolve().parent
    release = port.parents[2]  # release/source/src/pal/prepare.py
    if not (release / 'sdcard').is_dir():
        raise ValueError('Run prepare.py from the extracted public release directory.')
    files = game_files(args.game.resolve())
    is_windows = 'voc.mkf' not in files
    if args.with_avi and not is_windows:
        raise ValueError('--with-avi requires PAL98 resources.')
    if args.with_avi is None:
        args.with_avi = is_windows and all(f'{i}.avi' in files for i in range(1, 7))
        if args.with_avi and not shutil.which('ffmpeg'):
            print('FFmpeg not found; preparing the game without AVI movies.', flush=True)
            args.with_avi = False
    elif args.with_avi:
        missing = [f'{i}.avi' for i in range(1, 7) if f'{i}.avi' not in files]
        if missing:
            raise ValueError('Missing PAL98 videos: ' + ', '.join(missing))
        if not shutil.which('ffmpeg'):
            raise ValueError('AVI conversion requires ffmpeg in PATH.')
    output = args.output.resolve()
    if output.exists():
        raise ValueError('Output already exists; choose a new directory (existing saves are never touched).')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.pal-prepare-', dir=output.parent) as tmp:
        work = Path(tmp)
        ready = work / 'ready'
        shutil.copytree(release / 'sdcard', ready)
        assets = ready / 'Assets/pal/common'
        pack(files, port / 'sdlpal.cfg', assets / 'pal.pak', args.with_avi)
        if is_windows:
            # 音效按需从独立槽读取，资源包仍保持在 32 MiB 内。
            shutil.copyfile(files['sounds.mkf'], assets / 'pal-sfx.mkf')
            instance_path = ready / 'Assets/pal/Local.PAL/PAL.json'
            instance = json.loads(instance_path.read_text())
            slots = instance['instance']['data_slots']
            slots.append({'id': 6, 'filename': 'pal-sfx.mkf'})
            if args.with_avi:
                convert_movies(args.game.resolve(), assets / 'pal-avi.pav')
                slots.append({'id': 7, 'filename': 'pal-avi.pav'})
            instance_path.write_text(json.dumps(instance, indent=2) + '\n')
        print('Resource edition: ' + ('PAL98 (traditional Chinese)' if is_windows else 'DOS'), flush=True)
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
        if is_windows:
            windows_layout(ready)
        (ready / 'PAL-INSTALL.json').write_text(json.dumps({
            'version': '0.4.0', 'runtime_sdk_revision': SDK_REV,
            'edition': 'pal98' if is_windows else 'dos',
            'avi_enabled': args.with_avi,
            'game_files_sha256': {n: hashlib.sha256(files[n].read_bytes()).hexdigest() for n in
                                  ((WIN_NAMES if is_windows else NAMES) + ([f'{i}.avi' for i in range(1, 7)] if args.with_avi else []))},
            'private_installation': True,
        }, indent=2) + '\n')
        ready.rename(output)
    print('Ready: ' + str(output))
    print('Merge Assets, Cores and Platforms into your SD card. Do not redistribute generated game data.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', nargs='?', type=Path, help='Your DOS or Windows PAL game directory')
    parser.add_argument('--game', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--output', type=Path, default=Path('ready'), help='Installation directory (default: ready)')
    movies = parser.add_mutually_exclusive_group()
    movies.add_argument('--with-avi', action='store_true', default=None, help=argparse.SUPPRESS)
    movies.add_argument('--without-avi', action='store_false', dest='with_avi', help='Skip AVI movies')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--runtime', type=Path, help='Existing pinned SDK runtime/ directory (offline)')
    group.add_argument('--fetch-runtime', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.directory and args.game:
        parser.error('Specify the game directory only once.')
    args.game = args.directory or args.game
    if args.game is None:
        parser.error('Provide your DOS or Windows PAL game directory.')
    try:
        prepare(args)
    except (OSError, ValueError, UnicodeError, subprocess.CalledProcessError) as e:
        parser.exit(1, 'Error: ' + str(e) + '\n')


if __name__ == '__main__':
    main()
