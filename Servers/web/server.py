#!/usr/bin/env python3
"""
PlusOS 2.0 Web, Wiki, and Tester Operations Server
Created for plusos.d0a.net by the1Domo

A self-contained, zero-external-dependency Python HTTP server that hosts:
- PlusOS distribution landing page with hacker background art & matrix rain
- Multi-user authentication & role-based permissions (Admin, Wiki, Tester)
- Super Admin Control Panel (user management, page copy editor, wiki manager, tester manager)
- Dedicated Hardware Tester Operations Hub (build downloads, bug reporting, reviews, developer responses)
- Interactive technical documentation Wiki with permission-gated editor
"""

import sys
import os
import json
import re
import html
import time
import secrets
import hashlib
import mimetypes
from pathlib import Path
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs, unquote
from http.cookies import SimpleCookie

BASE_DIR = Path(__file__).resolve().parent
PROJECT_DIR = BASE_DIR.parent
CONFIG_PATH = BASE_DIR / "config.json"
DATA_DIR = BASE_DIR / "data"
WIKI_DIR = BASE_DIR / "wiki"
TEMPLATES_DIR = BASE_DIR / "templates"
STATIC_DIR = BASE_DIR / "static"
DOWNLOADS_DIR = BASE_DIR / "downloads"
# Keep served asset directories INSIDE the PlusOS project. Previously these
# resolved to PROJECT_DIR (the parent of PlusOS), which on a shared host points
# at an unrelated application's private docs — an info-disclosure hole. The
# per-request handlers also containment-check resolved paths against these roots.
DOCS_DIR = BASE_DIR / "docs"
IMAGES_DIR = BASE_DIR / "images"

USERS_FILE = DATA_DIR / "users.json"
TESTS_FILE = DATA_DIR / "tests.json"
FEEDBACK_FILE = DATA_DIR / "feedback.json"
CONTENT_FILE = DATA_DIR / "site_content.json"

# In-memory session store: token -> { username, role, display_name, expires }
SESSIONS = {}
SESSION_TTL = 7 * 86400  # 7 days

def load_config():
    if CONFIG_PATH.exists():
        try:
            with open(CONFIG_PATH, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception as e:
            print(f"[WARN] Error reading {CONFIG_PATH}: {e}")
    return {
        "site_name": "PlusOS 2.0",
        "domain": "plusos.d0a.net",
        "creator": "the1Domo",
        "port": 8080,
        "host": "0.0.0.0",
        "admin_key": "00000000000000000000000000000000"
    }

CONFIG = load_config()

# ---- Authentication & User Management ----
def hash_password(password, salt=None):
    if not salt:
        salt = secrets.token_hex(8)
    h = hashlib.sha256((salt + password).encode("utf-8")).hexdigest()
    return f"{salt}:{h}"

def verify_password(stored_hash, password):
    try:
        salt, h = stored_hash.split(":", 1)
        expected = hashlib.sha256((salt + password).encode("utf-8")).hexdigest()
        return secrets.compare_digest(h, expected)
    except Exception:
        return False

def load_users():
    if USERS_FILE.exists():
        try:
            with open(USERS_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception as e:
            print(f"[WARN] Failed to read {USERS_FILE}: {e}")
    # Default initial superuser
    default_admin = [{
        "username": "the1domo",
        "display_name": "the1Domo (Owner)",
        "password_hash": hash_password("plusos2026"),
        "role": "admin",
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "status": "active"
    }]
    save_users(default_admin)
    return default_admin

def save_users(users):
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(USERS_FILE, "w", encoding="utf-8") as f:
        json.dump(users, f, indent=2)

def find_user(username):
    users = load_users()
    for u in users:
        if u["username"].lower() == username.lower().strip():
            return u
    return None

def create_session(user):
    token = secrets.token_hex(24)
    SESSIONS[token] = {
        "username": user["username"],
        "display_name": user.get("display_name", user["username"]),
        "role": user.get("role", "tester"),
        "expires": time.time() + SESSION_TTL
    }
    return token

def get_session_user(token):
    if not token or token not in SESSIONS:
        return None
    sess = SESSIONS[token]
    if time.time() > sess["expires"]:
        del SESSIONS[token]
        return None
    return sess

# ---- Site Content Management ----
def load_site_content():
    if CONTENT_FILE.exists():
        try:
            with open(CONTENT_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return {
        "site_title": "PlusOS 2.0 by the1Domo — The Bare-Metal Operating System for Macintosh Plus & SE",
        "hero_badge": "✨ NEW RELEASE • BARE-METAL MOTOROLA 68000",
        "hero_title": "The Macintosh, Unleashed.",
        "hero_subtitle": "Meet PlusOS 2.0, created by the1Domo. A revolutionary, layered operating system engineered from the silicon up for the Macintosh Plus and Macintosh SE.",
        "pitch_headline": "Breathe Modern Power into 1980s Hardware",
        "pitch_body": "For over three decades, vintage compact Macs were constrained by cooperative multitasking and memory fragility. PlusOS 2.0 changes everything. Written in clean C and 68000 assembly, PlusOS replaces the traditional System Folder with a preemptive, multi-ring Unix-like kernel that runs directly on bare metal.",
        "announcement_banner": "Welcome to the official PlusOS 2.0 portal! Check out the new technical Wiki and tester operations hub."
    }

def save_site_content(content):
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(CONTENT_FILE, "w", encoding="utf-8") as f:
        json.dump(content, f, indent=2)

# ---- Tests & Feedback Management ----
def load_tests():
    if TESTS_FILE.exists():
        try:
            with open(TESTS_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return []

def save_tests(tests):
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(TESTS_FILE, "w", encoding="utf-8") as f:
        json.dump(tests, f, indent=2)

def load_feedback():
    if FEEDBACK_FILE.exists():
        try:
            with open(FEEDBACK_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return []

def save_feedback(items):
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with open(FEEDBACK_FILE, "w", encoding="utf-8") as f:
        json.dump(items, f, indent=2)

# ---- Wiki Catalog ----
def load_wiki_index():
    index_file = WIKI_DIR / "index.json"
    if index_file.exists():
        try:
            with open(index_file, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception as e:
            print(f"[WARN] Failed to read wiki index: {e}")
    return []

def save_wiki_index(catalog):
    index_file = WIKI_DIR / "index.json"
    with open(index_file, "w", encoding="utf-8") as f:
        json.dump(catalog, f, indent=2)

# ---- Pure Python Markdown to HTML ----
def markdown_to_html(md_text):
    lines = md_text.splitlines()
    html_lines = []
    in_code_block = False
    in_list = False
    in_table = False
    table_has_header = False

    def inline_format(text):
        links = []
        def link_repl(m):
            lbl = m.group(1)
            raw_url = m.group(2).strip()
            # If internal wiki markdown link, e.g. "13-plusos-disk-studio-and-compiler.md" or "./13-..."
            if not (raw_url.startswith("http://") or raw_url.startswith("https://") or raw_url.startswith("#") or raw_url.startswith("mailto:") or raw_url.startswith("/")):
                clean = raw_url
                if clean.startswith("./"):
                    clean = clean[2:]
                if clean.endswith(".md"):
                    clean = clean[:-3]
                url = f"/wiki/{clean}"
            elif raw_url.startswith("/wiki/") and raw_url.endswith(".md"):
                url = raw_url[:-3]
            else:
                url = raw_url
            idx = len(links)
            links.append(f'<a href="{html.escape(url, quote=True)}">{html.escape(lbl)}</a>')
            return f"__LINK_TOKEN_{idx}__"

        text = re.sub(r'\[(.*?)\]\((.*?)\)', link_repl, text)
        text = html.escape(text)
        text = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', text)
        text = re.sub(r'\*(.+?)\*', r'<em>\1</em>', text)
        text = re.sub(r'`([^`]+)`', r'<code>\1</code>', text)
        for idx, link_html in enumerate(links):
            text = text.replace(f"__LINK_TOKEN_{idx}__", link_html)
        return text

    for line in lines:
        stripped = line.strip()

        if stripped.startswith("```"):
            if in_code_block:
                html_lines.append("</code></pre>")
                in_code_block = False
            else:
                lang = stripped[3:].strip()
                html_lines.append(f'<pre><code class="language-{lang}">' if lang else '<pre><code>')
                in_code_block = True
            continue

        if in_code_block:
            html_lines.append(html.escape(line))
            continue

        if in_list and not stripped.startswith("- ") and not stripped.startswith("* "):
            html_lines.append("</ul>")
            in_list = False

        if in_table and not stripped.startswith("|"):
            html_lines.append("</tbody></table>")
            in_table = False
            table_has_header = False

        if not stripped:
            continue

        if re.match(r'^(---|___|\*\*\*)$', stripped):
            html_lines.append("<hr>")
            continue

        if stripped.startswith("### "):
            html_lines.append(f"<h3>{inline_format(stripped[4:])}</h3>")
            continue
        if stripped.startswith("## "):
            html_lines.append(f"<h2>{inline_format(stripped[3:])}</h2>")
            continue
        if stripped.startswith("# "):
            html_lines.append(f"<h1>{inline_format(stripped[2:])}</h1>")
            continue

        if stripped.startswith("> "):
            html_lines.append(f"<blockquote>{inline_format(stripped[2:])}</blockquote>")
            continue

        if stripped.startswith("- ") or stripped.startswith("* "):
            if not in_list:
                html_lines.append("<ul>")
                in_list = True
            html_lines.append(f"<li>{inline_format(stripped[2:])}</li>")
            continue

        if stripped.startswith("|") and stripped.endswith("|"):
            cells = [c.strip() for c in stripped[1:-1].split("|")]
            if all(re.match(r'^:?-+:?$', c) for c in cells):
                table_has_header = True
                continue
            if not in_table:
                html_lines.append('<table class="specs-table">')
                in_table = True
                if not table_has_header:
                    html_lines.append("<thead><tr>" + "".join(f"<th>{inline_format(c)}</th>" for c in cells) + "</tr></thead><tbody>")
                    continue
            html_lines.append("<tr>" + "".join(f"<td>{inline_format(c)}</td>" for c in cells) + "</tr>")
            continue

        html_lines.append(f"<p>{inline_format(line)}</p>")

    if in_list:
        html_lines.append("</ul>")
    if in_table:
        html_lines.append("</tbody></table>")
    if in_code_block:
        html_lines.append("</code></pre>")

    return "\n".join(html_lines)


class PlusOSRequestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        sys.stdout.write(f"[{self.log_date_time_string()}] {self.client_address[0]} - {format % args}\n")

    def get_current_user(self):
        cookie_header = self.headers.get("Cookie", "")
        if "plusos_session=" in cookie_header:
            cookie = SimpleCookie()
            try:
                cookie.load(cookie_header)
                if "plusos_session" in cookie:
                    token = cookie["plusos_session"].value
                    return get_session_user(token)
            except Exception:
                pass
        return None

    def _safe_join(self, root, rel_path):
        """Join rel_path onto root, returning the resolved path only if it stays
        inside root. Blocks ../ traversal and absolute-path escapes. Returns None
        if the result would fall outside root."""
        try:
            root = Path(root).resolve()
            candidate = (root / rel_path).resolve()
            if candidate == root or root in candidate.parents:
                return candidate
        except Exception:
            pass
        return None

    def send_response_file(self, file_path, content_type=None):
        path = Path(file_path)
        if not path.is_file():
            self.send_error(404, "File Not Found")
            return
        if not content_type:
            content_type, _ = mimetypes.guess_type(str(path))
            content_type = content_type or "application/octet-stream"

        try:
            with open(path, "rb") as f:
                data = f.read()
            self.send_response(200)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(data)
        except Exception as e:
            self.send_error(500, f"Error reading file: {e}")

    def render_template(self, template_name, replacements):
        path = TEMPLATES_DIR / template_name
        if not path.exists():
            return "Template not found"
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()
        for k, v in replacements.items():
            content = content.replace(f"{{{{{k}}}}}", str(v))
        return content

    def redirect(self, location, set_cookie=None):
        self.send_response(302)
        self.send_header("Location", location)
        if set_cookie:
            self.send_header("Set-Cookie", set_cookie)
        self.end_headers()

    def build_auth_menubar_html(self, user):
        if not user:
            return '<a href="/login" style="font-weight:bold; background:#e8f0fe; color:#1a73e8; padding:2px 8px; border:1px solid #1a73e8; border-radius:3px; margin-right:8px; white-space:nowrap !important; display:inline-flex; align-items:center; gap:4px; flex-shrink:0; text-decoration:none; line-height:1.2;">🔑&nbsp;Sign&nbsp;In</a>'
        role = user.get("role", "tester")
        links = []
        if role == "admin":
            links.append('<a href="/admin" style="font-weight:bold; background:#fff2cc; padding:2px 6px; border:1px solid #d6b656; border-radius:3px; white-space:nowrap !important; display:inline-flex; align-items:center; flex-shrink:0;">⚙️&nbsp;Admin</a>')
            links.append('<a href="/tester" style="font-weight:bold; background:#e8f4fd; padding:2px 6px; border:1px solid #0070ba; border-radius:3px; white-space:nowrap !important; display:inline-flex; align-items:center; flex-shrink:0;">🧪&nbsp;Tester&nbsp;Hub</a>')
        elif role == "tester":
            links.append('<a href="/tester" style="font-weight:bold; background:#e8f4fd; padding:2px 6px; border:1px solid #0070ba; border-radius:3px; white-space:nowrap !important; display:inline-flex; align-items:center; flex-shrink:0;">🧪&nbsp;Tester&nbsp;Hub</a>')
        
        links.append(f'<span style="font-size:11px; margin-right:4px; white-space:nowrap !important; display:inline-flex; align-items:center; flex-shrink:0;">👤&nbsp;{html.escape(user["display_name"])}</span>')
        links.append('<a href="/api/auth/logout" style="color:#aa0000; font-size:11px; white-space:nowrap !important; flex-shrink:0;">[Log&nbsp;Out]</a>')
        return "".join(links)

    # ---- GET ROUTING ----
    def do_GET(self):
        parsed = urlparse(self.path)
        path = unquote(parsed.path)
        user = self.get_current_user()

        # 1. Homepage (Landing Page)
        if path in ("/", "/index.html"):
            content = load_site_content()
            page = self.render_template("index.html", {
                "AUTH_MENUBAR_HTML": self.build_auth_menubar_html(user),
                "CONTENT_SITE_TITLE": html.escape(content.get("site_title", "")),
                "CONTENT_HERO_BADGE": html.escape(content.get("hero_badge", "")),
                "CONTENT_HERO_TITLE": html.escape(content.get("hero_title", "")),
                "CONTENT_HERO_SUBTITLE": html.escape(content.get("hero_subtitle", "")),
                "CONTENT_PITCH_HEADLINE": html.escape(content.get("pitch_headline", "")),
                "CONTENT_PITCH_BODY": html.escape(content.get("pitch_body", ""))
            })
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(page.encode("utf-8"))
            return

        # 1b. Archived 2.0 landing page (/2.0) - frozen snapshot of the PlusOS 2.0 site
        if path in ("/2.0", "/2.0/", "/2.0/index.html"):
            page = self.render_template("index_2_0.html", {
                "AUTH_MENUBAR_HTML": self.build_auth_menubar_html(user),
                "CONTENT_SITE_TITLE": "PlusOS 2.0 by the1Domo — The Bare-Metal Operating System for Macintosh Plus & SE",
                "CONTENT_HERO_BADGE": "✨ ARCHIVED RELEASE • BARE-METAL MOTOROLA 68000",
                "CONTENT_HERO_TITLE": "The Macintosh, Unleashed.",
                "CONTENT_HERO_SUBTITLE": "PlusOS 2.0, created by the1Domo. A revolutionary, layered operating system engineered from the silicon up for the Macintosh Plus and Macintosh SE. (Archived — the current release is PlusOS 3.1.)",
                "CONTENT_PITCH_HEADLINE": "Breathe Modern Power into 1980s Hardware",
                "CONTENT_PITCH_BODY": "For over three decades, vintage compact Macs were constrained by cooperative multitasking and memory fragility. PlusOS 2.0 changes everything. Written in clean C and 68000 assembly, PlusOS replaces the traditional System Folder with a preemptive, multi-ring Unix-like kernel that runs directly on bare metal."
            })
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(page.encode("utf-8"))
            return

        # 2. Login Page (/login)
        if path == "/login":
            if user:
                if user.get("role") == "admin":
                    self.redirect("/admin")
                elif user.get("role") == "tester":
                    self.redirect("/tester")
                else:
                    self.redirect("/wiki")
                return
            login_path = TEMPLATES_DIR / "login.html"
            self.send_response_file(login_path, "text/html; charset=utf-8")
            return

        # 3. Admin Panel (/admin)
        if path == "/admin" or path == "/admin/":
            if not user or user.get("role") != "admin":
                self.redirect(f"/login?next={path}")
                return

            users = load_users()
            catalog = load_wiki_index()
            tests = load_tests()
            feedback = load_feedback()
            content = load_site_content()

            # Build Users table rows
            user_rows = []
            for u in users:
                status_color = "#1e7e34" if u.get("status", "active") == "active" else "#c82333"
                uname = html.escape(u["username"])
                is_owner = u["username"] == "the1domo"
                cur_role = u.get("role", "tester")
                # Role selector — the owner is locked to admin.
                role_opts = "".join(
                    f'<option value="{r}"{" selected" if cur_role == r else ""}>{r}</option>'
                    for r in ("admin", "wiki", "tester")
                )
                role_cell = (
                    f'<select onchange="changeUserRole(\'{uname}\', this.value)" '
                    f'class="editor-input" style="font-size:11px; padding:2px 4px;"'
                    f'{" disabled" if is_owner else ""}>{role_opts}</select>'
                )
                toggle_btn = (
                    f'<button onclick="toggleUser(\'{uname}\')" class="btn-mac" style="font-size:10px; padding:2px 6px;">Toggle</button>'
                    if not is_owner else "—"
                )
                pw_btn = (
                    f'<button onclick="changeUserPassword(\'{uname}\')" class="btn-mac" '
                    f'style="font-size:10px; padding:2px 6px;">🔑 Password</button>'
                )
                user_rows.append(f"""
                <tr>
                  <td><strong>{uname}</strong></td>
                  <td>{html.escape(u.get("display_name", ""))}</td>
                  <td>{role_cell}</td>
                  <td style="color:{status_color}; font-weight:bold;">{u.get("status", "active")}</td>
                  <td style="white-space:nowrap;">{pw_btn} {toggle_btn}</td>
                </tr>
                """)

            # Build Wiki table rows
            wiki_rows = []
            for item in catalog:
                wiki_rows.append(f"""
                <tr>
                  <td><span style="font-weight:bold; color:#004080;">{html.escape(item.get("category","Wiki"))}</span></td>
                  <td><a href="/wiki/{item['slug']}" target="_blank"><strong>{html.escape(item['title'])}</strong></a></td>
                  <td><code>{html.escape(item['slug'])}</code></td>
                  <td><small>{html.escape(item.get('summary',''))}</small></td>
                  <td>
                    <a href="/wiki/edit/{item['slug']}" target="_blank" class="btn-mac" style="font-size:10px; padding:2px 6px;">✏️ Edit</a>
                    <button onclick="deleteWiki('{item['slug']}')" class="btn-mac" style="font-size:10px; padding:2px 6px; color:#c82333;">🗑️</button>
                  </td>
                </tr>
                """)

            # Build Feedback list
            fb_items = []
            for fb in reversed(feedback):
                status_color = "#c82333" if fb.get("status") == "Open" else "#1e7e34" if fb.get("status") in ("Fixed", "Verified") else "#b8860b"
                admin_reply_val = html.escape(fb.get("admin_reply", ""))
                fb_items.append(f"""
                <div style="background:#fafafa; border:1px solid #ccc; padding:12px; margin-bottom:12px; border-radius:3px;">
                  <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:6px;">
                    <div>
                      <strong style="font-size:14px;">#{fb.get('id')} — {html.escape(fb.get('title',''))}</strong>
                      <span style="font-size:11px; margin-left:8px; color:#555;">by <strong>{html.escape(fb.get('user',''))}</strong> on <em>{html.escape(fb.get('machine',''))}</em> ({fb.get('severity')})</span>
                    </div>
                    <div>
                      <span style="font-weight:bold; font-size:11px; color:{status_color}; padding:2px 8px; border:1px solid {status_color}; border-radius:3px;">
                        {fb.get('status','Open')}
                      </span>
                    </div>
                  </div>
                  <div style="font-size:12px; margin-bottom:6px; background:#fff; padding:8px; border:1px solid #e0e0e0;">
                    <div><strong>Repro Steps:</strong> {html.escape(fb.get('repro_steps',''))}</div>
                    <div><strong>Expected:</strong> {html.escape(fb.get('expected',''))} &bull; <strong>Actual:</strong> {html.escape(fb.get('actual',''))}</div>
                    <div><strong>Review:</strong> {html.escape(fb.get('review',''))}</div>
                  </div>
                  <div style="display:flex; gap:8px; align-items:center; margin-top:8px;">
                    <select id="bug-status-{fb.get('id')}" class="editor-input" style="max-width:140px; padding:3px 6px; font-size:12px;">
                      <option value="Open" {"selected" if fb.get("status")=="Open" else ""}>Open</option>
                      <option value="Investigating" {"selected" if fb.get("status")=="Investigating" else ""}>Investigating</option>
                      <option value="Fixed" {"selected" if fb.get("status")=="Fixed" else ""}>Fixed</option>
                      <option value="Verified" {"selected" if fb.get("status")=="Verified" else ""}>Verified</option>
                    </select>
                    <input type="text" id="bug-reply-{fb.get('id')}" class="editor-input" placeholder="Developer response to tester..." value="{admin_reply_val}" style="padding:3px 8px; font-size:12px;">
                    <button onclick="updateBugStatus({fb.get('id')})" class="btn-mac btn-primary" style="font-size:11px; padding:3px 10px;">Update Status</button>
                  </div>
                </div>
                """)

            page = self.render_template("admin.html", {
                "CURRENT_USER": html.escape(user["display_name"]),
                "CURRENT_ROLE": html.escape(user["role"].upper()),
                "STATS_USERS": len(users),
                "STATS_WIKI": len(catalog),
                "STATS_TESTS": len([t for t in tests if t.get("active", True)]),
                "STATS_BUGS": len([f for f in feedback if f.get("status") != "Verified"]),
                "USERS_TABLE_ROWS": "".join(user_rows),
                "WIKI_TABLE_ROWS": "".join(wiki_rows),
                "FEEDBACK_LIST_HTML": "".join(fb_items) if fb_items else "<p>No bug reports submitted yet.</p>",
                "CONTENT_SITE_TITLE": html.escape(content.get("site_title", "")),
                "CONTENT_HERO_BADGE": html.escape(content.get("hero_badge", "")),
                "CONTENT_HERO_TITLE": html.escape(content.get("hero_title", "")),
                "CONTENT_HERO_SUBTITLE": html.escape(content.get("hero_subtitle", "")),
                "CONTENT_PITCH_HEADLINE": html.escape(content.get("pitch_headline", "")),
                "CONTENT_PITCH_BODY": html.escape(content.get("pitch_body", "")),
                "CONTENT_ANNOUNCEMENT_BANNER": html.escape(content.get("announcement_banner", ""))
            })
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(page.encode("utf-8"))
            return

        # 4. Tester Hub (/tester)
        if path == "/tester" or path == "/tester/":
            if not user or user.get("role") not in ("tester", "admin"):
                self.redirect(f"/login?next={path}")
                return

            tests = load_tests()
            feedback = load_feedback()

            # Active tests HTML
            test_items = []
            for t in reversed(tests):
                if not t.get("active", True):
                    continue
                test_items.append(f"""
                <div style="background:#fff; border:2px solid #004080; padding:14px; margin-bottom:14px; border-radius:4px; box-shadow:2px 2px 0 rgba(0,0,0,0.1);">
                  <div style="display:flex; justify-content:space-between; align-items:center;">
                    <strong style="font-size:15px; color:#004080;">{html.escape(t.get('title',''))}</strong>
                    <span style="font-size:11px; background:#fffae0; border:1px solid #d4aa00; padding:2px 8px; border-radius:3px;">Build: {html.escape(t.get('version',''))}</span>
                  </div>
                  <div style="font-size:12px; color:#666; margin:4px 0 10px;">Target: <strong>{html.escape(t.get('target_hardware','All Hardware'))}</strong> &bull; Posted by {html.escape(t.get('author','the1Domo'))} on {t.get('date','')}</div>
                  <div style="font-size:13px; line-height:1.6; background:#fafafa; border:1px solid #eee; padding:10px; border-radius:3px; margin-bottom:12px;">
                    {html.escape(t.get('instructions',''))}
                  </div>
                  <div>
                    <a href="{t.get('download_url','/downloads/PlusOS.hda')}" class="btn-cta btn-cta-download" style="padding:6px 14px; font-size:12px;">💾 Download Test Image ({html.escape(t.get('version',''))})</a>
                  </div>
                </div>
                """)

            # User feedback list
            user_fb_items = []
            for fb in reversed(feedback):
                if fb.get("user") == user["username"] or user.get("role") == "admin":
                    status_color = "#c82333" if fb.get("status") == "Open" else "#1e7e34" if fb.get("status") in ("Fixed", "Verified") else "#b8860b"
                    reply_html = f'<div style="margin-top:6px; padding:6px 8px; background:#e8f4fd; border-left:3px solid #0070ba; font-size:11px;"><strong>the1Domo Reply:</strong> {html.escape(fb.get("admin_reply"))}</div>' if fb.get("admin_reply") else ''
                    user_fb_items.append(f"""
                    <div style="background:#fff; border:1px solid #ccc; padding:10px; margin-bottom:10px; border-radius:3px;">
                      <div style="display:flex; justify-content:space-between; font-size:13px;">
                        <strong>{html.escape(fb.get('title',''))}</strong>
                        <span style="color:{status_color}; font-weight:bold; font-size:11px;">[{fb.get('status','Open')}]</span>
                      </div>
                      <div style="font-size:11px; color:#666; margin:2px 0 6px;">Machine: {html.escape(fb.get('machine',''))} &bull; Severity: {fb.get('severity','')}</div>
                      <div style="font-size:12px; color:#333;">{html.escape(fb.get('review',''))}</div>
                      {reply_html}
                    </div>
                    """)

            page = self.render_template("tester.html", {
                "CURRENT_USER": html.escape(user["display_name"]),
                "CURRENT_ROLE": html.escape(user["role"].upper()),
                "ACTIVE_TESTS_HTML": "".join(test_items) if test_items else "<p>No active test assignments at this time. Check back soon!</p>",
                "USER_FEEDBACK_HTML": "".join(user_fb_items) if user_fb_items else "<p>No reports submitted yet.</p>"
            })
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(page.encode("utf-8"))
            return

        # 5. Static Assets (/static/...)
        if path.startswith("/static/"):
            rel_path = path[len("/static/"):]
            file_path = self._safe_join(STATIC_DIR, rel_path)
            if file_path is None:
                self.send_error(403, "Forbidden")
                return
            self.send_response_file(file_path)
            return

        # 6. Docs / Screenshots (/docs/...)
        if path.startswith("/docs/"):
            rel_path = path[len("/docs/"):]
            file_path = self._safe_join(DOCS_DIR, rel_path)
            if file_path is None:
                self.send_error(403, "Forbidden")
                return
            self.send_response_file(file_path)
            return

        # 7. Downloads (/downloads/...)
        if path.startswith("/downloads/"):
            filename = path[len("/downloads/"):].strip("/")
            file_path = DOWNLOADS_DIR / filename
            if not file_path.exists() and filename.lower().endswith(".hda"):
                file_path = IMAGES_DIR / filename
            if not file_path.exists() and filename == "PlusOS.hda":
                file_path = IMAGES_DIR / "PlusOS.hda"
            if not file_path.exists():
                # Check root directory or tools directory fallback
                root_cand = BASE_DIR.parent / filename
                if root_cand.exists():
                    file_path = root_cand
                else:
                    tools_cand = BASE_DIR.parent / "tools" / "disk_studio" / filename
                    if tools_cand.exists():
                        file_path = tools_cand

            if file_path.exists():
                self.send_response_file(file_path, "application/octet-stream")
            else:
                self.send_error(404, f"Download '{filename}' not found on server")
            return

        # 8. Wiki Redirect (/wiki or /wiki/)
        if path in ("/wiki", "/wiki/", "/wiki/index", "/wiki/index.html"):
            catalog = load_wiki_index()
            first_slug = catalog[0]["slug"] if catalog else "01-getting-started"
            self.redirect(f"/wiki/{first_slug}")
            return

        # 9. Wiki New Article Form (/wiki/new)
        if path == "/wiki/new":
            if not user or user.get("role") not in ("admin", "wiki"):
                self.redirect(f"/login?next={path}")
                return

            page = self.render_template("wiki_edit.html", {
                "ACTION_TITLE": "Create New Wiki Article",
                "SLUG": "",
                "SLUG_READONLY": "",
                "CATEGORY": "General",
                "TITLE": "",
                "SUMMARY": "",
                "RAW_CONTENT": "# New Article Title\n\nWrite your content here..."
            })
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(page.encode("utf-8"))
            return

        # 10. Wiki Edit Article Form (/wiki/edit/<slug>)
        if path.startswith("/wiki/edit/"):
            if not user or user.get("role") not in ("admin", "wiki"):
                self.redirect(f"/login?next={path}")
                return

            slug = path[len("/wiki/edit/"):].strip("/")
            if slug.endswith(".md"):
                slug = slug[:-3]

            catalog = load_wiki_index()
            entry = next((item for item in catalog if item["slug"] == slug), None)
            md_path = WIKI_DIR / f"{slug}.md"
            raw_content = ""
            if md_path.exists():
                with open(md_path, "r", encoding="utf-8") as f:
                    raw_content = f.read()

            page = self.render_template("wiki_edit.html", {
                "ACTION_TITLE": f"Edit Article: {entry['title'] if entry else slug}",
                "SLUG": slug,
                "SLUG_READONLY": "readonly",
                "CATEGORY": entry["category"] if entry else "General",
                "TITLE": entry["title"] if entry else slug,
                "SUMMARY": entry["summary"] if entry else "",
                "RAW_CONTENT": html.escape(raw_content)
            })
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(page.encode("utf-8"))
            return

        # 11. Wiki Article View (/wiki/<slug>)
        if path.startswith("/wiki/"):
            raw_slug = path[len("/wiki/"):].strip("/")
            slug = raw_slug
            if slug.endswith(".md"):
                slug = slug[:-3]

            catalog = load_wiki_index()
            entry = next((item for item in catalog if item["slug"] == slug), None)

            # Resilient lookup: match case-insensitively, by numeric prefix (e.g. "1" or "01"), or substring
            if not entry:
                for item in catalog:
                    islug = item["slug"]
                    if islug.lower() == slug.lower():
                        entry = item
                        slug = islug
                        break
                    num_prefix = islug.split("-")[0]
                    if slug == num_prefix or slug.zfill(2) == num_prefix:
                        entry = item
                        slug = islug
                        break
                    if islug.endswith(f"-{slug.lower()}") or slug.lower() in islug.lower():
                        entry = item
                        slug = islug
                        break

            md_path = WIKI_DIR / f"{slug}.md"
            if not md_path.exists() and entry:
                md_path = WIKI_DIR / f"{entry['slug']}.md"
                slug = entry['slug']

            if not md_path.exists():
                for md_file in WIKI_DIR.glob("*.md"):
                    if md_file.stem.lower() == slug.lower() or md_file.stem.endswith(f"-{slug.lower()}") or slug.lower() in md_file.stem.lower():
                        md_path = md_file
                        slug = md_file.stem
                        entry = next((item for item in catalog if item["slug"] == slug), None)
                        break

            if not md_path.exists():
                self.send_error(404, f"Wiki article '{raw_slug}' not found on server")
                return

            with open(md_path, "r", encoding="utf-8") as f:
                raw_markdown = f.read()

            content_html = markdown_to_html(raw_markdown)

            categories = {}
            for item in catalog:
                cat = item.get("category", "General")
                categories.setdefault(cat, []).append(item)

            nav_html_parts = []
            for cat, items in categories.items():
                nav_html_parts.append(f'<div class="wiki-nav-group"><div class="wiki-nav-title">{cat}</div><ul class="wiki-nav-list">')
                for it in items:
                    active_cls = ' class="active"' if it["slug"] == slug else ''
                    nav_html_parts.append(f'<li><a href="/wiki/{it["slug"]}"{active_cls}>{it["title"]}</a></li>')
                nav_html_parts.append('</ul></div>')

            nav_html = "".join(nav_html_parts)

            can_edit_wiki = user and user.get("role") in ("admin", "wiki")
            new_btn_html = '<a href="/wiki/new" class="btn-mac" style="padding:1px 6px; font-size:11px;">+ New Article</a>' if can_edit_wiki else ''
            edit_btn_html = f'<a href="/wiki/edit/{slug}" class="btn-mac" style="font-size:12px;">✏️ Edit Article</a>' if can_edit_wiki else ''

            page = self.render_template("wiki.html", {
                "TITLE": entry["title"] if entry else slug,
                "SLUG": slug,
                "CATEGORY": entry["category"] if entry else "Wiki",
                "SUMMARY": entry["summary"] if entry else "PlusOS documentation",
                "NAV_HTML": nav_html,
                "CONTENT_HTML": content_html,
                "CREATOR": CONFIG.get("creator", "the1Domo"),
                "AUTH_MENUBAR_HTML": self.build_auth_menubar_html(user),
                "WIKI_NEW_BTN_HTML": new_btn_html,
                "WIKI_EDIT_BTN_HTML": edit_btn_html
            })

            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(page.encode("utf-8"))
            return

        # 12. Logout
        if path == "/api/auth/logout":
            cookie_header = self.headers.get("Cookie", "")
            if "plusos_session=" in cookie_header:
                cookie = SimpleCookie()
                try:
                    cookie.load(cookie_header)
                    if "plusos_session" in cookie:
                        token = cookie["plusos_session"].value
                        if token in SESSIONS:
                            del SESSIONS[token]
                except Exception:
                    pass
            self.redirect("/login", set_cookie="plusos_session=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT")
            return

        self.send_error(404, "Page Not Found")

    # ---- POST ROUTING ----
    def do_POST(self):
        parsed = urlparse(self.path)
        path = unquote(parsed.path)
        user = self.get_current_user()

        content_length = int(self.headers.get("Content-Length", 0))
        post_body = self.rfile.read(content_length)

        try:
            data = json.loads(post_body.decode("utf-8")) if post_body else {}
        except Exception as e:
            self.send_json_error(400, f"Invalid JSON payload: {e}")
            return

        # 1. API: Sign In (/api/auth/login)
        if path == "/api/auth/login":
            username = data.get("username", "").strip()
            password = data.get("password", "")

            account = find_user(username)
            if not account or account.get("status") != "active":
                self.send_json_error(401, "Invalid username or password.")
                return

            if not verify_password(account.get("password_hash", ""), password):
                self.send_json_error(401, "Invalid username or password.")
                return

            token = create_session(account)
            cookie_val = f"plusos_session={token}; Path=/; HttpOnly; SameSite=Lax; Max-Age={SESSION_TTL}"
            
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Set-Cookie", cookie_val)
            self.end_headers()
            self.wfile.write(json.dumps({
                "ok": True,
                "username": account["username"],
                "role": account.get("role", "tester")
            }).encode("utf-8"))
            return

        # 2. API: Save Wiki Article (/api/wiki/save)
        if path == "/api/wiki/save":
            if not user or user.get("role") not in ("admin", "wiki"):
                self.send_json_error(403, "Permission Denied: Only Admins and Wiki Editors may edit or save articles.")
                return

            slug = data.get("slug", "").strip()
            title = data.get("title", "").strip()
            category = data.get("category", "General").strip()
            summary = data.get("summary", "").strip()
            content = data.get("content", "")

            if not slug or not title or not content:
                self.send_json_error(400, "Missing required fields: slug, title, and content are mandatory.")
                return

            slug = re.sub(r'[^a-zA-Z0-9_\-]', '-', slug).lower()
            md_path = WIKI_DIR / f"{slug}.md"
            try:
                with open(md_path, "w", encoding="utf-8") as f:
                    f.write(content)
            except Exception as e:
                self.send_json_error(500, f"Failed to write article file: {e}")
                return

            catalog = load_wiki_index()
            existing = next((item for item in catalog if item["slug"] == slug), None)
            if existing:
                existing["title"] = title
                existing["category"] = category
                existing["summary"] = summary
            else:
                catalog.append({
                    "slug": slug,
                    "title": title,
                    "category": category,
                    "summary": summary,
                    "order": len(catalog) + 1
                })
            save_wiki_index(catalog)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True, "slug": slug}).encode("utf-8"))
            return

        # 3. API: Delete Wiki Article (/api/wiki/delete)
        if path == "/api/wiki/delete":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied: Only Super Admin may delete articles.")
                return

            slug = data.get("slug", "").strip()
            md_path = WIKI_DIR / f"{slug}.md"
            if md_path.exists():
                try:
                    os.remove(md_path)
                except Exception as e:
                    self.send_json_error(500, f"Error deleting file: {e}")
                    return

            catalog = load_wiki_index()
            catalog = [item for item in catalog if item["slug"] != slug]
            save_wiki_index(catalog)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True}).encode("utf-8"))
            return

        # 4. API: Create User (/api/admin/users/create)
        if path == "/api/admin/users/create":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied: Only Super Admin may create users.")
                return

            username = data.get("username", "").strip().lower()
            display_name = data.get("display_name", "").strip()
            password = data.get("password", "")
            role = data.get("role", "tester").strip().lower()

            if not username or not password:
                self.send_json_error(400, "Username and password required.")
                return

            if role not in ("admin", "wiki", "tester"):
                self.send_json_error(400, "Invalid role. Must be 'admin', 'wiki', or 'tester'.")
                return

            users = load_users()
            if any(u["username"].lower() == username for u in users):
                self.send_json_error(400, f"User '{username}' already exists.")
                return

            new_user = {
                "username": username,
                "display_name": display_name or username,
                "password_hash": hash_password(password),
                "role": role,
                "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "status": "active"
            }
            users.append(new_user)
            save_users(users)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True, "username": username}).encode("utf-8"))
            return

        # 5. API: Toggle User Status (/api/admin/users/toggle)
        if path == "/api/admin/users/toggle":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied.")
                return

            username = data.get("username", "").strip().lower()
            if username == "the1domo":
                self.send_json_error(400, "Cannot disable the super administrator account.")
                return

            users = load_users()
            target = next((u for u in users if u["username"].lower() == username), None)
            if not target:
                self.send_json_error(404, "User not found.")
                return

            target["status"] = "disabled" if target.get("status") == "active" else "active"
            save_users(users)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True, "status": target["status"]}).encode("utf-8"))
            return

        # 5b. API: Change User Role / Level (/api/admin/users/role)
        if path == "/api/admin/users/role":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied.")
                return

            username = data.get("username", "").strip().lower()
            new_role = data.get("role", "").strip().lower()

            if new_role not in ("admin", "wiki", "tester"):
                self.send_json_error(400, "Invalid role. Must be 'admin', 'wiki', or 'tester'.")
                return
            if username == "the1domo" and new_role != "admin":
                self.send_json_error(400, "Cannot demote the super administrator account.")
                return

            users = load_users()
            target = next((u for u in users if u["username"].lower() == username), None)
            if not target:
                self.send_json_error(404, "User not found.")
                return

            target["role"] = new_role
            save_users(users)

            # Keep any live sessions for this user in sync with the new role.
            for sess in SESSIONS.values():
                if sess.get("username", "").lower() == username:
                    sess["role"] = new_role

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True, "role": new_role}).encode("utf-8"))
            return

        # 5c. API: Change User Password (/api/admin/users/password)
        if path == "/api/admin/users/password":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied.")
                return

            username = data.get("username", "").strip().lower()
            password = data.get("password", "")

            if not password or len(password) < 4:
                self.send_json_error(400, "Password must be at least 4 characters.")
                return

            users = load_users()
            target = next((u for u in users if u["username"].lower() == username), None)
            if not target:
                self.send_json_error(404, "User not found.")
                return

            target["password_hash"] = hash_password(password)
            save_users(users)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True}).encode("utf-8"))
            return

        # 6. API: Save Page Content (/api/admin/content/save)
        if path == "/api/admin/content/save":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied: Only Super Admin may edit page content.")
                return

            current_content = load_site_content()
            for key in ("site_title", "hero_badge", "hero_title", "hero_subtitle", "pitch_headline", "pitch_body", "announcement_banner"):
                if key in data:
                    current_content[key] = data[key]
            save_site_content(current_content)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True}).encode("utf-8"))
            return

        # 7. API: Create Test Assignment (/api/admin/tests/create)
        if path == "/api/admin/tests/create":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied.")
                return

            tests = load_tests()
            new_test = {
                "id": len(tests) + 1,
                "title": data.get("title", "New Hardware Test"),
                "version": data.get("version", "2.0.0"),
                "date": time.strftime("%Y-%m-%d"),
                "author": user.get("display_name", "the1Domo"),
                "target_hardware": data.get("target_hardware", "Mac Plus / SE"),
                "instructions": data.get("instructions", ""),
                "download_url": data.get("download_url", "/downloads/PlusOS.hda"),
                "active": True
            }
            tests.append(new_test)
            save_tests(tests)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True, "id": new_test["id"]}).encode("utf-8"))
            return

        # 8. API: Submit Tester Feedback / Bug Report (/api/tester/feedback/submit)
        if path == "/api/tester/feedback/submit":
            if not user or user.get("role") not in ("tester", "admin"):
                self.send_json_error(403, "Permission Denied: Must be registered tester or admin.")
                return

            feedback = load_feedback()
            new_fb = {
                "id": len(feedback) + 1,
                "test_id": data.get("test_id", 1),
                "user": user["username"],
                "machine": data.get("machine", "Macintosh SE"),
                "title": data.get("title", "Untitled Report"),
                "severity": data.get("severity", "Minor"),
                "repro_steps": data.get("repro_steps", ""),
                "expected": data.get("expected", ""),
                "actual": data.get("actual", ""),
                "review": data.get("review", ""),
                "status": "Open",
                "admin_reply": "",
                "date": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
            }
            feedback.append(new_fb)
            save_feedback(feedback)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True, "id": new_fb["id"]}).encode("utf-8"))
            return

        # 9. API: Update Bug Report & Reply (/api/admin/feedback/update)
        if path == "/api/admin/feedback/update":
            if not user or user.get("role") != "admin":
                self.send_json_error(403, "Permission Denied.")
                return

            bug_id = data.get("id")
            feedback = load_feedback()
            target = next((f for f in feedback if f.get("id") == bug_id), None)
            if not target:
                self.send_json_error(404, "Report not found.")
                return

            if "status" in data:
                target["status"] = data["status"]
            if "admin_reply" in data:
                target["admin_reply"] = data["admin_reply"]
            save_feedback(feedback)

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"ok": True}).encode("utf-8"))
            return

        self.send_error(404, "Endpoint not found")

    def send_json_error(self, code, message):
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.end_headers()
        self.wfile.write(json.dumps({"ok": False, "error": message}).encode("utf-8"))


def main():
    import argparse
    parser = argparse.ArgumentParser(description="PlusOS 2.0 Web, Wiki & Admin Server")
    parser.add_argument("--port", type=int, default=CONFIG.get("port", 8080), help="Port to listen on (default: 8080)")
    parser.add_argument("--host", type=str, default=CONFIG.get("host", "0.0.0.0"), help="Host IP to bind to (default: 0.0.0.0)")
    args = parser.parse_args()

    DOWNLOADS_DIR.mkdir(parents=True, exist_ok=True)
    WIKI_DIR.mkdir(parents=True, exist_ok=True)
    DATA_DIR.mkdir(parents=True, exist_ok=True)

    # Initialize users and content if needed
    load_users()
    load_site_content()

    server_address = (args.host, args.port)
    httpd = HTTPServer(server_address, PlusOSRequestHandler)

    print("=" * 70)
    print(f"  PlusOS 2.0 Super Server System by {CONFIG.get('creator', 'the1Domo')}")
    print(f"  Domain:     https://{CONFIG.get('domain', 'plusos.d0a.net')}")
    print(f"  Main Site:  http://{args.host}:{args.port}/")
    print(f"  Wiki:       http://{args.host}:{args.port}/wiki")
    print(f"  Admin:      http://{args.host}:{args.port}/admin")
    print(f"  Tester Hub: http://{args.host}:{args.port}/tester")
    print("=" * 70)
    print("Ready to serve requests. Press Ctrl+C to stop.")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server gracefully...")
        httpd.server_close()


if __name__ == "__main__":
    main()
