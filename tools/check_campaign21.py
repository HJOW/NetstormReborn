#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Gun 표적 감시를 연결하기 전 직접 영향 원문을 보존한다."""
import hashlib
import json
import argparse
import difflib
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 작업 공간이다.
HISTORY=ROOT/'cpppj/hist/run21-base'  # 이번 직접 영향 파일의 전체 원문이다.
BEFORE=('.gitattributes','LEFT_JOBS.md','cpppj/src/client/RawSurfaceWorld.h','cpppj/src/client/RawSurfaceWorld.cpp',
        'cpppj/src/client/RawCampaignAssetBindings.h','cpppj/src/client/RawCampaignAssetBindings.cpp',
        'cpppj/tests/CampaignInstallSupport.h','cpppj/tests/CMakeLists.txt')  # 편집 전 전체 보존 대상이다.
REPORT=ROOT/'cpppj/run21.json'  # 최종 실행/원문의 별도 기준이다.
PROTECTED={  # 기존 두 보호 PE의 불변 SHA다.
    'originals/Netstorm.exe':'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE':'613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}


def sha(path):
    """전체 원문 바이트의 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def log(name):
    """PowerShell의 UTF-16/UTF-8 로그 원문을 변경 없이 읽는다."""
    data=(ROOT/name).read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def history():
    """이전 전체 원문 및 보호 PE가 그대로인지 확인한다."""
    data=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))
    assert len(data['files'])==11
    # 직접 영향 원문 보존본을 전수 대조한다.
    for name,value in data['files'].items():
        assert sha(HISTORY/name)==value,name
    # 보호 원본 PE는 읽기만 한다.
    for name,value in PROTECTED.items():
        assert sha(ROOT/name)==value,name
    return data


def capture():
    """기준 전체 원문을 한 번 저장하며 이전 이력은 재귀 복사하지 않는다."""
    assert not (HISTORY/'capture.json').exists()
    files={}  # 경로별 원문 SHA다.
    # 직접 편집 파일만 원래 상대경로로 보존한다.
    for name in BEFORE:
        path=HISTORY/name
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes((ROOT/name).read_bytes())
        files[name]=hashlib.sha256(path.read_bytes()).hexdigest()
    (HISTORY/'capture.json').write_text(json.dumps(dict(baseline='acd7882',host='HJOW-Athlon',files=files),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위21 직접 영향 전체 원문 보존:',len(files))


def finalize():
    """검사 완료 뒤 직접 원문/Native/전체 소스/실행 파일을 전체 SHA로 고정한다."""
    saved=history()
    assert '1120 test(s), 0 failed check(s)' in log('extracted/run21-all.log')
    assert '100% tests passed' in log('extracted/run21-all.log')
    assert '4 test(s), 0 failed check(s)' in log('extracted/run21-focused3.log')
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+',log('extracted/run21-build5.log'),re.I)
    native=json.loads((ROOT/'cpppj/gun-depend.json').read_text(encoding='utf-8'))
    assert native['uniqueInputs']==138 and native['fixtureSha']==sha(ROOT/'cpppj/tests/fixtures/gun-depend-x86.tsv')
    assert native['toolSha']==sha(ROOT/'tools/decomp_gun_depend.py')
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/gun-depend-x86.tsv').read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # wrapper18개와 notify120개는 같은 최종 고유 입력만 집계한다.
    for edition,record in native['editions'].items():
        chosen=[r for r in rows if r[0]==edition]
        assert len(chosen)==record['samples']==record['returns']==69 and record['assertions']==0
        assert len({tuple(r[:6]) for r in chosen})==69
        for name,value in record['exports'].items():
            assert sha(ROOT/name)==value,name
    paths=set(saved['files']) | {
        'tools/check_campaign21.py','tools/decomp_gun_depend.py','tools/decomp_owner_oracle.py',
        'cpppj/gun-depend.json','docs/exe/cpp-run21-20261015.md','cpppj/tests/CampaignTargetTests.cpp',
        'cpppj/run20.json','originals/netstorm.tarc','originalCD/netstorm.tarc',
        'cpppj/build/bin/Release/Netstorm.exe','cpppj/build/bin/Release/netstorm_tests.exe','cpppj/build/lib/Release/netstorm.lib',
    }  # 이번 직접 근거와 현재 실행 파일이다.
    # 실제 현재 전체 C++/기존 fixture를 전체 SHA로 고정하고 복사하지 않는다.
    for folder in ('cpppj/src','cpppj/tests'):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp','.h','.inc','.tsv') or path.name=='CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 이번 직접 보존 이력/새 함수 export/로그만 포함하며 과거 hist는 재귀 복사하지 않는다.
    for pattern in ('cpppj/hist/run21*/**/*','extracted/gun-depend/**/*','extracted/gun-notify/**/*',
                    'extracted/run21-*.log','tools/ghidra/gun-depend-functions.json','tools/ghidra/gun-notify-functions.json'):
        for path in ROOT.glob(pattern):
            if path.is_file():
                paths.add(path.relative_to(ROOT).as_posix())
    changes={}  # 직접 영향 전체 원문의 실제 전후 차이다.
    for name,value in saved['files'].items():
        before=(HISTORY/name).read_bytes().decode('utf-8')
        after=(ROOT/name).read_bytes().decode('utf-8')
        changes[name]=dict(beforeSha=value,afterSha=sha(ROOT/name),diff=''.join(difflib.unified_diff(before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile=name,tofile=name)))
    data=dict(unit='20261015-06',baseline='acd7882',host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon',
              finalNativeInputs=138,cppNativeNotifyInputs=120,oldInputsRecounted=0,newTests=4,allTests=1120,failedChecks=0,
              native=native,protected=PROTECTED,changes=changes,
              sourceText={'cpppj/tests/CampaignTargetTests.cpp':(ROOT/'cpppj/tests/CampaignTargetTests.cpp').read_bytes().decode('utf-8')},
              files={name:sha(ROOT/name) for name in sorted(paths)},
              limitations=['wrapper18개는 pair 요청 관찰이며 C++ 완전 pair 재생으로 주장하지 않는다.',
                           '삭제 통지는 명시 dead 입력. 큰 사제 사망/피해/실제발사/GUI/완주 동일성 미완료.',
                           'ProcessForm watcher만 연결. 서버 Form 송신 요청은 명시 외부 경계이며 실제 직렬화/네트워크는 별도.',
                           'Ready/login/GameWorld/AI/생산/경제/UI/음향 및 기존 경로/참조/SHA 감사 별도.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위21 전체 SHA 고정:',len(paths),'파일, Native138/전체1120 실패0')


def verify():
    """마감한 현재 전체 바이트와 이전 원문 보존본을 읽기 전용으로 대조한다."""
    history()
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 고정한 전체 현재 원문/입력/실행 파일을 확인한다.
    for name,value in data['files'].items():
        assert sha(ROOT/name)==value,name
    # 신규 C++ 전체 원문도 직접 현재 바이트와 비교한다.
    for name,value in data['sourceText'].items():
        assert value.encode('utf-8')==(ROOT/name).read_bytes(),name
    print('단위21 전체 원문/입력/실행 SHA와 보호 PE/보존본 불변 확인')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)  # 요청한 단위 동작만 실행한다.
    parser.add_argument('action',choices=('capture','finalize','verify'))
    args=parser.parse_args()
    capture() if args.action=='capture' else finalize() if args.action=='finalize' else verify()
