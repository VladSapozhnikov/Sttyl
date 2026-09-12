"""Exercise real terminal settings in isolated Linux pseudo-terminals."""
from pathlib import Path
import os
import pty
import subprocess
import termios
import unittest

BINARY = str(Path(__file__).resolve().parents[1] / "sttyl")


class TerminalTests(unittest.TestCase):
    def setUp(self):
        master, self.slave = pty.openpty()
        self.addCleanup(os.close, master)
        self.addCleanup(os.close, self.slave)

    def run_tool(self, *args):
        return subprocess.run([BINARY, *args], stdin=self.slave,
                              capture_output=True, text=True, timeout=5)

    def test_baud_rates_and_delete_display(self):
        attributes = termios.tcgetattr(self.slave)
        attributes[4] = attributes[5] = termios.B9600
        attributes[6][termios.VERASE] = b"\x7f"
        termios.tcsetattr(self.slave, termios.TCSANOW, attributes)
        result = self.run_tool()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("input speed 9600 baud", result.stdout)
        self.assertIn("output speed 9600 baud", result.stdout)
        self.assertIn("erase = ^?;", result.stdout)

    def test_echo_toggle(self):
        for mode, expected in [("-echo", False), ("echo", True)]:
            self.assertEqual(self.run_tool(mode).returncode, 0)
            self.assertEqual(bool(termios.tcgetattr(self.slave)[3] & termios.ECHO), expected)

    def test_control_characters(self):
        for text, expected in [("^?", 127), ("^H", 8), ("^h", 8), ("#", 35), ("^^", 30)]:
            self.assertEqual(self.run_tool("erase", text).returncode, 0)
            self.assertEqual(termios.tcgetattr(self.slave)[6][termios.VERASE], bytes([expected]))
        self.assertEqual(self.run_tool("kill", "^U").returncode, 0)
        self.assertEqual(termios.tcgetattr(self.slave)[6][termios.VKILL], b"\x15")

    def test_undefined_control_display(self):
        self.assertEqual(self.run_tool("erase", "undef").returncode, 0)
        self.assertIn("erase = <undef>;", self.run_tool().stdout)

    def test_invalid_arguments_leave_terminal_unchanged(self):
        before = termios.tcgetattr(self.slave)
        for args in [("-echo", "invalid"), ("erase",), ("kill",),
                     ("erase", "^??"), ("erase", "^1"), ("kill", "long"), ("-",)]:
            with self.subTest(args=args):
                result = self.run_tool(*args)
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(result.stderr)
                self.assertEqual(termios.tcgetattr(self.slave), before)

    def test_tabs_match_linux_stty(self):
        for mode in ["-tabs", "tabs"]:
            self.assertEqual(self.run_tool(mode).returncode, 0)
            actual = termios.tcgetattr(self.slave)[1]
            reference = subprocess.run(["stty", mode], stdin=self.slave, capture_output=True)
            self.assertEqual(reference.returncode, 0, reference.stderr)
            self.assertEqual(actual, termios.tcgetattr(self.slave)[1])

    def test_help_without_terminal(self):
        result = subprocess.run([BINARY, "--help"], input="", capture_output=True, text=True)
        self.assertEqual(result.returncode, 0)
        self.assertIn("Usage:", result.stdout)

    def test_redirected_input_reports_error(self):
        result = subprocess.run([BINARY], input="", capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("terminal", result.stderr)


if __name__ == "__main__":
    unittest.main()
