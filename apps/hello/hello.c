/* hello.c - an example PlusOS program in C, running as a ring-2 process.
 * Uses the console, the keyboard, the ROM heap (malloc) and ROM traps. */
#include <plusos/plusos.h>
#include <plusrom/OSUtil.h>
#include <plusrom/ToolboxEvent.h>

int main(int argc, char **argv)
{
    printf("Hello from a PlusOS program written in C!\n");
    printf("argc=%d:", argc);
    for (int i = 0; i < argc; i++) printf(" [%s]", argv[i]);
    printf("\n");

    printf("ticks since boot: %lu\n", (unsigned long)TickCount());

    char *buf = malloc(1000);
    printf("malloc(1000) = %p, %lu bytes free in the ROM heap\n",
           (void *)buf, (unsigned long)plusos->mem_free_bytes());
    free(buf);

    printf("What's your name? ");
    char name[40];
    gets_s(name, sizeof name);
    printf("Nice to meet you, %s.\n", name[0] ? name : "stranger");
    return 0;
}
