# -*- coding: utf-8 -*-
"""이번 단위의 명시 파일만 스테이징하고 Git blob의 바이트를 확인한다."""
import hashlib
import json
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 동일 작업공간이다.
report=json.loads((ROOT/'cpppj/run25.json').read_text(encoding='utf-8'))  # 마감 후 고정된 장부다.
paths=set(report['changes'])|set(report['sourceText'])|{  # 사용자 작업 전체를 임의로 스테이징하지 않는다.
    'tools/check_campaign25.py','tools/ghidra/campaign-client-functions.json',
    'docs/exe/cpp-run25-20261015.md','cpppj/run25.json',
}
# 이번 단위의 직접 보존/정밀 export/로그만 명시한다.
for pattern in ('cpppj/hist/run25*/**/*','extracted/campaign-client/**/*','extracted/run25-*.log','extracted/run25_*.py'):
    for path in ROOT.glob(pattern):
        if path.is_file():paths.add(path.relative_to(ROOT).as_posix())
subprocess.run(['git','add','-f','--',*sorted(paths)],cwd=ROOT,check=True)
exact=0  # 바이트가 동일한 신규/이력/현재 소스 수다.
# 컴파일/근거 전체 바이트가 Git blob에도 유지되는지 확인한다.
for name in sorted(paths):
    working=(ROOT/name).read_bytes();blob=subprocess.check_output(['git','show',':'+name],cwd=ROOT)
    expected=working.replace(b'\r\n',b'\n') if name=='LEFT_JOBS.md' else working
    assert blob==expected,name
    if name!='LEFT_JOBS.md':exact+=1
print('단위25 스테이징:',len(paths),'파일, byte exact',exact,'LEFT LF 정규화1')
