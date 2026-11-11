#!/usr/bin/env python3
"""pluslive - PlusOS live remote debugger (host server + UI).

Runs on your computer and waits for the Macintosh's `netdbg` client to dial in
over the BlueSCSI DaynaPORT.  It then shows live machine state in a multi-tab
window: registers, a memory view with 68000 disassembly, heap + canary health,
the process table, the kernel log, and - the instant it happens - a crash with
its full post-mortem dump, which it also saves to a .pdmp file on disk.

    python3 pluslive.py [--port 53177] [--save-dir .]

Point the Mac at this computer with `reg set debug.host <your-ip>` (default
192.168.0.90), then run `netdbg` on the Mac.
"""
import socket
import threading
import time
import os
import sys
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dis68k
try:
    import plusdump
except Exception:
    plusdump = None

try:
    import tkinter as tk
    from tkinter import ttk
except Exception:
    tk = None

# syscall number -> name (enum starts at 1; see sdks/.../plusos/syscall.h)
SYSNAMES = {i + 1: n for i, n in enumerate(
    "exit yield sleep getpid spawn wait getkey pollkey readline reboot shutdown "
    "malloc free memfree blkio chainboot getcfg setcfg sysinfo kill reapany proclist "
    "regget regset regenum console win termspawn thread detach history sound scrap "
    "ramstats gc heapcheck pipecreate piperead pipewrite pipeclose seminit semwait "
    "sempost semdestroy mutexinit mutexlock mutexunlock signal alarm timer klog dmesg "
    "getenv setenv ramdisk shm power pram getrandom perfmetrics priority watchdog "
    "procpgid readdir klogflush dbgpeek dbgcrash dbgdump dbgtrace dbgmods".split())}
MODCLS = {0: "core", 1: "driver", 2: "service"}
MODSTATE = {0: "unreg", 1: "registered", 2: "running", 3: "FAILED", 4: "stopped"}


class Store:
    """Thread-safe snapshot of the latest telemetry from the Mac."""
    def __init__(self):
        self.lock = threading.RLock()   # reentrant: ev() may be called while held
        self.connected = False
        self.peer = ""
        self.uptime = 0
        self.regs = None          # dict
        self.heap = None          # dict
        self.procs = []           # list of dicts
        self.mem = (0, b"")       # (addr, bytes) manual window
        self.code = (0, b"")      # (base, bytes) code window around the live PC
        self.code_pc = 0          # the live PC to highlight within code
        self.kcode = (0, b"")     # (base, bytes) code window around the live KERNEL PC
        self.kpc = 0              # live kernel (supervisor) program counter
        self.ktask = ""           # kernel task currently running
        self.term = []            # interactive terminal output lines
        self.log = ""
        self.crashes = []         # list of summaries
        self.dump_chunks = {}     # off -> bytes (assembling)
        self.dump_total = 0
        self.last_dump_path = ""
        self.watch_addr = 0
        self.watch_len = 256
        self.events = []          # recent event strings
        self.recording = False    # capture every record to self.rec
        self.rec = []             # [(reltime, rawline)] recorded session
        self.rec_start = 0.0
        self.rec_saved_len = 0    # len(rec) at last save (for dirty/auto-save)
        self.saved_path = None    # where it was last saved
        self.breakpoints = []     # [addr] execution breakpoints (sampled)
        self.bp_hits = []         # breakpoint-hit event strings
        self.history = []         # EVERY record this session (bounded), for session export
        self.bp_active = set()    # addrs currently "at PC" (edge-trigger)
        self.attached_pid = None  # attach mode: focus on one process
        self.attached_name = ""
        self.syscalls = []        # live syscall trace: (pid, num, arg, tick)
        self.modules = []         # services/modules: dicts

    def ev(self, s):
        with self.lock:
            self.events.append("%s  %s" % (time.strftime("%H:%M:%S"), s))
            self.events = self.events[-200:]


def unhex(s):
    try:
        return bytes.fromhex(s.strip())
    except Exception:
        return b""


class Net(threading.Thread):
    """Listens for the Mac, parses records, updates the store."""
    def __init__(self, store, port, save_dir):
        super().__init__(daemon=True)
        self.store = store
        self.port = port
        self.save_dir = save_dir
        self.sock = None
        self.conn = None
        self.sendlock = threading.Lock()

    def send(self, s):
        with self.sendlock:
            try:
                if self.conn:
                    self.conn.sendall((s + "\n").encode())
            except Exception:
                pass

    def run(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("0.0.0.0", self.port))
        srv.listen(1)
        self.sock = srv
        self.store.ev("listening on tcp %d - run netdbg on the Mac" % self.port)
        while True:
            conn, addr = srv.accept()
            self.conn = conn
            with self.store.lock:
                self.store.connected = True
                self.store.peer = "%s:%d" % addr
            self.store.ev("Mac connected from %s:%d" % addr)
            # re-assert the current watch window
            self.send("A %x" % self.store.watch_addr)
            self.send("N %d" % self.store.watch_len)
            buf = b""
            try:
                while True:
                    data = conn.recv(4096)
                    if not data:
                        break
                    buf += data
                    while b"\n" in buf:
                        ln, buf = buf.split(b"\n", 1)
                        self.handle(ln.decode("latin1"))
            except Exception:
                pass
            with self.store.lock:
                self.store.connected = False
            self.store.ev("Mac disconnected")
            try:
                conn.close()
            except Exception:
                pass
            self.conn = None

    def handle(self, ln):
        if not ln:
            return
        s = self.store
        with s.lock:                    # always keep a bounded session history
            s.history.append((time.time(), ln))
            if len(s.history) > 8000:
                s.history = s.history[-8000:]
            if s.recording:             # and the raw record for save/replay
                s.rec.append((time.time() - s.rec_start, ln))
        t = ln[0]                       # single-char record type (P has no space: "P;...")
        rest = ln[1:].lstrip()
        if t == "V":
            with s.lock:
                s.uptime = int(rest or 0)
        elif t == "R":
            p = rest.split()
            if len(p) >= 21:
                d = {"d": [int(x, 16) for x in p[0:8]],
                     "a": [int(x, 16) for x in p[8:15]],
                     "ssp": int(p[15], 16), "usp": int(p[16], 16), "pc": int(p[17], 16),
                     "sr": int(p[18], 16), "vector": int(p[19]), "fault": int(p[20], 16)}
                with s.lock:
                    s.regs = d
        elif t == "H":
            p = rest.split()
            if len(p) >= 10:
                keys = ["total", "free", "halloc", "chunks", "salloc", "sfree", "largest", "gc", "corrupt", "heapcheck"]
                with s.lock:
                    s.heap = {k: int(v) for k, v in zip(keys, p)}
        elif t == "P":
            procs = []
            for rec in rest.split(";"):
                rec = rec.strip()
                if not rec:
                    continue
                f = rec.split(None, 5)
                if len(f) >= 6:
                    procs.append({"pid": int(f[0]), "ring": int(f[1]), "state": int(f[2]),
                                  "cpu": int(f[3]), "mem": int(f[4]), "name": f[5]})
            with s.lock:
                s.procs = procs
        elif t == "M":
            a, _, hx = rest.partition(" ")
            with s.lock:
                s.mem = (int(a, 16), unhex(hx))
        elif t == "X":                      # live code: <pc> <base> <hexbytes>
            p = rest.split(None, 2)
            if len(p) >= 3:
                with s.lock:
                    s.code_pc = int(p[0], 16)
                    s.code = (int(p[1], 16), unhex(p[2]))
        elif t == "K":                      # kernel: <kpc> <base> <hex> [taskname]
            p = rest.split(None, 3)
            if len(p) >= 3:
                with s.lock:
                    s.kpc = int(p[0], 16)
                    s.kcode = (int(p[1], 16), unhex(p[2]))
                    if len(p) >= 4:
                        s.ktask = p[3].strip()
        elif t == "Y":                      # syscall trace entry: pid num arg tick
            p = rest.split()
            if len(p) >= 4:
                with s.lock:
                    s.syscalls.append((int(p[0]), int(p[1]), int(p[2], 16), int(p[3])))
                    s.syscalls = s.syscalls[-600:]
        elif t == "Z":                      # services/modules: ;name cls state restarts;...
            mods = []
            for rec in rest.split(";"):
                rec = rec.strip()
                if not rec:
                    continue
                f = rec.split()
                if len(f) >= 4:
                    mods.append({"name": f[0], "cls": int(f[1]), "state": int(f[2]), "restarts": int(f[3])})
            with s.lock:
                s.modules = mods
        elif t == "O":                      # interactive terminal output
            txt = unhex(rest).decode("latin1", "replace") if rest and all(c in "0123456789abcdefABCDEF" for c in rest.replace(" ", "")) else rest
            with s.lock:
                s.term.append(txt)
                s.term = s.term[-500:]
        elif t == "L":
            with s.lock:
                s.log = unhex(rest).decode("latin1", "replace")
        elif t == "C":
            p = rest.split()
            if len(p) >= 3:
                summ = "crash #%s vector %s pc=%s" % (p[0], p[1], p[2])
                with s.lock:
                    s.crashes.append(summ)
                    s.dump_chunks = {}
                    s.dump_total = 0
                self.store.ev("CRASH: " + summ)
        elif t == "D":
            p = rest.split(None, 2)
            if len(p) >= 3:
                off, total, hx = int(p[0]), int(p[1]), unhex(p[2])
                with s.lock:
                    s.dump_chunks[off] = hx
                    s.dump_total = total
                    have = sum(len(v) for v in s.dump_chunks.values())
                if have >= total and total > 0:
                    self.save_dump()

    def save_dump(self):
        s = self.store
        with s.lock:
            data = b"".join(s.dump_chunks[o] for o in sorted(s.dump_chunks))
        path = os.path.join(self.save_dir, "PlusOS_%s.pdmp" % time.strftime("%Y%m%d_%H%M%S"))
        try:
            with open(path, "wb") as f:
                f.write(data)
            with s.lock:
                s.last_dump_path = path
            self.store.ev("saved crash dump -> %s (%d bytes)" % (path, len(data)))
        except Exception as e:
            self.store.ev("dump save failed: %s" % e)


class UdpClient(threading.Thread):
    """Mac-in-listen-mode: we poll the Mac over UDP, reusing Net.handle()."""
    def __init__(self, store, mac_ip, port, save_dir):
        super().__init__(daemon=True)
        self.store = store
        self.mac = (mac_ip, port)
        self.save_dir = save_dir
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(0.5)
        self._net = Net(store, port, save_dir)   # reuse its record parser + dump saver

    def send(self, s):
        try:
            self.sock.sendto((s + "\n").encode(), self.mac)
        except Exception:
            pass

    def run(self):
        self.store.ev("UDP client -> Mac %s:%d" % self.mac)
        with self.store.lock:
            self.store.connected = True
            self.store.peer = "%s:%d" % self.mac
        last = 0
        while True:
            now = time.time()
            if now - last > 0.25:
                last = now
                self.send("T")
                self.send("A %x" % self.store.watch_addr)
            try:
                data, _ = self.sock.recvfrom(2048)
                for ln in data.decode("latin1").split("\n"):
                    if ln:
                        self._net.handle(ln)
            except socket.timeout:
                pass
            except Exception:
                pass


class DemoFeeder(threading.Thread):
    """Synthetic live telemetry so the UI can be seen working with no Mac.
    Feeds the exact record format netdbg sends, through the real parser."""
    def __init__(self, store):
        super().__init__(daemon=True)
        self.store = store
        self.parser = Net(store, 0, ".")     # reuse handle()/save_dump, never started
        # a real 68000 function: link/movem/loop(add,subq,bne)/unlk/rts
        self.code = bytes([
            0x4e, 0x56, 0xff, 0xf8,          # link a6,#-8
            0x48, 0xe7, 0x38, 0x00,          # movem.l d2-d4,-(a7)
            0x74, 0x00,                      # moveq #0,d2
            0x26, 0x2e, 0x00, 0x08,          # move.l 8(a6),d3
            0xd4, 0x83,                      # add.l d3,d2     <- loop
            0x53, 0x83,                      # subq.l #1,d3
            0x66, 0xfa,                      # bne.s loop
            0x20, 0x02,                      # move.l d2,d0
            0x4c, 0xdf, 0x00, 0x1c,          # movem.l (a7)+,d2-d4
            0x4e, 0x5e,                      # unlk a6
            0x4e, 0x75,                      # rts
        ])
        self.base = 0x0002eba4
        # a kernel (ring-0) routine for the Kernel tab: movem/moveq/cmp/bne/movem/rts
        self.kcode = bytes([
            0x48, 0xe7, 0xc0, 0xc0,          # movem.l d0-d1/a0-a1,-(a7)
            0x70, 0x00,                      # moveq #0,d0
            0xb0, 0xb9, 0x00, 0x3b, 0xc0, 0x00,  # cmp.l $3bc000,d0  <- poll
            0x66, 0xf8,                      # bne.s poll
            0x4c, 0xdf, 0x03, 0x03,          # movem.l (a7)+,d0-d1/a0-a1
            0x4e, 0x75,                      # rts
        ])
        self.kbase = 0x003ba300
        self.koffs = [0, 4, 6, 12, 14, 16]

    def send(self, s):           # terminal commands come here as "E <cmd>"
        if not s.startswith("E "):
            return
        cmd = s[2:].strip()
        out = self._demo_cmd(cmd)
        with self.store.lock:
            self.store.term.append(out)

    def _demo_cmd(self, cmd):
        p = cmd.split()
        if not p:
            return ""
        c = p[0].lower()
        st = self.store
        if c in ("help", "?"):
            return ("commands: help, regs, procs, heap, peek <hexaddr> [len], log,\n"
                    "          watch <hexaddr>, go <hexaddr>, dump")
        if c == "regs":
            with st.lock:
                r = dict(st.regs) if st.regs else None
            if not r:
                return "no registers yet"
            return ("D0-3 %08x %08x %08x %08x\nD4-7 %08x %08x %08x %08x\n"
                    "A0-3 %08x %08x %08x %08x\nA4-7 %08x %08x %08x %08x\nPC %08x SR %04x"
                    % (tuple(r["d"]) + tuple(r["a"]) + (r["ssp"], r["pc"], r["sr"])))
        if c == "procs":
            with st.lock:
                ps = list(st.procs)
            return "\n".join("%-3d %-5s %s" % (x["pid"], "user" if x["ring"] == 2 else "kern", x["name"]) for x in ps) or "(none)"
        if c == "heap":
            with st.lock:
                hh = dict(st.heap) if st.heap else {}
            return "free=%s corrupt=%s heapcheck=%s" % (hh.get("free"), hh.get("corrupt"), hh.get("heapcheck"))
        if c == "peek" and len(p) > 1:
            try:
                a = int(p[1].lstrip("$"), 16)
            except Exception:
                return "bad address"
            return "%08x: (demo) %s" % (a, " ".join("%02x" % ((a + i) & 0xff) for i in range(16)))
        if c in ("watch", "go") and len(p) > 1:
            try:
                with st.lock:
                    st.watch_addr = int(p[1].lstrip("$"), 16)
                return "watching %s" % p[1]
            except Exception:
                return "bad address"
        if c == "log":
            with st.lock:
                return st.log[-400:] or "(empty)"
        if c == "dump":
            return "no crash captured yet" if not self.store.crashes else "last crash: %s" % self.store.crashes[-1]
        return "unknown command '%s' (try help)" % c

    def run(self):
        with self.store.lock:
            self.store.connected = True
            self.store.peer = "DEMO (synthetic)"
            self.store.watch_addr = self.base
        self.store.ev("DEMO mode - synthetic telemetry (no Mac needed)")
        t = 0
        pcoffs = [0, 4, 8, 10, 14, 16, 18, 14, 16, 18, 20, 24, 28, 30]  # walk the loop
        while True:
            t += 1
            h = self.parser.handle
            h("V %d" % (t * 15))
            pc = self.base + pcoffs[t % len(pcoffs)]
            d = [(0xdead0000 + t) & 0xffffffff] + [((i * 0x11111111) + t) & 0xffffffff for i in range(1, 8)]
            a = [self.base, 0x00035640, 0x0003161c, 0x00041000, 0x00042000, 0x0003f900, 0x00040000]
            h("R " + " ".join("%08x" % x for x in d) + " " + " ".join("%08x" % x for x in a) +
              " %08x %08x %08x %04x %d %08x" % (0x0003fe80, 0x00035640, pc, 0x2000, 0, 0))
            free = 3000000 - (t * 977) % 400000
            corrupt = 0 if (t % 40) else 1       # blip the canary once in a while
            h("H 4194304 %d 131072 24 8192 12 2000000 4096 %d %d" % (free, corrupt, corrupt))
            h("P;1 0 2 %d 64 init;2 0 1 %d 16 heartbeat;3 0 2 %d 8 supervisor;"
              "4 0 1 %d 8 syslog;5 2 2 %d 108 desktop;6 2 1 %d 20 netdbg"
              % (t, t // 2, t, t // 3, t * 3, t))
            watch = self.store.watch_addr or self.base
            if watch == self.base:
                mem = self.code + bytes((t + i) & 0xff for i in range(len(self.code), 256))
            else:
                mem = bytes((watch + t + i) & 0xff for i in range(256))
            h("M %08x %s" % (watch, mem.hex()))
            # live code window around PC (Live CPU tab auto-follows this)
            xwin = self.code + bytes((t + i) & 0xff for i in range(len(self.code), 160))
            h("X %08x %08x %s" % (pc, self.base, xwin.hex()))
            # kernel (ring 0) live execution (Kernel tab)
            kpc = self.kbase + self.koffs[t % len(self.koffs)]
            h("K %08x %08x %s supervisor/sched" % (kpc, self.kbase, self.kcode.hex()))
            # synthetic syscall trace (desktop pid 5 + shell/netdbg) and services
            sc = [(5, 27, 0x40000 + t), (5, 12, 0x2000), (5, 7, 0),
                  (6, 2, 0), (5, 15, 0x61000), (5, 23, 0x3a0000)][: 2 + (t % 4)]
            for (pid, num, arg) in sc:
                h("Y %d %d %08x %d" % (pid, num, arg, t * 15))
            h("Z;klog 0 2 0;video 1 2 0;kbd 1 2 0;sound 1 2 0;syslog 2 2 0;"
              "supervisor 2 2 0;netdbg 2 2 %d" % (t // 30))
            lines = [("[%7.2f] <6> demo: scheduler tick %d, task=%s"
                      % ((t * 15 + i) / 60.0, t, ("desktop" if i % 2 else "netdbg")))
                     for i in range(max(0, t - 18), t + 1)]
            h("L " + ("\n".join(lines)).encode().hex())
            if t % 60 == 0:                       # a crash every ~15s to show that path
                h("C %d 3 %08x" % (t // 60, pc))
            time.sleep(0.25)


VEC = {2: "BUS ERROR", 3: "ADDRESS ERROR", 4: "ILLEGAL", 5: "DIV0", 6: "CHK",
       7: "TRAPV", 8: "PRIVILEGE", 9: "TRACE", 11: "LINE-F", 0xFFFF: "SYSERR"}
STATE = {0: "unused", 1: "ready", 2: "RUN", 3: "blocked", 4: "zombie"}


class Player(threading.Thread):
    """Replays a recorded session (play / stop / skip / replay)."""
    def __init__(self, store):
        super().__init__(daemon=True)
        self.store = store
        self.parser = Net(store, 0, ".")
        self.records = []
        self.pos = 0
        self.playing = False
        self.speed = 2.0
        self._cv = threading.Condition()

    def load(self, records):
        with self._cv:
            self.records = records; self.pos = 0; self.playing = False
        self.store.ev("loaded recording: %d records" % len(records))

    def play(self):
        with self._cv:
            if self.records:
                self.playing = True; self._cv.notify()
    def pause(self):
        with self._cv:
            self.playing = False
    def replay(self):
        with self._cv:
            self.pos = 0; self.playing = bool(self.records); self._cv.notify()
    def skip(self, n=40):
        with self._cv:
            end = min(len(self.records), self.pos + n)
            for i in range(self.pos, end):
                self.parser.handle(self.records[i][1])
            self.pos = end

    def status(self):
        with self._cv:
            return self.playing, self.pos, len(self.records)

    def run(self):
        while True:
            with self._cv:
                while not (self.playing and self.pos < len(self.records)):
                    self._cv.wait(0.2)
                i = self.pos
                recs = self.records
            if i >= len(recs):
                with self._cv:
                    self.playing = False
                continue
            t0, line0 = recs[i]
            self.parser.handle(line0)
            with self._cv:
                self.pos += 1; nxt = self.pos
            if nxt < len(recs):
                dt = (recs[nxt][0] - t0) / max(0.1, self.speed)
                if dt > 0:
                    time.sleep(min(dt, 0.8))


def run_ui(store, net, port, player):
    import tkinter.filedialog as fd
    root = tk.Tk()
    root.title("PlusOS Live Debugger")
    root.geometry("1360x860")
    BG, PANEL, HDR, ACC = "#12121a", "#0b0b16", "#1b1b2a", "#7cc0ff"
    root.configure(bg=BG)
    mono = ("Consolas", 10); monob = ("Consolas", 10, "bold")

    style = ttk.Style()
    try:
        style.theme_use("clam")
    except Exception:
        pass
    style.configure(".", background=BG, foreground="#d8d8f0")
    style.configure("TPanedwindow", background=BG)
    style.configure("Sash", sashthickness=6, gripcount=8)
    style.configure("Treeview", background=PANEL, fieldbackground=PANEL, foreground="#d8d8f0", rowheight=20)
    style.configure("Treeview.Heading", background=HDR, foreground=ACC)
    style.map("Treeview", background=[("selected", "#2a4a6a")])

    views = {k: [] for k in ("cpu", "kernel", "mem", "heap", "proc", "rings",
                             "sys", "svc", "log", "term", "attach")}

    def scrolled_text(parent, **kw):
        f = tk.Frame(parent, bg=BG)
        ys = ttk.Scrollbar(f, orient="vertical")
        t = tk.Text(f, yscrollcommand=ys.set, bd=0, undo=False, **kw)
        ys.config(command=t.yview)
        ys.pack(side="right", fill="y"); t.pack(side="left", fill="both", expand=True)
        return f, t

    # ---- panel builders: each returns (frame, ctx) and registers ctx in views[kind] ----
    def panel_header(parent, title, kind, builder):
        hdr = tk.Frame(parent, bg=HDR)
        tk.Label(hdr, text=" " + title, bg=HDR, fg=ACC, font=monob, anchor="w").pack(side="left", fill="x", expand=True)
        tk.Button(hdr, text="⤢ pop out", bg=HDR, fg="#bbb", bd=0, font=("Consolas", 8),
                  activebackground="#333", command=lambda: popout(title, kind, builder)).pack(side="right")
        return hdr

    def build_cpu(parent):
        f = tk.Frame(parent, bg=PANEL)
        reg = tk.Text(f, font=monob, bg=PANEL, fg="#d8d8f0", height=6, bd=0)
        reg.pack(fill="x", padx=4, pady=(2, 0))
        tk.Label(f, text="  CPU  — live user/program execution  (>> = PC)", bg=PANEL, fg="#7cc0ff",
                 anchor="w", font=mono).pack(fill="x")
        fc, cdis = scrolled_text(f, font=mono, bg="#07120a", fg="#9fe6b0")
        cdis.tag_config("pc", background="#1e6f3a", foreground="#eaffea")
        fc.pack(fill="both", expand=True, padx=4, pady=(2, 4))
        ctx = {"reg": reg, "cdis": cdis}
        views["cpu"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["cpu"] and views["cpu"].remove(c))
        return f, ctx

    def build_kernel(parent):
        f = tk.Frame(parent, bg=PANEL)
        kinfo = tk.Text(f, font=monob, bg="#140a06", fg="#ffd0a0", height=2, bd=0)
        kinfo.pack(fill="x", padx=4, pady=(2, 0))
        tk.Label(f, text="  KERNEL  — ring-0 (supervisor) execution  (>> = kernel PC)", bg=PANEL,
                 fg="#ffb060", anchor="w", font=mono).pack(fill="x")
        fk, kdis = scrolled_text(f, font=mono, bg="#120a07", fg="#e6c09f")
        kdis.tag_config("pc", background="#7a3a1e", foreground="#ffeaea")
        fk.pack(fill="both", expand=True, padx=4, pady=(2, 4))
        ctx = {"kinfo": kinfo, "kdis": kdis}
        views["kernel"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["kernel"] and views["kernel"].remove(c))
        return f, ctx

    def build_sys(parent):
        f, t = scrolled_text(parent, font=mono, bg="#0a0a14", fg="#c8e0ff")
        ctx = {"t": t, "shown": [0]}
        views["sys"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["sys"] and views["sys"].remove(c))
        return f, ctx

    def build_svc(parent):
        f = tk.Frame(parent, bg=PANEL)
        cols = ("module", "class", "state", "restarts")
        tv = ttk.Treeview(f, columns=cols, show="headings")
        for c in cols:
            tv.heading(c, text=c); tv.column(c, width=90, anchor="w")
        ys = ttk.Scrollbar(f, orient="vertical", command=tv.yview); tv.config(yscrollcommand=ys.set)
        ys.pack(side="right", fill="y"); tv.pack(side="left", fill="both", expand=True)
        menu = tk.Menu(tv, tearoff=0, bg=PANEL, fg="#ddd")
        menu.add_command(label="Restart (kill worker)", command=lambda: _svc_cmd(tv, "kill"))
        menu.add_command(label="Inspect", command=lambda: _svc_cmd(tv, "inspect"))
        tv.bind("<Button-3>", lambda e: (tv.selection_set(tv.identify_row(e.y)), menu.tk_popup(e.x_root, e.y_root)) if tv.identify_row(e.y) else None)
        ctx = {"tv": tv}; views["svc"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["svc"] and views["svc"].remove(c))
        return f, ctx

    def _svc_cmd(tv, action):
        s = tv.selection()
        if s:
            name = tv.item(s[0], "values")[0]
            store.ev("service '%s': %s" % (name, action))

    def build_mem(parent):
        f = tk.Frame(parent, bg=PANEL)
        bar = tk.Frame(f, bg=PANEL); bar.pack(fill="x")
        tk.Label(bar, text=" addr $", bg=PANEL, fg="#9fe6b0", font=mono).pack(side="left")
        av = tk.StringVar()
        e = tk.Entry(bar, textvariable=av, width=10, font=mono, bg="#07120a", fg="#9fe6b0", insertbackground="#9fe6b0")
        e.pack(side="left")

        def go(*_):
            try:
                a = int(av.get().strip().lstrip("$"), 16)
            except Exception:
                return
            with store.lock:
                store.watch_addr = a
            net.send("A %x" % a)
        e.bind("<Return>", go)
        tk.Button(bar, text="watch", command=go, bg=HDR, fg="#ddd", bd=0).pack(side="left", padx=3)
        fm, mt = scrolled_text(f, font=mono, bg="#07120a", fg="#9fe6b0")
        fm.pack(fill="both", expand=True)
        ctx = {"mt": mt}
        views["mem"].append(ctx)
        f.bind("<Destroy>", lambda e2, c=ctx: c in views["mem"] and views["mem"].remove(c))
        return f, ctx

    def build_heap(parent):
        f, t = scrolled_text(parent, font=mono, bg=PANEL, fg="#e8d0a0")
        ctx = {"t": t}; views["heap"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["heap"] and views["heap"].remove(c))
        return f, ctx

    def proc_menu(tv, pid_idx, name_idx):
        """Shared right-click menu for process/ring tables."""
        menu = tk.Menu(tv, tearoff=0, bg=PANEL, fg="#ddd")

        def val():
            s = tv.selection()
            return tv.item(s[0], "values") if s else None

        def act(a):
            v = val()
            if not v:
                return
            pid, name = v[pid_idx], v[name_idx]
            if a == "inspect":
                store.ev("inspect pid %s '%s': %s" % (pid, name, v))
            elif a == "attach":
                with store.lock:
                    store.attached_pid = int(pid); store.attached_name = name
                store.ev("ATTACHED pid %s '%s'" % (pid, name)); net.send("E attach %s" % pid)
            elif a == "detach":
                with store.lock:
                    store.attached_pid = None; store.attached_name = ""
                store.ev("detached")
            elif a == "dump":
                net.send("E dump"); store.ev("dump requested (pid %s)" % pid)
            elif a == "debug":
                with store.lock:
                    store.attached_pid = int(pid); store.attached_name = name
                net.send("E watch 0"); store.ev("debugging pid %s '%s'" % (pid, name))
            elif a in ("pause", "stop"):
                net.send("E pause %s" % pid); store.ev("%s pid %s" % (a, pid))
            elif a == "kill":
                net.send("E kill %s" % pid); store.ev("kill pid %s" % pid)
        for lbl, a in (("Inspect", "inspect"), ("Attach", "attach"), ("Detach", "detach"),
                       ("Dump", "dump"), ("Debug", "debug"), ("Pause", "pause"),
                       ("Stop", "stop"), ("Kill", "kill")):
            menu.add_command(label=lbl, command=lambda x=a: act(x))

        def popup(ev):
            iid = tv.identify_row(ev.y)
            if iid:
                tv.selection_set(iid); menu.tk_popup(ev.x_root, ev.y_root)
        tv.bind("<Button-3>", popup)
        tv.bind("<Double-1>", lambda e: act("attach"))

    def build_proc(parent):
        f = tk.Frame(parent, bg=PANEL)
        cols = ("pid", "ring", "state", "cpu", "mem", "name")
        tv = ttk.Treeview(f, columns=cols, show="headings")
        for c in cols:
            tv.heading(c, text=c); tv.column(c, width=100 if c == "name" else 62, anchor="w")
        ys = ttk.Scrollbar(f, orient="vertical", command=tv.yview); tv.config(yscrollcommand=ys.set)
        ys.pack(side="right", fill="y"); tv.pack(side="left", fill="both", expand=True)
        proc_menu(tv, 0, 5)
        ctx = {"tv": tv}; views["proc"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["proc"] and views["proc"].remove(c))
        return f, ctx

    def build_rings(parent):
        f = tk.Frame(parent, bg=PANEL)
        cols = ("pid", "state", "cpu", "mem", "name")
        tv = ttk.Treeview(f, columns=cols, show="tree headings")
        tv.heading("#0", text="ring"); tv.column("#0", width=110)
        for c in cols:
            tv.heading(c, text=c); tv.column(c, width=90 if c == "name" else 58, anchor="w")
        ys = ttk.Scrollbar(f, orient="vertical", command=tv.yview); tv.config(yscrollcommand=ys.set)
        ys.pack(side="right", fill="y"); tv.pack(side="left", fill="both", expand=True)
        proc_menu(tv, 0, 4)
        ctx = {"tv": tv}; views["rings"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["rings"] and views["rings"].remove(c))
        return f, ctx

    def build_log(parent):
        f, t = scrolled_text(parent, font=mono, bg="#000", fg="#c0c0c0")
        ctx = {"t": t}; views["log"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["log"] and views["log"].remove(c))
        return f, ctx

    def build_attach(parent):
        f, t = scrolled_text(parent, font=mono, bg="#0a0f14", fg="#9fd0ff")
        ctx = {"t": t}; views["attach"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["attach"] and views["attach"].remove(c))
        return f, ctx

    def build_term(parent):
        f = tk.Frame(parent, bg="#000")
        ft, out = scrolled_text(f, font=mono, bg="#000", fg="#00e000", insertbackground="#00e000")
        ft.pack(fill="both", expand=True)
        out.insert("end", "PlusOS debug console. 'help' for commands.\nplusdbg> ")
        ib = tk.Frame(f, bg="#000"); ib.pack(fill="x")
        tk.Label(ib, text="plusdbg>", bg="#000", fg="#00e000", font=monob).pack(side="left")
        cv = tk.StringVar()
        ent = tk.Entry(ib, textvariable=cv, font=mono, bg="#000", fg="#00e000", insertbackground="#00e000", bd=0)
        ent.pack(side="left", fill="x", expand=True)

        def runc(*_):
            c = cv.get().strip(); cv.set("")
            if not c:
                return
            out.insert("end", c + "\n")
            p = c.split()
            if p and p[0] in ("watch", "go") and len(p) > 1:
                try:
                    with store.lock:
                        store.watch_addr = int(p[1].lstrip("$"), 16)
                except Exception:
                    pass
            net.send("E " + c); out.see("end")
        ent.bind("<Return>", runc)
        ctx = {"out": out, "shown": [0]}; views["term"].append(ctx)
        f.bind("<Destroy>", lambda e, c=ctx: c in views["term"] and views["term"].remove(c))
        return f, ctx

    BUILDERS = {"cpu": ("CPU", build_cpu), "kernel": ("Kernel", build_kernel),
                "mem": ("Memory", build_mem), "heap": ("Heap / Canaries", build_heap),
                "proc": ("Processes", build_proc), "rings": ("Rings", build_rings),
                "sys": ("Syscalls", build_sys), "svc": ("Services", build_svc),
                "log": ("Kernel Log", build_log), "term": ("Terminal", build_term),
                "attach": ("Attached", build_attach)}

    def popout(title, kind, builder):
        top = tk.Toplevel(root, bg=BG); top.title("PlusOS — " + title); top.geometry("760x560")
        panel_header(top, title + "  (pop-out)", kind, builder).pack(fill="x")
        fr, _ = builder(top); fr.pack(fill="both", expand=True)

    def framed(parent, kind):
        """A titled container holding one panel + its pop-out button."""
        box = tk.Frame(parent, bg=BG, highlightbackground="#2a2a3a", highlightthickness=1)
        title, builder = BUILDERS[kind]
        panel_header(box, title, kind, builder).pack(fill="x")
        fr, _ = builder(box); fr.pack(fill="both", expand=True)
        return box

    # ---------- main resizable layout ----------
    main = ttk.Panedwindow(root, orient="horizontal")
    main.pack(fill="both", expand=True)
    left = ttk.Panedwindow(main, orient="vertical")
    main.add(left, weight=3)
    # execution row: CPU | Kernel (separate panels, side by side)
    exec_row = ttk.Panedwindow(left, orient="horizontal")
    left.add(exec_row, weight=3)
    exec_row.add(framed(exec_row, "cpu"), weight=1)
    exec_row.add(framed(exec_row, "kernel"), weight=1)
    left.add(framed(left, "proc"), weight=1)
    # tabbed detail area (each tab also pops out to its own window)
    nb = ttk.Notebook(left)
    left.add(nb, weight=2)
    for kind in ("svc", "rings", "log", "term", "attach", "heap"):
        nb.add(framed(nb, kind), text=BUILDERS[kind][0])
    # right: memory + the live syscall trace (always visible)
    right = ttk.Panedwindow(main, orient="vertical")
    main.add(right, weight=2)
    right.add(framed(right, "mem"), weight=2)
    right.add(framed(right, "sys"), weight=3)

    # ---------- recording / playback control bar ----------
    ctrl = tk.Frame(root, bg=HDR); ctrl.pack(fill="x", side="bottom")
    rec_lbl = tk.StringVar(value="idle")

    def start_rec():
        with store.lock:
            if store.recording:
                return
            fresh = not store.rec
            if fresh:
                store.rec_start = time.time()
            store.recording = True
            n = len(store.rec)
        store.ev("recording started" if fresh else "recording resumed (appending, %d records)" % n)
    def stop_rec():
        with store.lock:
            store.recording = False; n = len(store.rec)
        store.ev("recording paused (%d records)" % n)
    def new_rec():
        with store.lock:
            store.rec = []; store.recording = False
            store.rec_saved_len = 0; store.saved_path = None
        store.ev("new recording (cleared)")

    def write_report(path, data):
        """Human-readable outline of everything in the recording."""
        from collections import Counter
        sysc = Counter(); pcs = set(); kpcs = set(); mems = set(); crashes = []; mods = {}
        tbeg = data[0][0] if data else 0; tend = data[-1][0] if data else 0
        for _, ln in data:
            t = ln[0]; rest = ln[1:].strip()
            if t == "Y":
                p = rest.split()
                if len(p) >= 2:
                    sysc[SYSNAMES.get(int(p[1]), "sys" + p[1])] += 1
            elif t == "X":
                p = rest.split()
                if p:
                    pcs.add(p[0])
            elif t == "K":
                p = rest.split()
                if p:
                    kpcs.add(p[0])
            elif t == "M":
                p = rest.split()
                if p:
                    mems.add(p[0])
            elif t == "C":
                crashes.append(rest)
            elif t == "Z":
                for r in rest.split(";"):
                    f = r.split()
                    if len(f) >= 3:
                        mods[f[0]] = MODSTATE.get(int(f[2]), f[2])
        with open(path, "w") as f:
            f.write("PlusOS Live Debugger - session report\n")
            f.write("=" * 60 + "\n")
            f.write("records: %d   duration: %.1fs\n\n" % (len(data), tend - tbeg))
            f.write("Syscalls executed (by name):\n")
            for name, c in sysc.most_common():
                f.write("  %-14s %d\n" % (name, c))
            f.write("\nAssembly PCs logged (CPU): %d unique\n" % len(pcs))
            for a in sorted(pcs)[:60]:
                f.write("  %s\n" % a)
            f.write("\nKernel PCs logged: %d unique\n" % len(kpcs))
            for a in sorted(kpcs)[:40]:
                f.write("  %s\n" % a)
            f.write("\nMemory addresses logged: %s\n" % (", ".join(sorted(mems)) or "(none)"))
            f.write("\nServices (final state):\n")
            for m, s in sorted(mods.items()):
                f.write("  %-14s %s\n" % (m, s))
            f.write("\nCrashes: %d\n" % len(crashes))
            for c in crashes:
                f.write("  %s\n" % c)

    def do_save(path):
        import json
        with store.lock:
            data = list(store.rec)
        with open(path, "w") as f:
            for t, l in data:
                f.write(json.dumps([t, l]) + "\n")
        txt = (path[:-5] if path.lower().endswith(".plog") else path) + ".txt"
        try:
            write_report(txt, data)
        except Exception as e:
            store.ev("report failed: %s" % e)
        with store.lock:
            store.rec_saved_len = len(data); store.saved_path = path
        store.ev("saved %d records -> %s  (+ report %s)" % (len(data), path, os.path.basename(txt)))

    def save_rec():
        path = fd.asksaveasfilename(defaultextension=".plog", filetypes=[("PlusOS log", "*.plog"), ("all", "*")])
        if path:
            do_save(path)

    def decode_record(ln):
        """One raw record -> a human-readable line for the session log."""
        t = ln[0]; rest = ln[1:].strip()
        if t == "V":
            return "tick %s" % rest
        if t == "R":
            p = rest.split()
            return "REGS pc=%s sr=%s d0=%s a0=%s" % (p[17], p[18], p[0], p[8]) if len(p) >= 19 else "REGS " + rest
        if t == "Y":
            p = rest.split()
            return "  syscall pid %s %s(#%s) a0=%s" % (p[0], SYSNAMES.get(int(p[1]), "?"), p[1], p[2]) if len(p) >= 3 else "syscall " + rest
        if t == "X":
            return "CPU pc=%s" % rest.split()[0]
        if t == "K":
            return "KERN pc=%s" % rest.split()[0]
        if t == "M":
            return "MEM @%s" % rest.split()[0]
        if t == "L":
            return "[log]\n" + unhex(rest).decode("latin1", "replace").rstrip()
        if t == "H":
            p = rest.split()
            return "HEAP free=%s corrupt=%s" % (p[1], p[8]) if len(p) >= 9 else "HEAP " + rest
        if t == "C":
            return "*** CRASH %s ***" % rest
        if t == "Z":
            return "services: " + rest
        if t == "P":
            return "procs: " + rest
        if t == "O":
            return "term: " + rest
        return ln

    def export_session():
        path = fd.asksaveasfilename(defaultextension=".txt", filetypes=[("text log", "*.txt"), ("all", "*")],
                                    initialfile=time.strftime("PlusOS_session_%Y%m%d_%H%M%S.txt"))
        if not path:
            return
        with store.lock:
            events = list(store.events); hist = list(store.history)
            regs = dict(store.regs) if store.regs else None
            procs = list(store.procs); modules = list(store.modules)
            syscalls = list(store.syscalls); log = store.log; crashes = list(store.crashes)
            maddr, mbytes = store.mem; bphits = list(store.bp_hits)
        with open(path, "w", encoding="utf-8") as f:
            f.write("PlusOS Live Debugger - FULL SESSION LOG\n" + "=" * 64 + "\n")
            f.write(time.strftime("exported %Y-%m-%d %H:%M:%S\n\n"))
            f.write("---- session events ----\n")
            for e in events:
                f.write(e + "\n")
            if bphits:
                f.write("\n---- breakpoint hits ----\n")
                for b in bphits:
                    f.write(b + "\n")
            f.write("\n---- current CPU registers ----\n")
            if regs:
                f.write("  D: " + " ".join("%08x" % x for x in regs["d"]) + "\n")
                f.write("  A: " + " ".join("%08x" % x for x in regs["a"]) + " ssp=%08x\n" % regs["ssp"])
                f.write("  PC=%08x SR=%04x USP=%08x\n" % (regs["pc"], regs["sr"], regs["usp"]))
            f.write("\n---- processes ----\n")
            for p in procs:
                f.write("  pid %-3d %-5s %s cpu=%s mem=%dKB\n" % (
                    p["pid"], "user" if p["ring"] == 2 else "kern", p["name"], p["cpu"], p["mem"] // 1024))
            f.write("\n---- services ----\n")
            for m in modules:
                f.write("  %-14s %-8s %s restarts=%d\n" % (
                    m["name"], MODCLS.get(m["cls"], m["cls"]), MODSTATE.get(m["state"], m["state"]), m["restarts"]))
            f.write("\n---- recent syscalls ----\n")
            for (pid, num, arg, tk) in syscalls[-120:]:
                f.write("  [%6.2fs] pid %-3d %-12s a0=%08x\n" % (tk / 60.0, pid, SYSNAMES.get(num, "?"), arg))
            f.write("\n---- kernel log (current) ----\n" + (log or "(empty)") + "\n")
            if mbytes:
                f.write("\n---- memory @ %08x ----\n" % maddr)
                for i in range(0, len(mbytes), 16):
                    row = mbytes[i:i + 16]
                    f.write("  %08x  %s  %s\n" % (maddr + i, " ".join("%02x" % b for b in row),
                            "".join(chr(b) if 32 <= b < 127 else "." for b in row)))
            if crashes:
                f.write("\n---- crashes ----\n")
                for c in crashes:
                    f.write("  " + c + "\n")
            f.write("\n---- full session timeline (%d records) ----\n" % len(hist))
            t0 = hist[0][0] if hist else 0
            for (ts, ln) in hist:
                f.write("[%8.2fs] %s\n" % (ts - t0, decode_record(ln)))
        store.ev("session log exported -> %s (%d records)" % (path, len(hist)))
    def open_rec():
        path = fd.askopenfilename(filetypes=[("PlusOS log", "*.plog"), ("all", "*")])
        if not path:
            return
        import json
        recs = []
        with open(path) as f:
            for line in f:
                line = line.strip()
                if line:
                    try:
                        t, l = json.loads(line); recs.append((t, l))
                    except Exception:
                        pass
        player.load(recs)

    menubar = tk.Menu(root)
    filem = tk.Menu(menubar, tearoff=0)
    filem.add_command(label="New Recording", command=new_rec)
    filem.add_command(label="Start / Resume Recording", command=start_rec)
    filem.add_command(label="Stop Recording", command=stop_rec)
    filem.add_separator()
    filem.add_command(label="Save Recording...", command=save_rec)
    filem.add_command(label="Open Recording...", command=open_rec)
    filem.add_separator()
    filem.add_command(label="Export Session Log (text)...", command=export_session)
    filem.add_separator()
    filem.add_command(label="Exit", command=lambda: on_close())
    menubar.add_cascade(label="File", menu=filem)
    ctrlm = tk.Menu(menubar, tearoff=0)
    ctrlm.add_command(label="Play", command=player.play)
    ctrlm.add_command(label="Pause", command=player.pause)
    ctrlm.add_command(label="Skip", command=player.skip)
    ctrlm.add_command(label="Replay", command=player.replay)
    menubar.add_cascade(label="Control", menu=ctrlm)
    winm = tk.Menu(menubar, tearoff=0)
    for kind, (title, builder) in BUILDERS.items():
        winm.add_command(label="Pop out: " + title, command=lambda t=title, k=kind, b=builder: popout(t, k, b))
    menubar.add_cascade(label="Window", menu=winm)
    dbgm = tk.Menu(menubar, tearoff=0)
    for lbl, cmd in (("Force Garbage Collect", "gc"), ("Heap / Canary Check", "heapcheck"),
                     ("Flush Kernel Log to Disk", "flush"), ("Capture Crash Dump", "dump"),
                     ("Show Registers", "regs"), ("Show Processes", "procs")):
        dbgm.add_command(label=lbl, command=lambda c=cmd: (net.send("E " + c), store.ev("debug: " + c)))
    dbgm.add_separator()
    dbgm.add_command(label="Detach from process", command=lambda: (
        store.__setattr__("attached_pid", None), store.ev("detached")))
    dbgm.add_command(label="Clear syscall trace", command=lambda: (
        store.syscalls.clear(), store.ev("trace cleared")))
    menubar.add_cascade(label="Debug", menu=dbgm)
    root.config(menu=menubar)

    btns = {}
    for key, txt, cmd in (("play", "▶ Play", player.play), ("ppause", "⏸ Pause", player.pause),
                          ("skip", "⏭ Skip", player.skip), ("replay", "↺ Replay", player.replay),
                          ("rec", "⏺ Rec", start_rec), ("stop", "⏹ Stop", stop_rec),
                          ("save", "\U0001f4be Save", save_rec), ("open", "\U0001f4c2 Open", open_rec)):
        b = tk.Button(ctrl, text=txt, command=cmd, bg=HDR, fg="#ddd", bd=0,
                      activebackground="#333", disabledforeground="#555", font=("Consolas", 9))
        b.pack(side="left", padx=2, pady=2)
        btns[key] = b
    btns["stop"].config(state="disabled")          # nothing to stop until we record
    # prominent recording indicator: a red dot + "RECORDING" banner, shown only while recording
    rec_dot = tk.Canvas(ctrl, width=16, height=16, bg=HDR, highlightthickness=0)
    rec_dot.pack(side="left", padx=(12, 2))
    rec_dot_id = rec_dot.create_oval(3, 3, 13, 13, fill="#ff2020", outline="#ff8080", state="hidden")
    rec_banner = tk.Label(ctrl, text="RECORDING", bg=HDR, fg="#ff3030", font=("Consolas", 10, "bold"))
    rec_banner.pack(side="left")
    tk.Label(ctrl, textvariable=rec_lbl, bg=HDR, fg="#ffcc66", font=mono).pack(side="left", padx=4)
    # breakpoints entry
    tk.Label(ctrl, text="  bp $", bg=HDR, fg="#ff9", font=mono).pack(side="left")
    bp_var = tk.StringVar()
    bp_ent = tk.Entry(ctrl, textvariable=bp_var, width=9, font=mono, bg="#222", fg="#ff9", insertbackground="#ff9")
    bp_ent.pack(side="left")

    def add_bp(*_):
        try:
            a = int(bp_var.get().strip().lstrip("$"), 16)
        except Exception:
            return
        with store.lock:
            if a not in store.breakpoints:
                store.breakpoints.append(a)
        store.ev("breakpoint set @ %08x" % a); bp_var.set("")
    bp_ent.bind("<Return>", add_bp)
    tk.Button(ctrl, text="+bp", command=add_bp, bg=HDR, fg="#ff9", bd=0).pack(side="left", padx=2)
    tk.Button(ctrl, text="clr bp", bg=HDR, fg="#ff9", bd=0,
              command=lambda: (store.breakpoints.clear(), store.bp_active.clear(), store.ev("breakpoints cleared"))).pack(side="left")

    status = tk.StringVar(value="waiting for Mac...  (run netdbg on the Mac, or restart with --demo)")
    tk.Label(root, textvariable=status, anchor="w", bg="#111", fg="#0f0", font=mono).pack(fill="x", side="bottom")

    def settext(w, s):
        if w.get("1.0", "end-1c") != s:
            w.delete("1.0", "end"); w.insert("1.0", s)

    def render_disasm(w, base, data, pc):
        lines, pcl = [], None
        off = 0
        while off < len(data):
            a = base + off
            try:
                ln, txt = dis68k.disasm(data, off)
            except Exception:
                ln, txt = 2, "dc.w ????"
            if ln < 2:
                ln = 2
            raw = " ".join("%02x" % x for x in data[off:off + ln])
            here = a <= pc < a + ln
            if here:
                pcl = len(lines)
            lines.append("%s %08x: %-14s %s" % (">>" if here else "  ", a, raw, txt))
            off += ln
        w.delete("1.0", "end"); w.insert("1.0", "\n".join(lines))
        if pcl is not None:
            w.tag_remove("pc", "1.0", "end")
            w.tag_add("pc", "%d.0" % (pcl + 1), "%d.end" % (pcl + 1))
            w.see("%d.0" % (pcl + 1))

    blink = [0]

    def refresh():
        try:
            _refresh()
        except Exception as e:
            status.set("refresh error: %s" % e)
        root.after(300, refresh)

    def _refresh():
        with store.lock:
            conn = store.connected; peer = store.peer; up = store.uptime
            regs = dict(store.regs) if store.regs else None
            heap = dict(store.heap) if store.heap else None
            procs = list(store.procs)
            maddr, mbytes = store.mem
            cbase, cbytes = store.code; code_pc = store.code_pc
            kbase, kbytes = store.kcode; kpc = store.kpc; ktask = store.ktask
            log = store.log
            crashes = list(store.crashes)
            dumppath = store.last_dump_path
            term = list(store.term)
            att = store.attached_pid; attname = store.attached_name
            rec_on = store.recording; rec_n = len(store.rec)
            syscalls = list(store.syscalls); modules = list(store.modules)
            unsaved = rec_n > store.rec_saved_len
            bps = list(store.breakpoints); bp_active = store.bp_active
        playing, ppos, pn = player.status()
        status.set("%s  |  %s  |  up %.1fs  |  port %d%s%s%s" % (
            "CONNECTED" if conn else "waiting for Mac...", peer or "-", up / 60.0, port,
            ("  |  dump: " + dumppath) if dumppath else "",
            ("  |  ATTACHED pid %s '%s'" % (att, attname)) if att else "",
            ("  |  %d breakpoint(s)" % len(bps)) if bps else ""))
        rec_lbl.set(("REC %d%s" % (rec_n, " *" if unsaved else "")) if rec_on
                    else (("PLAY %d/%d" % (ppos, pn)) if pn else "idle"))
        # recording indicators: solid red dot + blinking RECORDING banner,
        # the Rec button greys out and Stop enables, and the window title shows it
        blink[0] += 1
        rec_dot.itemconfig(rec_dot_id, state=("normal" if rec_on else "hidden"))
        rec_banner.config(fg=(("#ff3030" if blink[0] % 2 == 0 else "#7a1010") if rec_on else HDR))
        btns["rec"].config(state=("disabled" if rec_on else "normal"),
                           text=("⏺ Rec" if not rec_on else "⏺ REC●"))
        btns["stop"].config(state=("normal" if rec_on else "disabled"))
        root.title("PlusOS Live Debugger" + ("    ● RECORDING" if rec_on else ""))

        # sampled execution breakpoints: fire when the CPU or kernel PC enters one
        for bp in bps:
            at = (code_pc and code_pc <= bp < code_pc + 6) or (kpc and kpc <= bp < kpc + 6) \
                 or (regs and regs["pc"] <= bp < regs["pc"] + 6)
            if at and bp not in bp_active:
                bp_active.add(bp)
                msg = "BREAKPOINT HIT @ %08x  (pc=%08x kpc=%08x)" % (bp, code_pc or 0, kpc or 0)
                store.ev(msg)
                with store.lock:
                    store.bp_hits.append(msg)
                player.pause()               # halt playback on a breakpoint hit
            elif not at and bp in bp_active:
                bp_active.discard(bp)

        # CPU + kernel
        if regs:
            L = []
            for i in range(0, 8, 4):
                L.append("  ".join("D%d=%08x" % (j, regs["d"][j]) for j in range(i, i + 4)))
            a8 = regs["a"] + [regs["ssp"]]
            for i in range(0, 8, 4):
                L.append("  ".join("A%d=%08x" % (j, a8[j]) for j in range(i, i + 4)))
            vn = VEC.get(regs["vector"], "")
            L.append("PC=%08x  SR=%04x  USP=%08x   %s" % (
                regs["pc"], regs["sr"], regs["usp"], ("last fault: " + vn) if regs["vector"] else "running"))
            rtext = "\n".join(L)
        else:
            rtext = "(waiting for register data)"
        cdata = cbytes if cbytes else mbytes
        cb = cbase if cbytes else maddr
        cpc = code_pc or (regs["pc"] if regs else 0)
        for ctx in views["cpu"]:
            settext(ctx["reg"], rtext)
            if cdata:
                render_disasm(ctx["cdis"], cb, cdata, cpc)
        # kernel panel
        for ctx in views["kernel"]:
            settext(ctx["kinfo"], "kernel task: %-16s  kernel PC: %08x" % (ktask or "-", kpc))
            if kbytes:
                render_disasm(ctx["kdis"], kbase, kbytes, kpc)
        # syscalls (live trace by name, auto-scroll; filtered to attached pid if set)
        for ctx in views["sys"]:
            w = ctx["t"]; shown = ctx["shown"]
            if len(syscalls) < shown[0]:
                shown[0] = 0; w.delete("1.0", "end")
            new = syscalls[shown[0]:]
            if new:
                for (pid, num, arg, tick) in new:
                    if att and pid != att:
                        continue
                    nm = SYSNAMES.get(num, "sys%d" % num)
                    w.insert("end", "[%6.2fs] pid %-3d %-12s (#%d)  a0=%08x\n"
                             % (tick / 60.0, pid, nm, num, arg))
                w.see("end"); shown[0] = len(syscalls)
                # cap the widget
                if int(w.index("end-1c").split(".")[0]) > 2000:
                    w.delete("1.0", "500.0")
        # services / modules
        for ctx in views["svc"]:
            tv = ctx["tv"]; tv.delete(*tv.get_children())
            for m in modules:
                st = MODSTATE.get(m["state"], m["state"])
                tags = ("fail",) if st == "FAILED" else ()
                tv.insert("", "end", tags=tags, values=(m["name"], MODCLS.get(m["cls"], m["cls"]), st, m["restarts"]))
            tv.tag_configure("fail", background="#5a2020")
        # rings (processes grouped by ring)
        for ctx in views["rings"]:
            tv = ctx["tv"]; tv.delete(*tv.get_children())
            byring = {}
            for p in procs:
                byring.setdefault(p["ring"], []).append(p)
            for ring in sorted(byring):
                rn = {0: "Ring 0 (kernel)", 1: "Ring 1", 2: "Ring 2 (user)", 3: "Ring 3"}.get(ring, "Ring %d" % ring)
                parent = tv.insert("", "end", text="%s  [%d]" % (rn, len(byring[ring])), open=True)
                for p in byring[ring]:
                    tv.insert(parent, "end", values=(p["pid"], STATE.get(p["state"], p["state"]),
                              p["cpu"], "%dKB" % (p["mem"] // 1024), p["name"]))
        # memory
        if mbytes:
            hx = []
            for i in range(0, len(mbytes), 16):
                row = mbytes[i:i + 16]
                hx.append("%08x  %-47s  %s" % (maddr + i, " ".join("%02x" % b for b in row),
                          "".join(chr(b) if 32 <= b < 127 else "." for b in row)))
            mtext = "\n".join(hx)
            for ctx in views["mem"]:
                settext(ctx["mt"], mtext)
        # heap
        if heap:
            def kb(n): return "%d (%.1f KB)" % (n, n / 1024.0)
            warn = "  *** CANARY/HEAP CORRUPTION ***" if (heap["corrupt"] or heap["heapcheck"]) else "  OK"
            htext = ("Heap & canary health%s\n\n  total RAM   %s\n  free RAM    %s\n"
                     "  heap alloc  %s  (%d chunks)\n  slab alloc  %s  (%d free)\n"
                     "  largest     %s\n  gc reclaim  %s\n\n  canary fails %d\n  heapcheck    %d\n" % (
                         warn, kb(heap["total"]), kb(heap["free"]), kb(heap["halloc"]), heap["chunks"],
                         kb(heap["salloc"]), heap["sfree"], kb(heap["largest"]), kb(heap["gc"]),
                         heap["corrupt"], heap["heapcheck"]))
            for ctx in views["heap"]:
                settext(ctx["t"], htext)
        # processes
        for ctx in views["proc"]:
            tv = ctx["tv"]
            tv.delete(*tv.get_children())
            for p in procs:
                tags = ("att",) if att == p["pid"] else ()
                tv.insert("", "end", tags=tags, values=(p["pid"], "user" if p["ring"] == 2 else "kern",
                          STATE.get(p["state"], p["state"]), p["cpu"], "%dKB" % (p["mem"] // 1024), p["name"]))
            tv.tag_configure("att", background="#2a4a2a")
        # attached panel
        atext = "No process attached.\nRight-click a process -> Attach to focus on it."
        if att:
            ap = next((p for p in procs if p["pid"] == att), None)
            atext = "ATTACHED: pid %s  '%s'\n" % (att, attname)
            if ap:
                atext += "  ring=%s  state=%s  cpu=%s ticks  mem=%dKB\n" % (
                    "user" if ap["ring"] == 2 else "kern", STATE.get(ap["state"], ap["state"]),
                    ap["cpu"], ap["mem"] // 1024)
            atext += "\nLog lines mentioning this process:\n"
            for ln in (log or "").splitlines():
                if attname and attname in ln:
                    atext += "  " + ln + "\n"
        for ctx in views["attach"]:
            settext(ctx["t"], atext)
        # log
        if log:
            for ctx in views["log"]:
                if ctx["t"].get("1.0", "end-1c") != log:
                    settext(ctx["t"], log); ctx["t"].see("end")
        # terminal
        for ctx in views["term"]:
            if len(term) > ctx["shown"][0]:
                for ln in term[ctx["shown"][0]:]:
                    ctx["out"].insert("end", ln + ("" if ln.endswith("\n") else "\n") + "plusdbg> ")
                ctx["out"].see("end")
                ctx["shown"][0] = len(term)

    def on_close():
        with store.lock:
            unsaved = len(store.rec) > 0 and len(store.rec) > store.rec_saved_len
            path = store.saved_path
        if unsaved:
            if not path:
                path = os.path.join(os.getcwd(), time.strftime("PlusOS_%Y%m%d_%H%M%S.plog"))
            try:
                do_save(path)
                store.ev("auto-saved recording on exit -> %s" % path)
            except Exception as e:
                store.ev("auto-save failed: %s" % e)
        root.destroy()
    root.protocol("WM_DELETE_WINDOW", on_close)

    root.after(300, refresh)
    root.mainloop()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=53177)
    ap.add_argument("--save-dir", default=".")
    ap.add_argument("--connect", metavar="MAC_IP", default=None,
                    help="Mac is in listen mode: poll it over UDP at this IP")
    ap.add_argument("--demo", action="store_true",
                    help="feed synthetic live telemetry so you can see the UI with no Mac")
    args = ap.parse_args()
    store = Store()
    if args.demo:
        net = DemoFeeder(store)
    elif args.connect:
        net = UdpClient(store, args.connect, args.port, args.save_dir)
    else:
        net = Net(store, args.port, args.save_dir)
    net.start()
    if tk is None:
        print("Tkinter not available; running headless. Telemetry events:")
        last = 0
        while True:
            time.sleep(1)
            with store.lock:
                evs = store.events[last:]
                last = len(store.events)
            for e in evs:
                print(e)
    else:
        player = Player(store); player.start()
        run_ui(store, net, args.port, player)


if __name__ == "__main__":
    main()
