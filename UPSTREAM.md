# Vendored dependencies and project history

This repository contains the port and its vendored build dependencies.
Private game and test data are not included. Exact dependency revisions and
checksums are recorded in the release metadata, packaging script and NOTICE.

- SDLPAL: https://github.com/sdlpal/sdlpal
  Application license: GPL-3.0; retain per-file third-party licenses.
  Code lives under src/pal/engine/. Platform artwork, signing keys and unrelated
  executable/platform assets were excluded from this source distribution.
- openfpgaSDK: https://github.com/openfpgaOS/openfpgaSDK
  License: Apache-2.0, with per-file exceptions. See LICENSES/Apache-2.0.txt
  and src/pal/licenses/SDK-NOTICE.txt (original upstream NOTICE).
  Vendored paths: src/sdk/, tools/, scripts/ and runtime/MANIFEST.
  SDK-FILES.txt lists the files needed in corresponding-source bundles.
  The root README, Makefile, AGENTS.md, LICENSE and NOTICE are port-specific;
  they are not replacements for the SDK's original license.
- musl: https://musl.libc.org/
  SDK ships headers and static link libraries in src/sdk/musl/.
  Original source archive is included under src/pal/third_party/.
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
