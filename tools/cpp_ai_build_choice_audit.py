"""게임/native/C++ 검사 실행 없이 AI 건설TYPE 선택/자금과 최종 빌드를 기록한다."""
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
REPORT = ROOT / 'cpppj/ai-build-choice-static-20261022.json'  # 과거 기록과 구분하는 신규 보고서다.
ENTRIES = (0x411b40, 0x411a40, 0x411ad0, 0x413c20, 0x4709d0, 0x470a30,
           0x492340, 0x4922e0, 0x410280, 0x40ec50, 0x49a840, 0x4e49c0,
           0x41d7a0, 0x40ead0)  # 이번6개 복원/실제하위와다음좌표·0비교추가읽기2개를포함한14개근거다.
SOURCES = ('cpppj/src/o/RawAiBuildChoice.h', 'cpppj/src/o/RawAiBuildChoice.cpp',
           'cpppj/src/o/RawAiWallet.h', 'cpppj/src/o/RawAiWallet.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/AiBuildChoiceTests.cpp',
           'cpppj/tests/AiWalletTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/RawMissionOutcome.cpp', 'cpppj/src/o/RawAiDeployment.cpp')  # 실제 현재 컴파일/공유 하위 입력이다.
TESTS = ('AiBuildChoice_CollectorReserveTruncationWrapAndCurrentType',
         'AiBuildChoice_EnemyLocalSpRemoteWalletAndUnknownPlayer',
         'AiBuildChoice_DaisPrioritySkipsLaterFundsAndCandidate',
         'AiBuildChoice_DaisEffectsReadCurrentEnemyAndReturnType',
         'AiBuildChoice_DaisShortCircuitAndFallback', 'AiBuildChoice_DaisFundsFallbackToTempleAndCostBoundary',
         'AiBuildChoice_CurrentThemeMappingsAndSignedRemainder', 'AiBuildChoice_CommonThemeAndNullCandidateGates',
         'AiBuildChoice_WorkshopCostAndReserveAreSeparateGates', 'AiBuildChoice_DuplicatePreferencesAndReadOnlyCounts',
         'AiBuildChoice_FifthThemeAndSignedCounterWrap', 'AiBuildChoice_CountCopyOrderAndPreferenceBounds',
         'AiBuildChoice_SelectedMissingFundsKeepsPrefix', 'AiWallet_BalanceUsesCurrentBrainWithoutDemoOrWrites',
         'AiWallet_BalanceDisabledSignedOwnerAndPhysicalBounds',
         'CampaignSession_AiBuildChoiceUsesActualSpWalletAndLifetime')  # 새16개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """보호 PE 정밀 명령/상수·점프표와 최종 소스/빌드/링크/연결을 읽기만 한다."""
    previous = json.loads((ROOT / 'cpppj/ai-build-search-static-20261022.json').read_text(encoding='utf8'))
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
    # 기존 정밀 범위의 시작부터 해독해 이웃 함수/명령 중간 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 해시와 명령으로 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x411b40': ('call 0x492340', 'call 0x4922e0', 'call 0x410280', 'call 0x411ad0',
                            'call 0x470a30', 'fld dword ptr [eax + 0xc4]', 'call 0x4e49c0', 'call 0x411a40',
                            'call 0x4709d0', 'cmp edi, 5', 'dec ebx', 'js 0x411c82'),
                '0x411a40': ('[0x54126c]', 'call 0x4e49c0', 'imul eax, dword ptr [esi + 0x234]', 'setge cl'),
                '0x411ad0': ('[0x540c70]', '[0x595344]', 'fld dword ptr [0x500fc4]',
                            'call 0x40ec50', 'call 0x413c20', 'cmp eax, 0x3e8', 'setl cl'),
                '0x413c20': ('[0x594fc8]', 'imul eax, eax, 0xb4', '[eax + 0xb0]', '[eax + 0x2d8]'),
                '0x4709d0': ('call 0x4e0620', '[0x5411b0]', '[0x5411ac]'),
                '0x470a30': ('mov ecx, 5', 'idiv ecx', 'jmp dword ptr [edx*4 + 0x470a78]', 'call 0x4e0620'),
                '0x41d7a0': ('fadd dword ptr [0x502310]', 'call 0x4e49c0', 'fild dword ptr [esp + 4]'),
                '0x40ead0': ('fucompp', 'test ah, 0x44', 'jnp 0x40eae7')}  # 실제 단락/절삭·곱/수량·나머지와 다음 좌표/NaN 분기 근거다.
    # 필수 원본 명령이 없으면 성공 보고서를 만들지 않는다.
    for entry, items in expected.items():
        # 같은 정밀 함수 범위 안에서 모든 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    type_addresses = (0x54126c, 0x5412ec, 0x541244, 0x541248, 0x54124c, 0x5411b0, 0x5411b4, 0x5411b8, 0x5411ac)
    initial_types = [struct.unpack_from('<I', mapped, address-base)[0] for address in type_addresses]
    jumps = struct.unpack_from('<6I', mapped, 0x470a78-base)
    if initial_types != [133,165,123,124,125,86,87,88,85] or jumps != (0x470a47,0x470a4d,0x470a53,0x470a5f,0x470a5f,0x470a59):
        raise ValueError('원본 TYPE/신전 점프표 불일치')
    if struct.unpack_from('<f', mapped, 0x500fc4-base)[0] != 0.0:
        raise ValueError('로컬 미확정 SP 기본값 불일치')
    if struct.unpack_from('<I', mapped, 0x502310-base)[0] != 0x3f7fff58:
        raise ValueError('다음 사제 좌표 정렬의 원본 보정값 불일치')
    build_path = 'extracted/cpp-ai-build-choice-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-build-choice-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
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
    required = ('return AiPriestCaptured(owner);', 'return AiPriestDead(owner);', 'return aiWallet->Balance(owner);',
                'return std::bit_cast<float>(sp.Get(owner));', 'return tutorialTypes.at(0x541244U+theme*4U);',
                'templeProduct,runeState.daisType,aiWorkshops', 'impl_->aiBuildChoice->Select(token)',
                'impl_->aiBuildChoice->EnemyLowFunds(token)')
    # 같은 실제 자금/사제 상태와 현재 TYPE 표/명시 API를 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: ' + item)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                initial_types=dict(zip((hex(address) for address in type_addresses), initial_types)),
                temple_jump_table=[hex(value) for value in jumps], unreachable_temple_remainder=5,
                next_priest_dependencies=dict(snap_bias_bits='0x3f7fff58', snap_bias=struct.unpack_from('<f', mapped, 0x502310-base)[0],
                                             timer_nonzero_nan=True, implemented_in_this_unit=False),
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00411b40 다음 건설TYPE/00411a40 수집자 비용 여유/00411ad0 적 자금 조건',
                          '00413c20 실제 AI 지갑 조회/004709d0·00470a30 현재 테마TYPE', '같은 현재 SP/지갑/사제 상태·수량표/선호 목록 연결'],
                remaining=['00411cd0 전체 사제 행동/현재 실제 배치 탐색과 연결', '건물/다리/가짜부족좌표/원래 순서 자동 프레임·종료',
                           '15개 미션+Edit/custom Fort·시나리오 자연 플레이/화면/음향·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 예외/확장 정밀도·CD10.72 미검증',
                             '손상된 본체/TYPE/Player/임시선호 인덱스는 명시 경계', '명시TYPE API의0은 본체 또는 선택 없음이며 건설 실행 성공이 아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
