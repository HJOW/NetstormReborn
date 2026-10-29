#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 캠페인 충돌/피격 조립의 직접 영향 원문을 편집 전에 보존한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 저장소의 전체 바이트 기준이다.
HISTORY=ROOT/'cpppj/hist/run23-base'  # 직접 편집 전 전체 원문만 보존한다.
BEFORE=('.gitattributes','LEFT_JOBS.md','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
        'cpppj/tests/CampaignInstallSupport.h')  # 이번 첫 조립의 기존 원문이다.
REPORT=ROOT/'cpppj/run23.json'  # 현재 단위의 전체 원문/실행 SHA다.
PROTECTED={  # 기존 두 보호 PE의 불변 SHA다.
    'originals/Netstorm.exe':'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE':'613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}
NEW_SOURCE=('cpppj/src/client/RawCampaignHitBindings.h','cpppj/src/client/RawCampaignHitBindings.cpp',
            'cpppj/tests/CampaignHitTests.cpp')  # 신규 전체 C++ 원문이다.


def sha(path):
    """전체 원문 바이트의 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def log(name):
    """원래 UTF-16/UTF-8 PowerShell 로그를 변경 없이 읽는다."""
    data=(ROOT/name).read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def history():
    """현재 기준 원문5개와 보호 PE의 불변을 확인한다."""
    saved=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))
    assert len(saved['files'])==5 and saved['baseline']=='5f564e8'
    # 직접 영향 편집 전 전체 원문을 대조한다.
    for name,value in saved['files'].items():assert sha(HISTORY/name)==value,name
    # 보호 파일은 읽기만 한다.
    for name,value in PROTECTED.items():assert sha(ROOT/name)==value,name
    return saved


def capture():
    """명시 원문을 한 번 저장하며 기존 hist를 재귀 복사하지 않는다."""
    assert not (HISTORY/'capture.json').exists()
    files={}  # 원래 경로별 전체 SHA다.
    # 직접 영향 원문만 원래 상대 경로로 보존한다.
    for name in BEFORE:
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes());files[name]=hashlib.sha256(target.read_bytes()).hexdigest()
    (HISTORY/'capture.json').write_text(json.dumps(dict(baseline='5f564e8',host='HJOW-Athlon',files=files),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위23 직접 영향 원문 보존:',len(files))


def finalize():
    """마감한 현재 Native/소스/이력/실행 자료를 전체 SHA로 고정한다."""
    saved=history()
    assert '1129 test(s), 0 failed check(s)' in log('extracted/run23-all.log')
    assert '100% tests passed' in log('extracted/run23-all.log')
    assert '3 test(s), 0 failed check(s)' in log('extracted/run23-hit-final.log')
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+',log('extracted/run23-build-final.log'),re.I)
    native=json.loads((ROOT/'cpppj/campaign-hit.json').read_text(encoding='utf-8'))
    fixture=ROOT/'cpppj/tests/fixtures/campaign-hit-x86.tsv'
    assert native['uniqueInputs']==96 and native['fixtureSha']==sha(fixture)
    assert native['toolSha']==sha(ROOT/'tools/decomp_campaign_hit.py')
    rows=[r.split('\t') for r in fixture.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 양 판본의 새 고유 사슬 입력/반환만 집계하며 준비용 Adjust0은 재합산하지 않는다.
    for edition,data in native['editions'].items():
        chosen=[r for r in rows if r[0]==edition]
        assert len(chosen)==data['samples']==data['returns']==48 and data['assertions']==0
        assert len({tuple(r[:5]) for r in chosen})==48
        for name,value in data['exports'].items():assert sha(ROOT/name)==value,name
    paths=set(saved['files'])|set(NEW_SOURCE)|{
        'tools/check_campaign23.py','tools/decomp_campaign_hit.py','tools/decomp_priestdamage_oracle.py',
        'tools/decomp_prieststate_oracle.py','tools/decomp_gunhit_oracle.py','tools/decomp_owner_oracle.py',
        'cpppj/campaign-hit.json','cpppj/run22.json','cpppj/priestdamage.json','cpppj/gunhit.json','cpppj/gunimpact.json','cpppj/dudenotice.json',
        'docs/exe/cpp-run23-20261015.md','originals/netstorm.tarc','originalCD/netstorm.tarc',
        'cpppj/build/bin/Release/Netstorm.exe','cpppj/build/bin/Release/netstorm_tests.exe','cpppj/build/lib/Release/netstorm.lib',
    }  # 현재 직접 근거와 기존 원본 기록은 SHA로만 참조한다.
    # 실제 사용한 기존 정확 export 원문을 참조하며 새 관찰 수로 합산하지 않는다.
    for data in native['editions'].values():paths.update(data['exports'])
    # 현재 전체 C++/fixture를 고정하며 이전 전체 이력은 재귀 복사하지 않는다.
    for folder in ('cpppj/src','cpppj/tests'):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp','.h','.inc','.tsv') or path.name=='CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 이번 직접 이력/로그/lookup 및 stage 분석 스크립트만 고정한다.
    for pattern in ('cpppj/hist/run23*/**/*','extracted/run23-*.log','extracted/run23_*.py'):
        for path in ROOT.glob(pattern):
            if path.is_file():paths.add(path.relative_to(ROOT).as_posix())
    changes={}  # 직접 영향 전체 전후 원문 대응이다.
    for name,value in saved['files'].items():
        before=(HISTORY/name).read_bytes().decode('utf-8');after=(ROOT/name).read_bytes().decode('utf-8')
        changes[name]=dict(beforeSha=value,afterSha=sha(ROOT/name),diff=''.join(difflib.unified_diff(before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile=name,tofile=name)))
    data=dict(unit='20261015-08',baseline='5f564e8',host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon',
        finalNativeInputs=96,cppNativeInputs=48,oldInputsRecounted=0,newTests=3,allTests=1129,failedChecks=0,native=native,
        protected=PROTECTED,changes=changes,sourceText={name:(ROOT/name).read_bytes().decode('utf-8') for name in NEW_SOURCE},
        gitTextNormalization={'LEFT_JOBS.md':dict(workingSha=sha(ROOT/'LEFT_JOBS.md'),
            gitLfSha=hashlib.sha256((ROOT/'LEFT_JOBS.md').read_bytes().replace(b'\r\n',b'\n')).hexdigest())},
        files={name:sha(ROOT/name) for name in sorted(paths)},
        limitations=['실제1-1 저장 사제의 실제 공간 재배치 입력이다. 접근/플레이어 입력/캠페인 완주 검증은 아니다.',
            'HP1후12초에서 실제 표적/감시/83/160 종료와 보호막/프로세스를 확인한다. 실제 안내/AI/음향은 외부 경계다.',
            '새Native96은 명시 query/CD DPS/회복·가상 공간 경계를 포함한다. PATCH48만 새 C++ 조립기로 직접 비교했다.',
            'nonzeroBurning/Dude 대피 생성/fencemark/다른파생 삭제와 GameWorld/Guide/생산/경제/승패 등 목표는 미완료다.',
            '기존 ignored 이력의 파일명41자1건 및 과거 경로/참조/SHA 감사는 별도 후속이다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위23 전체 SHA 고정:',len(paths),'파일, Native96/CPP48/전체1129 실패0')


def verify():
    """고정한 현재 전체 파일과 보존 원문을 읽기 전용 대조한다."""
    history();data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 현재 원문/Native/실행의 전체 바이트를 대조한다.
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    # 신규 C++ 전체 원문도 현재 바이트와 직접 비교한다.
    for name,value in data['sourceText'].items():assert value.encode('utf-8')==(ROOT/name).read_bytes(),name
    print('단위23 전체 원문/입력/실행 SHA와 보호 PE/보존본 불변 확인')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)  # 요청한 단위 동작만 실행한다.
    parser.add_argument('action',choices=('capture','finalize','verify'));args=parser.parse_args()
    capture() if args.action=='capture' else finalize() if args.action=='finalize' else verify()
