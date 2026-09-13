"""Regression for LED shield selection. Host C compile with mocked API headers.

This deliberately compiles the real vendored led_breath.c, NOT a reimplementation.
It is NOT an ARM/Zephyr firmware build, nor a hardware LED test.
Windows without a host compiler skips compiler tests; CI/Linux should run them.
"""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
REL = 'modu-module/src/led_breath/led_breath.c'
SOURCE = ROOT/'vendor/modu-c-firmware'/REL
CC = shutil.which('cc') or shutil.which('gcc') or shutil.which('clang')
HEADERS = [
    'zephyr/kernel.h', 'zephyr/device.h', 'zephyr/drivers/pwm.h',
    'zephyr/logging/log.h', 'zmk/split/transport/central.h',
    'zmk/split/transport/peripheral.h', 'zmk/split/transport/types.h', 'zmk/usb.h',
]
API = r'''
#ifndef LED_HOST_TEST_API_H
#define LED_HOST_TEST_API_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define __weak __attribute__((weak))
#define _TEST_ENABLED1(x) _TEST_ENABLED2(_TEST_FLAG_##x)
#define _TEST_FLAG_1 _ignored,
#define _TEST_ENABLED2(args) _TEST_ENABLED3(args 1, 0)
#define _TEST_ENABLED3(ignore, value, ...) value
#define IS_ENABLED(x) _TEST_ENABLED1(x)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define DT_ALIAS(x) 1
#define DT_NODE_EXISTS(x) 1
#define PWM_DT_SPEC_GET(x) { .dev = &test_pwm_device, .period = 1000 }
#define LOG_MODULE_REGISTER(...)
#define LOG_INF(...)
#define LOG_WRN(...)
#define APPLICATION 0
#define SYS_INIT(fn, level, priority) int (*const test_init)(void) = fn
#define CONFIG_MODU_LED_BREATH_STEP_MS 20
#define K_MSEC(x) (x)
#define K_NO_WAIT 0
struct device { int dummy; };
extern const struct device test_pwm_device;
struct pwm_dt_spec { const struct device *dev; uint32_t period; };
struct k_work { int dummy; };
struct k_work_delayable { int dummy; };
static inline bool device_is_ready(const struct device *d) { return d != NULL; }
static inline int pwm_set_pulse_dt(const struct pwm_dt_spec *p, uint32_t v) {
    (void)p; (void)v; return 0;
}
static inline int k_work_schedule(struct k_work_delayable *p, int d) {
    (void)p; (void)d; return 0;
}
static inline void k_work_init_delayable(struct k_work_delayable *p,
                                       void (*handler)(struct k_work *)) {
    (void)p; (void)handler;
}
struct zmk_split_transport_status { bool available; bool enabled; int connections; };
#define ZMK_SPLIT_TRANSPORT_CONNECTIONS_STATUS_ALL_CONNECTED 2
struct test_central_api { struct zmk_split_transport_status (*get_status)(void); };
struct test_peripheral_api { struct zmk_split_transport_status (*get_status)(void); };
struct zmk_split_transport_central { const struct test_central_api *api; };
struct zmk_split_transport_peripheral { const struct test_peripheral_api *api; };
#if IS_ENABLED(CONFIG_ZMK_USB)
bool zmk_usb_is_powered(void);
#endif
#endif
'''


def compile_source(source: Path, definitions: list[str], *, preprocess=False):
    """Return a CompletedProcess; the caller decides whether failure is expected."""
    if CC is None:
        raise RuntimeError('No host C compiler')
    with tempfile.TemporaryDirectory(prefix='modu-led-cc-') as td:
        root = Path(td)
        include = root/'include'
        include.mkdir()
        (include/'led_test_api.h').write_text(API, encoding='utf-8')
        for header in HEADERS:
            path = include/header
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('#include "led_test_api.h"\n', encoding='utf-8')
        cmd = [CC, '-std=c99', '-Wall', '-Werror=implicit-int', '-Wno-error=implicit-function-declaration', '-Wfatal-errors',
               '-I', str(include)]
        cmd += ['-D'+d+'=1' for d in definitions]
        cmd += (['-E', '-P', str(source)] if preprocess else
                ['-c', str(source), '-o', str(root/'led_breath.o')])
        return subprocess.run(cmd, text=True, encoding='utf-8', errors='replace',
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)


class LedPatchManifestTests(unittest.TestCase):
    def test_led_hash_is_explicitly_approved(self):
        raw = json.loads((ROOT/'reference/hardware-sha256.json').read_text(encoding='utf-8-sig'))
        normalized = json.loads((ROOT/'reference/hardware-normalized-sha256.json').read_text(encoding='utf-8-sig'))
        digest = hashlib.sha256(SOURCE.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
        self.assertEqual(digest, normalized['files'][REL]['lf_sha256'])
        self.assertEqual(raw[REL], normalized['files'][REL]['source_sha256'])
        record = normalized['approved_local_patches'][REL]
        self.assertEqual(record['original_lf_sha256'],
                         '8161d6ff6437fac5bf9d6699b0d53e51c122e40322d64efacbef2c7a5d58ff6f')
        self.assertEqual(digest, record['patched_lf_sha256'])

    def test_both_new_shield_symbols_are_recognized(self):
        text = SOURCE.read_text(encoding='utf-8')
        self.assertIn('IS_ENABLED(CONFIG_SHIELD_MODU_PERIPHERAL_LEFT)', text)
        self.assertIn('IS_ENABLED(CONFIG_SHIELD_MODU_PERIPHERAL_RIGHT)', text)


@unittest.skipUnless(CC, 'Host C compiler unavailable; not a firmware test')
class LedHostCompilerTests(unittest.TestCase):
    def check_compile(self, *defs):
        result = compile_source(SOURCE, list(defs))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn('implicit declaration', result.stderr)

    def test_original_left_with_usb_compiles(self):
        self.check_compile('CONFIG_SHIELD_MODU_LEFT', 'CONFIG_ZMK_SPLIT_ROLE_CENTRAL', 'CONFIG_ZMK_USB')

    def test_original_right_compiles(self):
        self.check_compile('CONFIG_SHIELD_MODU_RIGHT')

    def test_dongle_left_peripheral_compiles(self):
        self.check_compile('CONFIG_SHIELD_MODU_PERIPHERAL_LEFT')

    def test_dongle_right_peripheral_compiles(self):
        self.check_compile('CONFIG_SHIELD_MODU_PERIPHERAL_RIGHT')

    def test_dongle_left_does_not_call_host_usb_api(self):
        result = compile_source(SOURCE, ['CONFIG_SHIELD_MODU_PERIPHERAL_LEFT'], preprocess=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('left_usb_connected', result.stdout)
        self.assertIn('white_color', result.stdout)
        self.assertNotIn('zmk_usb_is_powered', result.stdout)

    def test_dongle_right_selects_right_pwm_palette(self):
        result = compile_source(SOURCE, ['CONFIG_SHIELD_MODU_PERIPHERAL_RIGHT'], preprocess=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('right_connected_color', result.stdout)
        self.assertNotIn('profile_colors', result.stdout)
        self.assertNotIn('left_usb_connected', result.stdout)


if __name__ == '__main__':
    unittest.main()
