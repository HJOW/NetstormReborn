#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 AI 지갑 연결의 정적 PE 근거/컴파일 입력을 기록한다. 게임·C++검사·원본 명령은 실행하지 않는다."""
import argparse
import csv
import hashlib
import json
import re
import socket
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소와 읽기 전용 원본을 찾는 기준이다.
REPORT = ROOT / 'cpppj/ai-wallet-static-20261021.json'  # 과거 실행/지갑 값형 fixture와 구분한 최초 정적 기록이다.
SOURCES = ('cpppj/src/o/RawAiWallet.h', 'cpppj/src/o/RawAiWallet.cpp', 'cpppj/src/o/SquidReward.h', 'cpppj/src/o/SquidReward.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/tests/AiWalletTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp', 'cpppj/tests/CMakeLists.txt')  # 실제 최종 빌드 입력의 작업 트리 바이트다.
TESTS = ('AiWallet_DisabledSignedOwnerAndNullBrain', 'AiWallet_WrapDemoFloorAndUnrelatedBytes',
         'AiWallet_DemoBoundaryRereadsSavedBrain', 'AiWallet_RejectsMissingPhysicalBodyAndReaders',
         'AiWallet_RewardUsesSameBrainAndKeepsLegacySeparate', 'CampaignSession_RefundCreditsCurrentActualAiWallet')  # 통과 결과가 아닌 컴파일 등록 문자열이다.


def digest(path):
    """전체 파일 바이트를 SHA256으로 식별한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """기존 정밀 범위의 실제 명령과 현재 빌드 자료를 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'));base = pe.OPTIONAL_HEADER.ImageBase;mapped = pe.get_memory_mapped_image()
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호된 10.78 x86 PE가 아닙니다')
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32);functions = {}
    # 원본 지갑/현재 데모와 로컬·다른 소유자 SP 부모의 실제 범위를 보존한다.
    for entry in (0x413bb0, 0x463b50, 0x40eec0, 0x41df80):
        blocks = []
        # 불연속 범위는 별도로 기록하고 이웃 함수를 합치지 않는다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'));code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(part for block in blocks for part in block['instructions']) for entry, blocks in functions.items()}
    # 현재 Player/저장 ESI/데모 뒤 signed 하한의 정적 명령을 확인한다. 실행 횟수에 세지 않는다.
    for text in ('[0x594fc8]', 'jl 0x413c11', '[0x59534c]', '[eax + 0xb0]', '[esi + 0x2d8]', 'call 0x463b50', 'cmp eax, 0x4a6', 'jg 0x413c0a'):
        if text not in listings['0x413bb0']:
            raise ValueError('원본 지갑 정적 명령 누락: ' + text)
    if '[0x5634a0]' not in listings['0x463b50'] or '[eax + 0x68]' not in listings['0x463b50']:
        raise ValueError('현재 미션 데모 getter 근거 누락')
    # SP 저장→정수 지갑 가산→화면 갱신의 원본 호출 순서만 확인한다.
    for entry in ('0x40eec0', '0x41df80'):
        listing = listings[entry]
        if not listing.index('call 0x40edb0') < listing.index('call 0x413bb0') < listing.index('call 0x43dad0'):
            raise ValueError('원본 SP 부모의 호출 순서 불일치')
    build_path = 'extracted/cpp-ai-wallet-20261021-build2.log';result_path = 'extracted/cpp-ai-wallet-20261021-build2-result.json'
    result = json.loads((ROOT / result_path).read_text(encoding='utf-8'));log = (ROOT / build_path).read_text(encoding='utf-8')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe';binary = (ROOT / binary_path).read_bytes()
    # 실행 파일을 시작하지 않고 새 여섯 검사 이름의 등록만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일 등록 누락: ' + name)
    return dict(date='2026-10-21', host=socket.gethostname(), last_decompile_host='VM-W11-CODEX', last_decompile_date='2026-10-21',
                source_hash_basis='최종 빌드 입력의 작업 트리 바이트. Git 정규화 뒤 바이트와 구분한다.',
                source_sha256={path: digest(path) for path in SOURCES}, native_sha256=digest('originals/Netstorm.exe'),
                refined_sha256=digest('extracted/refined/originals/functions.tsv'), functions=functions,
                build=dict(path=build_path, sha256=digest(build_path), exit_code=0, warnings=0, errors=0),
                binary_sha256={path: digest(path) for path in ('cpppj/build/bin/Release/NetstormCpp.exe', binary_path)},
                compiled_test_names=list(TESTS), executed_cpp_tests=0, executed_game_frames=0, executed_native_instructions=0,
                limits=['현재 지갑 가산 연결이며 전략 AI/SP 테스트 옵션을 구현하거나 활성화하지 않았다.',
                        '생성/삭제의 별도 aiAttached와 전략 목록 통지는 여전히 미복원이며 이번 연결로 켜지 않는다.',
                        '실제 Player9/0xb08 저장소 계약 밖 접근과 해제 토큰은 호스트 예외로 거부한다.',
                        '새 여섯 검사는 작성/컴파일만 했으며 실행 성공 또는 전체 미션 동일성의 증거가 아니다.'])


def main():
    """최초 정적 기록을 덮어쓰지 않고 생성하거나 현재 같은 자료를 읽기 전용으로 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture', action='store_true');args = parser.parse_args();current = collect()
    if args.capture:
        if REPORT.exists():
            raise FileExistsError('최초 정적 기록은 덮어쓰지 않습니다')
        REPORT.write_text(json.dumps(current, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    elif json.loads(REPORT.read_text(encoding='utf-8')) != current:
        raise ValueError('최초 정적 근거/빌드 입력과 다릅니다')
    print('AI 지갑 정적 명령/연결·빌드0/경고0·새6검사 등록 확인. C++검사/게임/native 실행0.')


if __name__ == '__main__':
    main()
