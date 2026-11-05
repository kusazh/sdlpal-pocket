#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Convert the user's six PAL98 AVIs to a private Pocket movie stream."""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

WIDTH, HEIGHT, FPS, RATE = 288, 180, 15, 44100
SAMPLES = RATE // FPS
PIXELS = WIDTH * HEIGHT
HEADER = struct.Struct('<8s5I')
ENTRY = struct.Struct('<4I')


def convert_movies(game: Path, output: Path) -> None:
    """用每片固定调色板和交错 PCM 降低 Pocket 的 CPU 与内存需求。"""
    ffmpeg = shutil.which('ffmpeg')
    if not ffmpeg:
        raise ValueError('AVI conversion requires ffmpeg in PATH.')
    files = {}
    for path in game.iterdir():
        name = path.name.lower()
        if path.is_file() and name in {f'{i}.avi' for i in range(1, 7)}:
            if name in files:
                raise ValueError('Duplicate AVI filename: ' + name)
            files[name] = path.resolve()
    missing = [f'{i}.avi' for i in range(1, 7) if f'{i}.avi' not in files]
    if missing:
        raise ValueError('Missing PAL98 videos: ' + ', '.join(missing))
    if output.exists():
        raise ValueError('Movie output already exists: ' + str(output))
    output.parent.mkdir(parents=True, exist_ok=True)
    command = [ffmpeg, '-nostdin', '-v', 'error', '-y', '-threads', '1',
               '-filter_threads', '1', '-filter_complex_threads', '1']
    with tempfile.TemporaryDirectory(prefix='.pal-avi-', dir=output.parent) as directory:
        work = Path(directory)
        target = work / 'movies.pav'
        entries = []
        with target.open('wb') as stream:
            stream.write(HEADER.pack(b'PALAVI1\0', WIDTH, HEIGHT, FPS, RATE, 6))
            stream.write(bytes(6 * ENTRY.size))
            for number in range(1, 7):
                source = files[f'{number}.avi']
                palette = work / 'palette.png'
                audio = work / 'audio.s16'
                subprocess.run(command + ['-i', str(source), '-an', '-vf',
                    'scale=288:180,fps=15,palettegen=reserve_transparent=1',
                    '-frames:v', '1', '-update', '1', str(palette)], check=True)
                subprocess.run(command + ['-i', str(source), '-vn', '-map', '0:a:0',
                    '-af', 'aresample=async=1:first_pts=0', '-ac', '1', '-ar', str(RATE),
                    '-c:a', 'pcm_s16le', '-f', 's16le', str(audio)], check=True)
                offset, frames = stream.tell(), 0
                with audio.open('rb') as pcm, (work / 'ffmpeg.log').open('w+') as log:
                    decoder = subprocess.Popen(command + ['-i', str(source), '-i', str(palette),
                        '-filter_complex', '[0:v]scale=288:180,fps=15[v];[v][1:v]paletteuse=dither=bayer:bayer_scale=3',
                        '-an', '-pix_fmt', 'pal8', '-f', 'rawvideo', 'pipe:1'],
                        stdout=subprocess.PIPE, stderr=log)
                    try:
                        while True:
                            frame = decoder.stdout.read(PIXELS + 1024)
                            if not frame:
                                break
                            if len(frame) != PIXELS + 1024 or 255 in frame[:PIXELS]:
                                raise ValueError('Invalid or transparent PAL98 video frame.')
                            # FFmpeg pal8 的尾部调色板为 BGRA；255 留给纯黑边框。
                            colors = frame[PIXELS:]
                            stream.write(bytes(value if i < 255 else 0 for i in range(256)
                                               for value in (colors[i*4+2], colors[i*4+1], colors[i*4])))
                            stream.write(frame[:PIXELS])
                            stream.write(pcm.read(SAMPLES * 2).ljust(SAMPLES * 2, b'\0'))
                            frames += 1
                        decoder.wait()
                        if decoder.returncode:
                            log.seek(0)
                            raise ValueError('FFmpeg video conversion failed: ' + log.read())
                    finally:
                        decoder.stdout.close()
                        if decoder.poll() is None:
                            decoder.kill()
                            decoder.wait()
                if not frames or stream.tell() > 0x7fffffff:
                    raise ValueError('Empty video or movie archive exceeds 2 GiB.')
                entries.append((number, offset, frames, frames * SAMPLES))
                print(f'AVI {number}: {frames} frames, {frames / FPS:.2f}s', flush=True)
            stream.seek(HEADER.size)
            for entry in entries:
                stream.write(ENTRY.pack(*entry))
        target.rename(output)
    print('Movies: ' + str(output), flush=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        convert_movies(args.game, args.output)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, 'Error: ' + str(error) + '\n')


if __name__ == '__main__':
    main()
