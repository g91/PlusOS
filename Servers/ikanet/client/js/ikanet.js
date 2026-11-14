/* IKANET client. Vanilla JS, no deps.
   Talks to the FastAPI server over /api and /ws. */

const $  = (s, r = document) => r.querySelector(s);
const $$ = (s, r = document) => [...r.querySelectorAll(s)];

const API = ""; // same origin
const state = {
  token: null,
  user: null,            // {id, username, icon, appear_offline}
  ws: null,
  channels: [],          // [{id,name,is_dm}]
  current: null,         // current channel id
  quote: "LONG LIVE THE APPLE III",
  downloadMode: false,
  pingSamples: [],
  settings: loadSettings(),
};

/* ---------------------------------------------------------------- settings */
function loadSettings() {
  let s = {};
  try { s = JSON.parse(localStorage.getItem("ikanet.settings") || "{}"); } catch {}
  return Object.assign({
    systemCursor: false,   // squid cursor for the whole page/system
    dark: true,
    font: "ChicagoFLF",
    vramMB: 4,             // "video RAM available"
    depth: "1-bit",        // mono, matching PlusOS gfx
    resolution: "512x342", // Mac Plus default
    scanlines: false,
  }, s);
}
function saveSettings() {
  try { localStorage.setItem("ikanet.settings", JSON.stringify(state.settings)); } catch {}
  applySettings();
}
function applySettings() {
  const s = state.settings;
  document.body.classList.toggle("squidcursor", s.systemCursor);
  document.body.classList.toggle("light", !s.dark);
  const f = s.font === "compact" ? "ChicagoFLF" : s.font;
  document.documentElement.style.setProperty("--font",
    `"${f}", "Chicago", "Geneva", system-ui, sans-serif`);
  document.body.style.fontSize = s.font === "compact" ? "12px" : "14px";
  document.body.style.filter = s.scanlines
    ? "none" : "none";
  // scanline overlay
  let sl = $("#scanlines");
  if (s.scanlines) {
    if (!sl) {
      sl = document.createElement("div");
      sl.id = "scanlines";
      sl.style.cssText = "position:fixed;inset:0;pointer-events:none;z-index:9999;" +
        "background:repeating-linear-gradient(#0000 0 1px,#0003 1px 2px)";
      document.body.appendChild(sl);
    }
  } else if (sl) sl.remove();
}

/* ---------------------------------------------------------------- login */
async function post(path, body, auth = false) {
  const headers = { "Content-Type": "application/json" };
  if (auth && state.token) headers["Authorization"] = "Bearer " + state.token;
  const r = await fetch(API + path, { method: "POST", headers, body: JSON.stringify(body) });
  if (!r.ok) {
    let msg = "Error " + r.status;
    try { msg = (await r.json()).detail || msg; } catch {}
    throw new Error(msg);
  }
  return r.json();
}

function showLoginError(msg) { $("#loginErr").textContent = msg || ""; }

async function doAuth(kind) {
  const username = $("#luser").value.trim();
  const password = $("#lpass").value;
  showLoginError("");
  try {
    const path = kind === "join" ? "/api/join" : "/api/login";
    const body = { username, password };
    if (kind === "join") body.icon = state.user?.icon || defaultIcon();
    const res = await post(path, body);
    state.token = res.token;
    state.user = res.user;
    localStorage.setItem("ikanet.token", res.token);
    enterChat();
  } catch (e) { showLoginError(e.message); }
}

/* try to resume a saved session via the WS welcome */
async function tryResume() {
  const t = localStorage.getItem("ikanet.token");
  if (!t) return false;
  state.token = t;
  return new Promise((resolve) => {
    openSocket(() => resolve(true), () => { state.token = null; resolve(false); });
  });
}

/* ---------------------------------------------------------------- icon */
function defaultIcon() {
  // 16x16 all-off except a dot — user can edit later
  const a = Array(256).fill("0");
  a[8 * 16 + 8] = "1";
  return a.join("");
}
function iconToDataURL(bits, scale = 1, light = false) {
  if (!bits || bits.length < 256) bits = defaultIcon();
  const c = document.createElement("canvas");
  c.width = c.height = 16 * scale;
  const g = c.getContext("2d");
  g.imageSmoothingEnabled = false;
  g.fillStyle = light ? "#fff" : "#000";
  g.fillRect(0, 0, c.width, c.height);
  g.fillStyle = light ? "#000" : "#fff";
  for (let i = 0; i < 256; i++) {
    if (bits[i] === "1") g.fillRect((i % 16) * scale, ((i / 16) | 0) * scale, scale, scale);
  }
  return c.toDataURL();
}
function refreshIcons() {
  const url = iconToDataURL(state.user.icon, 2, !state.settings.dark);
  $("#tbIcon").src = url;
  $("#composerIcon").src = url;
}

/* ---------------------------------------------------------------- socket */
function wsURL() {
  const proto = location.protocol === "https:" ? "wss" : "ws";
  return `${proto}://${location.host}/ws?token=${encodeURIComponent(state.token)}`;
}
function openSocket(onReady, onFail) {
  let ws;
  try { ws = new WebSocket(wsURL()); } catch { onFail && onFail(); return; }
  state.ws = ws;
  ws.onopen = () => startPing();
  ws.onclose = (e) => { stopPing(); if (e.code === 4001) onFail && onFail(); };
  ws.onerror = () => {};
  ws.onmessage = (ev) => {
    const m = JSON.parse(ev.data);
    handleServer(m, onReady);
  };
}
function send(obj) {
  if (state.ws && state.ws.readyState === 1) state.ws.send(JSON.stringify(obj));
}

function handleServer(m, onReady) {
  switch (m.t) {
    case "welcome":
      state.user = m.user;
      state.channels = m.channels;
      state.quote = m.quote;
      $("#quote").textContent = m.quote;
      if (onReady) onReady();
      enterChat(true);
      // subscribe to first channel
      if (state.channels.length) switchChannel(state.channels[0].id);
      break;
    case "history":
      if (m.channel_id === state.current) {
        $("#messages").innerHTML = "";
        m.messages.forEach(renderMessage);
        scrollBottom();
      }
      break;
    case "msg":
      if (m.channel_id === state.current) { renderMessage(m); scrollBottom(); }
      break;
    case "presence": break; // (room roster — reserved for a future sidebar)
    case "pong": onPong(m.n); break;
    case "channel_added":
      upsertChannel(m.channel); switchChannel(m.channel.id); break;
    case "channel_left":
      state.channels = state.channels.filter(c => c.id !== m.channel_id);
      if (state.current === m.channel_id && state.channels[0]) switchChannel(state.channels[0].id);
      break;
    case "dm_opened":
      upsertChannel(m.channel); switchChannel(m.channel.id); break;
    case "channel_list":
      openChannelList(m.public, m.saved); break;
    case "icon_set":
      state.user.icon = m.icon; refreshIcons(); break;
    case "error":
      flashActivity(m.msg); break;
  }
}
function upsertChannel(ch) {
  if (!state.channels.find(c => c.id === ch.id)) state.channels.push(ch);
}

/* ---------------------------------------------------------------- ping meter */
let pingTimer = null;
function startPing() {
  stopPing();
  const beat = () => { send({ t: "ping", n: performance.now() }); };
  beat();
  pingTimer = setInterval(beat, 2000);
  // recompute the displayed average every 10s
  setInterval(updatePingDisplay, 10000);
}
function stopPing() { if (pingTimer) clearInterval(pingTimer); pingTimer = null; }
function onPong(n) {
  const rtt = performance.now() - n;
  state.pingSamples.push(rtt);
  if (state.pingSamples.length > 20) state.pingSamples.shift();
  if (state.pingSamples.length <= 3) updatePingDisplay();
}
function updatePingDisplay() {
  if (!state.pingSamples.length) return;
  const avg = Math.round(state.pingSamples.reduce((a, b) => a + b, 0) / state.pingSamples.length);
  let label, bars;
  if (avg < 80)       { label = "Nice Ping!"; bars = 4; }
  else if (avg < 200) { label = "OK Ping";    bars = 3; }
  else if (avg < 450) { label = "Slow Ping…"; bars = 2; }
  else                { label = "Slow Ping…"; bars = 1; }
  $("#pingLabel").textContent = label;
  $("#pingMs").textContent = avg;
  $$("#pingBars i").forEach((el, i) => el.classList.toggle("off", i >= bars));
}

/* ---------------------------------------------------------------- rendering */
function renderMessage(m) {
  const wrap = document.createElement("div");
  wrap.className = "msg" + (m.system ? " system" : "");
  if (m.system) {
    wrap.innerHTML = `<span class="text"><span class="sysbox">${esc(m.body)}</span></span>`;
  } else {
    const icon = document.createElement("img");
    icon.className = "icon";
    icon.src = iconToDataURL(m.icon, 1, !state.settings.dark);
    wrap.appendChild(icon);
    const text = document.createElement("span");
    text.className = "text";
    text.innerHTML = `|<b class="name">${esc(m.user)}</b>| `;
    if (m.body) text.appendChild(document.createTextNode(m.body));
    if (m.file) text.appendChild(fileChip(m.file));
    wrap.appendChild(text);
  }
  $("#messages").appendChild(wrap);
}

function fileChip(f) {
  const chip = document.createElement("span");
  chip.className = "filechip";
  const ic = document.createElement("img");
  ic.className = "ic";
  ic.src = f.is_image ? "assets/cursor.png" : "assets/cursor_dl.png";
  chip.appendChild(ic);
  const label = document.createElement("span");
  label.textContent = f.is_image ? "[Click Icon to Load Image File]" : "[Click Icon to See File Information.]";
  chip.appendChild(label);
  chip.onclick = () => {
    if (state.downloadMode) { downloadFile(f); exitDownloadMode(); }
    else if (f.is_image)    openImagePreview(f);
    else                    openFileInfo(f);
  };
  return chip;
}

function scrollBottom() { const m = $("#messages"); m.scrollTop = m.scrollHeight; }
function esc(s) { return (s ?? "").replace(/[&<>]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;" }[c])); }

/* ---------------------------------------------------------------- channels */
function switchChannel(cid) {
  state.current = cid;
  $("#messages").innerHTML = "";
  send({ t: "subscribe", channel_id: cid });
  const ch = state.channels.find(c => c.id === cid);
  if (ch) document.title = "IKANET — " + ch.name;
}

/* ---------------------------------------------------------------- sending */
function sendMessage() {
  const input = $("#composerInput");
  const body = input.value;
  if (!body.trim()) return;
  flashActivity("SWIMMING, SWIMMING");
  send({ t: "say", channel_id: state.current, body });
  input.value = "";
  setTimeout(() => flashActivity(""), 600);
}

/* ---------------------------------------------------------------- uploads */
function pickAndUpload() {
  const inp = $("#fileInput");
  inp.value = "";
  inp.onchange = async () => {
    const f = inp.files[0];
    if (!f) return;
    const isImg = f.type.startsWith("image/");
    flashActivity(isImg ? "INKING" : "SWIMMING, SWIMMING", true);
    try {
      const fd = new FormData();
      fd.append("file", f);
      const r = await fetch(API + "/api/upload", {
        method: "POST",
        headers: { Authorization: "Bearer " + state.token },
        body: fd,
      });
      if (!r.ok) throw new Error("upload failed");
      const { file } = await r.json();
      // uploading a file sends it immediately, independent of the text box
      send({ t: "say", channel_id: state.current, file_id: file.id });
    } catch (e) { flashActivity("INK SPILL!"); }
    finally { setTimeout(() => flashActivity(""), 800); }
  };
  inp.click();
}

async function downloadFile(f) {
  flashActivity("SAVING, SAVING", true);
  try {
    const r = await fetch(API + "/api/file/" + f.id, {
      headers: { Authorization: "Bearer " + state.token },
    });
    const blob = await r.blob();
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url; a.download = f.name; a.click();
    URL.revokeObjectURL(url);
  } catch {}
  finally { setTimeout(() => flashActivity(""), 900); }
}

/* animated activity text; keep=true keeps the squid "swimming" */
let activityKeep = false;
function flashActivity(text, keep = false) {
  activityKeep = keep;
  $("#activity").textContent = text || "";
}

/* ---------------------------------------------------------------- download squid mode */
function enterDownloadMode() {
  state.downloadMode = true;
  $("#chatScreen").classList.add("dlmode");
  flashActivity("CLICK A FILE TO DOWNLOAD");
}
function exitDownloadMode() {
  state.downloadMode = false;
  $("#chatScreen").classList.remove("dlmode");
}

/* ---------------------------------------------------------------- floating windows */
let zTop = 100;
function makeWindow(cls, title, bodyEl, x = 120, y = 90) {
  const w = document.createElement("div");
  w.className = "win " + cls;
  w.style.left = x + "px"; w.style.top = y + "px"; w.style.zIndex = ++zTop;
  const bar = document.createElement("div");
  bar.className = "wt";
  bar.innerHTML = `<span class="close"></span><span class="ttl">${esc(title)}</span><span style="width:12px"></span>`;
  const body = document.createElement("div");
  body.className = "wb";
  body.appendChild(bodyEl);
  w.appendChild(bar); w.appendChild(body);
  $("#chatScreen").appendChild(w);
  bar.querySelector(".close").onclick = () => w.remove();
  w.onmousedown = () => { w.style.zIndex = ++zTop; };
  dragify(w, bar);
  return w;
}
function dragify(win, handle) {
  let sx, sy, ox, oy, drag = false;
  handle.addEventListener("mousedown", e => {
    if (e.target.classList.contains("close")) return;
    drag = true; sx = e.clientX; sy = e.clientY;
    const r = win.getBoundingClientRect();
    const p = $("#chatScreen").getBoundingClientRect();
    ox = r.left - p.left; oy = r.top - p.top;
    e.preventDefault();
  });
  window.addEventListener("mousemove", e => {
    if (!drag) return;
    win.style.left = (ox + e.clientX - sx) + "px";
    win.style.top  = (oy + e.clientY - sy) + "px";
  });
  window.addEventListener("mouseup", () => drag = false);
}

function openImagePreview(f) {
  const img = document.createElement("img");
  img.className = "preview";
  img.src = API + "/api/thumb/" + f.id;
  img.style.maxWidth = "260px";
  makeWindow("imgwin", f.name, img, 180, 80);
}
function openFileInfo(f) {
  const el = document.createElement("div");
  el.innerHTML =
    `<div><b>${esc(f.name)}</b></div>` +
    `<div>Kind: ${esc(f.mime)}</div>` +
    `<div>Size: ${fmtSize(f.size)}</div>` +
    (f.width ? `<div>Dimensions: ${f.width}×${f.height}</div>` : "") +
    `<div style="margin-top:8px"><button class="btn sq" id="dlBtn">Download</button></div>`;
  const w = makeWindow("infowin", "File Information", el, 200, 110);
  w.querySelector("#dlBtn").onclick = () => downloadFile(f);
}
function fmtSize(n) {
  if (n < 1024) return n + " B";
  if (n < 1024 * 1024) return (n / 1024).toFixed(1) + " K";
  return (n / 1024 / 1024).toFixed(1) + " M";
}

/* ---------------------------------------------------------------- icon editor */
function openIconEditor() {
  const el = document.createElement("div");
  el.className = "iconed";
  const grid = document.createElement("div");
  grid.className = "grid";
  const bits = (state.user.icon && state.user.icon.length >= 256
    ? state.user.icon : defaultIcon()).split("");
  for (let i = 0; i < 256; i++) {
    const c = document.createElement("div");
    if (bits[i] === "1") c.classList.add("on");
    const paint = () => { c.classList.toggle("on"); bits[i] = c.classList.contains("on") ? "1" : "0"; };
    c.addEventListener("mousedown", paint);
    c.addEventListener("mouseenter", e => { if (e.buttons) { c.classList.add("on"); bits[i] = "1"; } });
    grid.appendChild(c);
  }
  el.appendChild(grid);
  const tools = document.createElement("div");
  tools.className = "tools";
  tools.innerHTML = `<button class="btn sq" id="icSave">Save</button>
                     <button class="btn sq" id="icClear">Clear</button>`;
  el.appendChild(tools);
  const w = makeWindow("iconwin", "User Icon", el, 240, 70);
  w.querySelector("#icSave").onclick = () => {
    const str = bits.join("");
    state.user.icon = str;
    send({ t: "set_icon", icon: str });
    refreshIcons();
    w.remove();
  };
  w.querySelector("#icClear").onclick = () => {
    $$(".iconed .grid div", w).forEach((d, i) => { d.classList.remove("on"); bits[i] = "0"; });
  };
}

/* ---------------------------------------------------------------- channel list */
function openChannelList(pub, saved) {
  const el = document.createElement("div");
  const savedIds = new Set(saved.map(c => c.id));
  const list = [...saved, ...pub.filter(c => !savedIds.has(c.id))];
  list.forEach(ch => {
    const row = document.createElement("div");
    row.className = "ch" + (ch.id === state.current ? " active" : "");
    row.innerHTML = `<span>${esc(ch.name)}</span><span>${savedIds.has(ch.id) ? "★" : ""}</span>`;
    row.onclick = () => {
      if (!savedIds.has(ch.id)) { upsertChannel(ch); }
      switchChannel(ch.id);
    };
    el.appendChild(row);
  });
  const add = document.createElement("div");
  add.style.marginTop = "8px";
  add.innerHTML = `<input id="newChan" placeholder="#new-room" style="width:120px">
                   <button class="btn sq" id="addChan">Add</button>
                   <div style="margin-top:6px"><input id="dmUser" placeholder="user for DM" style="width:120px">
                   <button class="btn sq" id="openDm">DM</button></div>`;
  el.appendChild(add);
  const w = makeWindow("chanlist", "Channel List", el, 150, 70);
  w.querySelector("#addChan").onclick = () => {
    const n = w.querySelector("#newChan").value.trim();
    if (n) send({ t: "add_channel", name: n });
    w.remove();
  };
  w.querySelector("#openDm").onclick = () => {
    const u = w.querySelector("#dmUser").value.trim();
    if (u) send({ t: "open_dm", username: u });
    w.remove();
  };
}

/* ---------------------------------------------------------------- settings window */
function openSettings() {
  const s = state.settings;
  const el = document.createElement("div");
  el.innerHTML = `
    <h3>Appearance</h3>
    <div class="field"><label>Dark Mode</label><input type="checkbox" id="setDark" ${s.dark ? "checked" : ""}></div>
    <div class="field"><label>Scanlines</label><input type="checkbox" id="setScan" ${s.scanlines ? "checked" : ""}></div>
    <div class="field"><label>Font</label>
      <select id="setFont">
        ${["ChicagoFLF", "Times New Roman", "Arial", "compact"].map(f =>
          `<option value="${f}" ${s.font === f ? "selected" : ""}>${f === "ChicagoFLF" ? "Chicago" : f === "compact" ? "Compact" : f}</option>`).join("")}
      </select>
    </div>

    <h3>Cursor</h3>
    <div class="field"><label>System Squid Cursor<br><small>applies the squid cursor to the whole system</small></label>
      <input type="checkbox" id="setCursor" ${s.systemCursor ? "checked" : ""}></div>

    <h3>Video</h3>
    <div class="field"><label>Resolution</label>
      <select id="setRes">
        ${["512x342", "640x480", "800x600"].map(r =>
          `<option ${s.resolution === r ? "selected" : ""}>${r}</option>`).join("")}
      </select>
    </div>
    <div class="field"><label>Color Depth</label>
      <select id="setDepth">
        ${["1-bit", "4-bit", "8-bit"].map(d =>
          `<option ${s.depth === d ? "selected" : ""}>${d}</option>`).join("")}
      </select>
    </div>
    <div class="field"><label>Video RAM</label><span id="vramLbl">${s.vramMB} MB</span></div>
    <input type="range" id="setVram" min="1" max="8" step="1" value="${s.vramMB}" style="width:100%">
    <div class="vram"><i id="vramBar"></i></div>
    <div id="vramInfo"><small></small></div>
  `;
  const w = makeWindow("settings", "Settings", el, 160, 50);

  const recalcVram = () => {
    const [rw, rh] = w.querySelector("#setRes").value.split("x").map(Number);
    const bpp = { "1-bit": 1, "4-bit": 4, "8-bit": 8 }[w.querySelector("#setDepth").value];
    const needBytes = Math.ceil(rw * rh * bpp / 8);
    const haveBytes = w.querySelector("#setVram").value * 1024 * 1024;
    const pct = Math.min(100, Math.round(needBytes / haveBytes * 100));
    w.querySelector("#vramBar").style.width = pct + "%";
    w.querySelector("#vramInfo").innerHTML =
      `<small>Framebuffer needs ${(needBytes / 1024).toFixed(0)} K of ${w.querySelector("#setVram").value} MB ` +
      `(${pct}% used)${needBytes > haveBytes ? " — NOT ENOUGH VRAM!" : ""}</small>`;
  };

  w.querySelector("#setVram").oninput = e => { w.querySelector("#vramLbl").textContent = e.target.value + " MB"; recalcVram(); };
  w.querySelector("#setRes").onchange = recalcVram;
  w.querySelector("#setDepth").onchange = recalcVram;
  const apply = () => {
    s.dark = w.querySelector("#setDark").checked;
    s.scanlines = w.querySelector("#setScan").checked;
    s.font = w.querySelector("#setFont").value;
    s.systemCursor = w.querySelector("#setCursor").checked;
    s.resolution = w.querySelector("#setRes").value;
    s.depth = w.querySelector("#setDepth").value;
    s.vramMB = +w.querySelector("#setVram").value;
    saveSettings();
    refreshIcons();
  };
  w.querySelectorAll("input,select").forEach(i => i.addEventListener("change", apply));
  recalcVram();
}

/* ---------------------------------------------------------------- menus */
function wireMenus() {
  const bar = $("#menubar");
  $$(".menubar .m").forEach(m => {
    m.addEventListener("click", (e) => {
      const act = m.dataset.menu;
      if (act === "darkmode") { toggleDark(); return; }
      if (act === "settings") { openSettings(); return; }
      // toggle dropdown
      const wasOpen = m.classList.contains("open");
      $$(".menubar .m").forEach(x => x.classList.remove("open"));
      if (!wasOpen && m.querySelector(".menu")) m.classList.add("open");
      e.stopPropagation();
    });
  });
  document.addEventListener("click", () => $$(".menubar .m").forEach(x => x.classList.remove("open")));

  $$(".menu .item").forEach(it => {
    it.addEventListener("click", (e) => {
      e.stopPropagation();
      $$(".menubar .m").forEach(x => x.classList.remove("open"));
      if (it.dataset.font) { setFont(it.dataset.font); return; }
      handleMenuAct(it.dataset.act);
    });
  });
  updateFontChecks();
}

function handleMenuAct(act) {
  switch (act) {
    case "upload": pickAndUpload(); break;
    case "download": enterDownloadMode(); break;
    case "appear-online":  setAppear(false); break;
    case "appear-offline": setAppear(true); break;
    case "icon-editor": openIconEditor(); break;
    case "logout": logout(); break;
    case "copy": document.execCommand("copy"); break;
    case "paste":
      navigator.clipboard?.readText().then(t => { $("#composerInput").value += t; }).catch(() => {});
      break;
    case "chan-list": send({ t: "list_channels" }); break;
    case "chan-add": {
      const n = prompt("Channel name (e.g. #retro):");
      if (n) send({ t: "add_channel", name: n });
      break;
    }
    case "chan-leave":
      if (state.current) send({ t: "leave_channel", channel_id: state.current });
      break;
  }
}

function setFont(f) { state.settings.font = f; saveSettings(); updateFontChecks(); }
function updateFontChecks() {
  $$(".menu .item[data-font]").forEach(it =>
    it.classList.toggle("checked", it.dataset.font === state.settings.font));
}
function toggleDark() {
  state.settings.dark = !state.settings.dark;
  $("#darkToggle").textContent = state.settings.dark ? "LightMode" : "DarkMode";
  saveSettings(); refreshIcons();
}
function setAppear(off) {
  state.user.appear_offline = off;
  send({ t: "appear_offline", value: off });
  $("#tbOnline").textContent = off ? "Offline" : "Online";
  $("#tbAppear").textContent = off ? "● Appearing As Offline" : "○ Appearing As Offline";
}
function logout() {
  post("/api/logout", {}, true).catch(() => {});
  localStorage.removeItem("ikanet.token");
  state.token = null; state.user = null;
  if (state.ws) state.ws.close();
  location.reload();
}

/* ---------------------------------------------------------------- enter chat */
function enterChat(fromWelcome = false) {
  $("#loginScreen").classList.add("hidden");
  $("#chatScreen").classList.remove("hidden");
  $("#tbName").textContent = state.user.username;
  $("#darkToggle").textContent = state.settings.dark ? "LightMode" : "DarkMode";
  refreshIcons();
  applySettings();
  if (!fromWelcome && !state.ws) openSocket();
}

/* ---------------------------------------------------------------- wire up */
function wire() {
  applySettings();
  $("#btnJoin").onclick = () => doAuth("join");
  $("#btnLogin").onclick = () => doAuth("login");
  $("#lpass").addEventListener("keydown", e => { if (e.key === "Enter") doAuth("login"); });

  $("#btnSend").onclick = sendMessage;
  $("#composerInput").addEventListener("keydown", e => { if (e.key === "Enter") sendMessage(); });
  $("#btnUpload").onclick = pickAndUpload;
  $("#btnDownload").onclick = () => state.downloadMode ? exitDownloadMode() : enterDownloadMode();
  $("#tbAppear").onclick = () => setAppear(!state.user.appear_offline);

  wireMenus();

  // attempt silent resume
  tryResume();
}

document.addEventListener("DOMContentLoaded", wire);
