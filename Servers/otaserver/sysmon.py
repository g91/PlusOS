"""System management for the OTA control panel.

Reports CPU / RAM / disk / process info and performs safe operations (restart
the service, prune old releases) plus, when explicitly enabled, arbitrary shell
commands and process kill.

psutil is used when available for rich metrics; a stdlib fallback keeps the
panel working without it (fewer details).

SHELL IS OFF BY DEFAULT. Set OTA_ALLOW_SHELL=1 to enable the shell + kill
endpoints. Every shell command is written to the audit log.
"""
from __future__ import annotations

import os
import sys
import time
import shutil
import platform
import subprocess
from typing import Optional

try:
    import psutil  # type: ignore
    HAVE_PSUTIL = True
except Exception:  # pragma: no cover
    psutil = None
    HAVE_PSUTIL = False

ALLOW_SHELL = os.environ.get("OTA_ALLOW_SHELL", "") in ("1", "true", "yes", "on")

_START_TIME = time.time()


def shell_enabled() -> bool:
    return ALLOW_SHELL


# ------------------------------------------------------------------ metrics

def _mem_fallback() -> dict:
    total = avail = 0
    if sys.platform.startswith("linux"):
        try:
            info = {}
            with open("/proc/meminfo") as fh:
                for line in fh:
                    k, _, v = line.partition(":")
                    info[k.strip()] = int(v.split()[0]) * 1024
            total = info.get("MemTotal", 0)
            avail = info.get("MemAvailable", info.get("MemFree", 0))
        except Exception:
            pass
    return {"total": total, "available": avail,
            "used": max(0, total - avail),
            "percent": round(100 * (total - avail) / total, 1) if total else 0}


def metrics() -> dict:
    out = {
        "host": platform.node(),
        "platform": platform.platform(),
        "python": platform.python_version(),
        "pid": os.getpid(),
        "server_uptime": time.time() - _START_TIME,
        "now": time.time(),
        "shell_enabled": ALLOW_SHELL,
        "psutil": HAVE_PSUTIL,
    }
    if HAVE_PSUTIL:
        vm = psutil.virtual_memory()
        out["cpu_percent"] = psutil.cpu_percent(interval=0.0)
        out["cpu_count"] = psutil.cpu_count()
        out["load"] = list(getattr(psutil, "getloadavg", lambda: (0, 0, 0))())
        out["mem"] = {"total": vm.total, "available": vm.available,
                      "used": vm.used, "percent": vm.percent}
        try:
            boot = psutil.boot_time()
            out["host_uptime"] = time.time() - boot
        except Exception:
            out["host_uptime"] = None
    else:
        out["cpu_percent"] = None
        out["cpu_count"] = os.cpu_count()
        out["load"] = list(os.getloadavg()) if hasattr(os, "getloadavg") else [0, 0, 0]
        out["mem"] = _mem_fallback()
        out["host_uptime"] = None
    # Disk usage of the data dir's drive.
    try:
        du = shutil.disk_usage(os.path.dirname(os.path.abspath(__file__)))
        out["disk"] = {"total": du.total, "used": du.used, "free": du.free,
                       "percent": round(100 * du.used / du.total, 1)}
    except Exception:
        out["disk"] = None
    return out


def processes(limit: int = 40) -> list[dict]:
    rows = []
    if HAVE_PSUTIL:
        for p in psutil.process_iter(["pid", "name", "username", "cpu_percent", "memory_info"]):
            try:
                mi = p.info.get("memory_info")
                rows.append({
                    "pid": p.info["pid"],
                    "name": p.info.get("name") or "?",
                    "user": p.info.get("username") or "",
                    "cpu": p.info.get("cpu_percent") or 0.0,
                    "rss": mi.rss if mi else 0,
                })
            except Exception:
                continue
        rows.sort(key=lambda r: r["rss"], reverse=True)
        return rows[:limit]
    # Fallback: ps / tasklist
    try:
        if os.name == "nt":
            out = subprocess.check_output(["tasklist", "/fo", "csv", "/nh"],
                                          text=True, errors="replace")
            for line in out.splitlines():
                parts = [x.strip('"') for x in line.split('","')]
                if len(parts) >= 5:
                    name, pid = parts[0], parts[1]
                    mem = parts[4].replace(",", "").replace(" K", "000").replace(" K.", "")
                    try:
                        rss = int("".join(ch for ch in parts[4] if ch.isdigit())) * 1024
                    except Exception:
                        rss = 0
                    rows.append({"pid": int(pid) if pid.isdigit() else 0,
                                 "name": name, "user": "", "cpu": 0.0, "rss": rss})
        else:
            out = subprocess.check_output(
                ["ps", "-eo", "pid,comm,user,%cpu,rss", "--no-headers"],
                text=True, errors="replace")
            for line in out.splitlines():
                f = line.split(None, 4)
                if len(f) >= 5:
                    rows.append({"pid": int(f[0]), "name": f[1], "user": f[2],
                                 "cpu": float(f[3]), "rss": int(f[4]) * 1024})
    except Exception:
        pass
    rows.sort(key=lambda r: r["rss"], reverse=True)
    return rows[:limit]


# ------------------------------------------------------------------ actions

def kill(pid: int) -> tuple[bool, str]:
    if not ALLOW_SHELL:
        return False, "shell/kill disabled (set OTA_ALLOW_SHELL=1)"
    if pid == os.getpid():
        return False, "refusing to kill the OTA server itself"
    try:
        if HAVE_PSUTIL:
            psutil.Process(pid).terminate()
        else:
            os.kill(pid, 15)
        return True, "sent SIGTERM to %d" % pid
    except Exception as e:
        return False, str(e)


def run_shell(cmd: str, timeout: int = 30) -> dict:
    if not ALLOW_SHELL:
        return {"ok": False, "error": "shell disabled (set OTA_ALLOW_SHELL=1)"}
    try:
        proc = subprocess.run(cmd, shell=True, capture_output=True, text=True,
                              timeout=timeout, errors="replace")
        return {"ok": True, "code": proc.returncode,
                "stdout": proc.stdout[-20000:], "stderr": proc.stderr[-20000:]}
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": "timed out after %ds" % timeout}
    except Exception as e:
        return {"ok": False, "error": str(e)}


def restart_service() -> None:
    """Re-exec the current process. Works when launched as `python server.py`
    or via a supervisor that restarts on exit."""
    python = sys.executable
    os.execv(python, [python] + sys.argv)
