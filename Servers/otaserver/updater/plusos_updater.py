#!/usr/bin/env python3
"""PlusOS Updater — host-side GUI that pulls OTA releases and applies them to a
PlusOS disk image or BlueSCSI SD card WITHOUT reflashing.

It talks to the OTA server (default https://ota.d0a.net, Cloudflare-friendly
HTTPS/WSS), shows the catalog of releases and which ones are newer than what is
already on the chosen disk, lets the user choose "update everything" or pick
individual items, downloads the artifacts, stages them, and runs
`tools/hdbuild/hdbuild.py update` so user files are preserved.

Pure stdlib (tkinter + urllib) so it runs on a plain Python install. The
`websocket` live feed is optional; without it the updater polls /api/catalog.

Run:
    python otaserver/updater/plusos_updater.py
"""
from __future__ import annotations

import os
import sys
import json
import threading
import tempfile
import subprocess
import urllib.request
import urllib.error

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
HDBUILD = os.path.join(REPO, "tools", "hdbuild", "hdbuild.py")

DEFAULT_SERVER = os.environ.get("OTA_SERVER", "https://ota.d0a.net")
STATE_FILE = os.path.join(HERE, ".updater_state.json")


def load_state() -> dict:
    try:
        with open(STATE_FILE, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except Exception:
        return {}


def save_state(st: dict) -> None:
    try:
        with open(STATE_FILE, "w", encoding="utf-8") as fh:
            json.dump(st, fh)
    except Exception:
        pass


def http_json(url, timeout=30):
    with urllib.request.urlopen(url, timeout=timeout) as r:
        return json.loads(r.read())


def http_download(url, dest, timeout=300):
    with urllib.request.urlopen(url, timeout=timeout) as r, open(dest, "wb") as fh:
        while True:
            chunk = r.read(1 << 20)
            if not chunk:
                break
            fh.write(chunk)


class Updater(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("PlusOS Updater")
        self.geometry("760x620")
        self.configure(bg="#ffffff")
        self.state = load_state()
        self.catalog = None
        self.applied = set(self.state.get("applied_releases", []))
        self._build_ui()
        self.after(200, self.refresh)

    # ---------------- UI ----------------
    def _build_ui(self):
        top = tk.Frame(self, bg="#fff"); top.pack(fill="x", padx=10, pady=8)
        tk.Label(top, text="PlusOS Software Update", font=("Chicago", 16, "bold"), bg="#fff").pack(side="left")
        self.conn = tk.Label(top, text="…", bg="#fff"); self.conn.pack(side="right")

        cfg = tk.LabelFrame(self, text="Source & Target", bg="#fff"); cfg.pack(fill="x", padx=10)
        tk.Label(cfg, text="Server:", bg="#fff").grid(row=0, column=0, sticky="e")
        self.server_var = tk.StringVar(value=self.state.get("server", DEFAULT_SERVER))
        tk.Entry(cfg, textvariable=self.server_var, width=42).grid(row=0, column=1, sticky="w", padx=4)

        tk.Label(cfg, text="Channel:", bg="#fff").grid(row=0, column=2, sticky="e")
        self.channel_var = tk.StringVar(value=self.state.get("channel", "stable"))
        ttk.Combobox(cfg, textvariable=self.channel_var, width=9,
                     values=["stable", "beta", "nightly"]).grid(row=0, column=3, padx=4)

        tk.Label(cfg, text="Machine:", bg="#fff").grid(row=1, column=2, sticky="e")
        self.machine_var = tk.StringVar(value=self.state.get("machine", "se"))
        ttk.Combobox(cfg, textvariable=self.machine_var, width=9,
                     values=["se", "plus", "classic", "lc3", "universal"]).grid(row=1, column=3, padx=4)

        tk.Label(cfg, text="Disk / SD card:", bg="#fff").grid(row=1, column=0, sticky="e")
        self.target_var = tk.StringVar(value=self.state.get("target", ""))
        tk.Entry(cfg, textvariable=self.target_var, width=42).grid(row=1, column=1, sticky="w", padx=4)
        tk.Button(cfg, text="Browse…", command=self.browse).grid(row=1, column=4, padx=4)
        tk.Button(cfg, text="Check for Updates", command=self.refresh).grid(row=0, column=4, padx=4)

        mid = tk.LabelFrame(self, text="Available Updates", bg="#fff"); mid.pack(fill="both", expand=True, padx=10, pady=6)
        cols = ("sel", "release", "item", "kind", "where", "size", "status")
        self.tree = ttk.Treeview(mid, columns=cols, show="headings", height=12)
        for c, w in zip(cols, (40, 150, 130, 70, 150, 70, 90)):
            self.tree.heading(c, text=c.title())
            self.tree.column(c, width=w, anchor="w")
        self.tree.pack(fill="both", expand=True, side="left")
        sb = ttk.Scrollbar(mid, command=self.tree.yview); sb.pack(side="right", fill="y")
        self.tree.configure(yscrollcommand=sb.set)
        self.tree.bind("<Button-1>", self._toggle_row)
        self.row_items = {}  # iid -> artifact dict

        btns = tk.Frame(self, bg="#fff"); btns.pack(fill="x", padx=10)
        tk.Button(btns, text="Select All New", command=lambda: self.select(all_new=True)).pack(side="left")
        tk.Button(btns, text="Select All", command=lambda: self.select(all_=True)).pack(side="left", padx=4)
        tk.Button(btns, text="Select None", command=lambda: self.select(none=True)).pack(side="left")
        tk.Button(btns, text="Update Selected", command=self.apply_updates,
                  font=("Chicago", 11, "bold")).pack(side="right")

        self.notes = tk.Text(self, height=4, wrap="word", bg="#f7f7f7"); self.notes.pack(fill="x", padx=10, pady=4)
        self.notes.insert("1.0", "Select a release to see its notes.")
        self.tree.bind("<<TreeviewSelect>>", self._show_notes)

        self.logbox = scrolledtext.ScrolledText(self, height=8, bg="#000", fg="#0f0",
                                                 font=("Consolas", 9)); self.logbox.pack(fill="both", padx=10, pady=(0, 8))
        self.log("PlusOS Updater ready.")

    def log(self, msg):
        self.logbox.insert("end", msg + "\n"); self.logbox.see("end"); self.update_idletasks()

    def browse(self):
        p = filedialog.askopenfilename(title="Choose PlusOS disk image or BlueSCSI .hda",
                                       filetypes=[("Disk images", "*.hda *.img *.dsk"), ("All", "*.*")])
        if p:
            self.target_var.set(p)

    def _persist(self):
        self.state.update({"server": self.server_var.get(), "channel": self.channel_var.get(),
                           "machine": self.machine_var.get(), "target": self.target_var.get(),
                           "applied_releases": list(self.applied)})
        save_state(self.state)

    # ---------------- catalog ----------------
    def refresh(self):
        self._persist()
        threading.Thread(target=self._refresh_worker, daemon=True).start()

    def _refresh_worker(self):
        server = self.server_var.get().rstrip("/")
        url = (server + "/api/catalog?channel=" + self.channel_var.get() +
               "&machine=" + self.machine_var.get())
        try:
            self.catalog = http_json(url)
            self.conn.config(text="● online", fg="#080")
            self.after(0, self._render)
        except Exception as e:
            self.conn.config(text="● offline", fg="#a00")
            self.after(0, lambda: self.log("cannot reach server: %s" % e))

    def _render(self):
        self.tree.delete(*self.tree.get_children())
        self.row_items.clear()
        rels = (self.catalog or {}).get("releases", [])
        if not rels:
            self.log("No updates on channel '%s'." % self.channel_var.get())
            return
        for r in rels:
            new = r["id"] not in self.applied
            status = "NEW" if new else "applied"
            parent = self.tree.insert("", "end", values=(
                "", "%s v%s" % (r.get("title") or r["channel"], r["version"]),
                "", "", "", self._fmt(r["total_bytes"]), status))
            self.row_items[parent] = {"release": r, "is_header": True, "checked": False}
            for a in r["artifacts"]:
                iid = self.tree.insert(parent, "end", values=(
                    "☐", "", a["name"], a["kind"], a.get("path") or "—",
                    self._fmt(a["size"]), status))
                self.row_items[iid] = {"release": r, "art": a, "checked": False}
            self.tree.item(parent, open=True)
        self.log("%d release(s) available." % len(rels))

    def _fmt(self, n):
        if n is None:
            return "?"
        u = ["B", "KB", "MB", "GB"]; i = 0
        while n >= 1024 and i < len(u) - 1:
            n /= 1024; i += 1
        return "%.0f %s" % (n, u[i]) if i == 0 else "%.1f %s" % (n, u[i])

    def _toggle_row(self, event):
        iid = self.tree.identify_row(event.y)
        if not iid or iid not in self.row_items:
            return
        info = self.row_items[iid]
        if info.get("is_header"):
            # toggle all children
            want = not info["checked"]
            info["checked"] = want
            for child in self.tree.get_children(iid):
                self._set_checked(child, want)
        else:
            self._set_checked(iid, not info["checked"])

    def _set_checked(self, iid, val):
        info = self.row_items.get(iid)
        if not info or info.get("is_header"):
            return
        info["checked"] = val
        vals = list(self.tree.item(iid, "values"))
        vals[0] = "☑" if val else "☐"
        self.tree.item(iid, values=vals)

    def select(self, all_=False, all_new=False, none=False):
        for iid, info in self.row_items.items():
            if info.get("is_header"):
                continue
            if none:
                self._set_checked(iid, False)
            elif all_:
                self._set_checked(iid, True)
            elif all_new:
                self._set_checked(iid, info["release"]["id"] not in self.applied)

    def _show_notes(self, _evt):
        sel = self.tree.selection()
        if not sel:
            return
        info = self.row_items.get(sel[0])
        if not info:
            return
        r = info["release"]
        self.notes.delete("1.0", "end")
        self.notes.insert("1.0", "%s  (v%s, %s)\n\n%s" % (
            r.get("title") or r["channel"], r["version"], r.get("machine") or "any",
            r.get("notes") or "(no notes)"))

    # ---------------- apply ----------------
    def apply_updates(self):
        chosen = [info for info in self.row_items.values()
                  if not info.get("is_header") and info["checked"]]
        if not chosen:
            messagebox.showinfo("PlusOS Updater", "Nothing selected.")
            return
        target = self.target_var.get().strip()
        if not target or not os.path.isfile(target):
            messagebox.showerror("PlusOS Updater", "Choose an existing disk image / SD-card .hda first.")
            return
        if not os.path.isfile(HDBUILD):
            messagebox.showerror("PlusOS Updater", "hdbuild.py not found at %s" % HDBUILD)
            return
        self._persist()
        threading.Thread(target=self._apply_worker, args=(chosen, target), daemon=True).start()

    def _apply_worker(self, chosen, target):
        server = self.server_var.get().rstrip("/")
        tmp = tempfile.mkdtemp(prefix="plusos_ota_")
        self.log("Downloading %d item(s) to %s" % (len(chosen), tmp))
        # Separate kernel/loader/apps from repos/files; map each to a staging path.
        kernel = loader = None
        app_overlay = os.path.join(tmp, "overlay")
        os.makedirs(os.path.join(app_overlay, "bin"), exist_ok=True)
        touched_releases = set()
        try:
            for info in chosen:
                a = info["art"]; r = info["release"]
                touched_releases.add(r["id"])
                dest = os.path.join(tmp, a["sha256"] + "_" + a["name"])
                self.log("  ↓ %s (%s)" % (a["name"], a["kind"]))
                http_download(server + "/api/blob/" + a["sha256"], dest)
                if a["kind"] == "kernel":
                    kernel = dest
                elif a["kind"] == "loader":
                    loader = dest
                elif a["kind"] == "app":
                    # place into overlay/bin/<NAME>
                    import shutil
                    shutil.copy2(dest, os.path.join(app_overlay, "bin", a["name"]))
                elif a["kind"] in ("repo", "file"):
                    import shutil
                    sub = (a.get("path") or "/repos/").strip("/")
                    outp = os.path.join(app_overlay, *sub.split("/"))
                    os.makedirs(os.path.dirname(outp), exist_ok=True)
                    shutil.copy2(dest, outp)

            cmd = [sys.executable, HDBUILD, "update", target]
            if kernel:
                cmd += ["--kernel", kernel]
            if loader:
                cmd += ["--loader", loader]
            # overlay carries new apps / repos; hdbuild merges it over the volume
            if os.listdir(os.path.join(app_overlay, "bin")) or \
               any(d != "bin" for d in os.listdir(app_overlay)):
                cmd += ["--overlay", app_overlay]

            self.log("Running: " + " ".join('"%s"' % c if " " in c else c for c in cmd))
            proc = subprocess.Popen(cmd, cwd=REPO, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, text=True)
            for line in proc.stdout:
                self.log("  " + line.rstrip())
            proc.wait()
            if proc.returncode == 0:
                self.applied |= touched_releases
                self._persist()
                self.log("✓ Update applied. User files preserved.")
                self.after(0, lambda: messagebox.showinfo("PlusOS Updater", "Update complete!"))
                self.after(0, self._render)
            else:
                self.log("✗ hdbuild update failed (exit %d)." % proc.returncode)
                self.after(0, lambda: messagebox.showerror("PlusOS Updater",
                           "Update failed — see the log."))
        except Exception as e:
            self.log("✗ error: %s" % e)
            self.after(0, lambda: messagebox.showerror("PlusOS Updater", str(e)))
        finally:
            import shutil
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    Updater().mainloop()
