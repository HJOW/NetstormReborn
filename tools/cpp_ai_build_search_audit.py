"""게임/native/C++ 검사 실행 없이 AI 배치 탐색과 최종 빌드 근거를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 보호 원문/빌드 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-build-search-static-20261022.json'  # 과거 기록과 구분하는 신규 보고서다.
ENTRIES = (0x40f660, 0x40f700, 0x40f130, 0x49a840, 0x49b510, 0x460d70, 0x411cd0, 0x4e49c0)  # 이번3개 복원과 실제 TYPE/배치/벽시각/사제caller/CRT8개 근거다.
SOURCES = ('cpppj/src/o/RawAiBuildSearch.h', 'cpppj/src/o/RawAiBuildSearch.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/AiBuildSearchTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp', 'cpppj/src/o/GameClock.cpp',
           'cpppj/src/client/CampaignClientServices.cpp')  # 실제 컴파일과 기존 벽시각 연결 입력이다.
TESTS = ('AiBuildSearch_InitializeRawStateBoundsAndSurroundings', 'AiBuildSearch_FreeFootprintMaskAndEmptyDimensions',
         'AiBuildSearch_TypeZeroAndCompletedClockOrder', 'AiBuildSearch_FarthestCandidateAndActualQueryArguments',
         'AiBuildSearch_StrictTieZeroAndUnorderedScore', 'AiBuildSearch_SpotGateKeepsLazyTypeAccess',
         'AiBuildSearch_BudgetedRowPauseAndResume', 'AiBuildSearch_BudgetEqualityAndUnorderedClock',
         'AiBuildSearch_BudgetChecksOnlyBetweenRows', 'AiBuildSearch_CurrentCachedTypeOwnerAndRowSnapshot',
         'AiBuildSearch_CurrentMaxAndSelectedMissingEffect', 'AiBuildSearch_DistanceComparisonBeforeFloatStore',
         'AiBuildSearch_InitializeFailurePrefixAndPhysicalBounds',
         'CampaignSession_AiBuildSearchUsesActualStatePlacementAndClock')  # 새14개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """보호 PE 정밀 명령/상수와 최종 소스/빌드/링크/연결을 읽기만 한다."""
    previous = json.loads((ROOT / 'cpppj/ai-carrier-actions-static-20261022.json').read_text(encoding='utf8'))
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
    # 기존 정밀 범위 시작부터 읽어 명령 경계를 보존한다.
    for entry in ENTRIES:
        blocks = []
        # 이웃 함수로 넓히지 않고 불연속 범위도 각각 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x40f660': ('call 0x49a840', '[esi + 0x18]', 'fild dword ptr [eax + 0x1d4]',
                            'call 0x4e49c0', '0xc61c4000', '[esi + 0x30]', 'ret 0x20'),
                '0x40f700': ('call 0x460d70', 'fadd qword ptr [0x501230]', 'test ah, 0x41', 'jp 0x40f867',
                            'call 0x40f130', 'fstp dword ptr [esp + 0x18]', 'fst dword ptr [esp + 0x18]',
                            'fcomp dword ptr [esi + 4]', 'jne 0x40f847', 'mov dl, byte ptr [esi + 0x1c]',
                            'mov ecx, dword ptr [esi + 0x18]', 'call 0x49b510', '[esi + 0x30]'),
                '0x40f130': ('call 0x49a840', '[eax + 0x1d8]', '[eax + 0x1d4]', 'and bl, 0x3f', 'cmp bl, 2', 'add eax, 0x100'),
                '0x49a840': ('imul eax, eax, 0x1f4',), '0x460d70': ('0x55b4a0', '0x55b4a4'),
                '0x411cd0': ('call 0x40f700', 'call 0x40f660')}  # 원본 인자폭/예산·거리/행 재개/전체 배치 호출 근거다.
    # 원본 명령이 없으면 성공 보고서를 기록하지 않는다.
    for entry, items in expected.items():
        # 같은 함수 범위 안의 필수 명령을 모두 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    budget = struct.unpack_from('<d', mapped, 0x501230-base)[0]
    if budget != struct.unpack('<f', struct.pack('<f', 0.03))[0]:
        raise ValueError('원본 행 예산 double(float0.03) 불일치')
    build_path = 'extracted/cpp-ai-build-search-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-build-search-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 실행 없이 실제 링크된 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    required = ('search.wall=services.construction.wallSeconds', 'PrepareConstruction();return MayPlaceGameplay(query);',
                'RawAiBuildSearch>(aiMemory,core.types,core.spots,kAiSearchTypeBase',
                'impl_->aiBuildSearch->Initialize(token,type,bounds,avoid,Read(raw,0x260))',
                'impl_->aiBuildSearch->Scan(token,budgeted)')
    # 실제 현재 spot/TYPE/벽시각과 전체 배치 연결을 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('같은 실제 Session 하위 누락: ' + item)
    client = (ROOT / 'cpppj/src/client/CampaignClientServices.cpp').read_text(encoding='utf-8-sig')
    if 'services.construction.wallSeconds=[&client] { return client.WallSeconds(); };' not in client:
        raise ValueError('프레임 snapshot과 구별한 실제 현재 벽시각 누락')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                constants=dict(budget_seconds=budget, state_offset='0x574', state_size=0x34,
                               opaque_type_base='0x6b000000', type_stride=500, native_virtual_address_claim=False),
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['0040f660 초기화/0040f700 행별 예산·가장 먼 합법 후보/0040f130 빈 footprint',
                          '같은 현재 TYPE/spot/전체 Canon 배치/현재 벽시각·원본574 상태 연결'],
                remaining=['00411b40 건설TYPE 선택/00411cd0 전체 사제 행동', '건물/다리/가짜부족좌표/원래 순서 자동 프레임·종료',
                           '15개 미션+Edit/custom Fort·시나리오 자연 플레이/화면/음향·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', 'TYPE는같은실제표의opaque32대응이며원본가상주소가아님',
                             '전체 x87 확장 정밀도/예외·CD10.72 미검증', '손상된 지도주소/캐시/본체는 명시 경계',
                             '명시API bool은 본체 존재/호출이며 후보 존재/배치 성공이 아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
