/* theme.h - PlusOS System-Wide Multi-OS Theme Engine.
 *
 * Provides 6 visual themes across the PlusOS Desktop, Window Compositor,
 * Taskbar/Menu Bar, Icons, and Applications:
 *   0: Mac OS 7.1   (Classic Macintosh Platinum / Pinstripe titlebars)
 *   1: Amiga WB     (Commodore Amiga Workbench 1.3/2.0 drawers & depth gadgets)
 *   2: Windows 95   (3D raised bevels, solid titlebar, bottom Start taskbar)
 *   3: Windows 98   (Bayer dither gradient titlebar, Quick Launch taskbar)
 *   4: Vista/7 Aero (Rounded glass frames, specular shine, Pearl Start Orb)
 *   5: Hacker Mode  (Inverted dark phosphor CRT terminals, Matrix rain, HUD)
 */
#ifndef PLUSOS_THEME_H
#define PLUSOS_THEME_H

#include <plusos/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PLUSOS_THEME_MAC    = 0,
    PLUSOS_THEME_AMIGA  = 1,
    PLUSOS_THEME_WIN95  = 2,
    PLUSOS_THEME_WIN98  = 3,
    PLUSOS_THEME_AERO   = 4,
    PLUSOS_THEME_HACKER = 5,
    PLUSOS_THEME_COUNT  = 6
};

#define PLUSOS_THEME_REGKEY "desktop.theme"

static inline const char *plusos_theme_key(int id)
{
    switch (id) {
    case PLUSOS_THEME_MAC:    return "mac";
    case PLUSOS_THEME_AMIGA:  return "amiga";
    case PLUSOS_THEME_WIN95:  return "win95";
    case PLUSOS_THEME_WIN98:  return "win98";
    case PLUSOS_THEME_AERO:   return "aero";
    case PLUSOS_THEME_HACKER: return "hacker";
    default:                  return "mac";
    }
}

static inline const char *plusos_theme_name(int id)
{
    switch (id) {
    case PLUSOS_THEME_MAC:    return "Mac OS 7.1";
    case PLUSOS_THEME_AMIGA:  return "Amiga WB";
    case PLUSOS_THEME_WIN95:  return "Windows 95";
    case PLUSOS_THEME_WIN98:  return "Windows 98";
    case PLUSOS_THEME_AERO:   return "Vista/7 Aero";
    case PLUSOS_THEME_HACKER: return "Hacker Mode";
    default:                  return "Mac OS 7.1";
    }
}

static inline const char *plusos_theme_desc(int id)
{
    switch (id) {
    case PLUSOS_THEME_MAC:    return "Classic Macintosh pinstripes, close/zoom boxes & platinum desktop";
    case PLUSOS_THEME_AMIGA:  return "Commodore Amiga Workbench 1.3 twin-bar windows, depth gadgets & drawers";
    case PLUSOS_THEME_WIN95:  return "Microsoft Windows 95 3D bevels, Start button taskbar & [X] buttons";
    case PLUSOS_THEME_WIN98:  return "Microsoft Windows 98 gradient titlebars, Quick Launch & Start taskbar";
    case PLUSOS_THEME_AERO:   return "Windows Vista/7 Frutiger Aero glass shine, rounded corners & Start Orb";
    case PLUSOS_THEME_HACKER: return "Cyberpunk CRT dark mode, inverted terminals, Matrix rain & HUD telemetry";
    default:                  return "";
    }
}

static inline int plusos_theme_ci_eq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    for (;; a++, b++) {
        int x = (unsigned char)*a, y = (unsigned char)*b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return 0;
        if (!x) return 1;
    }
}

static inline int plusos_theme_parse(const char *s)
{
    if (!s || !s[0]) return -1;
    if (s[0] >= '0' && s[0] <= '5' && s[1] == 0) return s[0] - '0';
    if (plusos_theme_ci_eq(s, "mac") || plusos_theme_ci_eq(s, "macos") || plusos_theme_ci_eq(s, "sys7"))
        return PLUSOS_THEME_MAC;
    if (plusos_theme_ci_eq(s, "amiga") || plusos_theme_ci_eq(s, "wb") || plusos_theme_ci_eq(s, "workbench"))
        return PLUSOS_THEME_AMIGA;
    if (plusos_theme_ci_eq(s, "win95") || plusos_theme_ci_eq(s, "95") || plusos_theme_ci_eq(s, "w95"))
        return PLUSOS_THEME_WIN95;
    if (plusos_theme_ci_eq(s, "win98") || plusos_theme_ci_eq(s, "98") || plusos_theme_ci_eq(s, "w98"))
        return PLUSOS_THEME_WIN98;
    if (plusos_theme_ci_eq(s, "aero") || plusos_theme_ci_eq(s, "vista") ||
        plusos_theme_ci_eq(s, "win7") || plusos_theme_ci_eq(s, "7"))
        return PLUSOS_THEME_AERO;
    if (plusos_theme_ci_eq(s, "hacker") || plusos_theme_ci_eq(s, "matrix") ||
        plusos_theme_ci_eq(s, "crt") || plusos_theme_ci_eq(s, "cyber"))
        return PLUSOS_THEME_HACKER;
    return -1;
}

static inline int plusos_theme_get(void)
{
    char buf[24];
    struct plusos_reg_args a;
    for (unsigned i = 0; i < sizeof a; i++) ((char *)&a)[i] = 0;
    buf[0] = 0;
    a.key = PLUSOS_THEME_REGKEY;
    a.vbuf = buf;
    a.vsize = sizeof buf;
    if (plusos_syscall(SYS_regget, &a) <= 0) return PLUSOS_THEME_MAC;
    int t = plusos_theme_parse(buf);
    return (t >= 0 && t < PLUSOS_THEME_COUNT) ? t : PLUSOS_THEME_MAC;
}

static inline int plusos_theme_set(int id)
{
    if (id < 0 || id >= PLUSOS_THEME_COUNT) return -1;
    struct plusos_reg_args a;
    for (unsigned i = 0; i < sizeof a; i++) ((char *)&a)[i] = 0;
    a.key = PLUSOS_THEME_REGKEY;
    a.value = plusos_theme_key(id);
    return (int)plusos_syscall(SYS_regset, &a);
}

#ifdef __cplusplus
}
#endif

#endif /* PLUSOS_THEME_H */
