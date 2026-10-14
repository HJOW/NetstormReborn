#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 캠페인 세션을 클라이언트에 연결하기 전 직접 영향 전체 원문을 보존한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 저장소의 전체 바이트 기준이다.
HISTORY=ROOT/'cpppj/hist/run24-base'  # 직접 편집 원문만 보존한다.
BEFORE=('.gitattributes','LEFT_JOBS.md','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
        'cpppj/src/client/GameWorld.h','cpppj/src/client/GameWorld.cpp','cpppj/src/client/UberGump.cpp',
        'cpppj/tests/CampaignInstallSupport.h')  # 세션/화면과 기존 검사 조립의 편집 전 원문이다.
REPORT=ROOT/'cpppj/run24.json'  # 이 준비 단위의 전체 원문/실행 SHA다.
PROTECTED={  # 두 보호 PE의 이전 불변 SHA다.
    'originals/Netstorm.exe':'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE':'613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}
NEW_SOURCE=('cpppj/src/o/RawGameRandomSeed.h','cpppj/src/o/RawGameRandomSeed.cpp','cpppj/tests/CampaignSeedTests.cpp')  # 새 전체 C++ 원문이다.


def sha(path):
    """전체 파일 바이트의 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def log(name):
    """PowerShell의 UTF-16/UTF-8 로그를 변경하지 않고 읽는다."""
    data=(ROOT/name).read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def history():
    """이전 전체 원문11개 및 보호 PE를 확인한다."""
    saved=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))
    assert len(saved['files'])==11 and saved['baseline']=='9ecedb6'
    # 처음8개 및 무편집 추가3개의 전체 원문을 확인한다.
    for name,value in saved['files'].items():assert sha(HISTORY/name)==value,name
    # 보호 원본은 읽기만 한다.
    for name,value in PROTECTED.items():assert sha(ROOT/name)==value,name
    return saved


def capture():
    """같은 기준 원문을 한 번만 저장하며 이전 hist를 재귀 복사하지 않는다."""
    assert not (HISTORY/'capture.json').exists()
    files={}  # 현재 원문별 전체 SHA다.
    # 직접 영향 원문을 원래 상대경로로 보존한다.
    for name in BEFORE:
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes());files[name]=hashlib.sha256(target.read_bytes()).hexdigest()
    (HISTORY/'capture.json').write_text(json.dumps(dict(baseline='9ecedb6',host='HJOW-Athlon',files=files),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위24 직접 영향 원문 보존:',len(files))


def finalize():
    """마감한 재시드 준비 결과와 현재 전체 소스/이력/실행 바이트를 고정한다."""
    saved=history()
    assert '1131 test(s), 0 failed check(s)' in log('extracted/run24-all.log')
    assert '100% tests passed' in log('extracted/run24-all.log')
    assert '2 test(s), 0 failed check(s)' in log('extracted/run24-seed-final.log')
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+',log('extracted/run24-build-final.log'),re.I)
    native=json.loads((ROOT/'cpppj/campaign-seed.json').read_text(encoding='utf-8'))
    fixture=ROOT/'cpppj/tests/fixtures/campaign-seed-x86.tsv'
    assert native['uniqueInputs']==native['returns']==96 and native['assertions']==0
    assert native['fixtureSha']==sha(fixture) and native['toolSha']==sha(ROOT/'tools/decomp_campaign_seed.py')
    assert native['peSha']==PROTECTED['originals/Netstorm.exe']
    rows=[r.split('\t') for r in fixture.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    assert len(rows)==len({tuple(r[:4]) for r in rows})==96 and all(r[8]=='104' for r in rows)
    # 신규 직접 관찰의 실제4몸체 실행을 확인한다.
    for address in ('00455970','004559d0','004e649e','004e64ab'):assert native['nativeCalls'].get(address,0)>0,address
    for name,value in native['exports'].items():assert sha(ROOT/name)==value,name
    paths=set(saved['files'])|set(NEW_SOURCE)|{
        'tools/check_campaign24.py','tools/decomp_campaign_seed.py','tools/decomp_owner_oracle.py','cpppj/campaign-seed.json','cpppj/run23.json',
        'docs/exe/cpp-run24-20261015.md','cpppj/build/bin/Release/NetstormCpp.exe','cpppj/build/bin/Release/netstorm_tests.exe','cpppj/build/lib/Release/netstorm.lib',
    }  # 준비 단위의 직접 근거다.
    # 현재 전체 C++/fixture를 고정하며 과거 전체 이력을 재귀 복사하지 않는다.
    for folder in ('cpppj/src','cpppj/tests'):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp','.h','.inc','.tsv') or path.name=='CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 첫 실패/후속 정확 export와 이번 원문 이력/로그/도구만 포함한다.
    for pattern in ('cpppj/hist/run24*/**/*','extracted/campaign-seed*/**/*','extracted/run24-*.log','extracted/run24_*.py','tools/ghidra/campaign-seed*-functions.json'):
        for path in ROOT.glob(pattern):
            if path.is_file():paths.add(path.relative_to(ROOT).as_posix())
    changes={}  # 직접 영향 전체 전후 원문이다. 무편집 준비 파일도 명시한다.
    for name,value in saved['files'].items():
        before=(HISTORY/name).read_bytes().decode('utf-8');after=(ROOT/name).read_bytes().decode('utf-8')
        changes[name]=dict(beforeSha=value,afterSha=sha(ROOT/name),diff=''.join(difflib.unified_diff(before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile=name,tofile=name)))
    data=dict(unit='20261015-09',baseline='9ecedb6',host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon',
        finalNativeInputs=96,cppNativeInputs=96,oldInputsRecounted=0,newTests=2,allTests=1131,failedChecks=0,native=native,
        protected=PROTECTED,changes=changes,sourceText={name:(ROOT/name).read_bytes().decode('utf-8') for name in NEW_SOURCE},
        gitTextNormalization={'LEFT_JOBS.md':dict(workingSha=sha(ROOT/'LEFT_JOBS.md'),gitLfSha=hashlib.sha256((ROOT/'LEFT_JOBS.md').read_bytes().replace(b'\r\n',b'\n')).hexdigest())},
        files={name:sha(ROOT/name) for name in sorted(paths)},
        limitations=['이번단위는 세션 경계 감사와 실제 재시드 전제 복원이다. GameWorld/Battle 화면/입력 연결은 아직 미구현이다.',
            'PATCH96만 직접 비교했다. CD10.72/time32 OS/TLS·스레드 초기화 및 전체 실제 SP caller는 별도다.',
            '기존 검사 seed 입력/관찰 효과는 production 동작으로 그대로 옮기지 않는다.',
            'Early Missions/1-1~1-2 플레이/완주 동일성과 세션/생산/경제/Guide/UI/음향/승패 등 목표는 미완료다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위24 준비 전체 SHA 고정:',len(paths),'파일, Native96/CPP96/전체1131 실패0')


def verify():
    """고정한 현재 원문/입력/실행 바이트를 읽기 전용 대조한다."""
    history();data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 전체 현재 파일의 SHA를 확인한다.
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    # 새 C++ 전체 원문도 직접 확인한다.
    for name,value in data['sourceText'].items():assert value.encode('utf-8')==(ROOT/name).read_bytes(),name
    print('단위24 준비 원문/입력/실행 SHA 및 보호 PE/보존본 불변 확인')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)  # 요청한 단위 동작만 수행한다.
    parser.add_argument('action',choices=('capture','finalize','verify'));args=parser.parse_args()
    capture() if args.action=='capture' else finalize() if args.action=='finalize' else verify()
