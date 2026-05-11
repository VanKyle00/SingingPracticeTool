"""JSON-RPC plumbing shared by handlers: thread-safe stdout, progress + cancellation."""
import json
import sys
import threading
from typing import Any, Dict, Optional


_stdout_lock = threading.Lock()


def _emit(obj: Dict[str, Any]) -> None:
    with _stdout_lock:
        sys.stdout.write(json.dumps(obj, separators=(",", ":")) + "\n")
        sys.stdout.flush()


def log(msg: str) -> None:
    sys.stderr.write(f"[practiceml] {msg}\n")
    sys.stderr.flush()


def send_result(rpc_id, result) -> None:
    _emit({"jsonrpc": "2.0", "id": rpc_id, "result": result})


def send_error(rpc_id, code: int, message: str) -> None:
    _emit({"jsonrpc": "2.0", "id": rpc_id, "error": {"code": code, "message": message}})


def send_progress(rpc_id, frac: float, text: str = "") -> None:
    _emit({
        "jsonrpc": "2.0",
        "method": "progress",
        "params": {"id": rpc_id, "frac": max(0.0, min(1.0, frac)), "text": text},
    })


class Cancelled(Exception):
    pass


class Job:
    def __init__(self, rpc_id):
        self.rpc_id = rpc_id
        self.cancel_event = threading.Event()
        self.thread: Optional[threading.Thread] = None

    def check_cancel(self):
        if self.cancel_event.is_set():
            raise Cancelled()


class JobRegistry:
    def __init__(self):
        self._jobs: Dict[int, Job] = {}
        self._lock = threading.Lock()

    def add(self, job: Job):
        with self._lock:
            self._jobs[job.rpc_id] = job

    def remove(self, rpc_id):
        with self._lock:
            self._jobs.pop(rpc_id, None)

    def cancel(self, rpc_id) -> bool:
        with self._lock:
            j = self._jobs.get(rpc_id)
            if j is None:
                return False
            j.cancel_event.set()
            return True


registry = JobRegistry()
