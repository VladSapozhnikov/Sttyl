#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

struct mode_s {
    const char *name;
    tcflag_t *flagset;
    tcflag_t flag;
};

static void show_settings(struct termios *tio) {
    speed_t ispeed = cfgetispeed(tio);
    speed_t ospeed = cfgetospeed(tio);

    printf("speed %d baud; ", (int)ispeed);
    printf("%sevenp ", (tio->c_cflag & PARENB)?"":"-");
    printf("%shupcl ", (tio->c_cflag & HUPCL)?"":"-");
    printf("%scread\n", (tio->c_cflag & CREAD)?"":"-");

    printf("intr = ^%c; ", tio->c_cc[VINTR] + '@');
    printf("erase = ");
    if (isprint(tio->c_cc[VERASE])) {
        printf("%c; ", tio->c_cc[VERASE]);
    } else {
        printf("^%c; ", tio->c_cc[VERASE] + '@');
    }
    printf("kill = ^%c; ", tio->c_cc[VKILL] + '@');
    printf("start = ^%c; ", tio->c_cc[VSTART] + '@');
    printf("stop = ^%c;\n", tio->c_cc[VSTOP] + '@');

    printf("%sbrkint ", (tio->c_iflag & BRKINT)?"":"-");
    printf("%sicrnl ", (tio->c_iflag & ICRNL)?"":"-");
    printf("%sixon ", (tio->c_iflag & IXON)?"":"-");
    printf("%sinpck ", (tio->c_iflag & INPCK)?"":"-");
    printf("%sixany\n", (tio->c_iflag & IXANY)?"":"-");

    printf("%sonlcr ", (tio->c_oflag & ONLCR)?"":"-");
    printf("%stabs\n", (tio->c_oflag & TAB3)?"":"-");

    printf("%sicanon ", (tio->c_lflag & ICANON)?"":"-");
    printf("%sisig ", (tio->c_lflag & ISIG)?"":"-");
    printf("%secho ", (tio->c_lflag & ECHO)?"":"-");
    printf("%sechoe\n", (tio->c_lflag & ECHOE)?"":"-");
}

static int parse_control_char(const char *arg) {
    if (arg[0] == '^' && arg[1] != '\0') {
        return arg[1] & 0x1F; 
    } else if (strlen(arg) == 1) {
        return arg[0];
    }
    return -1;
}

int main(int argc, char **argv) {
    struct termios tio;
    if (tcgetattr(STDIN_FILENO, &tio) < 0) {
        perror("tcgetattr");
        return 1;
    }

    if (argc == 1) {
        show_settings(&tio);
        return 0;
    }

    // Table for simple on/off modes
    struct mode_s modes[] = {
        {"icrnl", &tio.c_iflag, ICRNL},
        {"onlcr", &tio.c_oflag, ONLCR},
        {"echo",  &tio.c_lflag, ECHO},
        {"echoe", &tio.c_lflag, ECHOE},
        {"olcuc", &tio.c_oflag, OLCUC},
        {"tabs",  &tio.c_oflag, TAB3},
        {"icanon",&tio.c_lflag, ICANON},
        {"isig",  &tio.c_lflag, ISIG},
        {NULL,    NULL,         0}
    };

    int i;
    for (i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (!strcmp(arg, "erase")) {
            if (++i >= argc) break;
            int c = parse_control_char(argv[i]);
            if (c == -1) {
                fprintf(stderr, "unknown mode\n");
            } else {
                tio.c_cc[VERASE] = c;
            }
            continue;
        } 
        else if (!strcmp(arg, "kill")) {
            if (++i >= argc) break;
            int c = parse_control_char(argv[i]);
            if (c == -1) {
                fprintf(stderr, "unknown mode\n");
            } else {
                tio.c_cc[VKILL] = c;
            }
            continue;
        }
        else {
            int found = 0;
            int dash = (arg[0] == '-');
            if (dash) arg++;
            for (int j = 0; modes[j].name; j++) {
                if (!strcmp(arg, modes[j].name)) {
                    found = 1;
                    if (dash) {
                        *modes[j].flagset &= ~modes[j].flag;
                    } else {
                        *modes[j].flagset |=  modes[j].flag;
                    }
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "unknown mode\n");
            }
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &tio) < 0) {
        perror("tcsetattr");
        return 1;
    }

    return 0;
}
