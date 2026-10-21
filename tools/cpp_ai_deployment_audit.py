#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AI 생산 소비/배치의 보호 PE 명령·현재 소스·최종 빌드 링크를 읽기만 해 기록한다."""
import csv
import hashlib
import json
import re
import socket
import struct
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 저장소/보호 원본을 읽는 기준이다.
REPORT = ROOT / 'cpppj/ai-deployment-static-20261022.json'  # 이전 전략/경제 기록을 보존하는 신규 정적 보고서다.
ENTRIES = (0x4106e0, 0x410280, 0x4102b0, 0x496e80, 0x444760, 0x4433b0,
           0x4108f0, 0x415920, 0x415a10)  # 이번 몸체와 후속 수집자 배치의 실제 호출 부모다.
SOURCES = ('cpppj/src/o/RawAiDeployment.h', 'cpppj/src/o/RawAiDeployment.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/client/RawCampaignCursor.h', 'cpppj/src/client/RawCampaignCursor.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiDeploymentTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp')  # 현재 실제 컴파일 입력이다.
TESTS = ('AiDeployment_ServerProductAndCooldownOrder', 'AiDeployment_RegularAllocationFailureStillPlaces',
         'AiDeployment_MissingProductTypeBranches', 'AiDeployment_MissingProductAssertionRechecksType',
         'AiDeployment_ClientPredictionUsesSecondContainedForm', 'AiDeployment_CostTruncationAndDemoCurrentWallet',
         'AiDeployment_SelectedMissingEffectsPreservePrefix', 'AiDeployment_CurrentInputsAfterChildEffects',
         'CampaignSession_AiDeploymentUsesActualProductionAndConstruction')  # 새9개 작성/컴파일 이름이다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 반환한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """정밀 범위/실제 PE 명령과 최종 컴파일/링크 자료만 정적으로 확인한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('원본 x86 PE가 아닙니다')
    mapped = pe.get_memory_mapped_image()
    base = pe.OPTIONAL_HEADER.ImageBase
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이웃 함수와 합치지 않고 해당 함수의 실제 정밀 범위만 보존한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 별도 해시/명령으로 남긴다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x4106e0': ('call 0x4103c0', 'push 0x108', 'call 0x4afa70', 'call 0x496e80',
                             'call 0x4102b0', '[0x544d10], 1', 'call 0x444760', 'call 0x4433b0',
                             '[0x544d10], 0', 'fadd qword ptr [0x5012d8]', 'ret 0x14'),
                '0x410280': ('call 0x4e49c0', '[esi + 0x2d8]'),
                '0x4102b0': ('fld dword ptr [eax + 0xc4]', 'fchs', 'call 0x4e49c0', 'call 0x463b50'),
                '0x415920': ('call 0x49b510', 'call 0x4108f0'), '0x4108f0': ('call 0x4106e0',)}  # 실제 폭/순서/하위 연결의 필수 명령이다.
    # 누락된 명령을 추정 성공으로 채우지 않는다.
    for entry, items in expected.items():
        # 각 원본 계약을 실제 해독 문자열에 대조한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<d', mapped, 0x5012d8-base)[0] != 20.0:
        raise ValueError('원본 배치 뒤 시설 기한 불일치')
    flow = listings['0x4106e0']
    if not flow.index('call 0x496e80') < flow.index('call 0x4102b0') < flow.index('[0x544d10], 1'):
        raise ValueError('원본 생산 대기/비용/이동 플래그 순서 불일치')
    build_path = 'extracted/cpp-ai-deployment-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-deployment-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 프로세스를 시작하지 않고 링크된 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일 검사 이름 누락: ' + name)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['00415a10 수집자 선택·00415920 배치 후보·004108f0 통지/음향',
                           '다른 실제 행동·전체 자동 프레임/종료', '15개 미션+Edit 동일성/자연 플레이'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 정밀도/예외·CD10.72 미검증',
                             '잘못된 물리 SID/TYPE와 해제 본체는 호스트 오류'])


def main():
    """이 단위의 새 정적 JSON만 UTF-8로 기록하고 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
