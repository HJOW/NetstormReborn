#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AI 초기 전략의 보호 PE 명령·연결·컴파일 입력을 정적으로 기록한다. 게임/검사/원본 명령을 실행하지 않는다."""
import csv
import hashlib
import json
import re
import socket
import struct
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소의 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-strategy-setup-static-20261021.json'  # 이전 실행/정적 기록과 구분한 신규 보고서다.
ENTRIES = (0x414af0, 0x4139f0, 0x40fc60, 0x40fbd0, 0x40fb30, 0x410360,
           0x4103c0, 0x410420, 0x40e800, 0x417d30, 0x4ac970)  # 이번 제어/재고/생산의 원본 경계다.
SOURCES = ('cpppj/src/o/RawAiStrategySetup.h', 'cpppj/src/o/RawAiStrategySetup.cpp',
           'cpppj/src/o/RawAiPreparation.h', 'cpppj/src/o/RawAiPreparation.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/tests/AiStrategySetupTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt')  # 실제 컴파일과 공유 경계의 입력이다.
TESTS = ('AiStrategySetup_RegionPrerequisitesAndEnemyRetry', 'AiStrategySetup_LocalInitializationSharesContentsAndClock',
         'AiStrategySetup_RemoteProductAndExistingTeleport', 'AiStrategySetup_BridgeScanCountsPendingSeparately',
         'AiStrategySetup_BridgeRandomAndSignedSerial', 'AiStrategySetup_ProductAssertionsAndSelectedMissingEffects',
         'AiStrategySetup_GoalPrioritiesAndOverflow', 'AiStrategySetup_GoalResetOnlyOnChangeAndStrictPointBounds',
         'CampaignSession_AiStrategyInitializationUsesActualContents')  # 새9개 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 원문을 SHA256으로 식별한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """원본 명령/최종 빌드와 검사 이름을 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('원본 x86 PE가 아닙니다')
    mapped = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이웃 함수의 바이트를 섞지 않고 정밀 디컴파일의 실제 범위만 해시/해독한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위는 서로 다른 블록으로 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x414af0': ('call 0x410300', 'call 0x432210', 'call 0x40fc60', 'call 0x46f260',
                             'call 0x410360', 'call 0x410420', 'call 0x4ac970', '[esi + 0x258], 1'),
                '0x4139f0': ('add eax, 0x4b0', 'call 0x40e800', '0xc61c4000', '[esi + 0x5f0], 0'),
                '0x40fb30': ('push 0x2710', 'call 0x4558c0', 'call 0x4257c0', 'and edx, 0x80000003',
                             'call 0x4afa70', 'fadd qword ptr [0x55b4d0]'),
                '0x40fc60': ('call 0x40fbd0', 'call 0x40fb30'),
                '0x410360': ('push 8', 'call 0x4ac9f0'), '0x4103c0': ('push 8', 'call 0x4ac9f0'),
                '0x410420': ('call 0x410360', 'call 0x4103c0', 'call 0x4acac0', 'call 0x4afa70'),
                '0x417d30': ('call 0x414af0', 'call 0x416eb0', 'call 0x4139f0', 'call 0x417c70')}  # 실제 폭/호출과 선택 순서다.
    # 디컴파일 표기가 아닌 실제 PE 명령의 필수 계약을 확인한다.
    for entry, items in expected.items():
        # 누락된 명령은 성공으로 추정하지 않는다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 정적 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<f', mapped, 0x500ee4-base)[0] != 0.5:
        raise ValueError('원본 중심 배율 불일치')
    if not listings['0x414af0'].index('call 0x40fc60') < listings['0x414af0'].index('call 0x410420') < listings['0x414af0'].index('[esi + 0x258], 1'):
        raise ValueError('원본 다리/생산/완료 순서 불일치')
    result_path = 'extracted/cpp-ai-strategy-setup-20261021-build1-result.json'
    build_path = 'extracted/cpp-ai-strategy-setup-20261021-build1.log'
    result = json.loads((ROOT / result_path).read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 프로세스를 시작하지 않고 링크된 검사 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일 검사 이름 누락: ' + name)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['00417c70의 실제 행동 하위', '00417d30/00417de0 자동 프레임과 종료',
                           '15개 미션 자연 플레이/동일성 검증', 'Edit 커스텀 맵/시나리오/저장 수명'],
                limitations=['정적 PE 대조와 컴파일이며 실행 통과가 아님', '고정 재고8칸을 넘는 손상 입력은 호스트 오류',
                             '전체 x87 확장 정밀도/예외와 CD10.72는 미검증'])


def main():
    """새 정적 보고서만 UTF-8로 기록하고 요약을 출력한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
