#!/usr/bin/env python3
"""Compile the real adapter against mocks. This is NOT a firmware build."""
from pathlib import Path
import json, shutil, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
cc=shutil.which('cc') or shutil.which('gcc') or shutil.which('clang')
if not cc:
    raise SystemExit('C compiler missing; no host C tests were run.')
results=[]
with tempfile.TemporaryDirectory() as tmp:
    for meta in (0,1):
        exe=Path(tmp)/f'adapter-{meta}'
        command=[cc,'-std=c11','-Wall','-Wextra','-Werror',f'-DCONFIG_ZMK_BEHAVIOR_METADATA={meta}',
                 '-I',str(root/'tests/stubs'),str(root/'tests/test_adapter.c'),'-o',str(exe)]
        subprocess.run(command,check=True,capture_output=True,text=True)
        result=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
        print(result.stdout.strip())
        results.append({'metadata':meta,'result':result.stdout.strip()})
report={'test_type':'host compilation of actual adapter with mocked ZMK/Zephyr/transport',
        'firmware_build':False,'physical_device_test':False,'cases_per_configuration':25,'results':results}
(root/'reports').mkdir(exist_ok=True)
(root/'reports/host-tests.json').write_text(json.dumps(report,indent=2)+'\n')
