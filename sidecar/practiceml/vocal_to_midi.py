"""Vocal -> MIDI handler using basic-pitch (ONNX backend).

The model accepts any audio file ffmpeg/librosa can decode (WAV, MP3, FLAC, ...).
Output is a `.mid` file alongside the (optional) output directory.
"""
import contextlib
import sys
from pathlib import Path
from typing import Any, Dict

from .rpc import Job, Cancelled, send_progress, log


@contextlib.contextmanager
def _stdout_to_stderr():
    """basic_pitch.predict prints status to stdout, which would corrupt the JSON-RPC
    stream the host parses. Redirect it to stderr (free-form log channel) for the
    duration of inference."""
    saved = sys.stdout
    sys.stdout = sys.stderr
    try:
        yield
    finally:
        sys.stdout = saved


_MODEL = None  # Cached basic-pitch model; ONNX session is ~300-800 ms to create.


def _get_model():
    global _MODEL
    if _MODEL is None:
        from basic_pitch.inference import Model
        from basic_pitch import ICASSP_2022_MODEL_PATH
        _MODEL = Model(ICASSP_2022_MODEL_PATH)
    return _MODEL


def vocal_to_midi(params: Dict[str, Any], job: Job) -> Dict[str, Any]:
    from basic_pitch.inference import predict

    input_path = Path(params["input"]).expanduser().resolve()
    output_dir = Path(params.get("output") or input_path.parent).expanduser().resolve()
    onset_threshold = float(params.get("onset_threshold", 0.5))
    frame_threshold = float(params.get("frame_threshold", 0.3))
    minimum_note_length = float(params.get("minimum_note_length_ms", 127.70))
    min_freq = params.get("minimum_frequency")  # Hz or None
    max_freq = params.get("maximum_frequency")
    midi_tempo = float(params.get("midi_tempo", 120.0))

    if not input_path.exists():
        raise FileNotFoundError(f"input not found: {input_path}")
    output_dir.mkdir(parents=True, exist_ok=True)

    log(f"vocal_to_midi start: input={input_path.name} onset={onset_threshold} "
        f"frame={frame_threshold} min_ms={minimum_note_length}")

    send_progress(job.rpc_id, 0.05, "loading model")
    model = _get_model()

    job.check_cancel()
    send_progress(job.rpc_id, 0.20, "running inference")

    with _stdout_to_stderr():
        _model_output, midi_data, note_events = predict(
            str(input_path),
            model_or_model_path=model,
            onset_threshold=onset_threshold,
            frame_threshold=frame_threshold,
            minimum_note_length=minimum_note_length,
            minimum_frequency=float(min_freq) if min_freq is not None else None,
            maximum_frequency=float(max_freq) if max_freq is not None else None,
            midi_tempo=midi_tempo,
        )

    job.check_cancel()
    send_progress(job.rpc_id, 0.90, "writing MIDI")

    out_path = output_dir / f"{input_path.stem}_basic_pitch.mid"
    midi_data.write(str(out_path))

    send_progress(job.rpc_id, 1.0, "done")
    return {
        "input":     str(input_path),
        "output":    str(out_path),
        "note_count": len(note_events),
        "duration_s": float(midi_data.get_end_time()),
        "onset_threshold":  onset_threshold,
        "frame_threshold":  frame_threshold,
    }
