"""yt-dlp download handler."""
from pathlib import Path
from typing import Any, Dict

from .rpc import Job, Cancelled, send_progress, log


def youtube_download(params: Dict[str, Any], job: Job) -> Dict[str, Any]:
    import yt_dlp

    url = params["url"]
    output_dir = Path(params["output"]).expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    audio_format = params.get("audio_format", "wav")

    last_status = {"path": None}

    def hook(d):
        job.check_cancel()
        status = d.get("status")
        if status == "downloading":
            total = d.get("total_bytes") or d.get("total_bytes_estimate") or 0
            done = d.get("downloaded_bytes", 0)
            frac = (done / total) if total > 0 else 0.0
            send_progress(job.rpc_id, frac * 0.9,  # leave 10% for postprocess
                          f"downloading {d.get('_percent_str', '').strip()}")
        elif status == "finished":
            last_status["path"] = d.get("filename")
            send_progress(job.rpc_id, 0.95, "extracting audio")

    def pp_hook(d):
        job.check_cancel()
        if d.get("status") == "finished":
            info = d.get("info_dict") or {}
            fp = info.get("filepath") or info.get("_filename")
            if fp:
                last_status["path"] = fp

    ydl_opts = {
        "format": "bestaudio/best",
        "outtmpl": str(output_dir / "%(title)s.%(ext)s"),
        "postprocessors": [{
            "key": "FFmpegExtractAudio",
            "preferredcodec": audio_format,
        }],
        "progress_hooks": [hook],
        "postprocessor_hooks": [pp_hook],
        "quiet": True,
        "no_warnings": True,
        "noprogress": True,
        # If the URL has a &list=... param, only fetch the single video — not the
        # whole playlist. Matches yt-dlp's --no-playlist CLI flag.
        "noplaylist": True,
    }

    try:
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(url, download=True)
    except Cancelled:
        raise
    except Exception as e:
        raise RuntimeError(f"yt-dlp failed: {e}") from e

    out_path = last_status["path"]
    # The downloader's last reported filename has the source extension; the
    # postprocessor rewrites to <preferredcodec>. Reconstruct the final path.
    if out_path:
        out_path = Path(out_path).with_suffix("." + audio_format)
    else:
        title = info.get("title", "audio") if isinstance(info, dict) else "audio"
        candidates = list(output_dir.glob(f"{title}*.{audio_format}"))
        out_path = candidates[0] if candidates else None

    if out_path is None or not Path(out_path).exists():
        raise RuntimeError("downloaded file not located on disk")

    return {
        "url": url,
        "output_path": str(Path(out_path).resolve()),
        "title": (info.get("title") if isinstance(info, dict) else None),
        "duration": (info.get("duration") if isinstance(info, dict) else None),
    }
