# -*- coding: utf-8 -*-
"""단위22 변경과 직접 근거만 stage하고 명시 exact 자료의 Git blob 바이트를 확인한다."""
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]  # 사용자와 공유한 같은 저장소다.
paths=['.gitattributes','LEFT_JOBS.md','cpppj/src/client/CMakeLists.txt','cpppj/src/o/CMakeLists.txt',
       'cpppj/tests/CMakeLists.txt','cpppj/tests/CampaignInstallSupport.h','cpppj/run22.json',
       'cpppj/projectile-geom.json','cpppj/disc-regular.json','docs/exe/cpp-run22-20261015.md',
       'tools/check_campaign22.py','tools/decomp_projectile_geom.py','tools/decomp_disc_regular.py']  # 이번 직접 변경만 지정한다.
exact=[]  # 원문 보존/독립 결과의 전체 바이트 검사 대상이다.
# 새 소스/검사/fixture는 현재 명시한 단위22 원문만 추가한다.
for pattern in ('cpppj/src/client/RawCampaignArcherBindings.*','cpppj/src/o/RawProjectileGeometry.*','cpppj/src/o/RawDiscRegular.*',
                'cpppj/tests/CampaignFireTests.cpp','cpppj/tests/ProjectileGeometryTests.cpp','cpppj/tests/DiscRegularTests.cpp',
                'cpppj/tests/fixtures/projectile-geom-x86.tsv','cpppj/tests/fixtures/disc-regular-x86.tsv',
                'cpppj/hist/run22*/**/*','extracted/projectile-*/**/*','extracted/run22-*.log','extracted/run22_*.py',
                'tools/ghidra/projectile-*-functions.json'):
    for path in root.glob(pattern):
        if path.is_file():
            name=path.relative_to(root).as_posix();paths.append(name);exact.append(name)
subprocess.run(['git','add','-f','--',*sorted(set(paths))],cwd=root,check=True)
# 명시한 전체 원문/독립 결과는 줄바꿈 변환 없이 Git에도 그대로 보존해야 한다.
for name in sorted(set(exact)):
    blob=subprocess.run(['git','show',':'+name],cwd=root,stdout=subprocess.PIPE,check=True).stdout
    assert blob==(root/name).read_bytes(),name
print('단위22 stage:',len(set(paths)),'개, exact blob:',len(set(exact)),'개 전체 바이트 일치')
