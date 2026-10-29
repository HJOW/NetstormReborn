#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 캠페인 Kernel 실행 연결의 직접 영향 원문을 보존한다."""
import argparse
import difflib
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 전체 원문을 읽는 같은 저장소다.
HISTORY = ROOT / 'cpppj/hist/run20-base'  # 기존 이력은 재귀 복사하지 않는다.
BEFORE = ('.gitattributes', 'LEFT_JOBS.md', 'cpppj/src/client/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
          'cpppj/tests/CampaignInstallSupport.h')  # 첫 Kernel 실행에 직접 영향을 받는 기존 파일이다.
REPORT = ROOT / 'cpppj/run20.json'  # 실제 최종 검사 및 전체 원문의 새 기준이다.
NEW_SOURCES = ('cpppj/src/o/RawGunMode.h','cpppj/src/o/RawGunMode.cpp',
               'cpppj/src/client/RawCampaignGunQueries.h','cpppj/src/client/RawCampaignGunQueries.cpp',
               'cpppj/tests/CampaignFrameTests.cpp','cpppj/tests/CampaignNativeFrameTests.cpp',
               'cpppj/tests/VortexDisplayTests.cpp')  # 전체 UTF-8 원문을 남기는 신규 C++이다.
PROTECTED = {  # 두 보호 PE의 이전에 확인한 불변 SHA다.
    'originals/Netstorm.exe': 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236',
    'originalCD/NETSTORM.EXE': '613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233',
}


def sha(path):
    """원문 전체 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """편집 전에 지정한 전체 원문을 한 번 보존한다."""
    assert not (HISTORY / 'capture.json').exists()
    files = {}  # 실제 이전 원문 SHA다.
    # 직접 영향 파일만 원래 상대경로로 보존한다.
    for name in BEFORE:
        target = HISTORY / name  # 원래 경로의 전체 보존본이다.
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT / name).read_bytes())
        files[name] = sha(target)
    (HISTORY / 'capture.json').write_text(json.dumps(dict(baseline='6fabde2', host='HJOW-Athlon', files=files),
                                                    ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('단위20 편집 전 전체 원문 보존:', len(files))


def log(name):
    """기존 PowerShell 로그의 UTF-16/UTF-8 원문을 변경 없이 읽는다."""
    data = (ROOT / name).read_bytes()  # 실행 당시 로그 바이트다.
    return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')


def history():
    """편집 전 보존본과 보호 PE가 그대로인지 읽기 전용으로 확인한다."""
    saved = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))
    assert len(saved['files']) == 16
    # 보존 원문 전체의 SHA를 전수 확인한다.
    for name, value in saved['files'].items():
        assert sha(HISTORY / name) == value, name
    # 원본 PE는 보호 기준과 같은지 확인하고 수정하지 않는다.
    for name, value in PROTECTED.items():
        assert sha(ROOT / name) == value, name
    return saved


def native(name, fixture, counts):
    """새 독립 원본의 고유 입력/반환/assert/fixture/export SHA를 검사한다."""
    data = json.loads((ROOT / f'cpppj/{name}.json').read_text(encoding='utf-8'))
    assert data['uniqueInputs'] == sum(counts.values()) and data['fixtureSha'] == sha(ROOT / fixture)
    tool = 'decomp_campaign_frame.py' if name == 'campaign-frame' else 'decomp_vortex_display.py'
    assert data['toolSha'] == sha(ROOT / 'tools' / tool)
    rows = [line.split('\t') for line in (ROOT / fixture).read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 최종 fixture의 각 실제 입력은 한 번만 합산한다. 보존한 실험 재실행은 재합산하지 않는다.
    for edition, count in counts.items():
        record = data['editions'][edition]
        chosen = [row for row in rows if row[0] == edition]
        assert len(chosen) == count and record['samples'] == record['returns'] == count and record['assertions'] == 0
        assert len({tuple(row[:4 if name == 'campaign-frame' else 3]) for row in chosen}) == count
        for path, value in record['exports'].items():
            assert sha(ROOT / path) == value, path
    return data


def finalize():
    """최종 빌드/CTest 완료 뒤 전체 현재 바이트와 전후 원문 차이를 고정한다."""
    saved = history()
    assert '1116 test(s), 0 failed check(s)' in log('extracted/run20-all2.log')
    assert '100% tests passed' in log('extracted/run20-all2.log')
    # 새 실행 파일에 대한 집중검사를 각각 확인한다.
    for name in ('focused13','nfocused5','vfocused4'):
        assert '2 test(s), 0 failed check(s)' in log(f'extracted/run20-{name}.log')
    assert not re.search(r'\b(?:warning|error) [A-Z]\d+',log('extracted/run20-build20.log'),re.I)
    current = native('campaign-frame','cpppj/tests/fixtures/campaign-frame-x86.tsv',{'originals':378,'originalCD':386})
    display = native('vortex-display','cpppj/tests/fixtures/vortex-display-x86.tsv',{'originals':324,'originalCD':324})
    paths = set(saved['files']) | set(NEW_SOURCES) | {
        'tools/check_campaign20.py','tools/decomp_campaign_frame.py','tools/decomp_vortex_display.py',
        'cpppj/campaign-frame.json','cpppj/vortex-display.json','docs/exe/cpp-run20-20261015.md',
        'cpppj/run19.json','cpppj/assetregular.json','cpppj/display.json','cpppj/squid-level.json',
        'originals/netstorm.tarc','originalCD/netstorm.tarc',
        'cpppj/build/bin/Release/Netstorm.exe','cpppj/build/bin/Release/netstorm_tests.exe','cpppj/build/lib/Release/netstorm.lib',
    }  # 직접 이전 근거/현재 실행 파일이다.
    # 이번 실행이 재사용한 독립 도구 원문도 전체 SHA로 고정한다.
    for path in (ROOT / 'tools').glob('decomp_*.py'):
        paths.add(path.relative_to(ROOT).as_posix())
    # 현재 모든 C++ 및 불변 원본 fixture를 복사하지 않고 전체 SHA로 고정한다.
    for folder in ('cpppj/src','cpppj/tests'):
        for path in (ROOT / folder).rglob('*'):
            if path.is_file() and (path.suffix in ('.h','.cpp','.inc','.tsv') or path.name == 'CMakeLists.txt'):
                paths.add(path.relative_to(ROOT).as_posix())
    # 직접 보존한 이번 이력과 새 export/로그만 묶는다. 기존 hist는 재귀 복사하지 않는다.
    for pattern in ('cpppj/hist/run20*/**/*','extracted/campaign-frame/**/*','extracted/vortex-update/**/*',
                    'extracted/run20-*.log','tools/ghidra/campaign-frame-functions.json','tools/ghidra/vortex-update-functions.json'):
        for path in ROOT.glob(pattern):
            if path.is_file():
                paths.add(path.relative_to(ROOT).as_posix())
    # 실제 재사용 원본 실행 범위도 전수 포함한다.
    for report in (current,display):
        for record in report['editions'].values():
            paths.update(record['exports'])
    changes = {}  # 직접 영향16개 파일의 전체 전후 차이다.
    for name, value in saved['files'].items():
        before = (HISTORY / name).read_bytes().decode('utf-8')
        after = (ROOT / name).read_bytes().decode('utf-8')
        changes[name] = dict(beforeSha=value,afterSha=sha(ROOT / name),diff=''.join(difflib.unified_diff(
            before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile=name,tofile=name)))
    data = dict(unit='20261015-05',baseline='6fabde2',host='HJOW-Athlon',lastDecompileHost='HJOW-Athlon',
                finalNativeInputs=1412,oldInputsRecounted=0,newTests=6,allTests=1116,failedChecks=0,
                kernelFrames={'thewarbegins':600,'masterofwhirligigs':600},native={'frame':current,'display':display},protected=PROTECTED,
                changes=changes,sourceText={name:(ROOT / name).read_bytes().decode('utf-8') for name in NEW_SOURCES},
                files={name:sha(ROOT / name) for name in sorted(paths)},
                limitations=['실제 GameWorld 세션/GUI/AI/전투/완주 동일성은 미완료',
                             '큰Factory upgrade/패치 상위상태 입자효과는 명시 미연결',
                             'Native 합성TYPE/SHP. Frame 지정과 일부 외부 표시/CD 종료 등록은 명시 경계',
                             '일부 과거 경로/참조/SHA 최종 감사 별도'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위20 전체 SHA 고정:',len(paths),'파일, 최종Native1412/전체1116 실패0')


def verify():
    """마감한 전체 현재 원문/실행 바이트와 보존본을 읽기 전용으로 재검사한다."""
    history()
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    # 실제 모든 현재 파일이 고정 SHA와 일치하는지 확인한다.
    for name, value in data['files'].items():
        assert sha(ROOT / name) == value, name
    # 신규 C++의 전체 원문이 현재 바이트와 같은지 확인한다.
    for name, value in data['sourceText'].items():
        assert value.encode('utf-8') == (ROOT / name).read_bytes(), name
    print('단위20 전체 원문/입력/실행 SHA와 보호 원본/보존본 불변 확인')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)  # 요청한 동작만 실행한다.
    parser.add_argument('action', choices=('capture','finalize','verify'))
    args = parser.parse_args()  # 명시한 단위의 보존 요청이다.
    capture() if args.action == 'capture' else finalize() if args.action == 'finalize' else verify()
