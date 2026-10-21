"""원본/native/게임/C++ 검사 실행 없이 이번 AI DrySpot 근거와 빌드 기록을 확인한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 저장소/보호 PE/이번 빌드의 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-dry-spots-static-20261022.json'  # 과거 보고서와 구분하는 이번 신규 기록이다.
ENTRIES = (0x40f1c0, 0x40f260, 0x40f2b0, 0x417c70, 0x4105b0,
           0x473010, 0x4734d0, 0x4730c0, 0x473330)  # 복원한 세 목록 몸체/호출자 하위/기존 실제 에너지9함수다.
SOURCES = ('cpppj/src/o/RawAiDrySpots.h', 'cpppj/src/o/RawAiDrySpots.cpp', 'cpppj/src/o/RawAiBrain.cpp',
           'cpppj/src/o/RawEnergySupply.h', 'cpppj/src/o/RawEnergySupply.cpp', 'cpppj/src/o/CMakeLists.txt',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/tests/CMakeLists.txt', 'cpppj/tests/AiDrySpotsTests.cpp', 'cpppj/tests/EnergySupplyTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp')  # 같은 본체/에너지와 이번 실제 컴파일 입력이다.
TESTS = ('AiDrySpots_AddCoordinatesAndRawTypeBits', 'AiDrySpots_FullAndEmptyAvoidBackingAccess',
         'AiDrySpots_ScanOneAndPreservesOriginalCursor', 'AiDrySpots_RemovePreservesTailAndUncheckedEndCount',
         'AiDrySpots_EnergyMutationUsesCurrentBackingAndCursor', 'AiDrySpots_MaintainUsesCurrentFakeAndDebug',
         'AiDrySpots_SelectedMissingAndPhysicalBounds', 'AiDrySpots_ActualEnergySupplyChangesRemoveCoordinate',
         'CampaignSession_AiDrySpotsUseActualBrainEnergyAndLifetime')  # 새9개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """보호 PE 정밀 범위/폭·호출 순서와 최종 빌드/실제 Session 연결을 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    previous = json.loads((ROOT / 'cpppj/factory-input-static-20261022.json').read_text(encoding='utf-8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이번 실제 함수의 불연속 정밀 범위를 각각 해시/명령으로 보존한다.
    for entry in ENTRIES:
        blocks = []
        # 이웃 함수로 범위를 넓히지 않는다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x40f1c0': ('cmp edi, eax', 'jge 0x40f255', 'fucompp', 'test ah, 0x44', 'mov dword ptr [eax + 8], edx'),
                '0x40f260': ('mov dword ptr [eax + 8], edi', 'dec dword ptr [ecx + 4]'),
                '0x40f2b0': ('idiv ecx', 'mov dword ptr [esi + 0xc], edx', 'call 0x473010',
                            'call 0x4734d0', 'mov edi, dword ptr [ecx + edx*4 + 8]', 'call 0x40f260'),
                '0x417c70': ('call 0x40f2b0', 'call 0x4105b0'), '0x4734d0': ('call 0x4730c0', 'call 0x473330')}  # 실제 등록/복사 폭·순환/에너지 하위다.
    # 원본 명령이 없으면 성공으로 기록하지 않는다.
    for entry, items in expected.items():
        # 같은 함수 정밀 범위 안에서 모든 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    listing = listings['0x40f2b0']
    if not listing.index('mov dword ptr [esi + 0xc], edx') < listing.index('call 0x473010') < listing.index('call 0x4734d0') < listing.index('mov edi, dword ptr [ecx + edx*4 + 8]') < listing.index('call 0x40f260'):
        raise ValueError('원본 cursor/초기화/에너지/현재TYPE/제거 순서 불일치')
    if struct.unpack_from('<I', mapped, 0x52f3dc-base)[0] != 99:
        raise ValueError('원본 Debug Dry Spot 초기값 불일치')
    build_path = 'extracted/cpp-ai-dry-spots-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-dry-spots-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 프로세스는 실행하지 않고 새 이름의 실제 링크만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    brain = (ROOT / 'cpppj/src/o/RawAiBrain.cpp').read_text(encoding='utf-8-sig')
    if 'AiDrySpotHooks dry;' not in session or 'energySupply->Evaluate(text,x,y,type,owner)' not in session or 'aiDrySpots->Maintain(token)' not in session or 'aiDrySpots->Add(token,x,y,type)' not in session:
        raise ValueError('같은 실제 본체/에너지와 명시 등록·재검사 연결 누락')
    if 'Put(raw,0xae4,allocate(120))' not in brain:
        raise ValueError('기존 생성자의 실제120바이트 backing 계약 불일치')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions, sources={name: digest(name) for name in SOURCES},
                build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['0040f1c0 등록/0040f260 압축/0040f2b0 한항목 에너지 재검사', '00417c70의해당하위로그/같은Session본체·실제에너지'],
                remaining=['00415fe0 등 가짜 좌표 선정·다른 AI 실제 행동·원래 순서 자동 프레임/종료',
                           '15개 미션+Edit/custom Fort·시나리오 자연 입력/플레이·화면/음향 동일성', 'Template/State7·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 전략/자연 프레임의 좌표 채움·제거는 미검증',
                             '전체 x87 정밀도/예외·CD10.72(Scan 대응004d9480) 미검증'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
