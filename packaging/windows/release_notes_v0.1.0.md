First public build of **SingingPracticeTool** — a Windows desktop app for solo
vocal practice: sing along to an instrumental with live pitch on a piano roll,
run your mic through a VST chain, extract stems from any file or YouTube URL,
and transcribe vocal takes to MIDI.

## Install

1. Download `SingingPracticeTool-Setup-0.1.0.exe` below.
2. Run it. Per-user install, no admin prompt. Installs to
   `%LocalAppData%\Programs\SingingPracticeTool\`.
3. Launch from the Start menu (or the optional desktop shortcut).

> The installer is **unsigned** — Windows SmartScreen will show "Microsoft
> Defender prevented an unrecognized app from starting" on first run. Click
> **More info** → **Run anyway**. Code signing is on the roadmap.

The installer is ~1.9 GB and unpacks to ~4.5 GB on disk. The bulk is PyTorch
+ cuDNN inside the bundled Python sidecar.

## What's in this build

- **Practice tab** — instrumental playback, low-latency mic monitoring through
  a VST3 plugin chain, scrolling piano roll with live pitch trace, MIDI overlay,
  click/drag/wheel timeline scrub.
- **Stem Extract tab** — Demucs 4 separation (`htdemucs` / `htdemucs_ft` /
  `mdx_extra`); accepts files or YouTube URLs (single video only — playlists
  are stripped). Runs on CUDA or CPU; selectable in the UI.
- **Vocal → MIDI tab** — Spotify's basic-pitch (ICASSP 2022 model, ONNX
  backend) transcribes a vocal take to `.mid` with adjustable onset / frame /
  min-note-length thresholds.
- **Settings tab** — audio device, driver (ASIO + WASAPI), sample rate, buffer
  size, channel routing. Persists across launches.

## First-run paths

- Device settings: `%APPDATA%\SingingPracticeTool\device_settings.xml`
- Recordings: `~\Music\SingingPracticeTool\vocal_dry_<timestamp>.wav`
- Stems: `~\Music\SingingPracticeTool\Stems\`
- MIDI: `~\Music\SingingPracticeTool\MIDI\`

## Requirements

- Windows 10 / 11 x64
- Microsoft Visual C++ 2015–2022 Redistributable (the installer flags you if
  it's missing; download from <https://aka.ms/vs/17/release/vc_redist.x64.exe>)
- ASIO drivers for low-latency monitoring (interface-specific; falls back to
  WASAPI if none)
- Optional: NVIDIA GPU + recent driver for CUDA-accelerated Demucs / basic-
  pitch. CPU fallback is automatic.

## Known limitations

- Unsigned installer (SmartScreen warning on first run).
- macOS not yet supported (`SidecarProcess` macOS branch is stubbed).
- No wet recording yet — `Record` captures the dry mic, not the post-VST bus.
- No plugin scanner UI — VST3s are loaded via file picker.
- No drag-to-reorder for the plugin chain.
- No session save/load.

## Uninstall

Add/Remove Programs → "SingingPracticeTool" → Uninstall. Removes the install
dir and `%APPDATA%\SingingPracticeTool\device_settings.xml`. **Leaves your
recordings / stems / MIDI** at `~\Music\SingingPracticeTool\` alone.

---

Built from source at <https://github.com/VanKyle00/SingingPracticeTool>
(commit `15c0c78`). GPLv3 — see `LICENSE` in the install dir or the repo.
