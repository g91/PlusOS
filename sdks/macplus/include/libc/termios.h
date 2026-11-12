/* termios.h - POSIX terminal I/O attributes */
#ifndef PLUSOS_TERMIOS_H
#define PLUSOS_TERMIOS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define ECHO   0x0001u
#define ICANON 0x0002u
#define ISIG   0x0004u
#define ICRNL  0x0001u
#define OPOST  0x0001u
#define TCSANOW 0

struct termios {
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_cflag;
    uint32_t c_lflag;
    uint16_t rows;
    uint16_t cols;
    uint32_t baud;
};

int tcgetattr(int fd, struct termios *tp);
int tcsetattr(int fd, int optional_actions, const struct termios *tp);

#ifdef __cplusplus
}
#endif
#endif
