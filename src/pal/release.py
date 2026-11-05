#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Assemble a program-only release using explicit source/artifact lists."""
from pathlib import Path
import hashlib
import json
import platform
import shutil
import subprocess
import tempfile
import zipfile

PORT = Path(__file__).resolve().parent
ROOT = PORT.parents[1]
VERSION = '0.2.0'
PORT_FILES = '''Makefile README.md RELEASE.md RUNTIME.md NOTICE build.sh pack.rb package.rb prepare.py release.py pal_config.h platform_io.c platform_io.h platform_audio.c platform_avi.c platform_smoke.c platform_video.c platform_video.h sdlpal.cfg smoke.sh tools/render_rix.cpp tools/rix_compat.h licenses/SDK-NOTICE.txt licenses/MUSL-COPYRIGHT.txt third_party/musl-1.2.5.tar.gz'''.split()


def copy(src, dst):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def main():
    parent = ROOT / 'build/releases'
    parent.mkdir(parents=True, exist_ok=True)
    name = 'sdlpal-pocket-' + VERSION
    final = parent / name
    with tempfile.TemporaryDirectory(prefix='.pal-release-', dir=parent) as tmp:
        out = Path(tmp) / name
        out.mkdir()
        subprocess.run(['ruby', str(PORT / 'package.rb'), '--public', '--output', str(out / 'sdcard')], check=True)
        source = out / 'source'
        # SDK build dependencies only; no runtime, dist or example apps/media.
        tracked = (ROOT / 'SDK-FILES.txt').read_text().splitlines()
        source.mkdir(parents=True, exist_ok=True)
        (source / 'SDK-FILES.txt').write_text('\n'.join(filter(None, tracked)) + '\n')
        for rel in filter(None, tracked):
            copy(ROOT / rel, source / rel)
        for rel in ['.gitignore', 'LICENSE', 'NOTICE', 'README.md', 'Makefile', 'AGENTS.md', 'UPSTREAM.md']:
            copy(ROOT / rel, source / rel)
        copy(ROOT / 'runtime/MANIFEST', source / 'runtime/MANIFEST')
        for rel in PORT_FILES:
            copy(PORT / rel, source / 'src/pal' / rel)
        # Engine code and original licensing only. Excludes logos, icons,
        # platform artwork, signing keys, executables and game/test data.
        suffixes = {'.c', '.h', '.cpp', '.s', '.dat', '.mk', '.txt', '.md'}
        for p in (PORT / 'engine').rglob('*'):
            if p.is_file() and (p.suffix.lower() in suffixes or p.name in {'LICENSE', 'COPYING', 'Makefile'}):
                copy(p, source / 'src/pal/engine' / p.relative_to(PORT / 'engine'))
        copy(PORT / 'RELEASE.md', out / 'README.md')
        copy(PORT / 'RUNTIME.md', out / 'RUNTIME.md')
        copy(PORT / 'NOTICE', out / 'NOTICE')
        for src, name_ in [(PORT/'engine/LICENSE', 'GPL-3.0.txt'), (PORT/'engine/adplug/NOTES/COPYING', 'LGPL-2.1.txt'), (ROOT/'LICENSES/Apache-2.0.txt', 'Apache-2.0.txt'), (ROOT/'LICENSES/MIT.txt', 'MIT.txt')]:
            copy(src, out / 'LICENSES' / name_)
        copy(PORT / 'licenses/MUSL-COPYRIGHT.txt', out / 'LICENSES/MUSL-COPYRIGHT.txt')
        assert hashlib.sha256((PORT / 'third_party/musl-1.2.5.tar.gz').read_bytes()).hexdigest() == 'a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4'
        font_notices = []
        for name_ in ['fontglyph.h', 'fontglyph_cn.h', 'fontglyph_tw.h', 'fontglyph_jp.h']:
            text = (PORT / 'engine' / name_).read_text()
            end = text.index('#ifndef')
            font_notices.append(name_ + '\n' + text[:end])
        (out / 'LICENSES/FONT-NOTICES.txt').write_text('\n\n'.join(font_notices))
        (out / 'prepare.py').write_text('''#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).resolve().parent / "source/src/pal/prepare.py"), run_name="__main__")
''')
        if platform.system() == 'Darwin':
            (out / 'tools').mkdir()
            sources = ['tools/render_rix.cpp'] + ['engine/adplug/' + n + '.cpp' for n in ('rix', 'player', 'fprovide', 'binio', 'binfile', 'dosbox_opls')]
            subprocess.run(['c++', '-std=c++11', '-O2', '-arch', 'arm64', '-arch', 'x86_64', '-mmacosx-version-min=11.0', '-Iengine', '-include', 'tools/rix_compat.h', *sources, '-o', str(out / 'tools/render-rix-macos')], cwd=PORT, check=True)
        manifest = {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file()}
        forbidden = {'.pak', '.pcm', '.sav', '.rpg', '.mkf', '.ofsf', '.png', '.bmp', '.rbf_r', '.wav', '.pfx', '.bin'}
        for rel in manifest:
            assert Path(rel).suffix.lower() not in forbidden, 'Forbidden release asset: ' + rel
        (out / 'SHA256SUMS').write_text(''.join(f'{digest}  {rel}\n' for rel, digest in manifest.items()))
        (out / 'release.json').write_text(json.dumps({'version': VERSION, 'sdk_revision': 'a408ddc12aed0dfaa4aa22c06af82f829db77126', 'sdlpal_revision': 'd4bf69b64d335cd680555af9df90e7ec4f3006a3', 'game_data_included': False, 'runtime_binaries_included': False, 'application_sha256': manifest['sdcard/Assets/pal/common/pal.elf']}, indent=2) + '\n')
        if final.exists():
            shutil.rmtree(final)
        out.rename(final)
    archive = parent / (name + '.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for p in sorted(final.rglob('*')):
            if p.is_file():
                z.write(p, p.relative_to(parent))
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_suffix('.zip.sha256').write_text(digest + '  ' + archive.name + '\n')
    print(f'Release: {archive} ({archive.stat().st_size:,} bytes)\nSHA256: {digest}')


if __name__ == '__main__':
    main()
