#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 캠페인 세션의 원문 보존과 빌드/검증 근거를 고정한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 동일 공유 저장소다.
HISTORY=ROOT/'cpppj/hist/run25-base'
REPORT=ROOT/'cpppj/run25.json'  # 이번 단위의 전체 바이트 장부다.
PROTECTED={  # 보호 원본은 읽기만 하며 기준 SHA를 유지한다.
    'originals/Netstorm.exe':'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE':'613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}
NEW_SOURCE=('cpppj/src/client/RawCampaignSession.h','cpppj/src/client/RawCampaignSession.cpp',
            'cpppj/tests/CampaignSessionTests.cpp')  # 신규 전체 C++ 원문이다.
BEFORE=('.gitattributes','LEFT_JOBS.md','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
        'cpppj/src/client/GameWorld.h','cpppj/src/client/GameWorld.cpp','cpppj/src/client/UberGump.cpp',
        'cpppj/tests/CampaignInstallSupport.h')  # 이번 직접 영향의 이전 전체 원문이다.


def capture():
    """명시 원문만 한 번 보존하며 기존 hist를 재귀 복사하지 않는다."""
    assert not (HISTORY/'capture.json').exists()
    files={}  # 현재 전체 바이트의 SHA다.
    # 직접 영향 파일만 같은 상대경로에 기록한다.
    for name in BEFORE:
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes());files[name]=hashlib.sha256(target.read_bytes()).hexdigest()
    (HISTORY/'capture.json').write_text(json.dumps(dict(baseline='a24db9d',host='HJOW-Athlon',files=files),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위25 직접 영향 원문:',len(files))


def sha(path):
    """전체 파일 바이트의 SHA를 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def log(name):
    """UTF-16/UTF-8 빌드 로그를 변경하지 않고 읽는다."""
    data=(ROOT/name).read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def history():
    """이전 직접 영향 원문13개와 보호 PE를 확인한다."""
    saved=json.loads((HISTORY/'capture.json').read_text(encoding='utf-8'))
    assert len(saved['files'])==13 and saved['baseline']=='a24db9d'
    # 최초8개 및 창 제목 경계 교정의 추가5개 원문은 불변이다.
    for name,value in saved['files'].items():assert sha(HISTORY/name)==value,name
    # 보호 원본 바이트는 이번 단위에서 수정하지 않는다.
    for name,value in PROTECTED.items():assert sha(ROOT/name)==value,name
    return saved


def finalize():
    """생산 세션 단위의 전체 원문/빌드/검사 근거를 고정한다."""
    saved=history()
    assert '1135 test(s), 0 failed check(s)' in log('extracted/run25-all.log')
    assert '100% tests passed' in log('extracted/run25-all.log')
    assert '4 test(s), 0 failed check(s)' in log('extracted/run25-session-final.log')
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+',log('extracted/run25-build-final.log'),re.I)
    paths=set(saved['files'])|set(NEW_SOURCE)|{
        'tools/check_campaign25.py','docs/exe/cpp-run25-20261015.md',
        'cpppj/build/bin/Release/NetstormCpp.exe','cpppj/build/bin/Release/netstorm_tests.exe',
        'cpppj/build/lib/Release/netstorm.lib',
    }  # 이번 단위의 직접 근거다.
    # 현재 전체 C++ 원문/fixture는 해시로 고정하며 이전 이력은 재귀 복사하지 않는다.
    for folder in ('cpppj/src','cpppj/tests'):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.cpp','.h','.inc','.tsv') or path.name=='CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 이번 초기/후속 소스 생성, 보존본, 정밀 export와 모든 빌드 로그를 포함한다.
    for pattern in ('cpppj/hist/run25*/**/*','extracted/campaign-client/**/*','extracted/run25-*.log',
                    'extracted/run25_*.py','tools/ghidra/campaign-client-functions.json'):
        for path in ROOT.glob(pattern):
            if path.is_file():paths.add(path.relative_to(ROOT).as_posix())
    changes={}  # 직접 영향의 전후 전체 바이트와 설명 가능한 diff다.
    # 편집하지 않은 준비 파일도 장부에 명시한다.
    for name,value in saved['files'].items():
        before=(HISTORY/name).read_bytes().decode('utf-8');after=(ROOT/name).read_bytes().decode('utf-8')
        changes[name]=dict(beforeSha=value,afterSha=sha(ROOT/name),diff=''.join(difflib.unified_diff(
            before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile=name,tofile=name)))
    data=dict(unit='20261015-10',baseline='a24db9d',host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon',
        newNativeInputs=0,oldInputsRecounted=0,newDecompiledBodies=1,newTests=4,allTests=1135,failedChecks=0,
        protected=PROTECTED,changes=changes,sourceText={name:(ROOT/name).read_bytes().decode('utf-8') for name in NEW_SOURCE},
        gitTextNormalization={'LEFT_JOBS.md':dict(workingSha=sha(ROOT/'LEFT_JOBS.md'),
            gitLfSha=hashlib.sha256((ROOT/'LEFT_JOBS.md').read_bytes().replace(b'\r\n',b'\n')).hexdigest())},
        files={name:sha(ROOT/name) for name in sorted(paths)},
        limitations=[
            '생산 세션과 GameWorld의 동일 raw 상태 연결을 검사했다. UberGump Battle 진입은 아직 거부된다.',
            'UI/장치/송신/명시 no-fencemark 입력은 검사 경계다. 실제 클라이언트 서비스의 동작 완료가 아니다.',
            '프레임 출력 순서/깊이/선택/Guide/경제/AI/승패와 Early Missions 및 캠페인 완주 동일성은 미완료다.',
            '새 직접 PE 실행 관찰은0이다. 기존 Native 입력을 재합산하지 않았다.',
        ])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위25 전체 SHA 고정:',len(paths),'파일, 신규4/전체1135 실패0, 새 Native0')


def verify():
    """고정한 원문/로그/실행 바이트를 읽기 전용으로 재확인한다."""
    history();data=json.loads(REPORT.read_text(encoding='utf-8'))
    # 현재 전체 근거와 장부를 대조한다.
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    # 신규 C++ 원문은 복원 가능한 전체 텍스트도 확인한다.
    for name,value in data['sourceText'].items():assert value.encode('utf-8')==(ROOT/name).read_bytes(),name
    print('단위25 원문/로그/실행 SHA 및 보호 PE/보존본 불변 확인')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)  # 요청한 한 동작만 수행한다.
    parser.add_argument('action',choices=('capture','finalize','verify'));args=parser.parse_args()
    capture() if args.action=='capture' else finalize() if args.action=='finalize' else verify()
