# -*- coding: utf-8 -*-
"""단위26 명시 파일만 스테이징하고 원문/기록의 Git blob 바이트를 확인한다."""
import json
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 작업공간이다.
report=json.loads((ROOT/'cpppj/run26.json').read_text(encoding='utf-8'))  # 마감 후 장부다.
paths=set(report['changes'])|set(report['sourceText'])|{  # 다른 사용자 작업은 포함하지 않는다.
    'tools/check_campaign26.py','tools/decomp_battle_bcast.py','tools/ghidra/battle-bcast-functions.json',
    'cpppj/battle-bcast.json','cpppj/tests/fixtures/battle-bcast-x86.tsv','cpppj/run26.json',
    'docs/exe/cpp-run26-20261015.md',
}
# 이번 보존본/실패 원문/정밀 export/로그만 포함한다.
for pattern in ('cpppj/hist/run26*/**/*','extracted/battle-bcast/**/*','extracted/run26-*.log','extracted/run26_*.py'):
    for path in ROOT.glob(pattern):
        if path.is_file():paths.add(path.relative_to(ROOT).as_posix())
subprocess.run(['git','add','-f','--',*sorted(paths)],cwd=ROOT,check=True)
exact=0  # Git에 그대로 저장된 전체 원문 수다.
# LEFT의 명시 LF 정규화 외에는 컴파일/근거 바이트가 그대로 남아야 한다.
for name in sorted(paths):
    working=(ROOT/name).read_bytes();blob=subprocess.check_output(['git','show',':'+name],cwd=ROOT)
    expected=working.replace(b'\r\n',b'\n') if name=='LEFT_JOBS.md' else working
    assert blob==expected,name
    if name!='LEFT_JOBS.md':exact+=1
print('단위26 스테이징:',len(paths),'파일, byte exact',exact,'LEFT LF 정규화1')
