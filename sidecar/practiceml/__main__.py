"""Line-delimited JSON-RPC 2.0 dispatcher for the practiceml sidecar.

Long-running methods run on a worker thread so the dispatcher can handle
`cancel` requests while a job is in flight. stdout writes are serialised.
"""
# PEP 366 boot: make this runnable both via `python -m practiceml`
# and as a plain script (`python path/to/__main__.py`).
if __package__ in (None, ""):
    import sys as _sys
    import os as _os
    _sys.path.insert(0, _os.path.dirname(_os.path.dirname(_os.path.abspath(__file__))))
    __package__ = "practiceml"

import sys as _sys

# Force UTF-8 on stdin/stdout/stderr. Without this Python on Windows uses the
# active code page (cp1252) when stdio is piped, mangling any non-ASCII payload
# (e.g. unicode filenames coming back from yt-dlp via JSON-RPC).
for _stream, _writable in ((_sys.stdin, False), (_sys.stdout, True), (_sys.stderr, True)):
    try:
        _stream.reconfigure(encoding="utf-8", errors="strict",
                            newline="\n", line_buffering=_writable)
    except Exception:
        pass

import json
import sys
import threading
import traceback
from typing import Any, Callable, Dict

from .rpc import Job, Cancelled, registry, send_result, send_error, log


# Pre-import heavy ML deps on the main thread. Daemon threads on Windows can
# hang during import while the main thread is blocked on stdin reads (GIL +
# stdin TextIO interaction). Importing here means workers reuse cached modules.
def _preload_heavy():
    try:
        import torch  # noqa: F401
        import yt_dlp  # noqa: F401
        from demucs.pretrained import get_model  # noqa: F401
        from demucs.apply import apply_model  # noqa: F401
        from demucs.audio import AudioFile  # noqa: F401
        log("preloaded torch+demucs+yt_dlp")
    except Exception as e:
        log(f"preload warning: {e}")
    try:
        from basic_pitch.inference import predict, Model  # noqa: F401
        from basic_pitch import ICASSP_2022_MODEL_PATH  # noqa: F401
        log("preloaded basic_pitch")
    except Exception as e:
        log(f"basic_pitch preload warning: {e}")


_preload_heavy()


def _ping(params, job):
    return {"pong": True}


def _cancel(params, job):
    target_id = params.get("id")
    ok = registry.cancel(target_id) if target_id is not None else False
    return {"cancelled": ok, "id": target_id}


def _separate_stems(params, job):
    from .demucs_runner import separate_stems
    return separate_stems(params, job)


def _vocal_to_midi(params, job):
    from .vocal_to_midi import vocal_to_midi
    return vocal_to_midi(params, job)


def _youtube_download(params, job):
    from .youtube import youtube_download
    return youtube_download(params, job)


HANDLERS: Dict[str, Callable[[Dict[str, Any], Job], Dict[str, Any]]] = {
    "ping":             _ping,
    "cancel":           _cancel,
    "separate_stems":   _separate_stems,
    "vocal_to_midi":    _vocal_to_midi,
    "youtube_download": _youtube_download,
}

# Synchronous methods bypass the worker pool so they don't queue behind a long job.
SYNC_METHODS = {"ping", "cancel"}


def _run_handler(handler, params, job):
    log(f"worker start id={job.rpc_id}")
    try:
        result = handler(params, job)
        send_result(job.rpc_id, result)
    except Cancelled:
        send_error(job.rpc_id, -32001, "cancelled")
    except NotImplementedError as e:
        send_error(job.rpc_id, -32000, f"not implemented: {e}")
    except BaseException as e:
        log("handler error: " + traceback.format_exc())
        send_error(job.rpc_id, -32000, f"{type(e).__name__}: {e}")
    finally:
        registry.remove(job.rpc_id)
        log(f"worker end id={job.rpc_id}")


def main() -> int:
    log("ready")
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
        except json.JSONDecodeError as e:
            send_error(None, -32700, f"parse error: {e}")
            continue

        rpc_id = req.get("id")
        method = req.get("method")
        params = req.get("params") or {}
        handler = HANDLERS.get(method)

        if handler is None:
            send_error(rpc_id, -32601, f"method not found: {method}")
            continue

        job = Job(rpc_id)
        registry.add(job)

        if method in SYNC_METHODS:
            _run_handler(handler, params, job)
        else:
            t = threading.Thread(
                target=_run_handler, args=(handler, params, job),
                name=f"job-{rpc_id}-{method}", daemon=True)
            job.thread = t
            t.start()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
