"""Build practiceml as a frozen executable with PyInstaller.

Run from the project root (or `sidecar/`) after `pip install pyinstaller`:

    sidecar\\.venv\\Scripts\\python.exe sidecar\\build_sidecar.py

The result is `sidecar/dist/practiceml/practiceml.exe` (one-dir bundle —
single-file mode unpacks ~1.5 GB on every launch which is unacceptable).

The C++ host's `SidecarProcess::getDefaultCommand()` looks for
`practiceml.exe` next to the host binary; copy the entire `dist/practiceml/`
directory into the install staging dir (see `packaging/windows/stage.py`).
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path


def main() -> int:
    here = Path(__file__).parent.resolve()
    dist = here / "dist"
    build = here / "build"
    for p in (dist, build):
        if p.exists():
            shutil.rmtree(p)

    # Locate the basic-pitch saved_models dir so we can bundle the ONNX model.
    import basic_pitch
    bp_root = Path(basic_pitch.__file__).parent
    bp_models = bp_root / "saved_models"
    if not bp_models.exists():
        print(f"FATAL: basic_pitch saved_models not found at {bp_models}", file=sys.stderr)
        return 2

    # Demucs ships YAML/JSON config files inside the package — collect-all
    # ensures hidden-import + data-file resolution for them and for torch/onnx.
    sep = ";"  # PyInstaller uses ; on Windows for --add-data

    cmd = [
        sys.executable, "-m", "PyInstaller",
        # One-dir, not one-file: avoids 5-10 s startup unpack penalty.
        "--name", "practiceml",
        "--noconfirm",
        "--clean",
        "--console",

        # Make the practiceml package importable in the frozen bundle.
        "--collect-all", "practiceml",
        "--paths", str(here),

        # Heavy ML deps — PyInstaller's autodiscovery misses their data files.
        "--collect-all", "torch",
        "--collect-all", "torchaudio",
        "--collect-all", "torchcodec",
        "--collect-all", "demucs",
        "--collect-all", "dora",
        "--collect-all", "openunmix",
        "--collect-all", "julius",
        "--collect-all", "soundfile",
        "--collect-all", "soxr",
        "--collect-all", "librosa",
        "--collect-all", "basic_pitch",
        "--collect-all", "onnxruntime",
        "--collect-all", "numba",
        "--collect-all", "llvmlite",
        "--collect-all", "yt_dlp",
        "--collect-all", "lameenc",

        # Hidden imports that slip past static analysis.
        "--hidden-import", "encodings.utf_8",
        "--hidden-import", "encodings.cp1252",

        # Explicit data files: basic-pitch's saved_models (ONNX + others).
        "--add-data", f"{bp_models}{sep}basic_pitch/saved_models",

        # Disable UPX — slows startup and corrupts some torch DLLs.
        "--noupx",

        str(here / "practiceml_entry.py"),
    ]

    print(">>>", " ".join(cmd), flush=True)
    rc = subprocess.call(cmd, cwd=here)
    if rc != 0:
        return rc

    out = dist / "practiceml" / "practiceml.exe"
    if not out.exists():
        print(f"FATAL: expected output not found: {out}", file=sys.stderr)
        return 3

    size_mb = sum(p.stat().st_size for p in (dist / "practiceml").rglob("*") if p.is_file()) / (1024 * 1024)
    print(f"\nOK — {out} ({size_mb:.0f} MB bundle dir)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
