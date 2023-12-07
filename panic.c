#include <include/types.h>
#include <include/panic.h>
#include <tty/include/tty.h>
#include <include/low_level.h>
#include <include/string.h>

//defined in tty.c
extern uint64_t tty_height, tty_width;

__attribute__((noreturn))
void panic(char *reason) {
    tty_clear();
    set_tty_char_fg(TTY_COLOR_WHITE);
    tty_disable_bg(); //disable text background
    printf("\n\n\n");

    //draws background color
    for (uint64_t i = 0; i < tty_height; i++) {
        for (uint64_t j = 0; j < tty_width; j++) {
            plot_pixel(j, i, PANIC_SCREEN_BACKGROUND);
        }
    }

    char *title = "CamOS panic";
    uint32_t title_length = strlen(title);
    uint64_t grid_width = get_tty_grid_width();
    uint64_t grid_mid = grid_width / 2;
    uint64_t title_padding = grid_mid - title_length / 2;

    for (uint64_t i = 0; i < grid_width; i++) {
        printf("-");
    }

    printf("\n");

    for (uint64_t i = 0; i < title_padding; i++) {
        printf(" ");
    }

    printf("%s\n", title);

    for (uint64_t i = 0; i < grid_width; i++) {
        printf("-");
    }

    printf("\n\n\n\n");
    printf("An unexpected error occurred and the system has been halted.\n");
    printf("Reboot the system by holding power button or closing emulator.\n");

    if (reason) {
        printf("Reason: %s\n", reason);
    }

    printf("Other informations may follow...\n");

    while(true);
    sys_hlt();
}