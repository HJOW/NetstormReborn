# -*- coding: utf-8 -*-
"""단위23 변경과 직접 자료만 stage하여 명시 exact 자료의 Git blob을 확인한다."""
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]  # 같은 사용자 작업공간이다.
paths=['.gitattributes','LEFT_JOBS.md','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
       'cpppj/tests/CampaignInstallSupport.h','cpppj/campaign-hit.json','cpppj/run23.json',
       'tools/decomp_campaign_hit.py','tools/check_campaign23.py','docs/exe/cpp-run23-20261015.md']  # 이번 직접 변경만 지정한다.
exact=[]  # 전체 바이트 보존 대상이다.
# 직접 새 소스/fixture/이력/로그만 포함하며 과거 이력을 다시 복사하지 않는다.
for pattern in ('cpppj/src/client/RawCampaignHitBindings.*','cpppj/tests/CampaignHitTests.cpp','cpppj/tests/fixtures/campaign-hit-x86.tsv',
                'cpppj/hist/run23*/**/*','extracted/run23-*.log','extracted/run23_*.py'):
    for path in root.glob(pattern):
        if path.is_file():
            name=path.relative_to(root).as_posix();paths.append(name);exact.append(name)
subprocess.run(['git','add','-f','--',*sorted(set(paths))],cwd=root,check=True)
# 원문 보존/새 결과는 줄바꿈 변환 없이 Git에도 동일 바이트로 남겨야 한다.
for name in sorted(set(exact)):
    blob=subprocess.run(['git','show',':'+name],cwd=root,stdout=subprocess.PIPE,check=True).stdout
    assert blob==(root/name).read_bytes(),name
print('단위23 stage:',len(set(paths)),'개, exact blob:',len(set(exact)),'개 전체바이트 일치')
