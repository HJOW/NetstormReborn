#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AI 초기 월드 등록/준비의 정적 PE 근거와 컴파일 입력을 보존한다. 게임·C++검사·원본 명령을 실행하지 않는다."""
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

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소와 보호 원본을 찾는 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-preparation-static-20261021.json'  # 과거 지갑/통지 실행 기록과 구분한 최초 정적 보고서다.
SOURCES = ('cpppj/src/o/RawAiWorldRegistration.h', 'cpppj/src/o/RawAiWorldRegistration.cpp',
           'cpppj/src/o/RawAiPreparation.h', 'cpppj/src/o/RawAiPreparation.cpp',
           'cpppj/src/o/RawGeyserAiRegistration.h', 'cpppj/src/o/RawGeyserAiRegistration.cpp',
           'cpppj/src/o/RawAiObjectNotice.h', 'cpppj/src/o/RawAiObjectNotice.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/client/RawCampaignAssetBindings.h', 'cpppj/src/client/RawCampaignAssetBindings.cpp',
           'cpppj/src/client/CampaignClientServices.cpp', 'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiWorldRegistrationTests.cpp', 'cpppj/tests/AiPreparationTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp')  # 이번 변경과 직접 재사용한 통지 본체의 실제 작업 트리 입력이다.
TESTS = ('AiWorldRegistration_FullScanDefersGuardsAndSkipsSidOne', 'AiWorldRegistration_DelayedCapacityAndExceptionLifetime',
         'AiWorldRegistration_AllyDistanceKeepsNativeFloatBoundary', 'AiWorldRegistration_GeyserSurfaceAndRemovalContracts',
         'AiWorldRegistration_GeyserWrapperRereadsNextPlayer', 'AiPreparation_ForcedAndDirectionalEnemySelection',
         'AiPreparation_CombinedPreparationUsesCurrentWorldAndOneScan', 'AiPreparation_VulnerableEnemySelectionKeepsNativeOrder',
         'AiPreparation_LocalSpRechecksOwnerAndPreservesIntegerPrecision', 'AiPreparation_GeyserCountUsesNativeMismatchedFields',
         'AiPreparation_SelectedMissingHooksAndPhysicalContracts', 'CampaignSession_LocalGeysersUseActualAiRegistration',
         'CampaignSession_LocalAiPreparationSharesOriginalWorld')  # 새12개와 기존1개 개편이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트를 SHA256으로 식별한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """기존 정밀 범위/실제 PE 상수와 최종 컴파일 자료를 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'));base = pe.OPTIONAL_HEADER.ImageBase;mapped = pe.get_memory_mapped_image()
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호된 10.78 x86 PE가 아닙니다')
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32);functions = {}
    # 초기 등록/간헐천 하위와 현재 준비/참조 함수의 정밀 범위만 수집한다.
    for entry in (0x415720, 0x4153c0, 0x4152b0, 0x4112b0, 0x411000, 0x416720, 0x40eb40,
                  0x416eb0, 0x410300, 0x411a80, 0x445330, 0x4641d0, 0x432210, 0x40d900, 0x41cfc0):
        blocks = []
        # 불연속 구간과 이웃 함수를 합치지 않고 각 범위 원문 해시를 남긴다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'));code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x415720': ('push 0x4b0', 'call 0x40eb40', 'cmp ebx, 0x12c', 'call 0x414de0', '[edi + 0x3c8], 3'),
                '0x4153c0': ('call 0x4152b0', 'call 0x411000', 'call 0x40ea00'),
                '0x4152b0': ('fstp dword ptr [esp + 0x28]', 'fcomp dword ptr [esp + 0x28]', '[ecx*4 + 0x595200]'),
                '0x416720': ('[0x59534c]', 'call 0x4153c0'),
                '0x416eb0': ('call 0x410300', 'fadd qword ptr [0x55b4d0]', 'fild dword ptr [esi + 0x2d8]',
                             'call 0x432210', 'call 0x40d900', 'call 0x40eec0', 'call 0x415720', 'call 0x411a80'),
                '0x411a80': ('[esi + 0x338]', '[esi + 0x324]', 'call 0x462bc0'),
                '0x445330': ('[0x5325c8]',)}  # 실제 명령의 선택/현재 필드만 정적으로 대조하는 입력이다.
    # 각 함수의 실제 명령에서 원본 폭/호출/현재 포인터를 확인한다.
    for entry, items in expected.items():
        # 누락된 명령을 추정 성공으로 채우지 않는다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 정적 명령 누락: ' + entry + ' ' + item)
    if listings['0x4153c0'].count('call 0x40ea00') != 5 or 'call 0x415720' not in listings['0x416eb0']:
        raise ValueError('원본 다섯 제거/최초 등록의 실제 호출 수 불일치')
    if not listings['0x416eb0'].index('call 0x40eec0') < listings['0x416eb0'].index('call 0x415720') < listings['0x416eb0'].index('call 0x411a80'):
        raise ValueError('원본 SP→초기 등록→수량 판정 순서 불일치')
    half = struct.unpack_from('<f', mapped, 0x500ee4-base)[0]
    if half != 0.5:
        raise ValueError('원본 청크 중심 배율 불일치')
    build_path = 'extracted/cpp-ai-preparation-20261021-build2.log';result_path = 'extracted/cpp-ai-preparation-20261021-build2-result.json'
    result = json.loads((ROOT / result_path).read_text(encoding='utf-8'));log = (ROOT / build_path).read_text(encoding='utf-8')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('수정 빌드 종료/경고/오류를 확인해야 합니다')
    failed_path = 'extracted/cpp-ai-preparation-20261021-build1.log';failed_result = 'extracted/cpp-ai-preparation-20261021-build1-result.json'
    if json.loads((ROOT / failed_result).read_text(encoding='utf-8'))['exit_code'] != 1:
        raise ValueError('최초 실패 기록의 실제 종료 코드를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe';binary = (ROOT / binary_path).read_bytes()
    # 검사 프로세스를 시작하지 않고 링크된13개 이름의 존재만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일 등록 누락: ' + name)
    return dict(date='2026-10-21', host=socket.gethostname(), last_decompile_host='VM-W11-CODEX', last_decompile_date='2026-10-21',
                source_hash_basis='이번 관련 변경과 직접 재사용 통지 본체의 최종 작업 트리 바이트. Git 정규화 뒤 바이트와 구별한다.',
                source_sha256={path: digest(path) for path in SOURCES}, native_sha256=digest('originals/Netstorm.exe'),
                refined_sha256=digest('extracted/refined/originals/functions.tsv'), functions=functions,
                constants={'0x500ee4': dict(format='float', value=half)},
                build=dict(path=build_path, sha256=digest(build_path), exit_code=0, warnings=0, errors=0),
                failed_build=dict(path=failed_path, sha256=digest(failed_path), exit_code=1),
                binary_sha256={path: digest(path) for path in ('cpppj/build/bin/Release/Netstorm.exe', binary_path)},
                compiled_related_test_names=list(TESTS), new_tests_written=12, existing_tests_reworked=1,
                executed_cpp_tests=0, executed_game_frames=0, executed_native_instructions=0,
                limits=['같은 본체의 초기 자산 등록/준비이며14af0/139f0/17c70/전체 전략 프레임은 여전히 미복원이다.',
                        '우리 편/데모 자동 건설·스테이지 클리어 또는 테스트용 AI/SP/속도 옵션을 켜지 않았다.',
                        '합성 검사 입력은 native 실행 관측 fixture가 아니며13개 등록을 실행 통과 수로 세지 않는다.',
                        '실제 물리 저장소 밖/비유한 CRT 좌표는 호스트 예외이며 전체 x87/CRT/지역화 로그 동일성 주장이 아니다.'])


def main():
    """최초 기록을 덮어쓰지 않고 생성하거나 같은 입력을 읽기 전용으로 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture', action='store_true');args = parser.parse_args();current = collect()
    if args.capture:
        if REPORT.exists():
            raise FileExistsError('최초 정적 기록은 덮어쓰지 않습니다')
        REPORT.write_text(json.dumps(current, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    elif json.loads(REPORT.read_text(encoding='utf-8')) != current:
        raise ValueError('최초 정적 근거/최종 빌드 입력과 다릅니다')
    print('AI 초기 등록/준비 정적15함수·빌드0/경고0·관련13검사 등록 확인. C++검사/게임/native 실행0.')


if __name__ == '__main__':
    main()
