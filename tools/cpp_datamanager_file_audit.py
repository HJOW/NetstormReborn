#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""파일 읽기/서명의 정적 PE 근거와 최종 빌드를 식별한다. 게임·검사·원본 명령을 실행하지 않는다."""
import argparse
import csv
import hashlib
import json
import re
import socket
from pathlib import Path
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 읽기 전용 원본과 현재 cpppj 소스가 있는 저장소다.
REPORT = ROOT / 'cpppj/datamanager-file-static-20261021.json'  # 이번 정적 근거/컴파일의 기록이며 과거 실행 보고서를 덮지 않는다.
SOURCES = ('cpppj/src/o/RawDataManagerImage.h', 'cpppj/src/o/RawDataManagerImage.cpp',
           'cpppj/src/client/CampaignTemplateStorage.h', 'cpppj/src/client/CampaignTemplateStorage.cpp',
           'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/tests/DataManagerReadTests.cpp')  # 실제 최종 빌드 입력 바이트다.
TESTS = ('DataManagerFile_SignatureBytesAndSignedTypeLimit', 'DataManagerFile_SignatureFailureAndCurrentRereads',
         'DataManagerFile_ReadParentBranchesAndCleanupCurrent', 'DataManagerFile_BodyAndReleaseFailureOnlyOnce',
         'CampaignTemplates_FileHeaderAndRejectedImageOwnership')  # 등록 문자열만 확인한다. 실행/통과 수가 아니다.


def digest(path):
    """동일 자료의 전체 파일 바이트를 SHA256으로 식별한다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """원본 PE의 명령/가상 표/import와 현재 빌드 자료를 읽기만 한다."""
    native = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    mapped = native.get_memory_mapped_image();base = native.OPTIONAL_HEADER.ImageBase
    if native.FILE_HEADER.Machine != 0x14c:
        raise ValueError('10.78 x86 PE가 아닙니다')
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32);functions = {}
    # 기존 정밀 범위에 속한 두 실제 본문의 바이트/명령만 정적으로 기록한다.
    for entry in (0x44d980, 0x4bce10):
        blocks = []
        # 불연속 범위도 각각 보존하고 범위 밖의 다른 함수를 합치지 않는다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'));code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    slots = [int.from_bytes(mapped[0x514690-base+offset:0x514694-base+offset], 'little') for offset in (0, 4, 8, 12)]
    if slots != [0x4bcca0, 0x4bccb0, 0x4bcd80, 0x4bce10]:
        raise ValueError('Template의 실제 네 가상 슬롯이 달라졌습니다')
    imports = {item.address: item.name for group in native.DIRECTORY_ENTRY_IMPORT for item in group.imports}
    if imports.get(0x5000b0) != b'DeleteFileA':
        raise ValueError('원본 거부 경계의 파일 제거 import가 달라졌습니다')
    signature = '\n'.join(part for block in functions['0x4bce10'] for part in block['instructions'])
    parent = '\n'.join(part for block in functions['0x44d980'] for part in block['instructions'])
    # signed 진단/FE/F/두 manager 필드와 실제 하위 호출을 명령에서 확인한다. 원본 실행 검사가 아니다.
    for text in ('jl 0x4bce47', 'cmp cl, 0xfe', '[edi + 0x10c]', '[edi + 0x110]', '0x46'):
        if text not in signature:
            raise ValueError('서명 원본 명령 누락: ' + text)
    for text in ('call 0x41baa0', 'call dword ptr [edx + 0xc]', 'call 0x44d380', 'call 0x4e4354'):
        if text not in parent:
            raise ValueError('파일 읽기 부모 원본 명령 누락: ' + text)
    build_path = 'extracted/cpp-datamanager-file-20261021-build1.log'
    build = json.loads((ROOT / 'extracted/cpp-datamanager-file-20261021-build1-result.json').read_text(encoding='utf-8'))
    log = (ROOT / build_path).read_text(encoding='utf-8')
    if build['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe';binary = (ROOT / binary_path).read_bytes()
    # 바이너리는 실행하지 않고 새 검사 이름의 등록만 읽는다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('컴파일된 검사 등록 누락: ' + name)
    return dict(date='2026-10-21', host=socket.gethostname(), last_decompile_host='VM-W11-CODEX', last_decompile_date='2026-10-21',
                scope='기존 정밀 원문과 PE 명령/가상 표/import의 정적 대조. C++검사·게임·원본 에뮬레이션 실행 없음.',
                source_hash_basis='최종 빌드 입력의 실제 작업 트리 바이트이며 Git 줄바꿈 정규화 뒤 바이트와 구분한다.',
                source_sha256={path: digest(path) for path in SOURCES},
                native_sha256=digest('originals/Netstorm.exe'), refined_sha256=digest('extracted/refined/originals/functions.tsv'),
                virtual_slots=list(map(hex, slots)), functions=functions,
                rejection_import=dict(address='0x5000b0', name='DeleteFileA', thunk='0x4e58ba', wrapper='0x41aef0'),
                build=dict(path=build_path, sha256=digest(build_path), exit_code=0, warnings=0, errors=0),
                binary_sha256={path: digest(path) for path in ('cpppj/build/bin/Release/NetstormCpp.exe', binary_path)},
                compiled_test_names=list(TESTS), executed_cpp_tests=0, executed_game_frames=0, executed_native_instructions=0,
                limits=['파일 읽기는 기존 VFS 캡처 원문의 호스트 경계이며 CRT/VFS/OS 전체 복원이 아니다.',
                        '잘못된 서명은 실제 로더에서 보호 원문 삭제 대신 예외로 거부한다.',
                        '원본 ReadFile의 최소4BYTE 할당/미정 tail과 OS 오류 전체는 복원하지 않았다.',
                        'manager 생성자의 별도 전역 초기화, 저장/전체 이름 표 생성, 전체 Template Load/State7은 후속이다.'])


def main():
    """최초 정적 기록 작성 또는 같은 기록/현재 파일의 읽기 전용 감사를 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture', action='store_true');args = parser.parse_args()
    current = collect()
    if args.capture:
        if REPORT.exists():
            raise FileExistsError('최초 정적 기록은 덮어쓰지 않습니다')
        REPORT.write_text(json.dumps(current, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    elif json.loads(REPORT.read_text(encoding='utf-8')) != current:
        raise ValueError('정적 근거/빌드 입력이 최초 기록과 다릅니다')
    print('파일 부모/서명 정적 근거·빌드0/경고0·새5검사 등록 확인. 게임/C++검사/native 실행0.')


if __name__ == '__main__':
    main()
