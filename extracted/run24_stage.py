# -*- coding: utf-8 -*-
"""단위24 직접 변경/근거만 stage하고 원문 보존/새 결과의 Git blob 바이트를 확인한다."""
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]  # 사용자와 공유한 같은 저장소다.
paths=['.gitattributes','LEFT_JOBS.md','cpppj/src/o/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
       'cpppj/campaign-seed.json','cpppj/run24.json','tools/check_campaign24.py','tools/decomp_campaign_seed.py',
       'docs/exe/cpp-run24-20261015.md']  # 이번 직접 변경만 지정한다.
exact=[]  # 전체 바이트를 그대로 보존할 자료다.
# 새 소스/입력/직접 이력/export/로그만 포함한다.
for pattern in ('cpppj/src/o/RawGameRandomSeed.*','cpppj/tests/CampaignSeedTests.cpp','cpppj/tests/fixtures/campaign-seed-x86.tsv',
                'cpppj/hist/run24*/**/*','extracted/campaign-seed*/**/*','extracted/run24-*.log','extracted/run24_*.py','tools/ghidra/campaign-seed*-functions.json'):
    for path in root.glob(pattern):
        if path.is_file():
            name=path.relative_to(root).as_posix();paths.append(name);exact.append(name)
subprocess.run(['git','add','-f','--',*sorted(set(paths))],cwd=root,check=True)
# 원문 보존본과 새 결과를 Git의 전체 실제 바이트와 대조한다.
for name in sorted(set(exact)):
    blob=subprocess.run(['git','show',':'+name],cwd=root,stdout=subprocess.PIPE,check=True).stdout
    assert blob==(root/name).read_bytes(),name
print('단위24 stage:',len(set(paths)),'개, exact blob:',len(set(exact)),'개 전체바이트 일치')
