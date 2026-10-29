"""게임/C++ 검사/native 실행 없이 이번 Factory 입력 근거와 빌드 기록을 확인한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 저장소/보호 PE/이번 빌드의 읽기 기준이다.
REPORT = ROOT / 'cpppj/factory-input-static-20261022.json'  # 과거 보고서와 구분하는 이번 신규 기록이다.
ENTRIES = (0x4547d0, 0x453c90, 0x4537f0, 0x453bf0, 0x453780, 0x4cf960,
           0x4cf730, 0x40ec50, 0x4ae010, 0x453cc0, 0x4752b0)  # 실제 입력/진행 조회/요청/안내/SP/기존 연구·부모11함수다.
SOURCES = ('cpppj/src/o/RawFactoryInput.h', 'cpppj/src/o/RawFactoryInput.cpp',
           'cpppj/src/o/RawFactoryResearch.h', 'cpppj/src/o/RawFactoryResearch.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/src/client/RawCampaignSession.h',
           'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/client/CampaignClientServices.cpp',
           'cpppj/src/client/CampaignObjectMenuClient.h', 'cpppj/src/client/CampaignObjectMenuClient.cpp',
           'cpppj/src/client/UberGump.cpp', 'cpppj/src/client/RawFactoryMenu.h',
           'cpppj/tests/CMakeLists.txt', 'cpppj/tests/FactoryInputTests.cpp',
           'cpppj/tests/FactoryResearchTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp')  # 실제 게임/메뉴 연결과 이번 컴파일 입력이다.
TESTS = ('FactoryInput_AffordableBoundaryAndNoExtraGates', 'FactoryInput_NoBucksPrecedesExistingUpgrade',
         'FactoryInput_UnorderedAndFloatCostRounding', 'FactoryInput_EditorReadsMoneyAndKeepsPendingGate',
         'FactoryInput_CurrentOwnerAndEditorAfterChildren', 'FactoryInput_CyclePreservesSignedRemainder',
         'FactoryInput_DelegatesOriginalCodeAndPackedArgument', 'FactoryInput_MissingSelectedEffectsKeepPrefix',
         'FactoryResearch_InputLogBeforeClockAndCachedSlots', 'CampaignSession_FactoryInputUsesActualSpAndPendingTimer',
         'CampaignSession_FactoryInputCycleHelpAndParent')  # 새11개 작성·컴파일 이름이며 실행 통과 수가 아니다.
COMMANDS = {0x542884: 0x03000063, 0x5427c0: 0x03000032, 0x5428f8: 0x03000080}  # 보호 PE의 실제 명령 전역이다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """보호 PE 정밀 범위/상수·호출 순서와 최종 빌드/Client 연결을 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    previous = json.loads((ROOT / 'cpppj/factory-upgrade-lifecycle-static-20261022.json').read_text(encoding='utf-8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이번 실제 함수의 불연속 정밀 범위를 독립 해시/명령으로 보존한다.
    for entry in ENTRIES:
        blocks = []
        # 이웃 함수로 범위를 넓히지 않는다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x4547d0': ('and eax, 0x3ffffff', 'idiv ecx', 'call 0x453780',
                            'fild dword ptr [esp + 0x24]', 'fstp dword ptr [esp + 0x28]',
                            'call 0x40ec50', 'test ah, 0x41', 'jnp 0x45488c', 'call 0x4cf960',
                            'call 0x453c90', 'call 0x453bf0', 'call 0x4752b0', 'call 0x4c2630',
                            'call 0x453cc0', 'call 0x4e2dc0', 'call 0x4ae010'),
                '0x453c90': ('call 0x4537f0', 'call 0x4afb40'), '0x4cf960': ('call 0x4cf730',)}  # 실제 입력의 폭/분기·하위다.
    # 원본 명령을 찾지 못하면 성공으로 기록하지 않는다.
    for entry, items in expected.items():
        # 각 함수의 정밀 범위 안에서 모든 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    # 명령 토큰은 추정 이름이 아닌 보호 PE DWORD로 확인한다.
    for address, value in COMMANDS.items():
        if struct.unpack_from('<I', mapped, address-base)[0] != value:
            raise ValueError('원본 Factory 명령 전역 불일치: ' + hex(address))
    listing = listings['0x4547d0']
    if not listing.index('fstp dword ptr [esp + 0x28]') < listing.index('call 0x40ec50') < listing.index('call 0x453c90') < listing.index('call 0x453bf0'):
        raise ValueError('원본 비용float/SP/진행/요청 순서 불일치')
    build_path = 'extracted/cpp-factory-input-20261022-build2.log'
    result = json.loads((ROOT / 'extracted/cpp-factory-input-20261022-build2-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    first_path = 'extracted/cpp-factory-input-20261022-build1.log'
    first_result = json.loads((ROOT / 'extracted/cpp-factory-input-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    if first_result['exit_code'] or 'warning C4389' not in (ROOT / first_path).read_text(encoding='utf-8-sig'):
        raise ValueError('첫 빌드 비교 경고 보존 기록 불일치')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 프로세스를 실행하지 않고 새 이름이 실제 링크됐는지만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    client = (ROOT / 'cpppj/src/client/CampaignObjectMenuClient.cpp').read_text(encoding='utf-8-sig')
    uber = (ROOT / 'cpppj/src/client/UberGump.cpp').read_text(encoding='utf-8-sig')
    guard = uber[uber.index('RawMenuHooks menuHooks;'):uber.index('menuHooks.readInt=')]
    if 'factoryInput->Input(parent,code,parameter)' not in session or 'Session().CommandFactory(' not in client:
        raise ValueError('현재 메뉴/Session 실제 Factory 입력 연결 누락')
    # 이미 복원된 실제 메뉴 분배에 전달할 명령만 허용했는지 확인한다.
    for token in ('0x03000032U', '0x03000063U', '0x030000afU', '0x030000e2U', '0x03000015U', '0x0300011aU', '0x03000076U'):
        if token not in guard or token not in client:
            raise ValueError('Client 메뉴 전송/분배 경계 누락: ' + token)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                command_globals={hex(key): hex(value) for key, value in COMMANDS.items()}, sources={name: digest(name) for name in SOURCES},
                build=dict(result=result, log_sha256=digest(build_path), first_warning=dict(result=first_result, log_sha256=digest(first_path), cause='새 상태순환 검사의 signed/unsigned 비교를 uint32로 맞춤')),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['다른 AI 실제 행동·원래 순서 자동 프레임/종료', '15개 미션+Edit/custom Fort·시나리오 자연 입력/플레이·화면/음향 동일성',
                           'Template/State7·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '실제 GUI 클릭·Tell/Help 표시·업그레이드 반복은 미검증',
                             '전체 x87 확장 정밀도/예외·CD10.72 미검증'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
