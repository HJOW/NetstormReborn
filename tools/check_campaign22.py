#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""캠페인 실제 발사 연결의 직접 영향 원문을 편집 전에 보존한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 저장소의 전체 바이트 기준이다.
HISTORY=ROOT/'cpppj/hist/run22-base'  # 직접 영향 이전 원문만 보존한다.
BEFORE=('.gitattributes','LEFT_JOBS.md','cpppj/src/client/CMakeLists.txt','cpppj/src/o/CMakeLists.txt',
        'cpppj/tests/CMakeLists.txt','cpppj/tests/CampaignInstallSupport.h')  # 첫 연결의 기존 원문이다.
REPORT=ROOT/'cpppj/run22.json'  # 현재 검증의 전체 원문/실행 SHA다.
PROTECTED={  # 두 보호 PE의 이전 불변 SHA다.
    'originals/Netstorm.exe':'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE':'613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}
NEW_SOURCE=('cpppj/src/client/RawCampaignArcherBindings.h','cpppj/src/client/RawCampaignArcherBindings.cpp',
            'cpppj/src/o/RawProjectileGeometry.h','cpppj/src/o/RawProjectileGeometry.cpp',
            'cpppj/src/o/RawDiscRegular.h','cpppj/src/o/RawDiscRegular.cpp',
            'cpppj/tests/CampaignFireTests.cpp','cpppj/tests/ProjectileGeometryTests.cpp','cpppj/tests/DiscRegularTests.cpp')  # 새 전체 원문이다.


def sha(path):
    """전체 바이트의 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def log(name):
    """PowerShell의 원래 UTF-16/UTF-8 로그를 변경하지 않고 읽는다."""
    data=(ROOT/name).read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def history():
    """보존한 이전 원문6개와 보호 두 PE를 확인한다."""
    saved=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))
    assert len(saved['files'])==6 and saved['baseline']=='55c6f9e'
    # 직접 영향 파일의 이전 전체 원문을 확인한다.
    for name,value in saved['files'].items():assert sha(HISTORY/name)==value,name
    # 보호 원본은 읽기만 하며 이번 작업에서 수정하지 않는다.
    for name,value in PROTECTED.items():assert sha(ROOT/name)==value,name
    return saved


def capture():
    """명시 이전 파일을 한 번 저장하며 기존 hist는 재귀 복사하지 않는다."""
    assert not (HISTORY/'capture.json').exists()
    files={}  # 원래 경로별 전체 SHA다.
    # 직접 편집 원문만 원래 상대경로로 보존한다.
    for name in BEFORE:
        target=HISTORY/name
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes())
        files[name]=hashlib.sha256(target.read_bytes()).hexdigest()
    (HISTORY/'capture.json').write_text(json.dumps(dict(baseline='55c6f9e',host='HJOW-Athlon',files=files),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위22 직접 영향 원문 보존:',len(files))


def finalize():
    """최종 검사 뒤 현재 Native/전체 C++/직접 이력/실행 바이트를 고정한다."""
    saved=history()
    assert '1126 test(s), 0 failed check(s)' in log('extracted/run22-all.log')
    assert '100% tests passed' in log('extracted/run22-all.log')
    # 서로 겹치지 않는 새 검사6개의 최종 성공만 고정한다.
    for name in ('run22-geometry-final.log','run22-disc-final.log','run22-fire-final.log'):
        assert '2 test(s), 0 failed check(s)' in log('extracted/'+name),name
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+',log('extracted/run22-build-final.log'),re.I)
    native={}  # 새 고유 원본 입력432개만 포함한다.
    for stem,total in (('projectile-geom',240),('disc-regular',192)):
        record=json.loads((ROOT/f'cpppj/{stem}.json').read_text(encoding='utf-8'))
        fixture=ROOT/f'cpppj/tests/fixtures/{stem}-x86.tsv'
        tool='decomp_projectile_geom.py' if stem=='projectile-geom' else 'decomp_disc_regular.py'
        assert record['uniqueInputs']==total and record['fixtureSha']==sha(fixture)
        assert record['toolSha']==sha(ROOT/'tools'/tool)
        rows=[r.split('\t') for r in fixture.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
        # 현재 판본별 실제 정상반환/고유 입력을 전수 확인한다.
        for edition,data in record['editions'].items():
            chosen=[r for r in rows if r[0]==edition]
            assert len(chosen)==data['samples']==data['returns']==total//2 and data['assertions']==0
            assert len({tuple(r[:11] if stem=='projectile-geom' else r[:7]) for r in chosen})==total//2
            for name,value in data['exports'].items():assert sha(ROOT/name)==value,name
        native[stem]=record
    paths=set(saved['files'])|set(NEW_SOURCE)|{
        'tools/check_campaign22.py','tools/decomp_projectile_geom.py','tools/decomp_disc_regular.py','tools/decomp_owner_oracle.py',
        'cpppj/projectile-geom.json','cpppj/disc-regular.json','docs/exe/cpp-run22-20261015.md',
        'cpppj/run21.json','originals/netstorm.tarc','originalCD/netstorm.tarc',
        'cpppj/build/bin/Release/NetstormCpp.exe','cpppj/build/bin/Release/netstorm_tests.exe','cpppj/build/lib/Release/netstorm.lib',
    }  # 직접 검증과 현재 실행 근거다.
    # 전체 현재 소스/fixture를 SHA로 참조하며 과거 전체 이력은 재귀 복사하지 않는다.
    for folder in ('cpppj/src','cpppj/tests'):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp','.h','.inc','.tsv') or path.name=='CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 이번 직접 이력/export/로그/분석 스크립트만 포함한다.
    for pattern in ('cpppj/hist/run22*/**/*','extracted/projectile-*/**/*','extracted/run22-*.log','extracted/run22_*.py',
                    'tools/ghidra/projectile-*-functions.json'):
        for path in ROOT.glob(pattern):
            if path.is_file():paths.add(path.relative_to(ROOT).as_posix())
    changes={}  # 직접 영향 파일의 전체 전후 원문 대응이다.
    for name,value in saved['files'].items():
        before=(HISTORY/name).read_bytes().decode('utf-8');after=(ROOT/name).read_bytes().decode('utf-8')
        changes[name]=dict(beforeSha=value,afterSha=sha(ROOT/name),diff=''.join(difflib.unified_diff(before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile=name,tofile=name)))
    data=dict(unit='20261015-07',baseline='55c6f9e',host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon',
        finalNativeInputs=432,oldInputsRecounted=0,newTests=6,allTests=1126,failedChecks=0,native=native,
        protected=PROTECTED,changes=changes,sourceText={name:(ROOT/name).read_bytes().decode('utf-8') for name in NEW_SOURCE},
        gitTextNormalization={'LEFT_JOBS.md':dict(workingSha=sha(ROOT/'LEFT_JOBS.md'),
            gitLfSha=hashlib.sha256((ROOT/'LEFT_JOBS.md').read_bytes().replace(b'\r\n',b'\n')).hexdigest())},
        files={name:sha(ROOT/name) for name in sorted(paths)},
        limitations=['실제1-1의 저장 사제를 실제 Unpop/Pop으로 재배치한 명시 입력. 접근 조작/미션 완주 검증이 아니다.',
            '현재 Kernel의 실제 검색→Fire→Missile 생성/첫 이동만 검증. 충돌/피해/사망/완료 반복은 후속이다.',
            'fencemark 없음은 검사 외부 전제다. 교차 corner helper/실제 음향/네트워크/GameWorld/UI는 미완료.',
            '독립 TYPE/SHP 입력. Disc setter 요청 관찰과 기존 setter 검증 입력은 재합산하지 않는다.',
            '기존 이력의 파일명41자 한 건과 이전 경로/참조/과거 SHA 감사는 별도 후속이다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위22 전체 SHA 고정:',len(paths),'파일, Native432/전체1126 실패0')


def verify():
    """현재 전체 원문/실행 바이트와 이전 원문 보존본을 읽기 전용 대조한다."""
    history();data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 고정한 모든 현재 파일의 전체 바이트를 대조한다.
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    # 새 C++ 전체 원문도 직접 대조한다.
    for name,value in data['sourceText'].items():assert value.encode('utf-8')==(ROOT/name).read_bytes(),name
    print('단위22 전체 원문/입력/실행 SHA 및 보호 PE/보존본 불변 확인')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)  # 요청한 단위 동작만 실행한다.
    parser.add_argument('action',choices=('capture','finalize','verify'))
    args=parser.parse_args()
    capture() if args.action=='capture' else finalize() if args.action=='finalize' else verify()
