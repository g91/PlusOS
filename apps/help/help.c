/* help - list the programs in bin. Each is its own executable; the shell runs
 * any of them by name, and "name &" runs one in the background. */
#include <plusos/plusos.h>

int main(void)
{
    printf("PlusOS commands (programs in bin). Run one by typing its name.\n\n");
    printf("  System : sysinfo  ps  taskman  kill  mem  date  ver  sound  clip\n");
    printf("  Files  : ls  cat  hexedit  files  echo  clear\n");
    printf("  Net    : net  ping  telnet  irc   (BlueSCSI WiFi / DaynaPORT)\n");
    printf("  Desk   : calc  notes  puzzle  desktop  theme  startup  reg\n");
    printf("  Mac/VM : macrun  chip8  emu6502  basic\n");
    printf("  3D/Gfx : cube3d  solarsys  starflight  dungeon  qdemo  hello\n");
    printf("  Games  : asteroids breakout flappy invaders lunar mines pacman\n");
    printf("           racer snake tetris ponglander maze life spin\n");
    printf("  Power  : boot  reboot  off\n\n");
    printf("Shell built-ins: history [-c]  exit  jobs  fg\n");
    printf("Console keys   : Up/Down recalls last 20 commands (saved across reboots)\n");
    printf("                 Tab auto-completes commands and file paths\n");
    printf("Background a program with a trailing &, e.g.  sleep 20 &\n");
    return 0;
}
