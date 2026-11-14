// PlusOS OTA — admin control panel.
(function () {
  "use strict";
  const $ = (id) => document.getElementById(id);
  let overviewTimer = null, shellEnabled = false;

  async function api(path, opts) {
    const r = await fetch(path, Object.assign({ headers: { "Content-Type": "application/json" } }, opts));
    if (r.status === 401) { showLogin(); throw new Error("unauthorized"); }
    if (!r.ok) { let t = await r.text(); throw new Error(t || r.statusText); }
    return r.headers.get("content-type", "").includes("json") || r.status !== 204 ? r.json() : null;
  }
  function fmtBytes(n) {
    if (n == null) return "?"; const u = ["B", "KB", "MB", "GB", "TB"]; let i = 0;
    while (n >= 1024 && i < u.length - 1) { n /= 1024; i++; }
    return n.toFixed(n < 10 && i > 0 ? 1 : 0) + " " + u[i];
  }
  function fmtDur(s) {
    if (s == null) return "?"; s = Math.floor(s);
    const d = Math.floor(s / 86400); s %= 86400;
    const h = Math.floor(s / 3600); s %= 3600; const m = Math.floor(s / 60);
    return (d ? d + "d " : "") + (h ? h + "h " : "") + m + "m";
  }
  function fmtDate(ts) { return new Date(ts * 1000).toLocaleString(); }
  const esc = (s) => String(s == null ? "" : s).replace(/[&<>"']/g,
    (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));

  // ---------- auth ----------
  function showLogin() {
    $("loginWin").classList.remove("hidden");
    $("panelWin").classList.add("hidden");
    if (overviewTimer) { clearInterval(overviewTimer); overviewTimer = null; }
  }
  function showPanel() {
    $("loginWin").classList.add("hidden");
    $("panelWin").classList.remove("hidden");
    loadOverview(); loadReleases();
    overviewTimer = setInterval(loadOverview, 4000);
  }
  async function login() {
    $("loginErr").textContent = "";
    try {
      await api("/admin/login", { method: "POST", body: JSON.stringify({ password: $("pw").value }) });
      $("pw").value = "";
      await boot();
    } catch (e) { $("loginErr").textContent = "Wrong password"; }
  }
  async function boot() {
    const who = await fetch("/admin/api/whoami").then((r) => r.json());
    shellEnabled = who.shell_enabled;
    $("pubpill").textContent = who.public_url;
    $("pubUrl").textContent = who.public_url;
    $("shellOff").classList.toggle("hidden", shellEnabled);
    $("shellOn").classList.toggle("hidden", !shellEnabled);
    $("restartBtn").disabled = false;
    if (who.logged_in) showPanel(); else showLogin();
  }

  // ---------- overview ----------
  function bar(el, pct) { el.style.width = Math.max(0, Math.min(100, pct || 0)) + "%"; }
  async function loadOverview() {
    let o;
    try { o = await api("/admin/api/overview"); } catch (_) { return; }
    const s = o.stats, m = o.metrics;
    $("statGrid").innerHTML = [
      ["Releases", s.releases], ["Artifacts", s.artifacts],
      ["Store Size", fmtBytes(s.artifact_bytes)], ["Clients", o.connected_clients],
      ["Channels", (s.channels || []).join(", ") || "—"],
      ["Uptime", fmtDur(m.server_uptime)],
    ].map(([l, n]) => '<div class="stat"><div class="n">' + esc(n) + '</div><div class="l">' + l + "</div></div>").join("");
    $("hostInfo").innerHTML = esc(m.host) + " · " + esc(m.platform) + " · py" + esc(m.python) +
      " · pid " + m.pid + (m.host_uptime ? " · host up " + fmtDur(m.host_uptime) : "") +
      (m.psutil ? "" : " · <i>install psutil for full metrics</i>");
    $("cpuTxt").textContent = (m.cpu_percent == null ? "n/a" : m.cpu_percent + "%") +
      " · " + (m.cpu_count || "?") + " cores";
    bar($("cpuBar"), m.cpu_percent || 0);
    $("memTxt").textContent = fmtBytes(m.mem.used) + " / " + fmtBytes(m.mem.total) + " (" + m.mem.percent + "%)";
    bar($("memBar"), m.mem.percent);
    if (m.disk) {
      $("diskTxt").textContent = fmtBytes(m.disk.used) + " / " + fmtBytes(m.disk.total) + " (" + m.disk.percent + "%)";
      bar($("diskBar"), m.disk.percent);
    }
  }

  // ---------- releases ----------
  async function loadReleases() {
    const d = await api("/admin/api/releases");
    const box = $("relList"); box.innerHTML = "";
    if (!d.releases.length) { box.innerHTML = "<p class='muted'>No releases yet. Publish from a compile.</p>"; return; }
    d.releases.forEach((r) => {
      const c = document.createElement("div"); c.className = "card";
      c.innerHTML =
        '<div class="spread"><h3 style="border:0">' + esc(r.title || r.version) +
        ' <span class="badge">' + r.channel + '</span> <span class="badge">v' + esc(r.version) + '</span>' +
        (r.machine ? ' <span class="badge">' + r.machine + '</span>' : '') +
        (r.yanked ? ' <span class="badge sel">YANKED</span>' : '') + '</h3>' +
        '<div class="row">' +
        '<button class="btn sm" data-act="yank" data-id="' + r.id + '" data-y="' + (r.yanked ? 0 : 1) + '">' +
        (r.yanked ? "Un-yank" : "Yank") + '</button>' +
        '<button class="btn sm danger" data-act="del" data-id="' + r.id + '">Delete</button>' +
        '</div></div>' +
        '<div class="muted">' + fmtDate(r.created_at) + ' · ' + fmtBytes(r.total_bytes) +
        ' · by ' + esc(r.published_by) + '</div>' +
        '<textarea data-notes="' + r.id + '" rows="3">' + esc((r.title ? r.title + "\n" : "") + (r.notes || "")) + '</textarea>' +
        '<button class="btn sm" data-act="savenotes" data-id="' + r.id + '">Save Notes</button>' +
        '<table><tr><th>Item</th><th>Kind</th><th>Where</th><th>Size</th><th>Hash</th></tr>' +
        r.artifacts.map((a) => "<tr><td>" + esc(a.name) + "</td><td>" + a.kind + "</td><td>" +
          esc(a.path || "—") + "</td><td>" + fmtBytes(a.size) + "</td><td title='" + a.sha256 +
          "'>" + a.sha256.slice(0, 10) + "…</td></tr>").join("") + "</table>";
      box.appendChild(c);
    });
    box.querySelectorAll("button[data-act]").forEach((b) => b.onclick = onRelAction);
  }
  async function onRelAction(e) {
    const id = e.target.dataset.id, act = e.target.dataset.act;
    if (act === "yank") await api("/admin/api/release/" + id + "/yank", { method: "POST", body: JSON.stringify({ yank: e.target.dataset.y === "1" }) });
    else if (act === "del") { if (!confirm("Delete release " + id + "? This cannot be undone.")) return; await api("/admin/api/release/" + id + "/delete", { method: "POST", body: "{}" }); }
    else if (act === "savenotes") {
      const txt = document.querySelector('textarea[data-notes="' + id + '"]').value;
      const nl = txt.indexOf("\n");
      const title = nl >= 0 ? txt.slice(0, nl) : txt;
      const notes = nl >= 0 ? txt.slice(nl + 1) : "";
      await api("/admin/api/release/" + id + "/notes", { method: "POST", body: JSON.stringify({ title, notes }) });
    }
    loadReleases();
  }

  // ---------- system ----------
  async function loadProcesses() {
    const d = await api("/admin/api/processes?limit=40");
    const rows = d.processes.map((p) =>
      String(p.pid).padStart(7) + "  " + fmtBytes(p.rss).padStart(9) + "  " +
      (p.cpu ? (p.cpu.toFixed(0) + "%").padStart(4) : "   -") + "  " + p.name +
      (shellEnabled ? "   [kill:" + p.pid + "]" : "")).join("\n");
    $("procBox").textContent = "    PID        RSS   CPU  NAME\n" + rows +
      (shellEnabled ? "\n\n(double-click here to kill a PID)" : "");
  }

  async function runShell() {
    const cmd = $("cmd").value.trim(); if (!cmd) return;
    const out = $("shellOut");
    out.textContent = "$ " + cmd + "\n" + out.textContent;
    try {
      const r = await api("/admin/api/shell", { method: "POST", body: JSON.stringify({ cmd }) });
      if (r.ok) out.textContent = "$ " + cmd + "\n" + (r.stdout || "") + (r.stderr ? "\n[stderr]\n" + r.stderr : "") +
        "\n[exit " + r.code + "]\n\n" + out.textContent.split("\n").slice(1).join("\n");
      else out.textContent = "$ " + cmd + "\n[error] " + r.error + "\n\n" + out.textContent.split("\n").slice(1).join("\n");
    } catch (e) { out.textContent = "[error] " + e.message + "\n" + out.textContent; }
    $("cmd").value = "";
  }

  async function loadAudit() {
    const d = await api("/admin/api/audit?limit=150");
    $("auditBox").textContent = d.audit.map((a) =>
      fmtDate(a.ts) + "  " + (a.actor || "").padEnd(9) + " " + a.action.padEnd(16) + " " + (a.detail || "")).join("\n");
  }

  // ---------- tabs ----------
  document.querySelectorAll(".tab").forEach((t) => t.onclick = () => {
    document.querySelectorAll(".tab").forEach((x) => x.classList.toggle("on", x === t));
    const name = t.dataset.tab;
    document.querySelectorAll(".tabpage").forEach((p) => p.classList.toggle("on", p.id === "tab-" + name));
    if (name === "releases") loadReleases();
    if (name === "system") loadProcesses();
    if (name === "audit") loadAudit();
  });

  // ---------- wiring ----------
  $("loginBtn").onclick = login;
  $("pw").addEventListener("keydown", (e) => { if (e.key === "Enter") login(); });
  $("logoutBtn").onclick = async () => { await fetch("/admin/logout", { method: "POST" }); showLogin(); };
  $("darkBtn").onclick = () => { document.body.classList.toggle("dark"); document.body.classList.toggle("light"); };
  $("procRefresh").onclick = loadProcesses;
  $("auditRefresh").onclick = loadAudit;
  $("runBtn").onclick = runShell;
  $("cmd") && $("cmd").addEventListener("keydown", (e) => { if (e.key === "Enter") runShell(); });
  $("pruneBtn").onclick = async () => {
    if (!confirm("Delete blobs no release references?")) return;
    const r = await api("/admin/api/prune", { method: "POST", body: "{}" });
    alert("Freed " + fmtBytes(r.freed_bytes)); loadOverview();
  };
  $("restartBtn").onclick = async () => {
    if (!confirm("Restart the OTA service? Clients reconnect automatically.")) return;
    try { await api("/admin/api/restart", { method: "POST", body: "{}" }); } catch (_) {}
    alert("Restarting… reconnecting in a few seconds.");
    setTimeout(() => location.reload(), 4000);
  };
  // kill via click-to-prompt on a pid line
  $("procBox").addEventListener("dblclick", async () => {
    if (!shellEnabled) return;
    const pid = prompt("Kill which PID?"); if (!pid) return;
    const r = await api("/admin/api/kill", { method: "POST", body: JSON.stringify({ pid: +pid }) });
    alert(r.msg); loadProcesses();
  });
  $("pwBtn").onclick = async () => {
    $("pwMsg").textContent = "";
    try {
      await api("/admin/api/password", { method: "POST", body: JSON.stringify({ old: $("oldpw").value, new: $("newpw").value }) });
      $("pwMsg").textContent = "Password changed."; $("oldpw").value = $("newpw").value = "";
    } catch (e) { $("pwMsg").textContent = e.message; }
  };

  boot();
})();
