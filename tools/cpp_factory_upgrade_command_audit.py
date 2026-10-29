#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Factory 업그레이드 요청/수신의 원본 명령·최종 컴파일 입력을 정적으로 기록한다."""
import csv
import hashlib
import json
import re
import socket
import struct
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 저장소와 보호 PE를 읽는 기준이다.
REPORT = ROOT / 'cpppj/factory-upgrade-command-static-20261022.json'  # 이전 AI 시설 생산 기록과 구분하는 신규 정적 기록이다.
ENTRIES = (0x453bf0, 0x453d70, 0x40aba0, 0x496f00, 0x41df60, 0x41dff0, 0x4549c0, 0x4545e0)  # 요청/수신 근거6개와 미복원 진행/완료 경계2개다.
SOURCES = ('cpppj/src/o/RawFactoryUpgradeCommand.h', 'cpppj/src/o/RawFactoryUpgradeCommand.cpp',
           'cpppj/src/o/RawFactoryResearch.h', 'cpppj/src/o/RawFactoryResearch.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/FactoryUpgradeCommandTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp')  # 현재 실제 컴파일 입력이다.
TESTS = ('FactoryUpgradeCommand_RequestWaitAndPacketDeadline', 'FactoryUpgradeCommand_RequestAllocationFailureAndCurrentClock',
         'FactoryUpgradeCommand_ReceiveCostTimerAndExactBroadcast', 'FactoryUpgradeCommand_ReceiveDelayAndUnordered',
         'FactoryUpgradeCommand_StateAndCheckingGates', 'FactoryUpgradeCommand_CurrentOwnerClockAndBossAfterChildren',
         'FactoryUpgradeCommand_AllocationFailureAndSelectedMissingEffects', 'FactoryUpgradeCommand_ResearchDispatchKeepsUpgradeBranch',
         'CampaignSession_FactoryUpgradeCommandUsesActualHostAndSp')  # 새9개 작성/컴파일 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """보호 원본 정밀 범위/실제 명령과 최종 빌드/링크 이름을 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 해당 함수의 실제 정밀 범위만 기록하고 이웃 함수로 넓히지 않는다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 독립 해시/명령으로 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x453bf0': ('push 0x41a00000', 'call 0x496f00', 'call 0x40aba0', 'fadd qword ptr [0x508ed0]', 'call 0x4e2dc0'),
                '0x453d70': ('call 0x41df60', 'fchs', 'call 0x41dff0', 'call 0x496f00', 'call 0x4e2e30', 'test ah, 0x41'),
                '0x4549c0': ('call 0x4545e0',), '0x4545e0': ('call 0x453780',)}  # 구현한 요청/수신과 후속 실제 진행 경계를 구별한다.
    # 누락된 명령을 성공으로 추정하지 않는다.
    for entry, items in expected.items():
        # 원본의 필수 호출/폭/분기를 실제 PE에서 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<d', mapped, 0x508ed0-base)[0] != 15.0 or struct.unpack_from('<d', mapped, 0x501580-base)[0] != 1.0:
        raise ValueError('원본 완료15초/수신 최소1초 불일치')
    if not listings['0x453d70'].index('call 0x41dff0') < listings['0x453d70'].index('call 0x496f00') < listings['0x453d70'].index('call 0x4e2e30'):
        raise ValueError('원본 비용/SharedRegular/방송 순서 불일치')
    build_path = 'extracted/cpp-factory-upgrade-command-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-factory-upgrade-command-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 실제 검사 프로세스를 시작하지 않고 링크된 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    if 'factories.upgrade=SessionMissing<void,Sid>()' not in session:
        raise ValueError('AI 업그레이드 활성화 경계가 바뀌었으므로 후속 판정이 필요합니다')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['004549c0 event3/004545e0 진행·완료·예측 그림', '입력 자금/이미 진행 중 관문과 AI 업그레이드 활성화',
                           '다른 AI 실제 행동·전체 자동 프레임/종료', '15개 미션+Edit 동일성/자연 플레이'],
                limitations=['요청/수신과 실제 SP/예약 연결이며 전체 업그레이드 완료는 아님',
                             '작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 정밀도/예외·CD10.72 미검증'])


def main():
    """이번 새 정적 보고서만 UTF-8로 기록하고 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
