"""게임/native/C++ 검사를 실행하지 않고 가짜 부족좌표 시작점의 원본 근거와 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 보호 원문/현재 소스/빌드의 같은 저장소 기준이다.
REPORT = ROOT / 'cpppj/ai-dry-seed-static-20261022.json'  # 이전 보고서를 덮어쓰지 않는 이번 기록이다.
ENTRIES = (0x4133e0, 0x40f1c0, 0x4558c0, 0x415fe0, 0x415bb0, 0x41cfc0, 0x40e800, 0x4adea0)  # 이번1/기존하위2/후속분석5 함수다.
SOURCES = ('cpppj/src/o/RawAiDrySpots.h', 'cpppj/src/o/RawAiDrySpots.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/tests/AiDrySpotsTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/SpStore.cpp')  # 같은 실제 본체/목록/공용 난수와 컴파일 입력이다.
TESTS = ('AiDrySpots_SeedCapabilityUsesOnlyLowByteNoEffects', 'AiDrySpots_SeedEightDirectionsAndCurrentRawType',
         'AiDrySpots_SeedStoresAfterWholeSums', 'AiDrySpots_SeedFullPreservesPointAndConsumesRandom',
         'AiDrySpots_SeedDuplicateDoesNotReplaceType', 'AiDrySpots_SeedAllowsZeroTypeAndOutsideBounds',
         'AiDrySpots_SeedPhysicalFailureRetainsPointAndRandom',
         'CampaignSession_AiDrySeedUsesActualBrainListAndLifetime')  # 새8개 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """정밀 범위 명령/상수/8방향 분기표와 최종 빌드·연결을 정적으로 확인한다."""
    previous = json.loads((ROOT / 'cpppj/ai-drop-selection-static-20261022.json').read_text(encoding='utf8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 기존 정밀 범위의 시작에서 해독해 이웃 함수/명령 중간 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 전체 해시/명령을 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {
        '0x4133e0': ('test byte ptr [esi + 0x248], 0x20', 'push 8', 'fld dword ptr [esi + 0x2c0]',
                     'fadd dword ptr [esi + 0x2b8]', 'fadd dword ptr [esi + 0x278]',
                     'fadd dword ptr [esi + 0x270]', 'fmul dword ptr [0x501550]',
                     'fstp dword ptr [esp + 8]', 'fld dword ptr [esi + 0x2c4]',
                     'fadd dword ptr [esi + 0x2bc]', 'fadd dword ptr [esi + 0x27c]',
                     'fadd dword ptr [esi + 0x274]', 'fmul dword ptr [0x500ee4]',
                     'call 0x4558c0', 'cmp eax, 7', 'jmp dword ptr [eax*4 + 0x413548]',
                     '00413520 mov eax, dword ptr [esi + 0x280]',
                     '00413526 mov ecx, dword ptr [esi + 0xaf4]',
                     '0041352c mov edx, dword ptr [esi + 0xaf0]', 'call 0x40f1c0'),
        '0x415fe0': ('call 0x40f260', 'call 0x412640', 'call 0x4adea0',
                    '0041612f mov ecx, dword ptr [esp + 0x1c]',
                    '0041614e mov edx, dword ptr [esp + 0x1c]',
                    '00416153 push ecx', '00416154 push edx', 'call 0x415bb0',
                    'fmul dword ptr [0x501784]', 'call 0x4133e0'),
        '0x415bb0': ('test ah, 0x41', '00415d8a jp 0x415e4f',
                    'fadd dword ptr [0x500edc]', 'call 0x4e49c0', 'cmp esi, 0x100',
                    'mov edx, dword ptr [0x542514]', 'cmp eax, 4')
    }  # 구현1개와 후속 원문의 디컴파일 오독 방지 근거다.
    # 필수 명령이 현재 보호 PE에 없으면 성공 보고서를 기록하지 않는다.
    for entry, items in expected.items():
        # 해당 원본 정밀 범위 안에서 각 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    targets = list(struct.unpack_from('<8I', mapped, 0x413548-base))
    if targets != [0x41347d,0x413498,0x4134aa,0x4134c2,0x4134c8,0x4134e0,0x4134f2,0x41350a]:
        raise ValueError('원본 8방향 분기표 불일치')
    constants = {hex(address): dict(bits=mapped[address-base:address-base+4].hex(),
                                   value=struct.unpack_from('<f', mapped, address-base)[0])
                 for address in (0x501550,0x500ee4,0x500edc,0x501784)}
    if constants['0x501550']['value'] != 0.25 or constants['0x500ee4']['value'] != 0.5 or constants['0x501784']['bits'] != '3333333f':
        raise ValueError('원본 평균/후속 반경 상수 불일치')
    build_path = 'extracted/cpp-ai-dry-seed-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-dry-seed-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사를 실행하지 않고 실제 링크된 새 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    if 'impl_->aiDrySpots->Seed(token,impl_->random)' not in session:
        raise ValueError('같은 실제 Dry Spot/공용 난수 연결 누락')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                branch_targets=[hex(address) for address in targets], constants=constants,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path,'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0,ctest=0,game=0,original_native=0),
                restored=['004133e0 전체 경계 좌표 선택/같은 공용 난수8/현재 fake TYPE와 실제 Dry Spot 등록'],
                additional_read=dict(entries=['00415fe0','00415bb0','0041cfc0','0040e800','004adea0'], implemented_in_this_unit=False,
                                     facts=['415fe0의415bb0 시작 좌표는 중심Y,중심Y', '415bb0 경계는 양끝 포함/현재542514 지도/좌표+bias 후 ftol',
                                            '후속 거리 비교 상수501784는 float 0.7/순서 없는 비교도 폐기']),
                remaining=['00415fe0 전체 가짜좌표와00415bb0 수직 양방향 탐색·디버그 표식',
                           '004162e0 실제 배치행동/다리·원래 자동 AI 프레임·종료/15개 미션+Edit/custom Fort·시나리오 수명과 동일성'],
                limitations=['작성/컴파일/정적확인이며 실행 통과가 아님', 'x87 확장 정밀도/예외/NaN payload·CD10.72 미검증',
                             '잘못된 실제 본체/backing 물리 접근은 명시 오류', '명시 Seed API의 true는 본체 호출이며 등록 성공이 아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
