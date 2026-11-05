# PAL98 movie streams

`convert_avi.py --game /path/to/PAL98 --output /new/path/pal98-avi.pav`
uses an installed FFmpeg executable to convert the user's six AVIs. It does
not download media, modify the originals or include game media in a release.
The public installer automatically converts movies when all six AVIs and
FFmpeg are available, then enables AVI in the
packed game config only for that installation.

Video uses a fixed palette per clip, ordered Bayer dithering and the original
288×180 / 15 fps presentation. Index 255 is reserved for black borders.
Audio is resampled to signed little-endian 16-bit, 44,100 Hz mono, with silence
padding to the last video frame; stereo input is downmixed. This is a color
and audio-channel conversion, not a lossless copy of the original video.

## PALAVI1 layout

All integer fields are unsigned little-endian 32-bit. The current format is
deliberately fixed, with no runtime decompression and a 2 GiB file limit.

- Header: 8-byte `PALAVI1\0`, width (288), height (180), fps (15), audio
  rate (44100), clip count (6). Total 28 bytes.
- Six 16-byte entries: clip id (1–6), byte offset, frame count, audio sample
  count. Entries are ordered, non-overlapping, and entirely within the file.
  Sample count must equal frame count × 2940. First payload starts at 124.
- Each frame record: 768-byte RGB palette, 51,840 indexed pixels in top-down
  row order, then 2,940 mono PCM samples (5,880 bytes). Total 58,488 bytes.

The player reads only the requested clip. Its 32-frame queue occupies
1,683,456 bytes and its 131,072-sample audio ring occupies 262,144 bytes;
it also saves 64,000 game pixels and a palette for restoration. Video is
centered at (16,10) in the native 320×200 game surface. The final
320×240 display centers that surface without scaling, so movie borders
are 30 rows at both top and bottom (16 columns at left and right). Audio playback position
drives the frame index; late frames are dropped without stretching audio.
SD access never occurs in the audio callback. Starvation or a stalled clock
aborts playback and returns control to the engine fallback.

APF slot 7 binds the virtual `pal-avi.pav` name to the instance's physical
`pal98-avi.pav` file. This is independent of DOS resources and saves.

## Verification

Build `make -C src/pal -o pal.pak -o pal-fm.pcm desktop`. In a new ignored
working directory, bind the prepared files to their desktop virtual names:
`pal.pak`, `pal-fm.pcm`, `pal-sfx.mkf`, and `pal-avi.pav`. Then run the absolute
path to `src/pal/pal-desktop` with:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /path/to/pal-desktop --avi-smoke-test
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /path/to/pal-desktop --avi-realtime-test
```

The first command runs all six clips with the actual desktop audio callback
advancing the clock, reporting hashes of displayed frame data and emitted
PCM for comparison against the converter output. It also verifies full-window
pixels and black borders at representative frames, skip, muted playback,
frame dropping, disabled/unsupported playback, audio cleanup, and restoration
of the game pixels and palette. The second runs clip 1 in wall-clock time
through SDL's audio device. Neither option is compiled as a Pocket debug key.
The ordinary `--smoke-test` remains the scene, FM, WAV and save regression.

Desktop verification does not establish Pocket SD throughput, real mixer
timing, long-term audio synchronization, or hardware input responsiveness.
