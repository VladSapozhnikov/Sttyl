# Sttyl: Linux Terminal Settings Utility

A small C program for inspecting and changing Linux terminal settings through `termios`. It began as systems programming coursework and has been maintained with clearer output, argument validation, and regression tests.

The project explores a practical troubleshooting question: which terminal settings control how typed characters are displayed and processed?

## What it does

- Displays input/output speeds, terminal flags, and special characters.
- Changes echo, line input, selected input/output modes, erase, and line-kill characters.
- Reports invalid options with a nonzero exit status and leaves settings unchanged when argument validation fails.
- Includes tests that use disposable pseudo-terminals, so the test suite does not change your interactive shell's settings.

## Build and run

Requires **Linux**, a C compiler, and Make. Python 3 is needed only for tests. On Windows, run these commands inside WSL or an SSH session to a Linux machine; this is not a native PowerShell program.

```bash
git clone https://github.com/VladSapozhnikov/Sttyl.git
cd Sttyl
make
./sttyl
./sttyl --help
```

Without Make:

```bash
cc -std=gnu11 -Wall -Wextra -Wpedantic -O2 sttyl.c -o sttyl
```

Run `./sttyl` in an interactive terminal. Redirecting its input from a file or pipe produces an error because there is no terminal to configure.

## Try a small change

In a Bash shell, save your current settings, set the erase key to DEL, inspect the result, and restore the original settings:

```bash
saved=$(stty -g)
./sttyl erase '^?'
./sttyl
stty "$saved"
```

The output should include `erase = ^?;`. Speeds and other flags depend on the terminal. Changing echo or canonical input can make a shell behave unexpectedly; `stty "$saved"` restores the saved state. If it was not saved, `stty sane` restores common defaults.

## Supported options

| Option | Effect when enabled |
| --- | --- |
| `echo` | Echo typed characters |
| `echoe` | Visually erase the preceding character in canonical mode |
| `icanon` | Use line-oriented input |
| `isig` | Let special characters generate signals |
| `icrnl` | Translate carriage return to newline on input |
| `onlcr` | Translate newline to carriage return/newline on output |
| `olcuc` | Map lowercase to uppercase on output |
| `tabs` | Preserve tab characters; `-tabs` selects expansion to spaces |
| `erase CHAR` | Set the erase character |
| `kill CHAR` | Set the line-kill character |

Prefix a toggle with `-` to disable it, for example `-echo`. Character values accept a single byte, caret notation such as `^H`, DEL as `^?`, or `undef`. Quote caret notation in the shell. Output transformations depend on the terminal's existing output-processing setting.

## Tests and repairs

```bash
make test
```

All **8 tests passed on Linux**. The C build completed with `-Wall -Wextra -Wpedantic` and no warnings. Tests verify actual pseudo-terminal attributes and compare tab settings against Linux `stty`.

| Original issue | Maintained behavior |
| --- | --- |
| `erase '^?'` set byte 31 | Sets the DEL byte, 127 |
| DEL displayed incorrectly | Displays `^?` |
| Encoded speed printed as baud | Converts known speed constants to baud; identifies unrecognized codes |
| Invalid options and missing values returned success | Reports an error and applies no settings |
| Tab flag meaning was reversed | Matches Linux `stty tabs` and `stty -tabs` |

## Scope

This is an educational subset of `stty`, not a full replacement. It does not set baud rates, implement raw mode, or configure arbitrary device paths. It targets Linux-specific flags; macOS and native Windows are not supported. Hardware serial devices and every terminal emulator have not been tested. Pseudo-terminal baud values are configuration metadata, not measurements of typing or network speed.

Source: [`sttyl.c`](sttyl.c). Checks: [`tests/test_sttyl.py`](tests/test_sttyl.py). Reference: [Linux termios manual](https://man7.org/linux/man-pages/man3/termios.3.html).
