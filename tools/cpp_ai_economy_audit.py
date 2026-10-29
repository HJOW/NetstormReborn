#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AI 경제/재고 갱신의 PE 근거와 최종 컴파일을 읽기만 해 기록한다. 검사/게임/원본 명령 실행은 없다."""
import csv
import hashlib
import json
import re
import socket
import struct
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 저장소/보호 PE를 읽는 기준이다.
REPORT = ROOT / 'cpppj/ai-economy-static-20261022.json'  # 초기 전략 단위의 옛 보고서를 보존하는 새 기록이다.
ENTRIES = (0x4138c0, 0x40fcb0, 0x492340, 0x40edb0, 0x40ef30, 0x417c70, 0x417bd0)  # 원본 경제/재고와 실제 호출 부모다.
SOURCES = ('cpppj/src/o/RawAiEconomy.h', 'cpppj/src/o/RawAiEconomy.cpp',
           'cpppj/src/o/RawAiStrategySetup.h', 'cpppj/src/o/RawAiStrategySetup.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiEconomyTests.cpp', 'cpppj/tests/AiStrategySetupTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp')  # 이번 실제 컴파일 입력이다.
TESTS = ('AiEconomy_TickBoundaryAndSingleSupport', 'AiEconomy_SignedSupportAndDemoFloor',
         'AiEconomy_CaptureBudgetRereadsCurrentBody', 'AiEconomy_CaptureBudgetOverflowAndCurrentDemoFloor',
         'AiEconomy_FinalSpUsesSameStoreWithoutWalletCredit', 'AiEconomy_UnorderedAndSelectedMissingHooks',
         'AiStrategySetup_RefreshBridgeGatesAndLocalDemo', 'AiStrategySetup_RefreshRemoteDeadlineAndUnordered',
         'CampaignSession_AiEconomySharesActualSpAndWallet')  # 새9개 작성/컴파일 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """정밀 함수 범위/실제 기계어·빌드/링크 자료를 정적으로 대조한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이웃 함수와 합치지 않고 해당 원본의 정밀 범위만 보존한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 독립 해시/명령으로 기록한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x4138c0': ('fcomp qword ptr [esi + 0x5d8]', 'test ah, 1', 'fadd qword ptr [0x501580]',
                             'add ecx, 0x898', 'call 0x492340', 'fild dword ptr [esi + 0x2d8]',
                             'fstp dword ptr [esp]', 'call 0x40edb0'),
                '0x40fcb0': ('call 0x40fbd0', '[eax + 0x22]', '[edx + eax + 0x7c]', 'test ah, 1', 'call 0x463b50', 'call 0x40fb30'),
                '0x492340': ('test dl, 0x22', '[esi*4 + 0x595428], 1'),
                '0x417c70': ('call 0x4138c0', 'call 0x40ef30'), '0x417bd0': ('call 0x40fcb0',)}  # 실제 폭/선택/호출 부모의 필수 명령이다.
    # 누락된 원본 명령을 추정 성공으로 채우지 않는다.
    for entry, items in expected.items():
        # 각 명령 계약을 실제 해독 문자열에서 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<d', mapped, 0x501580-base)[0] != 1.0:
        raise ValueError('원본 지원 간격 불일치')
    if listings['0x4138c0'].count('call 0x492340') != 2 or 'call 0x413bb0' in listings['0x4138c0']:
        raise ValueError('포획 두 번 조회/재가산 없는 직접SP 계약 불일치')
    build_path = 'extracted/cpp-ai-economy-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-economy-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 실행 없이 최종 링크된 이름의 존재만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일 검사 이름 누락: ' + name)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['00417c70의 수집/건설/전투 실제 행동', '00417d30/00417de0 자동 프레임/종료',
                           '15개 미션+Edit의 원본 동일성/자연 플레이'],
                limitations=['작성/컴파일/정적 대조이며 실행 통과가 아님', '전체 x87 정밀도/예외와 CD10.72 미검증',
                             '이전 초기 전략 보고서는 당시 소스/바이너리 기록으로 보존'])


def main():
    """이번 단위의 새 정적 JSON만 UTF-8로 기록한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
