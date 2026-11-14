"""AI assistant for the OTA server.

Powers two things:
  * auto-generated, human-readable messages about each update (release notes
    and per-app "what changed" blurbs), produced at publish time;
  * a chat endpoint so people can ask the assistant about PlusOS / the updates.

Model: LiquidAI **LFM2-2.6B** (a genuinely small, capable ~2.6B model — the
closest real Liquid Foundation Model to the "~2-3B" size requested). It is
loaded through whichever local runtime is available, tried in this order:

  1. An OpenAI-compatible HTTP endpoint (llama.cpp server / Ollama / LM Studio)
     at $OTA_AI_BASE_URL — zero Python deps, best for a GPU box. Set
     $OTA_AI_MODEL to the served model name (default 'lfm2-2.6b').
  2. transformers + torch, loading $OTA_AI_MODEL_ID
     (default 'LiquidAI/LFM2-2.6B') locally in-process.
  3. A deterministic template fallback that still writes decent notes with no
     model at all, so publishing never blocks on AI being installed.

Everything is synchronous and offloaded to a thread by the server.
"""
from __future__ import annotations

import os
import json
import threading
import urllib.request
import urllib.error

# ---- configuration -----------------------------------------------------------
AI_BASE_URL = os.environ.get("OTA_AI_BASE_URL")            # e.g. http://127.0.0.1:11434/v1
AI_API_KEY = os.environ.get("OTA_AI_API_KEY", "none")
AI_MODEL = os.environ.get("OTA_AI_MODEL", "lfm2-2.6b")     # served name for the HTTP path
AI_MODEL_ID = os.environ.get("OTA_AI_MODEL_ID", "LiquidAI/LFM2-2.6B")  # HF id for transformers
AI_MAX_TOKENS = int(os.environ.get("OTA_AI_MAX_TOKENS", "384"))
# Loading a multi-GB local model is opt-in: it can take minutes and download
# weights. Set OTA_AI_LOCAL=1 to allow the in-process transformers backend.
AI_ALLOW_LOCAL = os.environ.get("OTA_AI_LOCAL", "") in ("1", "true", "yes", "on")

SYSTEM_PROMPT = (
    "You are the PlusOS Update Assistant. PlusOS is a small, from-scratch "
    "operating system for vintage 68k Macintoshes (Mac Plus, SE/30, Classic, "
    "LC III). You write short, friendly, concrete notes about software "
    "updates and answer user questions about them. Keep a retro, enthusiastic "
    "tone but stay accurate and never invent version numbers or features you "
    "were not told about."
)

_lock = threading.Lock()
_transformers = None          # lazily-loaded (model, tokenizer) tuple or False
_backend_cache = None         # 'http' | 'transformers' | 'template'


# ---- backend detection -------------------------------------------------------
def backend() -> str:
    global _backend_cache
    if _backend_cache:
        return _backend_cache
    if AI_BASE_URL:
        _backend_cache = "http"
    elif _try_load_transformers():
        _backend_cache = "transformers"
    else:
        _backend_cache = "template"
    return _backend_cache


def status() -> dict:
    b = backend()
    return {
        "backend": b,
        "model": AI_MODEL if b == "http" else AI_MODEL_ID if b == "transformers" else "template",
        "base_url": AI_BASE_URL,
        "available": b != "template",
    }


def _try_load_transformers() -> bool:
    global _transformers
    if _transformers is not None:
        return bool(_transformers)
    if not AI_ALLOW_LOCAL:
        _transformers = False
        return False
    try:
        import torch  # noqa
        from transformers import AutoModelForCausalLM, AutoTokenizer
    except Exception:
        _transformers = False
        return False
    try:
        tok = AutoTokenizer.from_pretrained(AI_MODEL_ID)
        model = AutoModelForCausalLM.from_pretrained(
            AI_MODEL_ID, torch_dtype="auto", device_map="auto")
        _transformers = (model, tok)
        return True
    except Exception as e:
        print("[ai] transformers load failed (%s); using template fallback" % e)
        _transformers = False
        return False


# ---- generation --------------------------------------------------------------
def chat(messages: list[dict], max_tokens: int = AI_MAX_TOKENS,
         temperature: float = 0.6) -> str:
    """messages: [{role, content}, ...]. Returns the assistant text."""
    b = backend()
    if b == "http":
        return _chat_http(messages, max_tokens, temperature)
    if b == "transformers":
        return _chat_transformers(messages, max_tokens, temperature)
    return _chat_template(messages)


def _chat_http(messages, max_tokens, temperature) -> str:
    url = AI_BASE_URL.rstrip("/") + "/chat/completions"
    body = json.dumps({"model": AI_MODEL, "messages": messages,
                       "max_tokens": max_tokens, "temperature": temperature}).encode()
    req = urllib.request.Request(url, data=body, method="POST", headers={
        "Content-Type": "application/json",
        "Authorization": "Bearer " + AI_API_KEY})
    try:
        with urllib.request.urlopen(req, timeout=120) as r:
            data = json.loads(r.read())
        return data["choices"][0]["message"]["content"].strip()
    except Exception as e:
        return "(AI endpoint error: %s)\n\n" % e + _chat_template(messages)


def _chat_transformers(messages, max_tokens, temperature) -> str:
    with _lock:
        model, tok = _transformers
        import torch
        prompt = tok.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
        inputs = tok(prompt, return_tensors="pt").to(model.device)
        with torch.no_grad():
            out = model.generate(**inputs, max_new_tokens=max_tokens,
                                 temperature=temperature, do_sample=temperature > 0,
                                 repetition_penalty=1.05)
        text = tok.decode(out[0][inputs["input_ids"].shape[1]:], skip_special_tokens=True)
        return text.strip()


def _chat_template(messages) -> str:
    """No model available: give a useful, deterministic reply."""
    user = next((m["content"] for m in reversed(messages) if m["role"] == "user"), "")
    return ("The PlusOS Update Assistant is running without a language model "
            "installed, so I can only give canned replies. Install LFM2-2.6B "
            "(set OTA_AI_BASE_URL to a llama.cpp/Ollama server, or pip install "
            "transformers torch) for real answers.\n\nYou asked: " + user[:200])


# ---- high-level helpers used at publish time ---------------------------------
def release_message(channel: str, version: str, machine: str,
                    artifacts: list[dict], raw_notes: str = "") -> str:
    """One friendly paragraph summarising a whole release, for the catalog."""
    items = ", ".join(sorted({a.get("name", "?") for a in artifacts}))
    kinds = {}
    for a in artifacts:
        kinds[a.get("kind", "file")] = kinds.get(a.get("kind", "file"), 0) + 1
    kindline = ", ".join("%d %s" % (n, k) for k, n in kinds.items())
    prompt = (
        "Write a short (2-4 sentence) update announcement for PlusOS users.\n"
        "Channel: %s   Version: %s   Target machine: %s\n"
        "Contents: %s (%s)\n"
        "%s"
        "Describe what the user gets and why it's worth updating. No markdown headers."
        % (channel, version, machine or "all", items, kindline,
           ("Maintainer notes:\n%s\n" % raw_notes) if raw_notes else "")
    )
    if backend() == "template":
        base = raw_notes.strip() or ("PlusOS %s is here for %s." % (version, machine or "all machines"))
        return "%s This update bundles %s (%s). Grab it from the Updater to refresh without reflashing." % (
            base, items, kindline)
    return chat([{"role": "system", "content": SYSTEM_PROMPT},
                 {"role": "user", "content": prompt}], max_tokens=256, temperature=0.5)


def app_changelog(app_name: str, version: str, raw_notes: str = "",
                  diffstat: str = "") -> str:
    """Markdown doc describing what changed in one application."""
    prompt = (
        "Write a concise Markdown changelog entry for the PlusOS application "
        "'%s' in version %s. Use a short intro sentence then a bullet list of "
        "likely user-facing changes. Keep it believable and specific to what "
        "the notes/diff say; do not invent unrelated features.\n"
        "%s%s"
        % (app_name, version,
           ("Maintainer notes:\n%s\n" % raw_notes) if raw_notes else "",
           ("Diffstat:\n%s\n" % diffstat) if diffstat else "")
    )
    if backend() == "template":
        body = raw_notes.strip() or "Rebuilt for PlusOS %s." % version
        return ("# %s — %s\n\n%s\n\n- Recompiled against the latest SDK\n"
                "- Bundled in release %s\n" % (app_name, version, body, version))
    md = chat([{"role": "system", "content": SYSTEM_PROMPT},
               {"role": "user", "content": prompt}], max_tokens=320, temperature=0.5)
    if not md.lstrip().startswith("#"):
        md = "# %s — %s\n\n%s" % (app_name, version, md)
    return md
