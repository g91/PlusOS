# PlusNET: DaynaPORT TCP/IP Stack

PlusOS brings the 1986 Macintosh Plus and 1987 Macintosh SE onto the modern Internet using **PlusNET**, an in-kernel TCP/IP networking stack created from scratch.

---

## 1. Hardware Architecture: BlueSCSI DaynaPORT Emulation

The **BlueSCSI V2** (based on the Raspberry Pi Pico W) contains built-in 802.11 Wi-Fi. It emulates a vintage **DaynaPORT SCSI/Link** Ethernet-over-SCSI adapter on the Macintosh SCSI bus:

```text
+-----------------------+
|  Internet / LAN WiFi  |
+-----------+-----------+
            | 802.11b/g/n
            v
+-----------------------+
|   BlueSCSI V2 Pico W  |
|  DaynaPORT Emulation  |
+-----------+-----------+
            | SCSI Bus (ID 4)
            v
+-----------------------+
|    Macintosh 5380     |
|    SCSI Controller    |
+-----------+-----------+
            |
            v
+-----------------------+
|   PlusNET Driver      |  Low-level SCSI packet ring buffer
+-----------+-----------+
            |
            v
+-----------------------+
| PlusNET TCP/IP Stack  |  ARP -> IPv4 -> ICMP / UDP / TCP
+-----------------------+
```

---

## 2. PlusNET Protocol Architecture

PlusNET does not depend on MacTCP or Open Transport. It is a completely autonomous C networking implementation (`<plusnet/net.h>`):
- **SCSI Link Layer**: Communicates with the DaynaPORT adapter by issuing SCSI commands through the ROM SCSI Manager (`_SCSIDispatch`).
- **ARP**: Address Resolution Protocol table with automatic cache aging.
- **IPv4**: Packet routing, header checksum verification, and fragmentation reassembly.
- **ICMP**: Echo request / reply engine powering the `ping` utility.
- **UDP**: Lightweight datagram transport powering DHCP and DNS.
- **DHCP Client**: Automatically obtains IP address, subnet mask, default gateway, and DNS servers on adapter startup.
- **DNS Resolver**: Resolves hostnames (such as `google.com` or `irc.libera.chat`) to IPv4 addresses.
- **TCP Engine**: Full 3-way handshake (`SYN`, `SYN-ACK`, `ACK`), sequence number tracking, sliding window flow control, retransmission timers, and connection teardown (`FIN`/`RST`).

---

## 3. Command Line & Network Utilities

| Command | Usage | Description |
|---|---|---|
| `net` | `net` | Initializes the DaynaPORT adapter via DHCP and prints IP, MAC, subnet, gateway, and DNS info. |
| `net ip` | `net ip <ip> <mask> <gw> <dns>` | Configures static IP settings. |
| `net stats`| `net stats` | Shows frame counters, packets received/transmitted, and SCSI error diagnostics. |
| `ping` | `ping google.com` | Sends ICMP echo requests and measures round-trip time in milliseconds. |
| `telnet` | `telnet towel.blinkenlights.nl` | Connects to remote Telnet servers (ASCII Star Wars, BBS networks, MUDs). |
| `irc` | `irc irc.libera.chat mynick #channel` | Live Internet Relay Chat client directly from your compact Mac! |
| `ikanet` | `ikanet` | The **IKANET** chat client — PlusOS's own chat network with accounts, rooms, DMs, and 16×16 profile icons. See [21. IKANET: Chat Client & Server](/wiki/21-ikanet-chat-client). |

---

## Navigation

- **Previous Chapter**: [← 08. Classic Mac 'APPL' Executable Loader](/wiki/08-classic-mac-loader)
- **Next Chapter**: [10. Fixed-Point 3D Engine & Graphics →](/wiki/10-graphics-and-3d-engine)
- **Package Management**: [15. Package Repository Server & Software Center →](/wiki/15-package-repository-and-network-management)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)
