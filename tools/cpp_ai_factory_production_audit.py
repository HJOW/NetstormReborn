#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AI 시설 생산의 원본 명령·최종 컴파일 입력을 읽기만 해 기록한다. 검사/게임/원본 명령 실행은 없다."""
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
REPORT = ROOT / 'cpppj/ai-factory-production-static-20261022.json'  # 이전 수집자/배치의 정적 증거와 구분하는 새 기록이다.
ENTRIES = (0x413570, 0x4136b0, 0x4acc60, 0x40e9b0, 0x40ea00, 0x453f30,
           0x453f40, 0x4ac970, 0x410620, 0x453bf0, 0x453d70, 0x4549c0)  # 구현 근거9개와 미복원 업그레이드 경계3개다.
SOURCES = ('cpppj/src/o/RawAiFactoryProduction.h', 'cpppj/src/o/RawAiFactoryProduction.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiFactoryProductionTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp')  # 이번 실제 컴파일 입력이다.
TESTS = ('AiFactoryProduction_FirstMatchingKnownUnregisteredType', 'AiFactoryProduction_BatteryThemeReplacementModes',
         'AiFactoryProduction_FinderKeepsSavedNextAcrossProductEffects', 'AiFactoryProduction_AddOneProductKeepsFactoryWithRoom',
         'AiFactoryProduction_FullFactoryMovesToUpgradeQueue', 'AiFactoryProduction_NoCandidateAndDuplicateProductCounting',
         'AiFactoryProduction_UpgradeGatesAndRequestOrder', 'AiFactoryProduction_SelectedMissingAndPhysicalContracts',
         'CampaignSession_AiFactoryProductionUsesOriginalWorkshops')  # 새9개 작성/컴파일 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """실제 원본 정밀 범위/명령과 최종 빌드/링크 입력을 정적으로 대조한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 각 함수의 실제 범위만 기록하고 이웃 함수와 합치지 않는다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 코드도 독립 해시/명령으로 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x413570': ('call 0x4b2270', 'test byte ptr [eax + 0x14], 4', '[ebp + eax*4 + 0x3b4]',
                             'push 8', 'call 0x4ac9f0', 'call 0x4b1b20'),
                '0x4136b0': ('call 0x453f30', 'call 0x4acc60', 'call 0x413570', 'call 0x410420',
                             'call 0x40ea00', 'call 0x453f40', 'call 0x40e9b0', 'call 0x453bf0',
                             'cmp dword ptr [edi + 0x2d8], 0x4b0', 'call 0x410620'),
                '0x453bf0': ('call 0x496f00', 'call 0x4e2dc0'), '0x4549c0': ('call 0x4545e0',)}  # 구현한 제어와 남은 실제 업그레이드 경계를 구별하는 명령이다.
    # 누락된 명령을 성공으로 추정하지 않는다.
    for entry, items in expected.items():
        # 각 필수 계약을 실제 PE 해독 문자열에서 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<I', mapped, 0x52f3e0-base)[0] != 99:
        raise ValueError('원본 Factory 로그 초기값 불일치')
    build_path = 'extracted/cpp-ai-factory-production-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-factory-production-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 실행 없이 링크된 새 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    if 'factories.upgrade=SessionMissing<void,Sid>()' not in session:
        raise ValueError('후속 업그레이드 경계가 바뀌었으므로 정적 판정을 갱신해야 합니다')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                remaining=['00453bf0/00453d70 업그레이드 요청·수신', '004549c0 event3/004545e0 진행·완료/예측 그림',
                           '다른 AI 실제 행동·전체 자동 프레임/종료', '15개 미션+Edit 동일성/자연 플레이'],
                limitations=['생산/목록 몸체 연결이며 업그레이드의 실제 효과는 미복원 경계',
                             '작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 정밀도/예외·CD10.72 미검증'])


def main():
    """이번 새 정적 보고서만 UTF-8로 기록하고 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
