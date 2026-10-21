#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Template 생성자/Load의 PE 정적 근거와 컴파일 입력을 기록한다. 검사/게임/원본 명령을 실행하지 않는다."""
import argparse
import csv
import hashlib
import json
import re
import socket
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소의 읽기 전용 근거와 컴파일 산출물이 있는 위치다.
REPORT = ROOT / 'cpppj/template-load-static-20261021.json'  # 기존 파일 읽기/실행 기록과 구분한 최초 정적 기록이다.
SOURCES = ('cpppj/src/o/RawTemplateLoad.h', 'cpppj/src/o/RawTemplateLoad.cpp', 'cpppj/src/o/CMakeLists.txt',
           'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/CampaignTemplateStorage.cpp',
           'cpppj/tests/TemplateLoadTests.cpp', 'cpppj/tests/CMakeLists.txt')  # 이 단위의 실제 작업 트리 입력 바이트다.
TESTS = ('TemplateLoad_ConstructorAndBorrowedFileName', 'TemplateLoad_ExistingFileKeepsAllHeaderFields',
         'TemplateLoad_NewReadOnlyAndCurrentTitleBranches', 'TemplateLoad_FailureRereadsPointerAndPreservesOtherFields',
         'TemplateLoad_NullAndUnselectedEffectBoundaries')  # 실행 결과가 아닌 컴파일 등록 문자열이다.


def digest(path):
    """전체 파일 바이트의 SHA256을 읽기만 한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """현재 호스트의 기존 정밀 범위와 PE 명령, 빌드/소스/바이너리를 식별한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'));base = pe.OPTIONAL_HEADER.ImageBase
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호된 10.78 x86 PE가 아닙니다')
    mapped = pe.get_memory_mapped_image();decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    functions = {}
    # 기존 범위의 세 함수와 네 BYTE getter를 정적으로만 해독한다.
    for entry in (0x4bcc60, 0x4bcca0, 0x4bf550, 0x41af80):
        ranges = facts[entry]['ranges'] if entry != 0x4bcca0 else '4bcca0-4bcca3'
        blocks = []
        # 범위마다 실제 바이트와 명령을 보존하며 함수 밖의 padding을 합치지 않는다.
        for item in ranges.split(';'):
            first, last = (int(value, 16) for value in item.split('-'));code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    # 문서의 핵심 분기/쓰기/실제 하위 주소가 해당 정적 명령에 존재해야 한다.
    for entry, expected in {
        '0x4bcc60': ('[eax + 4], ebx', '[eax + 8], ebx', '[eax], 0x514690', '[eax + 0x10c], ebx', '[eax + 0x110], 0xffffffff', '[0x5c89c0], ebx', 'ret 4'),
        '0x4bcca0': ('lea eax, [ecx + 0xc]', 'ret'),
        '0x4bf550': ('call 0x4e65ed', 'call 0x4bcc60', 'call 0x41af80', 'call 0x44d980', 'call 0x44da60', 'call 0x4e4354', 'call 0x460730', '[0x540cac]', '0x5c89f0'),
        '0x41af80': ('cmp eax, 2', 'cmp eax, 4', 'call 0x4f8344', 'js 0x41b01e'),
    }.items():
        listing = '\n'.join(part for block in functions[entry] for part in block['instructions'])
        # 기대 문자열은 정적 명령의 존재만 확인하고 실행 횟수에 세지 않는다.
        for text in expected:
            if text not in listing:
                raise ValueError('원본 정적 명령 누락: ' + text)
    build_path = 'extracted/cpp-template-load-20261021-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-template-load-20261021-build1-result.json').read_text(encoding='utf-8'))
    log = (ROOT / build_path).read_text(encoding='utf-8')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe';binary = (ROOT / binary_path).read_bytes()
    # 실행 파일을 시작하지 않고 등록 이름만 찾는다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일 등록 누락: ' + name)
    return dict(date='2026-10-21', host=socket.gethostname(), last_decompile_host='VM-W11-CODEX', last_decompile_date='2026-10-21',
                source_hash_basis='최종 빌드 입력 작업 트리 바이트. Git 정규화 뒤 커밋 바이트와 구분한다.',
                source_sha256={path: digest(path) for path in SOURCES}, native_sha256=digest('originals/Netstorm.exe'),
                refined_sha256=digest('extracted/refined/originals/functions.tsv'), functions=functions,
                build=dict(path=build_path, sha256=digest(build_path), exit_code=0, warnings=0, errors=0),
                binary_sha256={path: digest(path) for path in ('cpppj/build/bin/Release/NetstormCpp.exe', binary_path)},
                compiled_test_names=list(TESTS), executed_cpp_tests=0, executed_game_frames=0, executed_native_instructions=0,
                limits=['바깥 Load는 원본 제어 몸체/명시 효과 경계 복원이며 실제 Session의 교체용 InstallFile에 연결하지 않았다.',
                        '실제 보관소는 생성자/manager 파일명 getter만 새 원본 몸체를 사용한다.',
                        'readonly 판정의 원본 분기는 정적으로 확인했으며 전체 VFS/CRT stat 복원은 후속이다.',
                        '별도005c89c0은 같은 Client 수명 저장소에 생성자가 쓰며 전체 State7의 다른 소비자는 후속이다.',
                        '새 검사는 작성/컴파일만 했으며 실행 통과나 전체 미션 동일성의 증거가 아니다.'])


def main():
    """최초 기록을 덮어쓰지 않고 생성하거나 같은 입력/산출물을 읽기 전용으로 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture', action='store_true');args = parser.parse_args();current = collect()
    if args.capture:
        if REPORT.exists():
            raise FileExistsError('최초 정적 기록은 덮어쓰지 않습니다')
        REPORT.write_text(json.dumps(current, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    elif json.loads(REPORT.read_text(encoding='utf-8')) != current:
        raise ValueError('최초 정적 근거/컴파일 입력과 다릅니다')
    print('Template 생성자/Load 정적 근거·빌드0/경고0·새5검사 등록. C++검사/게임/native 실행0.')


if __name__ == '__main__':
    main()
