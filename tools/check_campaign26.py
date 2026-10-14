#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 캠페인 송신 분배 연결 전 직접 영향 원문을 보존한다."""
import hashlib
import json
import argparse
import difflib
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 작업공간이다.
HISTORY=ROOT/'cpppj/hist/run26-base'  # 이번 직접 영향 원문만 보관한다.
REPORT=ROOT/'cpppj/run26.json'  # 이번 단위의 전체 원문/실행 SHA 장부다.
PROTECTED={  # 보호 PE는 읽기만 한다.
    'originals/Netstorm.exe':'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE':'613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}
NEW_SOURCE=('cpppj/src/o/RawBattleBroadcast.h','cpppj/src/o/RawBattleBroadcast.cpp',
            'cpppj/tests/BattleBroadcastTests.cpp')  # 신규 C++ 전체 원문이다.
BEFORE=('.gitattributes','LEFT_JOBS.md','cpppj/src/client/RawCampaignSession.h',
        'cpppj/src/client/RawCampaignSession.cpp','cpppj/src/o/CMakeLists.txt',
        'cpppj/tests/CMakeLists.txt','cpppj/tests/CampaignSessionTests.cpp','cpppj/src/client/UberGump.cpp')  # 편집 전 전체 원문이다.


def capture():
    """이전 이력을 재귀 복사하지 않고 전체 원문을 한 번 보존한다."""
    assert not (HISTORY/'capture.json').exists();files={}  # 이전 원문의 전체 SHA다.
    # 직접 영향 원문만 명시 경로에 기록한다.
    for name in BEFORE:
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes());files[name]=hashlib.sha256(target.read_bytes()).hexdigest()
    (HISTORY/'capture.json').write_text(json.dumps(dict(baseline='7cb5494',host='HJOW-Athlon',files=files),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위26 직접 영향 원문:',len(files))


def sha(path):
    """전체 파일 바이트의 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def log(name):
    """PowerShell의 UTF-16/UTF-8 로그를 변경하지 않고 읽는다."""
    data=(ROOT/name).read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def history():
    """기준 원문8개와 보호 PE 불변을 확인한다."""
    saved=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))
    assert saved['baseline']=='7cb5494' and len(saved['files'])==8
    # 직접 영향의 전체 원문을 변경하지 않는다.
    for name,value in saved['files'].items():assert sha(HISTORY/name)==value,name
    # 보호 파일의 SHA를 대조한다.
    for name,value in PROTECTED.items():assert sha(ROOT/name)==value,name
    return saved


def finalize():
    """실제 송신 분배/세션 연결의 최종 근거를 전체 바이트로 고정한다."""
    saved=history()
    assert '1139 test(s), 0 failed check(s)' in log('extracted/run26-all.log')
    assert '100% tests passed' in log('extracted/run26-all.log')
    assert '3 test(s), 0 failed check(s)' in log('extracted/run26-bcast-final.log')
    assert '5 test(s), 0 failed check(s)' in log('extracted/run26-session-final.log')
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+',log('extracted/run26-build-final.log'),re.I)
    native=json.loads((ROOT/'cpppj/battle-bcast.json').read_text(encoding='utf-8'))
    fixture=ROOT/'cpppj/tests/fixtures/battle-bcast-x86.tsv'
    assert native['uniqueInputs']==1356 and native['fixtureSha']==sha(fixture)
    assert native['toolSha']==sha(ROOT/'tools/decomp_battle_bcast.py')
    assert fixture.read_bytes()==(ROOT/'cpppj/hist/run26-draft/battle-bcast-x86.tsv').read_bytes()
    rows=[row.split('\t') for row in fixture.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    assert len(rows)==len({tuple(row[:-1]) for row in rows})==1356
    # 최종 고유678개씩만 세며 최초 실패 후 재실행을 합산하지 않는다.
    for edition,result in native['editions'].items():
        assert result['inputs']==result['returns']==678 and result['assertions']==0
        binary='originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'
        assert result['peSha']==PROTECTED[binary]
        for name,value in result['exports'].items():assert sha(ROOT/name)==value,name
    paths=set(saved['files'])|set(NEW_SOURCE)|{
        'tools/check_campaign26.py','tools/decomp_battle_bcast.py','tools/decomp_owner_oracle.py','cpppj/battle-bcast.json',
        'docs/exe/cpp-run26-20261015.md','cpppj/build/bin/Release/NetstormCpp.exe',
        'cpppj/build/bin/Release/netstorm_tests.exe','cpppj/build/lib/Release/netstorm.lib',
    }  # 이번 단위 직접 근거다.
    # 현재 전체 C++/fixture를 고정하며 과거 이력은 재귀 복사하지 않는다.
    for folder in ('cpppj/src','cpppj/tests'):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp','.h','.inc','.tsv') or path.name=='CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 첫 보고서 오류/미빌드 초안/정밀 export를 모두 보존한다.
    for pattern in ('cpppj/hist/run26*/**/*','extracted/battle-bcast/**/*','extracted/run26-*.log',
                    'extracted/run26_*.py','tools/ghidra/battle-bcast-functions.json'):
        for path in ROOT.glob(pattern):
            if path.is_file():paths.add(path.relative_to(ROOT).as_posix())
    changes={}  # 직접 영향의 전체 전후 원문과 diff다.
    # 무편집 준비 파일도 명시한다.
    for name,value in saved['files'].items():
        before=(HISTORY/name).read_bytes().decode('utf-8');after=(ROOT/name).read_bytes().decode('utf-8')
        changes[name]=dict(beforeSha=value,afterSha=sha(ROOT/name),diff=''.join(difflib.unified_diff(
            before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile=name,tofile=name)))
    data=dict(unit='20261015-11',baseline='7cb5494',host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon',
        nativeInputs=1356,cppNativeInputs=1356,oldInputsRecounted=0,newTests=4,allTests=1139,failedChecks=0,native=native,
        protected=PROTECTED,changes=changes,sourceText={name:(ROOT/name).read_bytes().decode('utf-8') for name in NEW_SOURCE},
        gitTextNormalization={'LEFT_JOBS.md':dict(workingSha=sha(ROOT/'LEFT_JOBS.md'),
            gitLfSha=hashlib.sha256((ROOT/'LEFT_JOBS.md').read_bytes().replace(b'\r\n',b'\n')).hexdigest())},
        files={name:sha(ROOT/name) for name in sorted(paths)},
        limitations=['실제 분배 몸체를 복원했으며 실제 가상 송신 장치는 관찰 경계다. LAN I/O 완료가 아니다.',
            'Gun/Path 현재 writer 연결이며 dependency Form/자산 직렬화는 별도다.',
            '표 포인터 교체와 server0 assertion의 PE 실행 대조는 입력 범위 밖이다.',
            '실제 메뉴/UI/음향/Guide/Ready/경제/AI/승패와 Early Missions/1-1~1-2 완주 동일성은 미완료다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위26 전체 SHA 고정:',len(paths),'파일, Native/CPP1356/전체1139 실패0')


def verify():
    """현재 원문/입력/실행/보존본 바이트를 재확인한다."""
    history();data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 고정한 모든 근거의 SHA를 대조한다.
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    # 신규 전체 소스도 복원 가능한 바이트로 대조한다.
    for name,value in data['sourceText'].items():assert value.encode('utf-8')==(ROOT/name).read_bytes(),name
    print('단위26 원문/입력/실행 SHA 및 보호 PE/보존본 불변 확인')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)  # 명시 한 동작만 수행한다.
    parser.add_argument('action',choices=('capture','finalize','verify'));args=parser.parse_args()
    capture() if args.action=='capture' else finalize() if args.action=='finalize' else verify()
