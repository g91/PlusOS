"""Driver Descriptor Record (block 0) and Apple Partition Map (blocks 1..n).

Layout produced for `--format hda` (what Apple HD SC Setup writes, and what
BlueSCSI / the existing PlusOS images use):

    block 0        DDR  'ER'   block size 512, block count, driver table
    block 1..63    partition map ('PM' entries, one per block; rest zero)
    block 64..95   Apple_Driver43  (the SCSI driver)   - or Apple_Free
    block 96..     Apple_HFS       (the PlusOS volume)

The HFS partition starts at block 96 with or without a driver, so a driver
can be added later without moving the volume.
"""
import struct

SECTOR = 512
MAP_BLOCKS = 63            # partition map partition: blocks 1..63
DRIVER_START = 64
DRIVER_BLOCKS = 32
HFS_START = 96

PM_FMT = ">2sHIII32s32sIIIIIIIIII16s"
PM_FIELDS = ("sig pad map_entries start blocks name type data_start data_count status "
             "boot_start boot_size boot_addr boot_addr2 boot_entry boot_entry2 boot_cksum processor").split()
PM_SIZE = struct.calcsize(PM_FMT)
assert PM_SIZE == 136

# Boot arguments Apple's HD SC Setup driver ("Apple_Driver43") partitions carry
# right after pmProcessor; used only when --driver is given without metadata.
DEFAULT_BOOTARGS = "000106000000000000000001000700000000000000000000"


class APMError(Exception):
    pass


def driver_checksum(data):
    """The ROM's driver checksum over the first pmBootSize bytes."""
    s = 0
    for b in data:
        s = (s + b) & 0xFFFF
        s = ((s << 1) | (s >> 15)) & 0xFFFF
    return s or 0xFFFF


def _cstr(raw):
    return raw.split(b"\0")[0].decode("mac_roman")


def parse(f):
    """Parse DDR + partition map of an open image.

    Returns None if block 0 is not a DDR, else a dict:
      {'block_size', 'block_count', 'dev_type', 'dev_id', 'drivers': [(block, size, type)],
       'partitions': [ {index, name, type, start, blocks, status, boot_size, boot_cksum,
                        processor, bootargs(hex), raw} ]}
    """
    f.seek(0)
    ddr = f.read(SECTOR)
    if len(ddr) < SECTOR or ddr[:2] != b"ER":
        return None
    bsize, bcount, dtype, did, _data, ndrv = struct.unpack_from(">HIHHIH", ddr, 2)
    drivers = []
    for i in range(min(ndrv, 61)):
        drivers.append(struct.unpack_from(">IHH", ddr, 18 + 8 * i))
    parts, n, entries = [], 1, None
    while True:
        f.seek(n * SECTOR)
        pm = f.read(SECTOR)
        if len(pm) < SECTOR or pm[:2] != b"PM":
            break
        v = dict(zip(PM_FIELDS, struct.unpack_from(PM_FMT, pm)))
        if entries is None:
            entries = v["map_entries"]
        parts.append({
            "index": n, "name": _cstr(v["name"]), "type": _cstr(v["type"]),
            "start": v["start"], "blocks": v["blocks"], "status": v["status"],
            "data_start": v["data_start"], "data_count": v["data_count"],
            "boot_start": v["boot_start"], "boot_size": v["boot_size"],
            "boot_addr": v["boot_addr"], "boot_entry": v["boot_entry"],
            "boot_cksum": v["boot_cksum"], "processor": _cstr(v["processor"]),
            "bootargs": pm[PM_SIZE:PM_SIZE + 24].hex(), "map_entries": v["map_entries"], "raw": pm,
        })
        n += 1
        if entries and n > entries:
            break
    return {"block_size": bsize, "block_count": bcount, "dev_type": dtype, "dev_id": did,
            "drivers": drivers, "partitions": parts, "ddr": ddr}


def find_hfs(info):
    """First Apple_HFS partition of a parse() result (same rule as install.py)."""
    for p in info["partitions"]:
        if p["type"] == "Apple_HFS":
            return p
    return None


def find_driver(info):
    for p in info["partitions"]:
        if p["type"].startswith("Apple_Driver"):
            return p
    return None


def pm_entry(count, start, blocks, name, ptype, status, data_count=None, boot_size=0,
             boot_cksum=0, processor="", bootargs="", boot_addr=0, boot_entry=0):
    raw = struct.pack(PM_FMT, b"PM", 0, count, start, blocks,
                      name.encode("mac_roman"), ptype.encode("mac_roman"),
                      0, blocks if data_count is None else data_count, status,
                      0, boot_size, boot_addr, 0, boot_entry, 0, boot_cksum,
                      processor.encode("ascii"))
    raw += bytes.fromhex(bootargs) if bootargs else b""
    return raw.ljust(SECTOR, b"\0")


def build(total_blocks, driver=None, hfs_name="MacOS", hfs_start=HFS_START):
    """Build blocks 0..hfs_start-1 for a fresh disk.

    `driver` is None or a dict {data, type, name, processor, bootargs, boot_size,
    blocks, dd_type, status}.  Returns (prefix bytes, hfs_start, hfs_blocks).
    """
    if hfs_start < 2:
        raise APMError("the HFS partition cannot start before block 2")
    map_blocks = min(MAP_BLOCKS, hfs_start - 1)
    hfs_blocks = total_blocks - hfs_start
    if hfs_blocks < 64:
        raise APMError("disk too small")
    entries = [("Apple", "Apple_partition_map", 1, map_blocks, 0x37, {})]
    gap_start = 1 + map_blocks
    gap = hfs_start - gap_start
    ddr_drivers = b""
    ndrv = 0
    drv_blob = b""
    if driver:
        data = driver["data"]
        boot_size = driver.get("boot_size") or len(data)
        if boot_size > len(data):
            raise APMError("driver boot_size %d exceeds the driver file (%d bytes)" % (boot_size, len(data)))
        blocks = driver.get("blocks") or max(DRIVER_BLOCKS, -(-len(data) // SECTOR))
        if len(data) > blocks * SECTOR:
            raise APMError("driver (%d bytes) does not fit its %d-block partition" % (len(data), blocks))
        if blocks > gap:
            raise APMError("driver partition needs %d blocks but only %d are available before the "
                           "HFS partition at block %d (use a larger --hfs-start)" % (blocks, gap, hfs_start))
        extra = {"boot_size": boot_size, "boot_cksum": driver_checksum(data[:boot_size]),
                 "processor": driver.get("processor", "68000"),
                 "bootargs": driver.get("bootargs", DEFAULT_BOOTARGS)}
        entries.append((driver.get("name", "Macintosh"), driver.get("type", "Apple_Driver43"),
                        gap_start, blocks, driver.get("status", 0x7F), extra))
        ddr_drivers = struct.pack(">IHH", gap_start, -(-boot_size // SECTOR), driver.get("dd_type", 1))
        ndrv = 1
        drv_blob = data
        drv_start = gap_start
        gap_start += blocks
        gap -= blocks
    if gap:
        entries.append(("Extra", "Apple_Free", gap_start, gap, 0, {}))
    entries.append((hfs_name, "Apple_HFS", hfs_start, hfs_blocks, 0xB7, {}))
    entries.sort(key=lambda e: e[2])
    if len(entries) > map_blocks:
        raise APMError("partition map too small")

    out = bytearray(hfs_start * SECTOR)
    out[0:SECTOR] = (b"ER" + struct.pack(">HIHHIH", SECTOR, total_blocks, 1, 1, 0, ndrv)
                     + ddr_drivers).ljust(SECTOR, b"\0")
    for i, (name, ptype, start, blocks, status, extra) in enumerate(entries):
        out[(1 + i) * SECTOR:(2 + i) * SECTOR] = pm_entry(len(entries), start, blocks, name, ptype,
                                                         status, **extra)
    if drv_blob:
        out[drv_start * SECTOR:drv_start * SECTOR + len(drv_blob)] = drv_blob
    return bytes(out), hfs_start, hfs_blocks


def retarget(prefix, hfs_index, total_blocks, hfs_start, hfs_blocks):
    """Patch a template's DDR block count and Apple_HFS entry for a new disk size."""
    out = bytearray(prefix)
    struct.pack_into(">I", out, 4, total_blocks)
    off = hfs_index * SECTOR
    struct.pack_into(">II", out, off + 8, hfs_start, hfs_blocks)      # pmPyPartStart, pmPartBlkCnt
    struct.pack_into(">I", out, off + 84, hfs_blocks)                 # pmDataCnt
    return bytes(out)
