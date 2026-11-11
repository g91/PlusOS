"""dis68k - a compact Motorola 68000 disassembler.

Good enough to read a PlusOS crash site: covers the instructions a C compiler
and the kernel actually emit.  Returns (length_in_bytes, text) for one insn.
Shared by the CLI (plusdump.py) and exported to JSON for the GUI.
"""

COND = ["t", "f", "hi", "ls", "cc", "cs", "ne", "eq",
        "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le"]
SIZES = {0: ".b", 1: ".w", 2: ".l"}


class _R:
    def __init__(self, code, base):
        self.c = code
        self.p = base      # byte offset of this instruction within code
        self.i = 0         # words consumed past the opcode word

    def w(self, n):
        """signed/unsigned 16-bit word at opcode+2*n (n counts extension words)"""
        off = self.p + 2 + 2 * n
        if off + 1 >= len(self.c):
            return None
        return (self.c[off] << 8) | self.c[off + 1]


def _imm(r, size, nword):
    """Read an immediate of the given size starting at extension word nword.
    Returns (value, words_consumed)."""
    if size == 2:
        hi = r.w(nword); lo = r.w(nword + 1)
        if hi is None or lo is None:
            return None, 2
        return ((hi << 16) | lo), 2
    v = r.w(nword)
    if v is None:
        return None, 1
    if size == 0:
        v &= 0xFF
    return v, 1


def _ea(r, mode, reg, size, nword):
    """Decode an effective address. Returns (text, extra_words)."""
    if mode == 0:
        return "d%d" % reg, 0
    if mode == 1:
        return "a%d" % reg, 0
    if mode == 2:
        return "(a%d)" % reg, 0
    if mode == 3:
        return "(a%d)+" % reg, 0
    if mode == 4:
        return "-(a%d)" % reg, 0
    if mode == 5:
        d = r.w(nword)
        if d is None:
            return "(?,a%d)" % reg, 1
        if d >= 0x8000:
            d -= 0x10000
        return "%d(a%d)" % (d, reg), 1
    if mode == 6:
        ext = r.w(nword)
        if ext is None:
            return "(?,a%d,?)" % reg, 1
        da = "a" if (ext & 0x8000) else "d"
        xr = (ext >> 12) & 7
        xs = ".l" if (ext & 0x0800) else ".w"
        disp = ext & 0xFF
        if disp >= 0x80:
            disp -= 0x100
        return "%d(a%d,%s%d%s)" % (disp, reg, da, xr, xs), 1
    if mode == 7:
        if reg == 0:       # abs.w
            a = r.w(nword)
            if a is None:
                return "($?).w", 1
            if a >= 0x8000:
                a -= 0x10000
            return "$%x.w" % (a & 0xFFFFFFFF), 1
        if reg == 1:       # abs.l
            hi = r.w(nword); lo = r.w(nword + 1)
            if hi is None or lo is None:
                return "($?).l", 2
            return "$%x.l" % ((hi << 16) | lo), 2
        if reg == 2:       # d16(pc)
            d = r.w(nword)
            if d is None:
                return "(?,pc)", 1
            if d >= 0x8000:
                d -= 0x10000
            tgt = r.p + 2 + 2 * nword + d
            return "$%x(pc)" % tgt, 1
        if reg == 3:       # d8(pc,xn)
            ext = r.w(nword)
            if ext is None:
                return "(?,pc,?)", 1
            da = "a" if (ext & 0x8000) else "d"
            xr = (ext >> 12) & 7
            xs = ".l" if (ext & 0x0800) else ".w"
            disp = ext & 0xFF
            if disp >= 0x80:
                disp -= 0x100
            return "%d(pc,%s%d%s)" % (disp, da, xr, xs), 1
        if reg == 4:       # immediate
            v, n = _imm(r, size, nword)
            if v is None:
                return "#?", n
            return "#$%x" % v, n
    return "?", 0


def _movem_regs(mask, predec):
    names = []
    order = (["a7", "a6", "a5", "a4", "a3", "a2", "a1", "a0",
              "d7", "d6", "d5", "d4", "d3", "d2", "d1", "d0"]
             if predec else
             ["d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
              "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"])
    for i in range(16):
        if mask & (1 << i):
            names.append(order[i])
    return "/".join(names) if names else "-"


def disasm(code, off):
    """Disassemble one instruction at byte offset `off` in bytes `code`.
    Returns (length, mnemonic_text)."""
    if off + 1 >= len(code):
        return 2, "dc.w  ????"
    op = (code[off] << 8) | code[off + 1]
    r = _R(code, off)

    def ea(mode, reg, size, nword):
        return _ea(r, mode, reg, size, nword)

    top = op >> 12
    m = (op >> 3) & 7
    rg = op & 7
    sz2 = (op >> 6) & 3

    # --- simple fixed opcodes ---
    fixed = {0x4E71: "nop", 0x4E75: "rts", 0x4E77: "rtr", 0x4E73: "rte",
             0x4E70: "reset", 0x4E72: "stop", 0x4AFC: "illegal"}
    if op in fixed:
        if op == 0x4E72:
            v = r.w(0)
            return 4, "stop  #$%x" % (v or 0)
        return 2, fixed[op]
    if (op & 0xFFF0) == 0x4E40:
        return 2, "trap  #%d" % (op & 0xF)
    if (op & 0xFFF8) == 0x4E50:
        d = r.w(0) or 0
        if d >= 0x8000:
            d -= 0x10000
        return 4, "link  a%d,#%d" % (rg, d)
    if (op & 0xFFF8) == 0x4E58:
        return 2, "unlk  a%d" % rg
    if (op & 0xFFF8) == 0x4E60:
        return 2, "move  a%d,usp" % rg
    if (op & 0xFFF8) == 0x4E68:
        return 2, "move  usp,a%d" % rg
    if (op & 0xFFC0) == 0x4E80:
        t, n = ea(m, rg, 2, 0)
        return 2 + 2 * n, "jsr   " + t
    if (op & 0xFFC0) == 0x4EC0:
        t, n = ea(m, rg, 2, 0)
        return 2 + 2 * n, "jmp   " + t

    # --- MOVEM ---
    if (op & 0xFB80) == 0x4880:
        size = 2 if (op & 0x40) else 1
        to_mem = (op & 0x0400) == 0
        mask = r.w(0) or 0
        predec = (m == 4)
        t, n = ea(m, rg, size, 1)
        regs = _movem_regs(mask, predec and to_mem)
        if to_mem:
            return 4 + 2 * n, "movem%s %s,%s" % (SIZES[size], regs, t)
        return 4 + 2 * n, "movem%s %s,%s" % (SIZES[size], t, regs)

    # --- MOVE / MOVEA ---
    if top in (1, 2, 3):
        size = {1: 0, 2: 2, 3: 1}[top]     # byte, long, word
        sreg = (op >> 9) & 7
        smode = (op >> 6) & 7
        src, sn = ea(m, rg, size, 0)
        dst, dn = ea(smode, sreg, size, sn)
        mn = "movea" if smode == 1 else "move"
        return 2 + 2 * (sn + dn), "%s%s %s,%s" % (mn, SIZES[size], src, dst)

    # --- MOVEQ ---
    if (op & 0xF100) == 0x7000:
        v = op & 0xFF
        if v >= 0x80:
            v -= 0x100
        return 2, "moveq #%d,d%d" % (v, (op >> 9) & 7)

    # --- Bcc / BSR / BRA ---
    if top == 6:
        cc = (op >> 8) & 0xF
        disp = op & 0xFF
        if disp == 0:
            d16 = r.w(0) or 0
            if d16 >= 0x8000:
                d16 -= 0x10000
            tgt = off + 2 + d16
            length = 4
        elif disp == 0xFF:
            hi = r.w(0); lo = r.w(1)
            d32 = ((hi << 16) | lo) if hi is not None else 0
            tgt = off + 2 + d32
            length = 6
        else:
            d = disp - 0x100 if disp >= 0x80 else disp
            tgt = off + 2 + d
            length = 2
        name = {0: "bra", 1: "bsr"}.get(cc, "b" + COND[cc])
        return length, "%-5s $%x" % (name, tgt)

    # --- DBcc / Scc / addq / subq ---
    if top == 5:
        if m == 1 and ((op >> 6) & 3) == 3:   # DBcc
            cc = (op >> 8) & 0xF
            d = r.w(0) or 0
            if d >= 0x8000:
                d -= 0x10000
            return 4, "db%-3s d%d,$%x" % (COND[cc], rg, off + 2 + d)
        if ((op >> 6) & 3) == 3:              # Scc
            cc = (op >> 8) & 0xF
            t, n = ea(m, rg, 0, 0)
            return 2 + 2 * n, "s%-4s %s" % (COND[cc], t)
        data = (op >> 9) & 7
        if data == 0:
            data = 8
        size = sz2
        t, n = ea(m, rg, size, 0)
        mn = "subq" if (op & 0x0100) else "addq"
        return 2 + 2 * n, "%s%s #%d,%s" % (mn, SIZES.get(size, ""), data, t)

    # --- LEA / CHK ---
    if top == 4:
        if (op & 0x01C0) == 0x01C0:           # LEA
            areg = (op >> 9) & 7
            t, n = ea(m, rg, 2, 0)
            return 2 + 2 * n, "lea   %s,a%d" % (t, areg)
        if (op & 0x01C0) == 0x0180:           # CHK
            dreg = (op >> 9) & 7
            t, n = ea(m, rg, 1, 0)
            return 2 + 2 * n, "chk   %s,d%d" % (t, dreg)
        hi = op & 0xFF00
        if hi == 0x4200:
            t, n = ea(m, rg, sz2, 0); return 2 + 2 * n, "clr%s  %s" % (SIZES.get(sz2, ""), t)
        if hi == 0x4A00:
            if sz2 == 3:
                t, n = ea(m, rg, 0, 0); return 2 + 2 * n, "tas   " + t
            t, n = ea(m, rg, sz2, 0); return 2 + 2 * n, "tst%s  %s" % (SIZES.get(sz2, ""), t)
        if hi == 0x4400:
            t, n = ea(m, rg, sz2, 0); return 2 + 2 * n, "neg%s  %s" % (SIZES.get(sz2, ""), t)
        if hi == 0x4600:
            t, n = ea(m, rg, sz2, 0); return 2 + 2 * n, "not%s  %s" % (SIZES.get(sz2, ""), t)
        if hi == 0x4000:
            t, n = ea(m, rg, sz2, 0); return 2 + 2 * n, "negx%s %s" % (SIZES.get(sz2, ""), t)
        if (op & 0xFFB8) == 0x4880:
            return 2, "ext%s  d%d" % (".l" if (op & 0x40) else ".w", rg)
        if (op & 0xFFF8) == 0x4840:
            return 2, "swap  d%d" % rg
        if (op & 0xFFC0) == 0x4840:
            t, n = ea(m, rg, 2, 0); return 2 + 2 * n, "pea   " + t
        if (op & 0xFF00) == 0x4800:
            t, n = ea(m, rg, 0, 0); return 2 + 2 * n, "nbcd  " + t

    # --- ADDQ handled above; ADDI/SUBI/CMPI/ANDI/ORI/EORI/BTST imm (0x0xxx) ---
    if top == 0:
        imm_ops = {0x0600: "addi", 0x0400: "subi", 0x0C00: "cmpi",
                   0x0200: "andi", 0x0000: "ori", 0x0A00: "eori"}
        base = op & 0x0F00
        if base in imm_ops and sz2 != 3:
            size = sz2
            v, vn = _imm(r, size, 0)
            t, n = ea(m, rg, size, vn)
            vv = 0 if v is None else v
            return 2 + 2 * (vn + n), "%s%s #$%x,%s" % (imm_ops[base], SIZES[size], vv, t)
        # bit ops: static (0x08xx) or dynamic (register in bits 9-11)
        bit = {0x00: "btst", 0x40: "bchg", 0x80: "bclr", 0xC0: "bset"}
        if (op & 0x0F00) == 0x0800:           # static bit op, imm bit number
            sub = op & 0xC0
            if sub in bit:
                b = r.w(0) or 0
                t, n = ea(m, rg, 0, 1)
                return 4 + 2 * n, "%s  #%d,%s" % (bit[sub], b & 0xFF, t)
        if (op & 0xC0) in bit and (op & 0x0100):
            dreg = (op >> 9) & 7
            t, n = ea(m, rg, 0, 0)
            return 2 + 2 * n, "%s  d%d,%s" % (bit[op & 0xC0], dreg, t)

    # --- ADD/SUB/AND/OR/EOR/CMP (reg forms), ADDA/SUBA/CMPA, MUL/DIV ---
    def rmw(name, allow_a=False):
        dreg = (op >> 9) & 7
        if sz2 == 3 and allow_a:              # xxxA.w/.l
            asz = 2 if (op & 0x0100) else 1
            t, n = ea(m, rg, asz, 0)
            return 2 + 2 * n, "%sa%s %s,a%d" % (name, SIZES[asz], t, dreg)
        size = sz2
        t, n = ea(m, rg, size, 0)
        if op & 0x0100:
            return 2 + 2 * n, "%s%s  d%d,%s" % (name, SIZES.get(size, ""), dreg, t)
        return 2 + 2 * n, "%s%s  %s,d%d" % (name, SIZES.get(size, ""), t, dreg)

    if top == 0xD:
        return rmw("add", True)
    if top == 0x9:
        return rmw("sub", True)
    if top == 0xC:
        if sz2 == 3:
            dreg = (op >> 9) & 7
            t, n = ea(m, rg, 1, 0)
            return 2 + 2 * n, "mulu  %s,d%d" % (t, dreg)
        if (op & 0x01F0) == 0x0100:
            return 2, "abcd  d%d,d%d" % (rg, (op >> 9) & 7)
        return rmw("and")
    if top == 0x8:
        if sz2 == 3:
            dreg = (op >> 9) & 7
            t, n = ea(m, rg, 1, 0)
            return 2 + 2 * n, "divu  %s,d%d" % (t, dreg)
        return rmw("or")
    if top == 0xB:
        dreg = (op >> 9) & 7
        if sz2 == 3:
            asz = 2 if (op & 0x0100) else 1
            t, n = ea(m, rg, asz, 0)
            return 2 + 2 * n, "cmpa%s %s,a%d" % (SIZES[asz], t, dreg)
        if op & 0x0100:
            if m == 1:
                return 2, "cmpm  (a%d)+,(a%d)+" % (rg, dreg)
            t, n = ea(m, rg, sz2, 0)
            return 2 + 2 * n, "eor%s  d%d,%s" % (SIZES.get(sz2, ""), dreg, t)
        t, n = ea(m, rg, sz2, 0)
        return 2 + 2 * n, "cmp%s  %s,d%d" % (SIZES.get(sz2, ""), t, dreg)
    if top == 0x5 >> 0 and False:
        pass

    # MULS/DIVS share 0xC/0x8 with signed bit handled loosely above; refine:
    if top == 0xC and sz2 == 3 and (op & 0x0100):
        dreg = (op >> 9) & 7
        t, n = ea(m, rg, 1, 0)
        return 2 + 2 * n, "muls  %s,d%d" % (t, dreg)
    if top == 0x8 and sz2 == 3 and (op & 0x0100):
        dreg = (op >> 9) & 7
        t, n = ea(m, rg, 1, 0)
        return 2 + 2 * n, "divs  %s,d%d" % (t, dreg)

    # --- shifts/rotates (0xE) ---
    if top == 0xE:
        if sz2 == 3:                           # memory shift by 1
            typ = (op >> 9) & 3
            dirc = "l" if (op & 0x0100) else "r"
            names = ["as", "ls", "rox", "ro"]
            t, n = ea(m, rg, 1, 0)
            return 2 + 2 * n, "%s%s   %s" % (names[typ] + dirc, "", t)
        cnt = (op >> 9) & 7
        ir = (op >> 5) & 1
        typ = (op >> 3) & 3
        dirc = "l" if (op & 0x0100) else "r"
        names = ["as", "ls", "rox", "ro"]
        mn = names[typ] + dirc + SIZES.get(sz2, "")
        if ir:
            return 2, "%s d%d,d%d" % (mn, cnt, rg)
        if cnt == 0:
            cnt = 8
        return 2, "%s #%d,d%d" % (mn, cnt, rg)

    return 2, "dc.w  $%04x" % op
