# IKANET: Chat Client & Server

**IKANET** is a retro chat network for PlusOS — accounts, chat rooms, private messages, 16×16 pixel profile icons, and file/image sharing — presented in a pixel-perfect System 6/7 style. It comes in two matching clients that talk to the same server and share the same rooms:

- a **native Macintosh application** (`ikanet`) that runs on PlusOS and connects over [PlusNET](/wiki/09-networking-plusnet), and
- a **standalone web client** you open in any modern browser.

A browser user and a 68000 Macintosh user can sit in `#general` together and see each other's messages in real time.

---

## 1. What It Looks Like

```text
+---------------------------------------------------------------+
| @ File  Edit  Channel  Font  DarkMode                         |  menu bar
+---------------------------------------------------------------+
| [IC] skittles666 (Online)  o Appearing As Offline  Nice Ping! |  status row
+---------------------------------------------------------------+
| [IC] |DoodleBop|  mf jdsomf jwmfv jredmf jwk               ^  |
| [ DoodleBop was timed out for 24 hours ]                   #  |  messages
| [IC] |JustSomeGuy|  its really starting to hurt            v  |
+---------------------------------------------------------------+
| [IC] | what is your problem m8              | DL | UP | SEND  |  composer
+---------------------------------------------------------------+
| LONG LIVE THE APPLE III        SWIMMING, SWIMMING   IKANET -> |  status strip
+---------------------------------------------------------------+
```

Every account has a **16×16 one-bit profile icon** that shows next to each of its messages, in the title bar, and in the composer — just like the little icons beside names in a classic Mac chat.

---

## 2. Getting On IKANET

### From a browser

1. Open **https://ikanet.d0a.net/**.
2. Type a **User** name and **Pass**, then click **JOIN IKANET** to create an account (or **LOGIN** if you already have one).
3. You start in **#general**. Open a second window, join as someone else, and chat.

### From PlusOS (a real Macintosh)

The native client is the `ikanet` application. It needs a working [PlusNET](/wiki/09-networking-plusnet) connection (a BlueSCSI / DaynaPORT adapter), because the web browser emulator does **not** emulate network hardware.

```text
ikanet                         # connects to ikanet.d0a.net automatically
ikanet myserver.example 80 /ws # or point it anywhere: HOST PORT PATH
```

At the login screen, click **Edit Icon** to draw your 16×16 icon, enter a user and pass, then **JOIN IKANET** or **LOGIN**.

---

## 3. Using the Client

### The menu bar

| Menu | Items |
|---|---|
| **File** | Upload File, Download File, Appear Online, Appear Offline, **User Icon…**, Log Out |
| **Edit** | Copy, Paste |
| **Channel** | Go To Channel List, Add Channel, Leave Channel |
| **Font** | Chicago, Times New Roman, Arial, Compact |
| **DarkMode** | Toggles DarkMode / LightMode (a simple black↔white invert) |

### Rooms and private messages

- **Channels** are named with a `#`, e.g. `#general`, `#swimming`, `#gameboy`. Use **Channel ▸ Add Channel** to join or create one, and **Leave Channel** to drop it.
- **Direct messages** open a private room between you and one other user.

### Your 16×16 profile icon

Choose **File ▸ User Icon…** (or **Edit Icon** on the login screen) to open the editor: a 16×16 grid you click and drag to paint. **Save** sends it to the server, and from then on it rides along with every message you post so everyone sees it next to your name.

### Sharing files and images (the squids)

- The **UP** (upload) squid opens a file picker. The file uploads and posts to the room **immediately** — on its own, separate from whatever you've typed in the box.
- The **DL** (download) squid turns the pointer into a download cursor; click any file or image in the chat to save it. The status strip reads **SAVING, SAVING** while it transfers.
- Click an **image** attachment to open a small draggable preview window; click a non-image **file** to see its information (kind, size, dimensions) with a Download button.

### Status strip

Along the bottom: a random **Quote of the Day** (a cheeky vintage-computing line), an **activity** indicator (`SWIMMING, SWIMMING` while sending text, `INKING` while sending a picture), and the fixed **IKANET** mark.

### Ping meter

The top-right shows your live connection quality — **Nice Ping!**, **OK Ping**, or **Slow Ping…** — with signal bars and the average round-trip in milliseconds, refreshed every few seconds.

---

## 4. How It Works

Both clients speak **WebSocket** to a single endpoint, `/ws`, so everything rides one port behind Cloudflare (`wss://ikanet.d0a.net/ws` on 443). The native Macintosh client implements the WebSocket protocol itself on top of [PlusNET](/wiki/09-networking-plusnet) TCP — the 68000 never does TLS; that is terminated at the edge.

```text
 Browser   --- wss://ikanet.d0a.net/ws --+
                                          +-- Cloudflare (443, TLS) --> origin :8787
 PlusOS    --- ws:// (plain) ------------+          (FastAPI + WebSocket + SQLite)
```

The server is a small **FastAPI + WebSocket + SQLite** service. It stores accounts (passwords hashed with bcrypt), rooms, message history, 16×16 icons, and uploaded files, and broadcasts each message to everyone in the room — browser and Macintosh alike.

| Piece | Where |
|---|---|
| Native PlusOS app | the `ikanet` application (built with the [SDK](/wiki/12-sdk-and-app-development)) |
| Web client + server | the IKANET server (FastAPI, WebSocket, SQLite) |
| Transport | WebSocket on `/ws`, one port, Cloudflare-friendly |
| Profile icons | 16×16 one-bit bitmaps, drawn in the in-app editor |

> **Note:** live chat from the native client requires real networking hardware (a BlueSCSI / DaynaPORT adapter). Inside the browser-based PlusOS emulator the app opens and draws, but reports *PlusNET not available* because the emulator has no network adapter. Use the **web client** to chat from a browser.

---

## Navigation

- **Previous Chapter**: [← 20. OTA Update Server: Over-the-Air Updates](/wiki/20-ota-update-server)
- **Next Chapter**: [22. PlusUI: GUI Framework, Designer & Code Generator →](/wiki/22-plusui-gui-framework-and-designer)
- **Networking**: [9. PlusNET: DaynaPORT TCP/IP Stack →](/wiki/09-networking-plusnet)
- **Build Your Own App**: [12. SDK & Application Development Guide →](/wiki/12-sdk-and-app-development)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
