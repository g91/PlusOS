// PlusOS OTA — client update GUI. Browses the catalog over WebSocket, shows
// what's available / what's new, lets the user select artifacts and download
// them. Live-updates when the maintainer publishes.
(function () {
  "use strict";
  const $ = (id) => document.getElementById(id);
  let ws = null, catalog = null, selected = new Set();

  // Per-browser memory of which versions this client has already "seen" so we
  // can flag genuinely-new releases. (localStorage is per-viewer; fine here.)
  function seenSet() {
    try { return new Set(JSON.parse(localStorage.getItem("ota_seen") || "[]")); }
    catch (_) { return new Set(); }
  }
  function markSeen(ids) {
    try {
      const s = seenSet(); ids.forEach((i) => s.add(i));
      localStorage.setItem("ota_seen", JSON.stringify([...s]));
    } catch (_) {}
  }

  function log(msg) {
    const a = $("activity");
    a.textContent = "[" + new Date().toLocaleTimeString() + "] " + msg + "\n" + a.textContent;
  }
  function fmtBytes(n) {
    if (n == null) return "?";
    const u = ["B", "KB", "MB", "GB"]; let i = 0;
    while (n >= 1024 && i < u.length - 1) { n /= 1024; i++; }
    return n.toFixed(n < 10 && i > 0 ? 1 : 0) + " " + u[i];
  }
  function fmtDate(ts) { return new Date(ts * 1000).toLocaleString(); }

  function connect() {
    const proto = location.protocol === "https:" ? "wss:" : "ws:";
    const qs = "channel=" + encodeURIComponent($("channel").value) +
               "&machine=" + encodeURIComponent($("machine").value);
    ws = new WebSocket(proto + "//" + location.host + "/ws/updates?" + qs);
    ws.onopen = () => { $("conn").textContent = "online"; log("connected to update server"); };
    ws.onclose = () => { $("conn").textContent = "offline"; setTimeout(connect, 3000); };
    ws.onerror = () => { $("conn").textContent = "error"; };
    ws.onmessage = (ev) => {
      const m = JSON.parse(ev.data);
      if (m.t === "catalog") { catalog = m; render(); }
      else if (m.t === "new_release") {
        log("NEW: " + m.channel + " " + m.version + " just published");
        refresh();
      } else if (m.t === "catalog_changed") { refresh(); }
    };
  }

  function refresh() {
    if (ws && ws.readyState === 1) {
      ws.send(JSON.stringify({ t: "refresh", channel: $("channel").value, machine: $("machine").value }));
    }
  }

  function render() {
    if (!catalog) return;
    $("srvtime").textContent = "server " + fmtDate(catalog.server_time) + " · " + catalog.public_url;
    const seen = seenSet();
    const rels = catalog.releases || [];
    const box = $("releases");
    box.innerHTML = "";
    const freshCount = rels.filter((r) => !seen.has(r.id)).length;
    $("summary").textContent = rels.length
      ? (rels.length + " release(s) available" + (freshCount ? ", " + freshCount + " new since you last looked" : ", all seen"))
      : "No updates published on this channel yet.";

    rels.forEach((r) => {
      const fresh = !seen.has(r.id);
      const card = document.createElement("div");
      card.className = "card";
      const head = document.createElement("h3");
      head.innerHTML =
        '<input type="checkbox" class="chk relchk" data-rel="' + r.id + '"> ' +
        '<span>' + escapeHtml(r.title || (r.channel + " " + r.version)) + '</span> ' +
        '<span class="badge">' + r.channel + "</span> " +
        '<span class="badge">v' + escapeHtml(r.version) + "</span> " +
        (r.machine ? '<span class="badge">' + r.machine + "</span> " : "") +
        (fresh ? '<span class="badge sel">NEW</span>' : "") +
        (r.yanked ? '<span class="badge">yanked</span>' : "");
      card.appendChild(head);

      const meta = document.createElement("div");
      meta.className = "muted";
      meta.textContent = fmtDate(r.created_at) + " · " + fmtBytes(r.total_bytes) +
                         " · " + r.artifacts.length + " item(s)";
      card.appendChild(meta);

      if (r.notes) {
        const n = document.createElement("div");
        n.style.margin = "6px 0"; n.style.whiteSpace = "pre-wrap";
        n.textContent = r.notes;
        card.appendChild(n);
      }

      const tbl = document.createElement("table");
      tbl.innerHTML = "<tr><th></th><th>Item</th><th>Kind</th><th>Where</th><th>Size</th></tr>";
      r.artifacts.forEach((a) => {
        const tr = document.createElement("tr");
        const key = a.sha256;
        tr.innerHTML =
          '<td><input type="checkbox" class="chk artchk" data-sha="' + key +
          '" data-name="' + escapeHtml(a.name) + '"></td>' +
          "<td>" + escapeHtml(a.name) + "</td>" +
          "<td>" + a.kind + "</td>" +
          "<td>" + escapeHtml(a.path || "—") + "</td>" +
          "<td>" + fmtBytes(a.size) + "</td>";
        tbl.appendChild(tr);
      });
      card.appendChild(tbl);
      box.appendChild(card);
    });

    // restore selection state
    document.querySelectorAll(".artchk").forEach((c) => {
      c.checked = selected.has(c.dataset.sha);
      c.addEventListener("change", onArtToggle);
    });
    document.querySelectorAll(".relchk").forEach((c) => {
      c.addEventListener("change", (e) => {
        const rid = +e.target.dataset.rel;
        const rel = rels.find((x) => x.id === rid);
        rel.artifacts.forEach((a) => {
          if (e.target.checked) selected.add(a.sha256); else selected.delete(a.sha256);
        });
        render();
      });
    });
    markSeen(rels.map((r) => r.id));
  }

  function onArtToggle(e) {
    const sha = e.target.dataset.sha;
    if (e.target.checked) selected.add(sha); else selected.delete(sha);
  }

  function selectAll(on) {
    selected = new Set();
    if (on && catalog) catalog.releases.forEach((r) => r.artifacts.forEach((a) => selected.add(a.sha256)));
    render();
  }

  async function downloadSelected() {
    if (!selected.size) { log("nothing selected"); return; }
    log("downloading " + selected.size + " item(s)…");
    // Map sha -> friendly name from the current catalog.
    const names = {};
    catalog.releases.forEach((r) => r.artifacts.forEach((a) => { names[a.sha256] = a.name; }));
    for (const sha of selected) {
      const a = document.createElement("a");
      a.href = "/api/blob/" + sha;
      a.download = (names[sha] || sha);
      document.body.appendChild(a); a.click(); a.remove();
      log("↓ " + (names[sha] || sha));
      await new Promise((r) => setTimeout(r, 300)); // stagger so the browser allows multiple
    }
  }

  function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, (c) =>
      ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
  }

  // wiring
  $("refreshBtn").onclick = refresh;
  $("channel").onchange = () => { if (ws) ws.close(); connect(); };
  $("machine").onchange = refresh;
  $("selAll").onclick = () => selectAll(true);
  $("selNone").onclick = () => selectAll(false);
  $("downloadBtn").onclick = downloadSelected;
  $("darkBtn").onclick = () => {
    document.body.classList.toggle("dark");
    document.body.classList.toggle("light");
  };

  connect();
})();
