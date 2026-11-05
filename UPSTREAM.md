# Vendored dependencies and project history

This repository starts from the source shipped with the 0.2.0 public release.
The earlier local SDK prototype and private game/test data are not imported.

- SDLPAL: https://github.com/sdlpal/sdlpal
  Revision: d4bf69b64d335cd680555af9df90e7ec4f3006a3.
  Application license: GPL-3.0; retain per-file third-party licenses.
  Code lives under src/pal/engine/. Platform artwork, signing keys and unrelated
  executable/platform assets were excluded from this source distribution.
- openfpgaSDK: https://github.com/openfpgaOS/openfpgaSDK
  Revision: a408ddc12aed0dfaa4aa22c06af82f829db77126.
  License: Apache-2.0, with per-file exceptions. See LICENSES/Apache-2.0.txt
  and src/pal/licenses/SDK-NOTICE.txt (original upstream NOTICE).
  Vendored paths: src/sdk/, tools/, scripts/ and runtime/MANIFEST.
  SDK-FILES.txt lists the files needed in corresponding-source bundles.
  The root README, Makefile, AGENTS.md, LICENSE and NOTICE are port-specific;
  they are not replacements for the SDK's original license.
- musl 1.2.5: https://musl.libc.org/releases/musl-1.2.5.tar.gz
  SHA256: a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4.
  SDK ships headers and static link libraries in src/sdk/musl/.
  Original source archive: src/pal/third_party/musl-1.2.5.tar.gz.
  Full copyright and permissions: LICENSES/MUSL-COPYRIGHT.txt.
- Offline FM rendering uses the SDLPAL-vendored AdPlug/libbinio RIX player
  (LGPL-2.1-or-later) and DOSBox integer OPL (GPL-2.0-or-later), under the
  combined application's GPLv3 terms. Original headers are preserved.

Port modifications include APF file slots/save mapping, indexed video and
input integration, offline FM conversion/streaming, resource packaging,
release tooling, native 320x200 presentation and removal of debug overlays.
See src/pal/README.md for the implementation scope.

No game MKF/MSG/WORD/WOR16 data, soundtrack PCM, save fixture, SoundFont,
FPGA bitstream or OS/loader binary is part of this repository. Runtime files
are obtained separately by the end user; see src/pal/RUNTIME.md.
