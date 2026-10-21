#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 AI 수집자 선택의 원본 PE/현재 컴파일 입력을 정적으로 기록한다. 검사/게임/원본 명령 실행은 없다."""
import csv
import hashlib
import json
import re
import socket
import struct
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 저장소와 보호 원본을 읽는 기준이다.
REPORT = ROOT / 'cpppj/ai-collectors-static-20261022.json'  # 이전 배치 단위의 정적 증거와 구분하는 신규 기록이다.
ENTRIES = (0x415a10, 0x415920, 0x4115e0, 0x411ca0, 0x40ea40, 0x492280,
           0x40eab0, 0x4106c0, 0x410580, 0x4108f0, 0x410620, 0x4105b0)  # 선택/후보/통지와 실제 상태 하위다.
SOURCES = ('cpppj/src/o/RawAiCollectors.h', 'cpppj/src/o/RawAiCollectors.cpp',
           'cpppj/src/o/RawAiDeployment.h', 'cpppj/src/o/RawAiDeployment.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiCollectorsTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp')  # 최종 실제 컴파일 입력이다.
TESTS = ('AiCollectors_PrerequisitesHaveNoRandomEffects', 'AiCollectors_NormalProductionAndRandomOrder',
         'AiCollectors_ReadyPendingMoneyAndSatisfiedGates', 'AiCollectors_MissingRandomProductFallsBackToFirst',
         'AiCollectors_VulnerableEnemyPrefersFlyingWithoutExtraGates', 'AiCollectors_AttemptLimitAndCurrentBoundsSnapshot',
         'AiCollectors_DeploymentNoticeAndLocalSoundUseCurrentValues', 'AiCollectors_PhysicalAndSelectedMissingContracts',
         'CampaignSession_AiCollectorSelectionUsesActualDeployment')  # 새9개 작성/컴파일 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트를 SHA256으로 식별한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """실제 원본 정밀 범위/명령·최종 빌드/링크 이름을 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 해당 함수의 실제 정밀 범위만 보존하며 이웃 함수로 넓히지 않는다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 코드도 별도 해시/명령으로 기록한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x415a10': ('push 0x3e8', 'cmp eax, 0x64', 'call 0x492280', 'call 0x411ca0', 'call 0x40eab0',
                             'call 0x4115e0', 'call 0x410580', 'call 0x410280', 'call 0x415920'),
                '0x415920': ('[esp + 0x10], 0x14', 'call 0x41d3f0', 'call 0x41d3e0', 'call 0x49b510', 'call 0x4108f0'),
                '0x410580': ('call 0x4103c0', '[ecx + edx + 0x15], 1'),
                '0x411ca0': ('[ecx + 0x1a]', 'call 0x4adcb0', 'sar eax, 1'),
                '0x4108f0': ('call 0x4106e0', 'call 0x410620', 'call 0x4a9cb0'),
                '0x410620': ('fadd qword ptr [0x55b4d0]', 'fadd dword ptr [0x5012d0]', '[esi + 0x5b0]', 'call 0x4105b0'),
                '0x492280': ('test dl, 0x22', '[esi*4 + 0x595428]', 'sete al')}  # 실제 명령의 선택/폭/호출 순서를 확인하는 입력이다.
    # 누락된 명령은 성공으로 추정하지 않는다.
    for entry, items in expected.items():
        # 각 필수 계약을 실제 PE 해독 문자열에서 찾는다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if listings['0x415920'].count('call 0x4558c0') != 2 or not listings['0x415920'].index('call 0x41d3f0') < listings['0x415920'].index('call 0x41d3e0'):
        raise ValueError('원본 y/x 난수 횟수/순서 불일치')
    if struct.unpack_from('<f', mapped, 0x5012d0-base)[0] != 1.0 or struct.unpack_from('<I', mapped, 0x52f3d0-base)[0] != 99:
        raise ValueError('원본 생각 간격/기본 Lay 로그 종류 불일치')
    build_path = 'extracted/cpp-ai-collectors-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-collectors-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 실행 없이 최종 링크된 새 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['시설/기술·고갈·사제·건물·다리·수집/포획 실제 행동', '전체 자동 프레임/종료',
                           '15개 미션+Edit 동일성/자연 플레이'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 정밀도/예외·CD10.72 미검증',
                             '현재 Debug Lay Piece 초기값99만 연결하며 해당 디버그 메뉴는 후속'])


def main():
    """이번 새 정적 보고서만 UTF-8로 기록하고 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
