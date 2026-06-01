"""Stage the Windows installer payload.

Run from the project root after a Release build of the host and a successful
`python sidecar/build_sidecar.py`:

    py packaging\\windows\\stage.py

Layout produced under `packaging/windows/stage/`:

    SingingPracticeTool.exe         (the JUCE host)
    practiceml/                  (PyInstaller bundle dir — practiceml.exe + all deps)
        practiceml.exe
        ffmpeg.exe               (real static build, next to practiceml.exe)
        ffprobe.exe              (real static build, next to practiceml.exe)
        ...
    LICENSE
    README.md

ffmpeg.exe/ffprobe.exe live INSIDE practiceml/ (next to practiceml.exe) because
that is the first location the sidecar's youtube.py::_find_ffmpeg() checks in a
frozen build (Path(sys.executable).parent). The JUCE host never uses ffmpeg, so
no copy is placed in the install root.

The Inno Setup script `installer.iss` reads from this dir.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

# A real static ffmpeg/ffprobe binary is tens of MB; WinGet app-execution-alias
# shims are <1 MB. Anything below this threshold is treated as a shim and
# rejected so a broken (non-functional) binary can never be shipped.
MIN_FFMPEG_BYTES = 20_971_520  # 20 MiB
# Env var pointing at a static ffmpeg 'bin' dir (e.g. the gyan.dev full build).
FFMPEG_DIR_ENV = "FFMPEG_DIR"


def find_in_path(name: str):
    for p in os.environ.get("PATH", "").split(os.pathsep):
        candidate = Path(p) / name
        if candidate.is_file():
            return candidate
    return None


def resolve_ffmpeg_bin_dir():
    """Resolve the source 'bin' dir holding real ffmpeg.exe/ffprobe.exe.

    Precedence:
      1. $FFMPEG_DIR (explicit, deterministic — preferred for releases).
      2. parent dir of the first ffmpeg.exe found on PATH.
    Returns (dir, source_label) or (None, reason) if nothing usable is found.
    """
    env_dir = os.environ.get(FFMPEG_DIR_ENV)
    if env_dir:
        return Path(env_dir), f"${FFMPEG_DIR_ENV}={env_dir}"
    ff = find_in_path("ffmpeg.exe")
    if ff is not None:
        return ff.parent, f"PATH ({ff.parent})"
    return None, "no source dir (set $FFMPEG_DIR or put ffmpeg.exe on PATH)"


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

    # 3. ffmpeg.exe + ffprobe.exe — real static builds, placed INSIDE
    #    practiceml/ next to practiceml.exe (the first dir youtube.py probes
    #    in a frozen build). A shim-sized binary is fatal: it would shadow the
    #    sidecar's PATH/winget fallback at runtime and guarantee breakage.
    src_dir, label = resolve_ffmpeg_bin_dir()
    if src_dir is None:
        print(f"FATAL: cannot locate a real ffmpeg — {label}.", file=sys.stderr)
        print(
            f"       Set {FFMPEG_DIR_ENV} to a static ffmpeg 'bin' dir "
            "(e.g. the gyan.dev full build) and re-run.",
            file=sys.stderr,
        )
        return 4
    target = stage / "practiceml"  # next to practiceml.exe (copied in step 2)
    for name in ("ffmpeg.exe", "ffprobe.exe"):
        src = src_dir / name
        if not src.is_file():
            print(f"FATAL: {name} not found in {src_dir} (resolved via {label}).", file=sys.stderr)
            print(
                f"       Set {FFMPEG_DIR_ENV} to a static ffmpeg 'bin' dir "
                "containing both ffmpeg.exe and ffprobe.exe (e.g. the gyan.dev full build).",
                file=sys.stderr,
            )
            return 4
        size = src.stat().st_size
        if size < MIN_FFMPEG_BYTES:
            print(
                f"FATAL: {name} at {src} is only {size / 1024 / 1024:.2f} MB "
                f"(< {MIN_FFMPEG_BYTES / 1024 / 1024:.0f} MB) — looks like a shim, not a real binary.",
                file=sys.stderr,
            )
            print(
                f"       Set {FFMPEG_DIR_ENV} to a real static ffmpeg 'bin' dir "
                "(e.g. the gyan.dev full build) and re-run.",
                file=sys.stderr,
            )
            return 4
        shutil.copy2(src, target / name)
        print(f"  + practiceml/{name}   ({size / 1024 / 1024:.0f} MB, from {label})")

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
