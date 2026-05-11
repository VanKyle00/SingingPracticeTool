"""Stage the Windows installer payload.

Run from the project root after a Release build of the host and a successful
`python sidecar/build_sidecar.py`:

    py packaging\\windows\\stage.py

Layout produced under `packaging/windows/stage/`:

    SingingPracticeTool.exe         (the JUCE host)
    practiceml/                  (PyInstaller bundle dir — practiceml.exe + all deps)
        practiceml.exe
        ...
    ffmpeg.exe                   (optional; placed if present in PATH)
    LICENSE
    README.md

The Inno Setup script `installer.iss` reads from this dir.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path


def find_in_path(name: str):
    for p in os.environ.get("PATH", "").split(os.pathsep):
        candidate = Path(p) / name
        if candidate.is_file():
            return candidate
    return None


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    stage = root / "packaging" / "windows" / "stage"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)

    # 1. Host exe (built via cmake --target App --config Release).
    host_release = root / "build" / "App_artefacts" / "Release" / "SingingPracticeTool.exe"
    host_relwd   = root / "build" / "App_artefacts" / "RelWithDebInfo" / "SingingPracticeTool.exe"
    host = host_release if host_release.exists() else host_relwd
    if not host.exists():
        print(f"FATAL: build the host first ({host_release} or {host_relwd} missing)", file=sys.stderr)
        return 2
    shutil.copy2(host, stage / "SingingPracticeTool.exe")
    print(f"  + {host.name}  ({host.stat().st_size / 1024 / 1024:.0f} MB)")

    # 2. Sidecar bundle dir (PyInstaller one-dir output).
    bundle = root / "sidecar" / "dist" / "practiceml"
    if not (bundle / "practiceml.exe").exists():
        print(f"FATAL: run sidecar/build_sidecar.py first (missing {bundle / 'practiceml.exe'})", file=sys.stderr)
        return 3
    shutil.copytree(bundle, stage / "practiceml")
    n_files = sum(1 for _ in (stage / "practiceml").rglob("*"))
    bundle_mb = sum(p.stat().st_size for p in (stage / "practiceml").rglob("*") if p.is_file()) / 1024 / 1024
    print(f"  + practiceml/   ({n_files} files, {bundle_mb:.0f} MB)")

    # 3. ffmpeg.exe — bundled if found on PATH; otherwise warn.
    ff = find_in_path("ffmpeg.exe")
    if ff is not None:
        shutil.copy2(ff, stage / "ffmpeg.exe")
        print(f"  + ffmpeg.exe   (from {ff})")
    else:
        print("  ! ffmpeg.exe not found on PATH — YouTube download will fail at runtime.")
        print("    Drop a static ffmpeg.exe into packaging/windows/stage/ before running iscc.")

    # 4. Top-level docs.
    for name in ("LICENSE", "README.md"):
        src = root / name
        if src.exists():
            shutil.copy2(src, stage / name)
            print(f"  + {name}")

    total = sum(p.stat().st_size for p in stage.rglob("*") if p.is_file()) / 1024 / 1024
    print(f"\nStaged at {stage}  (~{total:.0f} MB total)")
    print("Next: iscc packaging\\windows\\installer.iss")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
