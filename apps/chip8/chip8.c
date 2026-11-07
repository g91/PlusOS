/* chip8.c - CHIP-8 Virtual Machine & Arcade Emulator for PlusOS
 *
 * Emulates the classic COSMAC VIP / CHIP-8 8-bit virtual machine:
 *   - 4 KB RAM ($000..$FFF), program entry at $200
 *   - 16 registers V0..VF, 16-bit index register I, PC, 16-level stack
 *   - 60 Hz delay_timer & sound_timer (drives Compact Mac PWM speaker)
 *   - 64x32 monochrome display scaled 6x (384x192) on the 512x342 backbuffer
 *   - 5 built-in playable ROMs (Tab to switch) + external .ch8 file loader
 */
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>
#include <plusrom/FileMgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t  ram[4096];
static uint8_t  V[16];
static uint16_t I_reg;
static uint16_t pc;
static uint16_t stack[16];
static uint8_t  sp;
static uint8_t  delay_timer;
static uint8_t  sound_timer;
static uint8_t  vram[64 * 32];
static uint8_t  keys[16];
static uint8_t  key_hold[16];
static uint16_t last_opcode;

static const uint8_t fontset[80] = {
    0xF0,0x90,0x90,0x90,0xF0, 0x20,0x60,0x20,0x20,0x70,
    0xF0,0x10,0xF0,0x80,0xF0, 0xF0,0x10,0xF0,0x10,0xF0,
    0x90,0x90,0xF0,0x10,0x10, 0xF0,0x80,0xF0,0x10,0xF0,
    0xF0,0x80,0xF0,0x90,0xF0, 0xF0,0x10,0x20,0x40,0x40,
    0xF0,0x90,0xF0,0x90,0xF0, 0xF0,0x90,0xF0,0x10,0xF0,
    0xF0,0x90,0xF0,0x90,0x90, 0xE0,0x90,0xE0,0x90,0xE0,
    0xF0,0x80,0x80,0x80,0xF0, 0xE0,0x90,0x90,0x90,0xE0,
    0xF0,0x80,0xF0,0x80,0xF0, 0xF0,0x80,0xF0,0x80,0x80
};

/* ROM 1: Playable Squash / Solo Pong (Keys: 1/Q = Up, 4/A = Down) */
static const uint8_t rom_squash[] = {
    0x60,0x02, 0x61,0x0C, 0x62,0x1E, 0x63,0x10, 0x64,0x01, 0x65,0x01, 0x66,0x00,
    /* loop ($20E): draw paddle at (V0, V1) and ball at (V2, V3) */
    0xA2,0x58, 0xD0,0x16, 0xA2,0x5E, 0xD2,0x32,
    /* delay 2 ticks */
    0x67,0x01, 0xF7,0x15, 0xF7,0x07, 0x37,0x00, 0x12,0x1A,
    /* erase paddle and ball */
    0xA2,0x58, 0xD0,0x16, 0xA2,0x5E, 0xD2,0x32,
    /* check key 1 (up) and key 4 (down) */
    0x68,0x01, 0xE8,0xA1, 0x71,0xFE, 0x68,0x04, 0xE8,0xA1, 0x71,0x02,
    0x68,0x1A, 0x88,0x10, 0x78,0xE6, /* clamp V1 */
    /* move ball: V2 += V4, V3 += V5 */
    0x82,0x44, 0x83,0x54,
    /* bounce right wall (V2 == 62) */
    0x32,0x3E, 0x12,0x3E, 0x64,0xFF,
    /* bounce top/bottom (V3 == 0 or V3 == 30) */
    0x33,0x00, 0x12,0x44, 0x65,0x01,
    0x33,0x1E, 0x12,0x4A, 0x65,0xFF,
    /* check left paddle hit (V2 == 4) */
    0x32,0x04, 0x12,0x0E, 0x64,0x01, 0x68,0x02, 0xF8,0x18, 0x12,0x0E,
    0x00,0x00, 0x00,0x00,
    /* $258: paddle sprite (6 rows), $25E: ball sprite (2 rows) */
    0x80,0x80,0x80,0x80,0x80,0x80, 0xC0,0xC0
};

/* ROM 2: Classic CHIP-8 Maze Generator */
static const uint8_t rom_maze[] = {
    0x60,0x00, 0x61,0x00, 0xA2,0x22, 0xC2,0x01, 0x32,0x01, 0xA2,0x1E,
    0xD0,0x14, 0x70,0x04, 0x30,0x40, 0x12,0x04, 0x60,0x00, 0x71,0x04,
    0x31,0x20, 0x12,0x04, 0x60,0x10, 0xF0,0x15, 0xF0,0x07, 0x30,0x00,
    0x12,0x18, 0x00,0xE0, 0x12,0x00,
    0x80,0x40,0x20,0x10, 0x20,0x40,0x80,0x00
};

/* ROM 3: Target Interceptor (Press 5 / W / Space to fire upward at moving UFO) */
static const uint8_t rom_target[] = {
    0x60,0x04, 0x61,0x04, 0x62,0x1E, 0x63,0x1A, 0x64,0x00,
    /* $20A: main loop */
    0xA2,0x42, 0xD0,0x13, 0xA2,0x45, 0xD2,0x33,
    0x65,0x01, 0xF5,0x15, 0xF5,0x07, 0x35,0x00, 0x12,0x16,
    0xA2,0x42, 0xD0,0x13, 0xA2,0x45, 0xD2,0x33,
    0x70,0x01, 0x66,0x38, 0x80,0x62,
    0x65,0x04, 0xE5,0xA1, 0x72,0xFE,
    0x65,0x06, 0xE5,0xA1, 0x72,0x02,
    0x65,0x05, 0xE5,0xA1, 0x73,0xFE,
    0x33,0x02, 0x12,0x0A, 0x63,0x1A, 0x65,0x03, 0xF5,0x18, 0x12,0x0A,
    /* $242: UFO sprite (3 bytes), $245: Ship sprite (3 bytes) */
    0x70,0xF8,0x70, 0x20,0x70,0xF8
};

/* ROM 4: Bouncing Dual Sprites & Hex Counter */
static const uint8_t rom_bounce[] = {
    0x60,0x08, 0x61,0x06, 0x62,0x01, 0x63,0x01, 0x64,0x00,
    0xF4,0x29, 0xD0,0x15,
    0x65,0x01, 0xF5,0x15, 0xF5,0x07, 0x35,0x00, 0x12,0x12,
    0xF4,0x29, 0xD0,0x15,
    0x80,0x24, 0x81,0x34,
    0x30,0x3A, 0x12,0x26, 0x62,0xFF, 0x74,0x01,
    0x30,0x02, 0x12,0x2E, 0x62,0x01, 0x74,0x01,
    0x31,0x1A, 0x12,0x36, 0x63,0xFF,
    0x31,0x02, 0x12,0x3C, 0x63,0x01,
    0x65,0x0F, 0x84,0x52, 0x12,0x0A
};

/* ROM 5: CHIP-8 Hexadecimal Font & Sprite Showcase */
static const uint8_t rom_fontdemo[] = {
    0x60,0x02, 0x61,0x04, 0x62,0x00,
    0xF2,0x29, 0xD0,0x15, 0x70,0x07, 0x72,0x01,
    0x32,0x08, 0x12,0x16, 0x60,0x02, 0x61,0x10,
    0x32,0x10, 0x12,0x06,
    0x63,0x04, 0xF3,0x15, 0xF3,0x07, 0x33,0x00, 0x12,0x1E,
    0x12,0x1A
};

typedef struct {
    const char    *name;
    const uint8_t *data;
    uint16_t       size;
} BuiltinRom;

static const BuiltinRom builtin_roms[5] = {
    { "1: SQUASH / PONG (Q/A=PADDLE)", rom_squash,   sizeof(rom_squash) },
    { "2: 10-PRINT MAZE GENERATOR",    rom_maze,     sizeof(rom_maze) },
    { "3: UFO INTERCEPTOR (A/D/W)",    rom_target,   sizeof(rom_target) },
    { "4: BOUNCING HEX SPRITES",       rom_bounce,   sizeof(rom_bounce) },
    { "5: CHIP-8 FONT SHOWCASE",       rom_fontdemo, sizeof(rom_fontdemo) }
};

static void chip8_reset(const uint8_t *rom, uint16_t len)
{
    memset(ram, 0, sizeof ram);
    memset(V, 0, sizeof V);
    memset(vram, 0, sizeof vram);
    memset(stack, 0, sizeof stack);
    memset(keys, 0, sizeof keys);
    memset(key_hold, 0, sizeof key_hold);
    memcpy(ram + 0x050, fontset, sizeof fontset);
    if (rom && len > 0 && len <= 4096 - 0x200)
        memcpy(ram + 0x200, rom, len);
    pc = 0x200;
    I_reg = 0;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;
    last_opcode = 0;
}

static void chip8_step(void)
{
    if (pc >= 4094) pc = 0x200;
    uint16_t op = ((uint16_t)ram[pc] << 8) | ram[pc + 1];
    last_opcode = op;
    pc += 2;

    uint8_t  x   = (op >> 8) & 0x0F;
    uint8_t  y   = (op >> 4) & 0x0F;
    uint8_t  kk  = op & 0xFF;
    uint16_t nnn = op & 0x0FFF;
    uint8_t  n   = op & 0x0F;

    switch (op & 0xF000) {
    case 0x0000:
        if (op == 0x00E0) memset(vram, 0, sizeof vram);
        else if (op == 0x00EE && sp > 0) pc = stack[--sp];
        break;
    case 0x1000: pc = nnn; break;
    case 0x2000: if (sp < 16) stack[sp++] = pc; pc = nnn; break;
    case 0x3000: if (V[x] == kk) pc += 2; break;
    case 0x4000: if (V[x] != kk) pc += 2; break;
    case 0x5000: if (V[x] == V[y]) pc += 2; break;
    case 0x6000: V[x] = kk; break;
    case 0x7000: V[x] = (uint8_t)(V[x] + kk); break;
    case 0x8000:
        switch (n) {
        case 0x0: V[x] = V[y]; break;
        case 0x1: V[x] |= V[y]; break;
        case 0x2: V[x] &= V[y]; break;
        case 0x3: V[x] ^= V[y]; break;
        case 0x4: { uint16_t s = (uint16_t)V[x] + V[y]; V[0xF] = s > 255; V[x] = (uint8_t)s; break; }
        case 0x5: { uint8_t nb = V[x] >= V[y]; V[x] = (uint8_t)(V[x] - V[y]); V[0xF] = nb; break; }
        case 0x6: V[0xF] = V[x] & 1; V[x] >>= 1; break;
        case 0x7: { uint8_t nb = V[y] >= V[x]; V[x] = (uint8_t)(V[y] - V[x]); V[0xF] = nb; break; }
        case 0xE: V[0xF] = (V[x] >> 7) & 1; V[x] <<= 1; break;
        }
        break;
    case 0x9000: if (V[x] != V[y]) pc += 2; break;
    case 0xA000: I_reg = nnn & 0x0FFF; break;
    case 0xB000: pc = (nnn + V[0]) & 0x0FFF; break;
    case 0xC000: V[x] = (uint8_t)(rand() & kk); break;
    case 0xD000: {
        uint8_t vx = V[x] & 63, vy = V[y] & 31;
        V[0xF] = 0;
        for (int row = 0; row < n; row++) {
            uint8_t sb = ram[(I_reg + row) & 0x0FFF];
            int py = (vy + row) & 31;
            for (int col = 0; col < 8; col++) {
                if (sb & (0x80 >> col)) {
                    int px = (vx + col) & 63;
                    int idx = py * 64 + px;
                    if (vram[idx]) V[0xF] = 1;
                    vram[idx] ^= 1;
                }
            }
        }
        break;
    }
    case 0xE000:
        if (kk == 0x9E) { if (keys[V[x] & 0xF]) pc += 2; }
        else if (kk == 0xA1) { if (!keys[V[x] & 0xF]) pc += 2; }
        break;
    case 0xF000:
        switch (kk) {
        case 0x07: V[x] = delay_timer; break;
        case 0x0A: {
            int found = -1;
            for (int i = 0; i < 16; i++) if (keys[i]) { found = i; break; }
            if (found >= 0) V[x] = (uint8_t)found;
            else pc -= 2;
            break;
        }
        case 0x15: delay_timer = V[x]; break;
        case 0x18: sound_timer = V[x]; break;
        case 0x1E: I_reg = (I_reg + V[x]) & 0x0FFF; break;
        case 0x29: I_reg = (uint16_t)(0x050 + (V[x] & 0xF) * 5); break;
        case 0x33:
            ram[I_reg & 0xFFF]       = V[x] / 100;
            ram[(I_reg + 1) & 0xFFF] = (V[x] / 10) % 10;
            ram[(I_reg + 2) & 0xFFF] = V[x] % 10;
            break;
        case 0x55:
            for (int i = 0; i <= x; i++) ram[(I_reg + i) & 0xFFF] = V[i];
            break;
        case 0x65:
            for (int i = 0; i <= x; i++) V[i] = ram[(I_reg + i) & 0xFFF];
            break;
        }
        break;
    }
}

static int map_key(int ch)
{
    switch (ch) {
    case '1': return 0x1;
    case '2': return 0x2;
    case '3': return 0x3;
    case '4': return 0xC;
    case 'q': case 'Q': case 30: return 0x1; /* Up -> key 1 */
    case 'w': case 'W': case ' ': return 0x5;
    case 'e': case 'E': return 0x6;
    case 'r': case 'R': return 0xD;
    case 'a': case 'A': case 28: return 0x4; /* Left/Down -> key 4 */
    case 's': case 'S': case 31: return 0x8;
    case 'd': case 'D': case 29: return 0x6; /* Right -> key 6 */
    case 'f': case 'F': return 0xE;
    case 'z': case 'Z': return 0xA;
    case 'x': case 'X': return 0x0;
    case 'c': case 'C': return 0xB;
    case 'v': case 'V': return 0xF;
    }
    return -1;
}

int main(int argc, char **argv)
{
    srand((unsigned)plusos->ticks());
    int cur_rom = 0;
    uint8_t custom_rom[3584];
    uint16_t custom_len = 0;
    char custom_name[48] = "";

    if (argc >= 2) {
        Str255 pname;
        c2pstr_buf(pname, argv[1]);
        INTEGER ref;
        if (FSOpen(pname, (INTEGER)plusos->boot_vrefnum, &ref) == 0) {
            LONGINT n = sizeof custom_rom;
            FSRead(ref, &n, custom_rom);
            FSClose(ref);
            if (n > 0) {
                custom_len = (uint16_t)n;
                snprintf(custom_name, sizeof custom_name, "ROM: %s", argv[1]);
                chip8_reset(custom_rom, custom_len);
            }
        }
    }
    if (!custom_len)
        chip8_reset(builtin_roms[0].data, builtin_roms[0].size);

    gfx_target(gfx_backbuffer());
    int paused = 0;

    for (;;) {
        for (int i = 0; i < 16; i++) {
            if (key_hold[i] > 0) {
                key_hold[i]--;
                keys[i] = (key_hold[i] > 0) ? 1 : 0;
            }
        }
        for (int kp_; (kp_ = plusos->key_poll()) != 0; ) {
            int ch = (int)kp_;
            if (ch == 27) return 0;
            if (ch == '\t') {
                custom_len = 0;
                cur_rom = (cur_rom + 1) % 5;
                chip8_reset(builtin_roms[cur_rom].data, builtin_roms[cur_rom].size);
                continue;
            }
            if (ch == 'p' || ch == 'P') { paused = !paused; continue; }
            if (ch == 'n' || ch == 'N') { chip8_step(); continue; }
            int mk = map_key(ch);
            if (mk >= 0) { keys[mk] = 1; key_hold[mk] = 8; }
        }

        if (!paused) {
            for (int s = 0; s < 8; s++) chip8_step();
            if (delay_timer > 0) delay_timer--;
            if (sound_timer > 0) {
                sound_timer--;
                struct plusos_sound_args sa = { PLUSOS_SND_TONE, 660, 1, 0, 0, 0 };
                plusos_syscall(SYS_sound, &sa);
            }
        }

        gfx_fill(0, 0, 512, 342, 0);
        gfx_fill(0, 0, 512, 20, 1);
        gfx_text(8, 6, "PLUSOS CHIP-8 VIRTUAL MACHINE", 1);
        gfx_text(256, 6, custom_len ? custom_name : builtin_roms[cur_rom].name, 1);

        /* Draw 64x32 screen scaled 6x (384x192) at (12, 36) */
        int ox = 12, oy = 36, scale = 6;
        gfx_frame(ox - 2, oy - 2, ox + 64 * scale + 2, oy + 32 * scale + 2);
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                if (vram[y * 64 + x]) {
                    gfx_fill(ox + x * scale, oy + y * scale,
                             ox + (x + 1) * scale - 1, oy + (y + 1) * scale - 1, 1);
                }
            }
        }

        /* CPU Register Inspector Panel on right side */
        int rx = 408, ry = 36;
        gfx_frame(rx - 4, ry - 2, 504, oy + 32 * scale + 2);
        gfx_text(rx + 4, ry + 4, "CPU STATE", 0);
        char buf[32];
        snprintf(buf, sizeof buf, "PC: %03X", pc);       gfx_text(rx + 4, ry + 20, buf, 0);
        snprintf(buf, sizeof buf, "OP: %04X", last_opcode); gfx_text(rx + 4, ry + 32, buf, 0);
        snprintf(buf, sizeof buf, "I : %03X", I_reg);    gfx_text(rx + 4, ry + 44, buf, 0);
        snprintf(buf, sizeof buf, "DT: %02X", delay_timer); gfx_text(rx + 4, ry + 56, buf, 0);
        for (int i = 0; i < 8; i++) {
            snprintf(buf, sizeof buf, "V%X:%02X V%X:%02X", i, V[i], i + 8, V[i + 8]);
            gfx_text(rx + 2, ry + 76 + i * 14, buf, 0);
        }

        gfx_hline(0, 512, 318, 1);
        gfx_text(8, 326, "TAB: SWITCH ROM (5 BUILT-IN)  Q/A/W/D: KEYS  P: PAUSE  N: STEP  ESC: QUIT", 0);

        gfx_present();
        plusos->sleep_ticks(1);
    }
}
