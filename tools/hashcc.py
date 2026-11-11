#!/usr/bin/env python3
"""
tools/hashcc.py  –  Smart MD5 build ledger for PlusOS.

MODES
─────
Wrapper mode  (invoked per-compile by the Makefile's SMARTCC variable):

    python3 tools/hashcc.py <compiler> [flags...] -c <src> -o <obj>

    Wraps a single compiler call.  Looks up the source in the central ledger;
    if the content hash matches the last successful compile, touches <obj> and
    skips the compiler.  On a cache miss it runs the compiler and updates the
    ledger with the new hash.

Pre-scan mode  (run once before the build begins):

    python3 tools/hashcc.py --prescan [--root DIR] [--machine NAME]

    Scans all source trees, computes MD5s for every .c/.cpp/.S/.s file,
    compares against the central ledger, and pre-touches .o files for unchanged
    sources.  make sees those .o files as newer than their sources and skips
    those compiler invocations entirely.

    This is the "compile environment instructions" pass: before the build
    starts it determines exactly which files have changed and reports a
    per-file build plan.

Dump mode  (inspect the ledger):

    python3 tools/hashcc.py --dump [--root DIR] [--machine NAME]

    Prints a human-readable table showing every entry's current status
    (OK / CHANGED / MISSING) so you can see the full ledger as a document.

Central ledger
─────────────
    build/.build_ledger.json
    JSON dict  "relative/source/path" → entry.
    Entry: { "hash": "<hex>", "obj": "<abs>", "deps": [...], "machine": "se" }
    Written atomically (temp-file + rename) so parallel -j builds are safe.
"""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

LEDGER_REL = os.path.join("build", ".build_ledger.json")
LEDGER_VER = 2

CACHE_EXT = ".hashcc"   # legacy per-object sidecar (kept for backward compat)
HIT_EXT   = ".hit"      # touch-file: present when the .o was served from cache

SRC_EXTS  = {".c", ".cpp", ".cxx", ".cc", ".S", ".s", ".asm"}


# ---------------------------------------------------------------------------
# Hashing helpers
# ---------------------------------------------------------------------------

def _md5_path(path: str):
    """Return the hex MD5 of a file, or None if unreadable."""
    try:
        h = hashlib.md5()
        with open(path, "rb") as fh:
            for chunk in iter(lambda: fh.read(65536), b""):
                h.update(chunk)
        return h.hexdigest()
    except OSError:
        return None


def _read_dep_file(dep_path: str):
    """
    Parse a GCC -MMD .d file; return all prerequisite paths.

    Format example:
        build/kernel/main.c.o: kernel/main.c include/types.h \\
                               include/util.h
        include/types.h:
    """
    try:
        with open(dep_path, errors="replace") as fh:
            raw = fh.read()
    except OSError:
        return []

    colon = raw.find(":")
    if colon < 0:
        return []

    deps_part = raw[colon + 1:].replace("\\\n", " ")
    return [t for t in deps_part.split() if t and t != "\\"]


def _dep_path_for(obj: str) -> str:
    """Derive the GCC -MMD .d file path from an object file path."""
    base, ext = os.path.splitext(obj)
    return (base + ".d") if ext == ".o" else (obj + ".d")


def _compute_input_hash(src: str, dep_path: str) -> "str | None":
    """
    MD5 over: source content + content of every header listed in dep_path.
    Returns None if the source file cannot be read (force recompile).
    """
    h = hashlib.md5()

    src_hash = _md5_path(src)
    if src_hash is None:
        return None
    h.update(f"src:{src}:{src_hash}\n".encode())

    for dep in sorted(_read_dep_file(dep_path)):
        dh = _md5_path(dep)
        if dh:
            h.update(f"dep:{dep}:{dh}\n".encode())

    return h.hexdigest()


# ---------------------------------------------------------------------------
# Central Ledger
# ---------------------------------------------------------------------------

class Ledger:
    """
    Central build ledger stored as a single JSON file.

    Ledger schema (version 2)
    ─────────────────────────
    {
      "version": 2,
      "entries": {
        "kernel/core/main.c": {
          "hash":    "<hex md5 of src+headers>",
          "obj":     "<absolute path to .o>",
          "deps":    ["<abs path to header>", ...],
          "machine": "se"
        },
        ...
      }
    }

    Writes are atomic via temp-file + os.replace so -j parallel builds
    never corrupt the file (last writer wins, which is acceptable for a cache).
    """

    def __init__(self, path: str):
        self.path = path
        self.data: dict = {"version": LEDGER_VER, "entries": {}}

    def load(self) -> bool:
        """Load from disk.  Returns False if absent or corrupt."""
        try:
            with open(self.path, "r", encoding="utf-8") as fh:
                raw = json.load(fh)
            if isinstance(raw, dict) and "entries" in raw:
                self.data = raw
                return True
        except (OSError, json.JSONDecodeError, ValueError):
            pass
        return False

    def save(self):
        """Atomically write ledger to disk."""
        os.makedirs(os.path.dirname(self.path) or ".", exist_ok=True)
        tmp = self.path + ".tmp"
        try:
            with open(tmp, "w", encoding="utf-8") as fh:
                json.dump(self.data, fh, indent=2, sort_keys=True)
            os.replace(tmp, self.path)
        except OSError:
            try:
                os.remove(tmp)
            except OSError:
                pass

    @property
    def entries(self) -> dict:
        return self.data.setdefault("entries", {})

    def upsert(self, src_rel: str, hash_val: str, obj_abs: str,
               deps: list, machine: str):
        """Update or insert a ledger entry after a successful compile."""
        self.entries[src_rel] = {
            "hash":    hash_val,
            "obj":     obj_abs,
            "deps":    sorted(deps),
            "machine": machine,
        }

    def get(self, src_rel: str) -> "dict | None":
        return self.entries.get(src_rel)


def _repo_root() -> str:
    """Derive the repo root from this script's location (tools/hashcc.py)."""
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _src_rel(src_abs: str, root: str) -> str:
    """Return src path relative to repo root (always with / separators)."""
    try:
        rel = os.path.relpath(src_abs, root)
        return rel.replace("\\", "/")
    except ValueError:
        return src_abs.replace("\\", "/")


# ---------------------------------------------------------------------------
# Wrapper mode  (called per-compile from Makefile's SMARTCC variable)
# ---------------------------------------------------------------------------

def wrapper_mode(argv: list) -> int:
    """
    argv[0] = compiler binary; the rest = compiler arguments.
    Checks the central ledger; on hit, touches .o and returns 0 without
    invoking the compiler.  On miss, compiles and updates the ledger.
    """
    root    = _repo_root()
    machine = os.environ.get("MACHINE", "se")

    # Find -c SRC and -o OBJ in the argument list
    src = obj = None
    for i, a in enumerate(argv):
        if a == "-c" and i + 1 < len(argv):
            src = argv[i + 1]
        elif a == "-o" and i + 1 < len(argv):
            obj = argv[i + 1]

    # Pass-through: link steps, preprocessing, etc. don't have both -c and -o
    if not src or not obj:
        return subprocess.call(argv)

    dep_path   = _dep_path_for(obj)
    cache_file = obj + CACHE_EXT
    hit_file   = obj + HIT_EXT

    src_abs = os.path.abspath(src)
    obj_abs = os.path.abspath(obj)
    src_key = _src_rel(src_abs, root)

    # --- Check central ledger ------------------------------------------------
    ledger = Ledger(os.path.join(root, LEDGER_REL))
    ledger.load()

    entry = ledger.get(src_key)
    if entry and entry.get("machine") == machine and os.path.exists(obj_abs):
        cached = entry.get("hash", "")
        if cached:
            current = _compute_input_hash(src_abs, dep_path)
            if current and current == cached:
                try:
                    os.utime(obj_abs, None)
                    with open(hit_file, "w") as fh:
                        fh.write("1")
                except OSError:
                    pass
                return 0  # cache hit — skip compiler

    # --- Legacy .hashcc sidecar (for objects built before ledger existed) ----
    if os.path.exists(obj_abs) and not entry:
        try:
            with open(cache_file) as fh:
                cached_legacy = fh.read().strip()
        except OSError:
            cached_legacy = None

        if cached_legacy:
            current = _compute_input_hash(src_abs, dep_path)
            if current and current == cached_legacy:
                try:
                    os.utime(obj_abs, None)
                    with open(hit_file, "w") as fh:
                        fh.write("1")
                except OSError:
                    pass
                return 0

    # --- Cache miss: compile -------------------------------------------------
    ret = subprocess.call(argv)

    if ret == 0:
        new_hash = _compute_input_hash(src_abs, dep_path)
        deps     = _read_dep_file(dep_path)
        if new_hash:
            # Update central ledger
            ledger.upsert(src_key, new_hash, obj_abs, deps, machine)
            ledger.save()
            # Update legacy sidecar
            try:
                with open(cache_file, "w") as fh:
                    fh.write(new_hash)
                try:
                    os.remove(hit_file)
                except OSError:
                    pass
            except OSError:
                pass

    return ret


# ---------------------------------------------------------------------------
# Pre-scan mode  (run once before the build; generates the build plan)
# ---------------------------------------------------------------------------

def prescan_mode(root: str, machine: str, verbose: bool) -> int:
    """
    Load the central ledger, compute current MD5s for every tracked source,
    and pre-touch .o files whose content is unchanged so make skips them.

    Outputs a human-readable "compile environment instructions" report:
      - which files are cached (will be skipped)
      - which files changed or are new (will be recompiled)
    """
    ledger_path = os.path.join(root, LEDGER_REL)
    ledger = Ledger(ledger_path)
    has_ledger = ledger.load()

    if not has_ledger or not ledger.entries:
        print("  hashcc: ledger empty — first build will populate it.")
        return 0

    # Filter to this machine's entries
    entries = {k: v for k, v in ledger.entries.items()
               if v.get("machine") == machine}

    if not entries:
        print(f"  hashcc: no ledger entries for machine={machine}")
        return 0

    skip_list    = []
    build_list   = []
    no_obj_list  = []

    for src_rel, entry in sorted(entries.items()):
        src_abs = os.path.join(root, src_rel.replace("/", os.sep))
        obj_abs = entry.get("obj", "")
        cached  = entry.get("hash", "")

        if not obj_abs or not os.path.exists(obj_abs):
            no_obj_list.append(src_rel)
            continue

        dep_path = _dep_path_for(obj_abs)
        current  = _compute_input_hash(src_abs, dep_path)

        if current and current == cached:
            try:
                os.utime(obj_abs, None)   # mark .o as up-to-date for make
                skip_list.append(src_rel)
            except OSError:
                build_list.append((src_rel, "touch failed"))
        else:
            # Determine why it needs recompiling
            reason = "source changed"
            if current is None:
                reason = "source unreadable"
            elif cached != current:
                # Check if a header changed
                for dep in _read_dep_file(dep_path):
                    dh = _md5_path(dep)
                    if dh:
                        # We can't easily tell which header changed without storing
                        # per-dep hashes, but we know something changed
                        reason = "source or header changed"
                        break
            build_list.append((src_rel, reason))

    total      = len(skip_list) + len(build_list) + len(no_obj_list)
    will_build = len(build_list) + len(no_obj_list)
    pct_skip   = int(100 * len(skip_list) / total) if total else 0

    # ── Compile environment instructions report ──────────────────────────────
    print(f"  hashcc prescan [{machine}]: {len(skip_list)}/{total} cached "
          f"({pct_skip}% skip) — {will_build} file(s) to compile")

    if verbose or build_list or no_obj_list:
        # Always show what WILL be compiled (the actionable part)
        if no_obj_list:
            print(f"  → NEW ({len(no_obj_list)} files — no prior .o):")
            for s in no_obj_list[:10]:
                print(f"      {s}")
            if len(no_obj_list) > 10:
                print(f"      ... and {len(no_obj_list) - 10} more")

        if build_list:
            print(f"  → RECOMPILE ({len(build_list)} files):")
            for s, reason in build_list[:10]:
                print(f"      {s}  [{reason}]")
            if len(build_list) > 10:
                print(f"      ... and {len(build_list) - 10} more")

        if verbose and skip_list:
            print(f"  → SKIP ({len(skip_list)} files, hash unchanged):")
            for s in skip_list[:20]:
                print(f"      {s}")
            if len(skip_list) > 20:
                print(f"      ... and {len(skip_list) - 20} more")

    return 0


# ---------------------------------------------------------------------------
# Dump mode  (inspect the ledger as a human-readable document)
# ---------------------------------------------------------------------------

def dump_mode(root: str, machine: str) -> int:
    """
    Print the full ledger as a human-readable table — the 'document'
    showing every tracked source file and its build status.
    """
    ledger_path = os.path.join(root, LEDGER_REL)
    ledger = Ledger(ledger_path)
    if not ledger.load():
        print(f"No ledger found at: {ledger_path}")
        print("Build the project once to generate it.")
        return 1

    all_entries = ledger.entries
    if machine:
        entries = {k: v for k, v in all_entries.items()
                   if v.get("machine") == machine}
        print(f"Build Ledger: {ledger_path}")
        print(f"Machine: {machine}  |  {len(entries)}/{len(all_entries)} entries")
    else:
        entries = all_entries
        print(f"Build Ledger: {ledger_path}")
        print(f"{len(entries)} total entries across all machines")

    if not entries:
        print("(empty)")
        return 0

    print()
    col_src = 48
    col_hash = 10
    hdr = f"{'STATUS':<8}  {'HASH':<{col_hash}}  {'SOURCE':<{col_src}}  OBJ"
    print(hdr)
    print("-" * (len(hdr) + 20))

    counts = {"OK": 0, "CHANGED": 0, "MISSING": 0, "GONE": 0}

    for src_rel, entry in sorted(entries.items()):
        src_abs  = os.path.join(root, src_rel.replace("/", os.sep))
        obj_abs  = entry.get("obj", "")
        cached   = entry.get("hash", "")
        h8       = cached[:col_hash] if cached else "—"

        obj_tag = ""
        if not obj_abs:
            status  = "MISSING"
            obj_tag = "(no obj path)"
        elif not os.path.exists(obj_abs):
            status  = "MISSING"
            obj_tag = "(obj gone)"
        elif not os.path.exists(src_abs):
            status  = "GONE"
            obj_tag = "(src deleted)"
        else:
            dep_path = _dep_path_for(obj_abs)
            current  = _compute_input_hash(src_abs, dep_path)
            if current and current == cached:
                status  = "OK"
                obj_tag = os.path.relpath(obj_abs, root).replace("\\", "/")
            else:
                status  = "CHANGED"
                obj_tag = os.path.relpath(obj_abs, root).replace("\\", "/")

        counts[status] = counts.get(status, 0) + 1
        src_disp = src_rel[:col_src] if len(src_rel) > col_src else src_rel
        print(f"  {status:<6}  {h8:<{col_hash}}  {src_disp:<{col_src}}  {obj_tag}")

    print()
    parts = [f"{v} {k.lower()}" for k, v in counts.items() if v]
    print("Summary: " + ", ".join(parts))
    print()
    print("Tip: run  python3 tools/hashcc.py --prescan  to pre-touch cached .o files.")
    return 0


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    # Detect mode: tool-mode flags start with '--'
    if len(sys.argv) > 1 and sys.argv[1].startswith("--"):
        p = argparse.ArgumentParser(
            prog="hashcc.py",
            description="PlusOS smart MD5 build ledger",
        )
        g = p.add_mutually_exclusive_group(required=True)
        g.add_argument("--prescan", action="store_true",
                       help="Pre-scan source trees, touch cached .o files, "
                            "print compile-environment instructions")
        g.add_argument("--dump", action="store_true",
                       help="Dump ledger as a human-readable document/table")
        p.add_argument("--root",    default=None,
                       help="Repo root (default: parent directory of this script)")
        p.add_argument("--machine", default=os.environ.get("MACHINE", "se"),
                       help="Target machine name (default: $MACHINE or 'se')")
        p.add_argument("--verbose", "-v", action="store_true",
                       help="Print every file, including unchanged ones")
        args = p.parse_args()

        root = args.root or _repo_root()

        if args.prescan:
            sys.exit(prescan_mode(root, args.machine, args.verbose))
        if args.dump:
            sys.exit(dump_mode(root, args.machine))

    else:
        # Wrapper mode: sys.argv[1:] = <compiler> [flags...]
        argv = sys.argv[1:]
        if not argv:
            sys.stderr.write("hashcc.py: no compiler specified\n")
            sys.exit(1)
        sys.exit(wrapper_mode(argv))


if __name__ == "__main__":
    main()
