# SingingPractice

A Windows + macOS desktop app for solo vocal practice. Three tabs:

1. **Practice** — sing along to an instrumental with live pitch on a piano
   roll, MIDI overlay, VST chain on the vocal bus.
2. **Stem Extract** — Demucs separation of any file or YouTube URL.
3. **Vocal → MIDI** — basic-pitch transcription (not yet implemented).

Plus a **Settings** tab for audio device / driver / channel config.

## Tech stack

- **Audio shell**: JUCE 8 (CMake `FetchContent`, tag `8.0.4`), C++20, MSVC.
- **Audio I/O**: ASIO + WASAPI on Windows via `juce::AudioDeviceManager`. ASIO
  SDK is in `third_party/asiosdk/` (Steinberg, not redistributable; CMake
  auto-detects).
- **Pitch detection**: inline YIN-style detector in `Source/Pitch/`. Designed
  so aubio can drop in later without callsite churn. RT-safe.
- **VST hosting**: `juce::AudioPluginFormatManager` (VST3). File-picker only;
  no automatic plugin scanner yet.
- **ML sidecar**: Python in `sidecar/practiceml/`. Spawned as a child process
  over real Win32 pipes (juce::ChildProcess can't write stdin). JSON-RPC 2.0
  line protocol over stdin/stdout. Demucs (CUDA 12.8 / Blackwell), yt-dlp.

## Build

```
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config RelWithDebInfo --target App -- /m /v:minimal
```

Output: `build\App_artefacts\RelWithDebInfo\Singing Practice.exe`.

First configure clones JUCE (~500 MB, ~45 s). Incremental rebuilds are seconds.

## Python sidecar

Venv at `sidecar\.venv\` (CUDA-enabled, ~3 GB on disk):

```
sidecar\.venv\Scripts\python.exe -m pip install ...   # already installed:
# torch 2.11.0+cu128, demucs 4.0.1, yt-dlp 2026.3.17, soundfile, numpy
```

Verify: `sidecar\.venv\Scripts\python.exe -c "import torch; print(torch.cuda.is_available())"` → `True` on the RTX 5080.

The host's `SidecarProcess::getDefaultCommand()` walks up from the exe looking
for `sidecar/` and prefers `.venv/Scripts/python.exe`. No PyInstaller yet.

## Layout

```
CMakeLists.txt               # JUCE FetchContent, ASIO detection
Source/
  Main.cpp                   # JUCEApplication entry
  App/                       # MainWindow, TabsComponent
  Audio/                     # Engine, Recorder, DeviceSettings
  Pitch/                     # PitchDetector (YIN), PitchRingBuffer (SPSC)
  VST/                       # PluginHost, PluginScanCache
  UI/                        # PianoRoll, PracticeTab, StemTab,
                             #   VocalToMidiTab (stub), SettingsTab,
                             #   PluginChainView, PluginEditorWindow
  Sidecar/                   # SidecarProcess (Win32 pipes),
                             #   JsonRpcClient, Backend
sidecar/
  practiceml/                # Python package
    __main__.py              # JSON-RPC dispatcher (threaded), preloads heavy
                             #   modules on main thread to avoid the Windows
                             #   daemon-thread + stdin GIL deadlock
    rpc.py                   # thread-safe stdout, progress, cancellation
    demucs_runner.py         # demucs 4 low-level API (get_model + apply_model)
    youtube.py               # yt-dlp + ffmpeg postprocessor → WAV
  .venv/                     # CUDA torch + demucs + yt-dlp
third_party/
  asiosdk/                   # Steinberg ASIO SDK 2.3.3 (fetched manually)
```

## Critical architectural rules

- **Audio thread**: no allocations, no locks (use `ScopedTryLock`), no logging.
  `Engine::audioDeviceIOCallback` and `PitchDetector::process` follow this.
  `PluginHost::processBlock` uses try-lock — falls through to dry on edits.
- **Clock alignment**: `Engine::samplesProcessed` AND
  `PitchDetector::samplesProcessed` are both reset in
  `audioDeviceAboutToStart`. They must share an origin or pitch samples render
  off-screen (this was a real bug). Playhead reads engine wall clock; MIDI is
  offset by `engineTime − transport.position` so it stays anchored to the
  instrumental.
- **Sidecar safety**: `SidecarProcess` is `juce::WeakReference`-friendly;
  reader-thread callbacks check the weak ref before dispatching.
- **Sidecar threading**: dispatcher is single-stdin; heavy modules MUST be
  imported on the main thread before the stdin loop starts (Python on Windows
  blocks the GIL during stdin reads, daemon threads can't import torch).

## Slice status (most recent → oldest)

| Slice | Status |
|---|---|
| Stem Extraction (Demucs + yt-dlp + UI) | DONE — error+cancel paths verified end-to-end; GUI-side separation not yet manually tested |
| Sidecar transport (real Win32 pipes, RPC, Backend) | DONE |
| VST hosting (chain UI, editor windows, mic → chain → monitor) | DONE — no scanner UI, no drag-reorder, no wet recording |
| Mouse-scrollable piano roll + clock-alignment fix | DONE |
| Multi-channel input selector + ASIO mono channel listing | DONE |
| ASIO support (CMake detect + SDK fetched) | DONE |
| Practice tab MVP (audio engine, pitch, piano roll, MIDI, transport, recording) | DONE |
| Project scaffold + first successful build | DONE |

## Open follow-ups (not yet started)

- **Wet recording** — currently captures pre-VST dry vocal only. Add post-chain
  capture from the existing stereo scratch buffer.
- **Plugin scanner UI** — popup browser populated from `KnownPluginList` /
  `PluginScanCache`, with rescan option.
- **Drag-to-reorder** in the plugin chain strip.
- **"Open instrumental in Practice tab"** in the Stem tab — currently just
  reveals the file in Explorer; needs a cross-tab callback wire.
- **Vocal→MIDI tab** with basic-pitch (~500 MB tensorflow install).
- **PyInstaller build** for the sidecar (`sidecar/build_sidecar.py` exists but
  hasn't been run).
- **macOS port** — code has `#if JUCE_WINDOWS` gates; `SidecarProcess` macOS
  bits are stub.
- **Session save/load** (loaded MIDI, instrumental, plugin chain).

## Things that look weird but are intentional

- `JsonRpcClient::call` returns the id AFTER constructing the lambda. Backend
  uses `std::make_shared<int>` so the lambda can capture a cell that's filled
  in after `call` returns. Don't "fix" by adding a peek-next-id method.
- `__main__.py` does a PEP 366 dance setting `__package__` so it works both
  as `python __main__.py` and `python -m practiceml`.
- The `JUCE_DISPLAY_SPLASH_SCREEN=0` define was removed because JUCE 8 ignores
  it and warns; the splash isn't there at all.
