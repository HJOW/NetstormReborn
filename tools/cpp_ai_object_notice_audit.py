#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AI 생성·삭제 통지의 정적 원본 근거와 컴파일 입력을 보존한다. C++검사·게임·native 명령을 실행하지 않는다."""
import argparse
import csv
import hashlib
import json
import re
import socket
import struct
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소의 읽기/기록 기준이다.
REPORT = ROOT / 'cpppj/ai-object-notice-static-20261021.json'  # 과거 실행 fixture와 구별한 최초 정적 근거다.
SOURCES = ('cpppj/src/o/RawAiObjectNotice.h', 'cpppj/src/o/RawAiObjectNotice.cpp',
           'cpppj/src/o/SquidPostPop.h', 'cpppj/src/o/SquidPostPop.cpp',
           'cpppj/src/o/SquidDestroyLifecycle.h', 'cpppj/src/o/SquidDestroyLifecycle.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/tests/AiObjectNoticeTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/tests/CMakeLists.txt')  # 실제 최종 작업 트리 빌드 입력이며 Git 정규화 뒤 바이트와 구별한다.
TESTS = ('AiObjectNotice_CurrentPlayerAndPhysicalContracts', 'AiObjectNotice_CombinedCreationKeepsDistinctListRules',
         'AiObjectNotice_CombinedDeletionReschedulesAndKeepsInactiveTail', 'AiObjectNotice_PriestAndCurrentOutpostBranches',
         'CampaignSession_ObjectNoticesUseActualPopAndDeletion')  # 실행 통과 수가 아닌 새 컴파일 등록 문자열이다.


def digest(path):
    """현재 파일 전체 바이트를 SHA256으로 식별한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """보호 PE를 읽기만 하여 실제 함수 범위/상수와 빌드 결과를 수집한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'));base = pe.OPTIONAL_HEADER.ImageBase;mapped = pe.get_memory_mapped_image()
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호된 10.78 x86 PE가 아닙니다')
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32);functions = {}
    # 두 caller와 wrapper/본문 및 실제 하위의 정밀 범위만 기록한다.
    for entry in (0x4166e0, 0x414de0, 0x410f80, 0x410620, 0x4105b0, 0x40ea00, 0x414450, 0x426150, 0x411120, 0x4ac200, 0x4b0d30, 0x4b0950):
        blocks = []
        # 불연속 범위는 이웃 함수와 합치지 않고 각각 해시한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'));code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(part for block in blocks for part in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x4166e0': ('[0x594fc8]', 'jl 0x41671b', '[0x59534c]', '[eax + 0xb0]', 'call 0x414de0'),
                '0x414de0': ('0x6d4', 'call 0x426150', 'call 0x410f80', 'call 0x411120', 'call 0x414450', 'call 0x410620'),
                '0x410620': ('fadd qword ptr [0x55b4d0]', 'fadd dword ptr [0x5012d0]', '[esi + 0x5a8]', '[esi + 0x5b0]', 'call 0x4105b0'),
                '0x4b0d30': ('call 0x4166e0',), '0x4b0950': ('call 0x4166e0',)}  # 원본 명령의 선택을 정적으로 확인하는 입력이며 실행 결과가 아니다.
    # 기록 대상의 실제 명령에서 호출 대상과 현재 포인터를 확인한다.
    for entry, items in expected.items():
        # 각 부분 문자열이 없으면 추측으로 성공을 채우지 않는다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 정적 명령 누락: ' + entry + ' ' + item)
    if listings['0x410620'].count('fadd qword ptr [0x55b4d0]') != 2 or 'fadd dword ptr [0x55b4d0]' in listings['0x410620']:
        raise ValueError('Built QWORD 시각의 실제 명령과 다릅니다')
    # 원본 Pop의 목록→AI→통계와 삭제의 공급→AI→작업장 호출 순서를 확인한다.
    for entry, chain in (('0x4b0d30', ('call 0x44f770', 'call 0x4166e0', 'call 0x4c25b0')),
                         ('0x4b0950', ('call 0x472ff0', 'call 0x4166e0', 'call 0x44f7a0'))):
        positions = [listings[entry].index(part) for part in chain]
        if positions != sorted(positions):
            raise ValueError('원본 통지의 호출 순서 불일치')
    constants = {}
    # 거리/기한/증분/0/제외 타입/로그 그룹은 실제 보호 PE 바이트에서 읽는다.
    for address, fmt, value in ((0x501700, 'd', 180.5), (0x501708, 'd', 30.0), (0x5012d0, 'f', 1.0),
                                (0x500460, 'd', 0.0), (0x541250, 'I', 126), (0x52f3d4, 'I', 29), (0x52f3e4, 'I', 29)):
        actual = struct.unpack_from('<' + fmt, mapped, address-base)[0]
        if actual != value:
            raise ValueError('원본 상수 불일치: ' + hex(address))
        constants[hex(address)] = dict(format=fmt, value=actual)
    build_path = 'extracted/cpp-ai-object-notice-20261021-build1.log';result_path = 'extracted/cpp-ai-object-notice-20261021-build1-result.json'
    result = json.loads((ROOT / result_path).read_text(encoding='utf-8'));log = (ROOT / build_path).read_text(encoding='utf-8')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe';binary = (ROOT / binary_path).read_bytes()
    # 프로세스를 시작하지 않고 링크된 새 검사 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일 등록 누락: ' + name)
    return dict(date='2026-10-21', host=socket.gethostname(), last_decompile_host='VM-W11-CODEX', last_decompile_date='2026-10-21',
                source_hash_basis='최종 빌드 입력의 작업 트리 바이트이며 Git 정규화 뒤 바이트와 구별한다.',
                source_sha256={path: digest(path) for path in SOURCES}, native_sha256=digest('originals/Netstorm.exe'),
                refined_sha256=digest('extracted/refined/originals/functions.tsv'), functions=functions, constants=constants,
                build=dict(path=build_path, sha256=digest(build_path), exit_code=0, warnings=0, errors=0),
                binary_sha256={path: digest(path) for path in ('cpppj/build/bin/Release/Netstorm.exe', binary_path)},
                compiled_test_names=list(TESTS), executed_cpp_tests=0, executed_game_frames=0, executed_native_instructions=0,
                limits=['현재 생성/삭제 사건의 같은 본체 갱신이며 초기 전체 월드 등록00415720/전략00416eb0은 여전히 미복원이다.',
                        '테스트용 AI/SP 옵션·상황 판단기를 구현하거나 활성화하지 않았다.',
                        '실제 Player9/0xb08/목록 backing 밖 접근과 해제 토큰은 호스트 예외로 거부한다.',
                        '형식화 로그는 기존 Session 장치 경계이며 전체 CRT/원본 지역화 출력의 동일성을 주장하지 않는다.',
                        '새 다섯 검사는 작성/컴파일만 했으며 통과 또는 전체 미션 동일성 증거가 아니다.'])


def main():
    """최초 정적 기록을 덮어쓰지 않고 생성하거나 같은 입력을 읽기 전용으로 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture', action='store_true');args = parser.parse_args();current = collect()
    if args.capture:
        if REPORT.exists():
            raise FileExistsError('최초 정적 기록은 덮어쓰지 않습니다')
        REPORT.write_text(json.dumps(current, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    elif json.loads(REPORT.read_text(encoding='utf-8')) != current:
        raise ValueError('최초 정적 근거/빌드 입력과 다릅니다')
    print('AI 생성·삭제 정적 명령/상수/연결·빌드0/경고0·새5검사 등록 확인. C++검사/게임/native 실행0.')


if __name__ == '__main__':
    main()
