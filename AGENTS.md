# SDLPAL for Analogue Pocket

This is the active repository for this port. Use this checkout for future
PAL port work, rather than the earlier ../pocket-pal SDK prototype.

## Ownership and dependencies

- Port code and modified SDLPAL engine live in src/pal/.
- Treat src/sdk/, tools/, and scripts/ as vendored openfpgaSDK dependencies.
  Make game changes in src/pal/; preserve SDK APIs and hardware capability checks.
- SDK provenance and redistribution scope are documented in UPSTREAM.md.
- Application license: GPLv3; SDK: Apache-2.0; retain third-party notices.

## Build and checks

- `make build`: RISC-V ELF, Docker/OrbStack, no game data required.
- `make release`: resource-free ZIP with corresponding source and licenses.
- `make test GAME_DIR=/absolute/path/to/PAL`: desktop integration check using
  private DOS data. Requires host SDL2; see src/pal/README.md.
- `make renderer`: offline FM conversion tool, host C++11 only.
- Release layout and user setup: src/pal/RELEASE.md.

Never commit/distribute game MKF/MSG/DAT files, WOR16 fonts, pal.pak,
pal-fm.pcm, saves, screenshots, test fixtures, generated installation folders,
SoundFonts, or FPGA/OS runtime binaries. Do not import the old prototype's
build/ directory. build/, .obj/ and generated port artifacts are ignored.
Keep publication scripts independent of private game data. Do not restore FPS
overlays or debug hotkeys unless the user requests them.

For actual Pocket changes, retain the desktop path and test relevant behavior.
A desktop check does not confirm Pocket timing/audio/hardware persistence.
