"""게임/C++검사/native 실행 없이 이번 Factory 진행·완료 근거와 컴파일 결과를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 빌드 기록을 읽는 저장소 기준이다.
REPORT = ROOT / 'cpppj/factory-upgrade-lifecycle-static-20261022.json'  # 과거 증거와 구분하는 이번 신규 기록이다.
ENTRIES = (0x4549c0, 0x4545e0, 0x453750, 0x4537f0, 0x453c90, 0x453780,
           0x411180, 0x413b70, 0x40f100, 0x4b22d0, 0x4b1b80, 0x4b1bd0,
           0x4b2390, 0x4ae510, 0x42b050, 0x443fe0)  # 실제 진행/완료/그림/AI 통지·기존 하위16함수다.
SOURCES = ('cpppj/src/o/RawFactoryUpgrade.h', 'cpppj/src/o/RawFactoryUpgrade.cpp',
           'cpppj/src/o/RawAssetRegular.h', 'cpppj/src/o/RawAssetRegular.cpp',
           'cpppj/src/o/RawFactoryPostPop.h', 'cpppj/src/o/RawAiFactoryProduction.h',
           'cpppj/src/o/RawAiFactoryProduction.cpp', 'cpppj/src/client/RawCampaignAssetBindings.h',
           'cpppj/src/client/RawCampaignAssetBindings.cpp', 'cpppj/src/client/RawCampaignSession.h',
           'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/o/CMakeLists.txt',
           'cpppj/tests/CMakeLists.txt', 'cpppj/tests/FactoryUpgradeTests.cpp',
           'cpppj/tests/AiFactoryProductionTests.cpp', 'cpppj/tests/AssetRegularTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp')  # 이번 실제 컴파일 입력이다.
TESTS = ('FactoryUpgrade_BeginUsesActualRawOrder', 'FactoryUpgrade_NextLevelRereadsAndCaps',
         'FactoryUpgrade_FinishOrderAndCurrentOwner', 'FactoryUpgrade_OutpostSkipsCompleteButKeepsAiNotice',
         'FactoryUpgrade_CompleteRemovesActualTimers', 'FactoryUpgrade_PictureFiltersAndSavedNext',
         'FactoryUpgrade_AssertionsAndSelectedMissingEffects',
         'AiFactoryProduction_UpgradedRequeuesAndSubtractsTypeCost',
         'AiFactoryProduction_UpgradedFullQueueAndCurrentDemoWallet',
         'AssetRegular_FactoryUpgradeDispatchAndUpperFlare',
         'CampaignSession_FactoryUpgradePictureAndCompletionShareWorld')  # 새11개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """보호 PE 정밀 범위/호출·상수와 실제 빌드/링크 이름을 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    previous = json.loads((ROOT / 'cpppj/factory-upgrade-command-static-20261022.json').read_text(encoding='utf-8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이번 실제 함수의 불연속 정밀 범위를 각각 보존한다.
    for entry in ENTRIES:
        blocks = []
        # 이웃 함수로 넓히지 않고 각 원문 범위의 해시/명령을 기록한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x4549c0': ('call 0x4af530', 'call 0x453750', 'call 0x42b050', 'call 0x4ac220',
                            'fadd dword ptr [0x509144]', 'call 0x443fe0', 'call 0x4b1510', 'call 0x4545e0',
                            'call 0x40f100', 'call 0x413b70', 'fld dword ptr [0x502350]'),
                '0x4545e0': ('call 0x4adcb0', 'call 0x4adca0', 'call 0x453780', 'call 0x4a9d70',
                            'call 0x4a9cb0', 'call 0x45a6c0', 'call 0x4afb40', 'call 0x41bf10',
                            'call 0x4b22d0', 'call 0x4b2390', 'call 0x4ae510', 'call 0x4b1b80'),
                '0x411180': ('fld dword ptr [eax + 0xc4]', 'fchs', 'call 0x4e49c0', 'call 0x463b50', 'cmp eax, 0x4a6'),
                '0x453c90': ('call 0x4537f0', 'call 0x4afb40')}  # 복원한 실제 몸체의 필수 명령이다.
    # 보호 PE에 기대 명령이 없으면 정적 확인도 성공으로 기록하지 않는다.
    for entry, items in expected.items():
        # 같은 함수 정밀 범위 안에서 모든 필수 하위를 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<f', mapped, 0x509144-base)[0] != 0.0010000000474974513 or struct.unpack_from('<f', mapped, 0x502350-base)[0] != -1.0:
        raise ValueError('원본 예측 그림 오프셋/-1 완료 반환 상수 불일치')
    build_path = 'extracted/cpp-factory-upgrade-lifecycle-20261022-build2.log'
    result = json.loads((ROOT / 'extracted/cpp-factory-upgrade-lifecycle-20261022-build2-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    first_path = 'extracted/cpp-factory-upgrade-lifecycle-20261022-build1.log'
    first_result = json.loads((ROOT / 'extracted/cpp-factory-upgrade-lifecycle-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    if first_result['exit_code'] != 1 or 'error C2248' not in (ROOT / first_path).read_text(encoding='utf-8-sig'):
        raise ValueError('첫 빌드 실패 보존 기록 불일치')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # C++ 검사 프로세스를 시작하지 않고 링크된 실제 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 링크 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    if 'factories.upgrade=SessionMissing<void,Sid>()' in session or 'factoryUpgradeCommand->Request(parent)' not in session or 'aiFactoryProduction->Upgraded(token,parent)' not in session:
        raise ValueError('같은 실제 AI 업그레이드 요청/완료 연결 불일치')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions, sources={name: digest(name) for name in SOURCES},
                build=dict(result=result, log_sha256=digest(build_path), first_failed=dict(result=first_result, log_sha256=digest(first_path), cause='통합 검사의 비공개 Host 메서드 호출을 공개 RegularProcess::RunFrame으로 수정')),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['입력03000032의 현재 자금/이미 진행 중 안내와 메뉴 입력 연결',
                           '다른 AI 실제 행동·원래 순서 자동 프레임/종료', '15개 미션+Edit 자연 입력·플레이·표시/음향 동일성',
                           'Template/State7·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 확장 정밀도/예외·CD10.72 미검증'])


def main():
    """이번 새 정적 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
