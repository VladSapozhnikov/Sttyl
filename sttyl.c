/* A small Linux termios utility, developed from systems programming coursework. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct mode_s {
    const char *name;
    tcflag_t *flagset;
    tcflag_t mask;
    tcflag_t enabled;
};

static void usage(void) {
    puts("Usage: sttyl [MODE ...] [erase CHAR] [kill CHAR]\n"
         "With no arguments, display settings for the terminal on standard input.\n"
         "Modes: echo echoe icanon isig icrnl onlcr olcuc tabs\n"
         "Prefix a mode with '-' to disable it. tabs preserves tab characters;\n"
         "-tabs expands tabs to spaces. CHAR is one byte, ^C notation, ^?, or undef.\n"
         "Examples: sttyl echo; sttyl erase '^?'; sttyl kill '^U'\n"
         "Use stty -g to save settings and stty \"$saved\" to restore them.\n"
         "Linux only; this is a subset of stty, not a full replacement.");
}

static unsigned long baud_rate(speed_t speed) {
    switch (speed) {
        case B0: return 0;
        case B50: return 50;
        case B75: return 75;
        case B110: return 110;
        case B134: return 134;
        case B150: return 150;
        case B200: return 200;
        case B300: return 300;
        case B600: return 600;
        case B1200: return 1200;
        case B1800: return 1800;
        case B2400: return 2400;
        case B4800: return 4800;
        case B9600: return 9600;
        case B19200: return 19200;
        case B38400: return 38400;
#ifdef B57600
        case B57600: return 57600;
#endif
#ifdef B115200
        case B115200: return 115200;
#endif
#ifdef B230400
        case B230400: return 230400;
#endif
        default: return (unsigned long)-1;
    }
}

static void show_speed(const char *name, speed_t speed) {
    unsigned long baud = baud_rate(speed);
    if (baud == (unsigned long)-1)
        printf("%s unknown (termios code %lu); ", name, (unsigned long)speed);
    else
        printf("%s %lu baud; ", name, baud);
}

static void show_control(const char *name, cc_t value) {
    printf("%s = ", name);
    if (value == (cc_t)_POSIX_VDISABLE)
        printf("<undef>");
    else if (value == 127)
        printf("^?");
    else if (value < 32)
        printf("^%c", value + '@');
    else if (isprint((unsigned char)value))
        printf("%c", value);
    else
        printf("0x%02x", (unsigned int)value);
    printf("; ");
}

static void show_settings(const struct termios *tio) {
    show_speed("input speed", cfgetispeed(tio));
    show_speed("output speed", cfgetospeed(tio));
    putchar('\n');
    printf("%sparenb %sparodd %shupcl %scread\n",
           (tio->c_cflag & PARENB) ? "" : "-",
           (tio->c_cflag & PARODD) ? "" : "-",
           (tio->c_cflag & HUPCL) ? "" : "-",
           (tio->c_cflag & CREAD) ? "" : "-");
    show_control("intr", tio->c_cc[VINTR]);
    show_control("erase", tio->c_cc[VERASE]);
    show_control("kill", tio->c_cc[VKILL]);
    show_control("start", tio->c_cc[VSTART]);
    show_control("stop", tio->c_cc[VSTOP]);
    putchar('\n');
    printf("%sbrkint %sicrnl %sixon %sinpck %sixany\n",
           (tio->c_iflag & BRKINT) ? "" : "-",
           (tio->c_iflag & ICRNL) ? "" : "-",
           (tio->c_iflag & IXON) ? "" : "-",
           (tio->c_iflag & INPCK) ? "" : "-",
           (tio->c_iflag & IXANY) ? "" : "-");
    printf("%sonlcr %solcuc %stabs\n",
           (tio->c_oflag & ONLCR) ? "" : "-",
           (tio->c_oflag & OLCUC) ? "" : "-",
           (tio->c_oflag & TABDLY) == TAB3 ? "-" : "");
    printf("%sicanon %sisig %secho %sechoe\n",
           (tio->c_lflag & ICANON) ? "" : "-",
           (tio->c_lflag & ISIG) ? "" : "-",
           (tio->c_lflag & ECHO) ? "" : "-",
           (tio->c_lflag & ECHOE) ? "" : "-");
}

static int parse_control_char(const char *arg) {
    size_t length = strlen(arg);
    if (!strcmp(arg, "undef")) return _POSIX_VDISABLE;
    if (length == 1) return (unsigned char)arg[0];
    if (length == 2 && arg[0] == '^') {
        unsigned char c = (unsigned char)arg[1];
        if (c == '?') return 127;
        c = (unsigned char)toupper(c);
        if (c >= '@' && c <= '_') return c & 0x1f;
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--help")) {
        usage();
        return 0;
    }
    struct termios tio;
    if (tcgetattr(STDIN_FILENO, &tio) < 0) {
        perror("sttyl: standard input must be a terminal");
        return 1;
    }
    if (argc == 1) {
        show_settings(&tio);
        return 0;
    }

    struct mode_s modes[] = {
        {"icrnl", &tio.c_iflag, ICRNL, ICRNL},
        {"onlcr", &tio.c_oflag, ONLCR, ONLCR},
        {"echo", &tio.c_lflag, ECHO, ECHO},
        {"echoe", &tio.c_lflag, ECHOE, ECHOE},
        {"olcuc", &tio.c_oflag, OLCUC, OLCUC},
        {"tabs", &tio.c_oflag, TABDLY, 0},
        {"icanon", &tio.c_lflag, ICANON, ICANON},
        {"isig", &tio.c_lflag, ISIG, ISIG},
        {NULL, NULL, 0, 0}
    };

    /* Validate all arguments in a local copy before applying any changes. */
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (!strcmp(arg, "erase") || !strcmp(arg, "kill")) {
            if (i + 1 == argc) {
                fprintf(stderr, "sttyl: %s requires a character\n", arg);
                return 1;
            }
            int value = parse_control_char(argv[++i]);
            if (value < 0) {
                fprintf(stderr, "sttyl: invalid control character '%s'\n", argv[i]);
                return 1;
            }
            tio.c_cc[!strcmp(arg, "erase") ? VERASE : VKILL] = (cc_t)value;
            continue;
        }
        int disable = arg[0] == '-';
        const char *name = arg + disable;
        int found = 0;
        for (int j = 0; modes[j].name; j++) {
            if (!strcmp(name, modes[j].name)) {
                tcflag_t value = disable ? modes[j].mask & ~modes[j].enabled
                                         : modes[j].enabled;
                *modes[j].flagset = (*modes[j].flagset & ~modes[j].mask) | value;
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "sttyl: unknown mode '%s' (try --help)\n", arg);
            return 1;
        }
    }
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tio) < 0) {
        perror("sttyl: tcsetattr");
        return 1;
    }
    return 0;
}
