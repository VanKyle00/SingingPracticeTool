Patch release — **YouTube stem extraction now works out of the box.**

## The fix

v0.1.0 failed any YouTube download with `ERROR: Postprocessing: audio
conversion failed: ffmpeg not found`. The sidecar relied on a system `ffmpeg`
being on `PATH`, and the installer only bundled a tiny PATH **shim** (≈383 KB),
not a real binary — placed in a directory the downloader never searched.

v0.1.1 makes the app fully self-contained:

- **Bundles a real static ffmpeg + ffprobe** (217 MB each) next to
  `practiceml.exe`, where the sidecar resolves them first — zero dependency on
  a system ffmpeg or `PATH`.
- The sidecar now passes an explicit `ffmpeg_location` to yt-dlp and
  size-gates any bundled binary, so a shim can never shadow a working ffmpeg.
- The release packaging (`stage.py`) now refuses to build with a shim-sized or
  missing ffmpeg, so a broken installer can't ship again.

No other functional changes — Practice, Stem Extract, Vocal→MIDI, and Settings
are identical to v0.1.0.

## Install

1. Download `SingingPracticeTool-Setup-0.1.1.exe` below.
2. Run it. Per-user install, no admin prompt. Installs to
   `%LocalAppData%\Programs\SingingPracticeTool\`.
3. Launch from the Start menu (or the optional desktop shortcut).

> The installer is **unsigned** — Windows SmartScreen will show "Microsoft
> Defender prevented an unrecognized app from starting" on first run. Click
> **More info** → **Run anyway**. Code signing is on the roadmap.

The installer unpacks to ~5 GB on disk. The bulk is PyTorch + cuDNN inside the
bundled Python sidecar; ffmpeg/ffprobe add ~430 MB.

## Requirements

- Windows 10 / 11 x64
- Microsoft Visual C++ 2015–2022 Redistributable (the installer flags you if
  it's missing; download from <https://aka.ms/vs/17/release/vc_redist.x64.exe>)
- Optional: NVIDIA GPU + recent driver for CUDA-accelerated Demucs / basic-
  pitch. CPU fallback is automatic.

## Known limitations

- Unsigned installer (SmartScreen warning on first run).
- macOS not yet supported (`SidecarProcess` macOS branch is stubbed).
- No wet recording, plugin scanner UI, chain reordering, or session save/load yet.

---

Built from source at <https://github.com/VanKyle00/SingingPracticeTool>.
GPLv3 — see `LICENSE` in the install dir or the repo.
