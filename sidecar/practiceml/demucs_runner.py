"""Demucs stem separation handler.

Uses the low-level `demucs.pretrained` + `demucs.apply` API that ships in
demucs 4.0 (the higher-level `demucs.api.Separator` is a newer addition).
"""
from pathlib import Path
from typing import Any, Dict

from .rpc import Job, Cancelled, send_progress, log


def separate_stems(params: Dict[str, Any], job: Job) -> Dict[str, Any]:
    import torch
    import soundfile as sf
    from demucs.pretrained import get_model
    from demucs.apply import apply_model
    from demucs.audio import AudioFile

    input_path = Path(params["input"]).expanduser().resolve()
    output_dir = Path(params["output"]).expanduser().resolve()
    model_name = params.get("model", "htdemucs")
    vocals_only = bool(params.get("vocals_only", False))
    device_pref = str(params.get("device", "auto")).lower()

    if not input_path.exists():
        raise FileNotFoundError(f"input not found: {input_path}")
    output_dir.mkdir(parents=True, exist_ok=True)

    if device_pref == "cuda":
        if not torch.cuda.is_available():
            raise RuntimeError("device='cuda' requested but CUDA is not available")
        device = "cuda"
    elif device_pref == "cpu":
        device = "cpu"
    else:
        device = "cuda" if torch.cuda.is_available() else "cpu"
    log(f"separate_stems start: model={model_name} device={device} input={input_path.name}")

    send_progress(job.rpc_id, 0.02, "loading model")
    try:
        model = get_model(model_name)
    except Exception as e:
        raise RuntimeError(f"failed to load model '{model_name}': {e}") from e
    model.to(device).eval()

    job.check_cancel()
    send_progress(job.rpc_id, 0.1, "reading audio")
    try:
        wav = AudioFile(input_path).read(
            streams=0,
            samplerate=model.samplerate,
            channels=model.audio_channels,
        )
    except Exception as e:
        raise RuntimeError(f"failed to read audio: {e}") from e

    job.check_cancel()

    # Standard demucs normalisation (helps separation quality).
    ref = wav.mean(0)
    mean = ref.mean()
    std = ref.std() if ref.std() > 1e-8 else 1.0
    wav_norm = (wav - mean) / std
    wav_batched = wav_norm.unsqueeze(0).to(device)

    # apply_model's callback is invoked per segment with a dict that varies between
    # versions. We try to extract a usable fraction and re-emit; on KeyError just
    # bump the bar a notch so the UI shows movement.
    progress_state = {"emitted": 0.10, "ticks": 0}

    def cb(d):
        job.check_cancel()
        progress_state["ticks"] += 1
        frac = None
        try:
            if isinstance(d, dict):
                if "segment_offset" in d and "audio_length" in d:
                    frac = 0.20 + 0.70 * float(d["segment_offset"]) / max(1, float(d["audio_length"]))
                elif "models_done" in d and "models_total" in d:
                    frac = 0.20 + 0.70 * float(d["models_done"]) / max(1, float(d["models_total"]))
        except Exception:
            pass
        if frac is None:
            frac = min(0.85, progress_state["emitted"] + 0.02)
        if frac > progress_state["emitted"] + 0.005:
            progress_state["emitted"] = frac
            send_progress(job.rpc_id, frac, "separating")

    send_progress(job.rpc_id, 0.2, "separating")
    try:
        sources = apply_model(model, wav_batched, device=device, progress=False,
                              callback=cb)[0]
    except Cancelled:
        raise
    except TypeError:
        # Older signature with no callback kwarg.
        sources = apply_model(model, wav_batched, device=device, progress=False)[0]
    except Exception as e:
        raise RuntimeError(f"demucs apply_model failed: {e}") from e

    job.check_cancel()
    sources = sources * std + mean

    # Stem layout: model.sources is e.g. ['drums','bass','other','vocals'].
    stem_tensors = {name: sources[i] for i, name in enumerate(model.sources)}

    if vocals_only:
        if "vocals" not in stem_tensors:
            raise RuntimeError(f"model '{model_name}' has no 'vocals' stem")
        vocals = stem_tensors["vocals"]
        accomp = sum(t for n, t in stem_tensors.items() if n != "vocals")
        stem_tensors = {"vocals": vocals, "no_vocals": accomp}

    out_files: Dict[str, str] = {}
    base = input_path.stem
    n = len(stem_tensors)
    for i, (name, tensor) in enumerate(stem_tensors.items()):
        job.check_cancel()
        out_path = output_dir / f"{base}_{name}.wav"
        # Write via soundfile (channels-last float32) — bypasses demucs.audio.save_audio,
        # which calls torchaudio.save and pulls in torchcodec + ffmpeg shared libs.
        pcm = tensor.detach().cpu().clamp_(-1.0, 1.0).t().contiguous().numpy()
        sf.write(str(out_path), pcm, int(model.samplerate), subtype="PCM_16")
        out_files[name] = str(out_path)
        send_progress(job.rpc_id, 0.9 + 0.1 * (i + 1) / n, f"writing {name}.wav")

    send_progress(job.rpc_id, 1.0, "done")
    return {
        "input": str(input_path),
        "output_dir": str(output_dir),
        "stems": out_files,
        "sample_rate": int(model.samplerate),
        "device": device,
        "model": model_name,
    }
