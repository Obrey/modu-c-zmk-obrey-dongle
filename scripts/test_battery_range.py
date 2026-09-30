#!/usr/bin/env python3
"""v8 native display tests. NOT a Zephyr/ARM build or battery calibration."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "local-modules/modu-dongle/src"


class BatteryRangeTests(unittest.TestCase):
    def compile_and_run(self, fixture, definitions=(), expect_compile_failure=False):
        cc = shutil.which("cc") or shutil.which("gcc")
        self.assertIsNotNone(cc)
        with tempfile.TemporaryDirectory() as name:
            tmp = Path(name)
            includes = tmp / "include"
            for include in re.findall(r"^#include <([^>]+)>", (SRC / "dongle_status.c").read_text(), re.M):
                if include.startswith(("zephyr/", "zmk/")) or include == "lvgl.h":
                    p = includes / include
                    p.parent.mkdir(parents=True, exist_ok=True)
                    p.write_text('#include "status_ui_mock.h"\n')
            executable = tmp / "test"
            command = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror",
                       "-Wno-unused-function", "-Wno-unused-variable",
                       "-I" + str(includes), "-I" + str(ROOT / "tests"),
                       *definitions, str(ROOT / "tests" / fixture), "-o", str(executable)]
            result = subprocess.run(command, capture_output=True, text=True)
            if expect_compile_failure:
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("observed battery range must have max > min", result.stderr)
            else:
                self.assertEqual(result.returncode, 0, result.stderr)
                subprocess.run([str(executable)], check=True)

    def test_full_range_arithmetic_and_marked_display(self):
        self.compile_and_run("battery_range_test.c")

    def test_oled_adapter_default_range(self):
        self.compile_and_run("battery_range_ui_test.c")

    def test_each_side_uses_its_own_profile_not_connection_order(self):
        self.compile_and_run("battery_range_ui_test.c", (
            "-DCONFIG_MODU_BATTERY_RANGE_RIGHT_MIN_MV=3000",
            "-DCONFIG_MODU_BATTERY_RANGE_RIGHT_MAX_MV=3500"))

    def test_invalid_profile_cannot_build(self):
        for side in ("LEFT", "RIGHT"):
            for minimum in (2950, 3500):
                with self.subTest(side=side, minimum=minimum):
                    self.compile_and_run("battery_range_ui_test.c", (
                        f"-DCONFIG_MODU_BATTERY_RANGE_{side}_MIN_MV={minimum}",),
                        expect_compile_failure=True)

    def test_feature_off_keeps_original_ui(self):
        self.compile_and_run("battery_ui_test.c")

    def test_only_oled_enables_display_mode(self):
        shield = ROOT / "local-modules/modu-dongle/boards/shields/modu_dongle"
        oled = (shield / "modu_dongle_oled.conf").read_text()
        plain = (shield / "modu_dongle.conf").read_text()
        self.assertIn("CONFIG_MODU_BATTERY_RANGE_DISPLAY=y", oled)
        self.assertNotIn("CONFIG_MODU_BATTERY_RANGE_DISPLAY=y", plain)
        for side in ("LEFT", "RIGHT"):
            for suffix, value in (("MIN", 1750), ("MAX", 2950)):
                self.assertIn(f"CONFIG_MODU_BATTERY_RANGE_{side}_{suffix}_MV={value}", oled)
        for source in ("battery_telemetry_peripheral.c", "battery_telemetry_central.c",
                       "battery_telemetry_protocol.h", "peripheral_status_led.c"):
            self.assertNotIn("MODU_BATTERY_RANGE_DISPLAY", (SRC / source).read_text())

    def test_uncertainty_is_visible_and_ci_runs_tests(self):
        code = (SRC / "battery_range_display.h").read_text()
        self.assertIn('"%c %3u%%"', code)
        self.assertIn('"%c~%3u%%"', code)
        self.assertNotIn("d->percent =", code)
        self.assertNotIn("d->millivolts =", code)
        self.assertIn("empirical MODU-C gauge", code)
        workflow = (ROOT / ".github/workflows/build.yml").read_text()
        self.assertIn("python3 scripts/test_battery_range.py", workflow)


if __name__ == "__main__":
    unittest.main(verbosity=2)
