"""게임/native/C++ 검사 실행 없이 AI 자원 선택의 보호 원문과 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소/보호 PE의 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-resource-choice-static-20261022.json'  # 이전 보고서와 구분하는 신규 기록이다.
ENTRIES = (0x411900, 0x410a30, 0x4112b0, 0x410980, 0x4109d0, 0x411350,
           0x4acc60, 0x4105b0, 0x462bc0, 0x4adea0, 0x4e49c0,
           0x412130, 0x40eab0)  # 이번7개 복원/기존 하위와 다음 행동의 실제 NaN 타이머 분기13개 근거다.
SOURCES = ('cpppj/src/o/RawAiResourceChoice.h', 'cpppj/src/o/RawAiResourceChoice.cpp',
           'cpppj/src/o/RawContainedFinder.h', 'cpppj/src/o/RawContainedFinder.cpp',
           'cpppj/src/o/RawAiFactoryProduction.cpp', 'cpppj/src/o/CMakeLists.txt',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/tests/CMakeLists.txt', 'cpppj/tests/AiResourceChoiceTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp')  # 실제 현재 게임/검사 컴파일 입력이다.
TESTS = ('AiResourceChoice_HomeCenterRawFallbackAndCurrentTemple',
         'AiResourceChoice_TruncatedDistanceTieAndInitialLimit',
         'AiResourceChoice_GraphFiltersAndCurrentInvalidSentinel',
         'AiResourceChoice_GraphEffectRereadsCurrentList',
         'AiResourceChoice_NonfiniteFtolAndPhysicalBounds',
         'AiResourceChoice_CollectionGatesAndCarriedCrystal',
         'AiResourceChoice_CollectionUsesHomeCenterAndActualMovement',
         'AiResourceChoice_CollectionEffectsUseCurrentCapabilityAndSentinel',
         'AiResourceChoice_CaptureQueueFlyingDuplicateCapacityAndNoTimer',
         'AiResourceChoice_DisconnectedStableRemovalAndFullDestination',
         'AiResourceChoice_DisconnectedAssertionAndSelectedMissingLog',
         'AiResourceChoice_CountContainedUsesFullWordAndDwordFilters',
         'CampaignSession_AiResourceChoiceUsesActualGraphContentsAndLifetime')  # 새13개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """보호 PE의 정밀 명령/상수와 현재 소스/빌드/링크를 읽기만 한다."""
    previous = json.loads((ROOT / 'cpppj/ai-movement-static-20261022.json').read_text(encoding='utf-8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이번 함수의 기존 정밀 범위만 읽고 이웃 함수로 넓히지 않는다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 독립 해시와 명령으로 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x411900': ('call 0x4acc60', 'test byte ptr [esi + 0x248], 0x10', 'call 0x4112b0',
                            'call 0x462bc0', 'call 0x410a30', 'call 0x4116d0', 'call 0x4118a0'),
                '0x410a30': ('0x47800000', 'faddp st(1)', 'call 0x4e49c0', 'call 0x462bc0',
                            'fild dword ptr [esp + 0x28]', 'fst dword ptr [esp + 0x24]', 'test ah, 5', 'ret 0x14'),
                '0x4112b0': ('call 0x4adea0', '[eax + 0x268]', '[eax + 0x26c]'),
                '0x410980': ('call 0x4ac200', 'test eax, 0x20000'),
                '0x4109d0': ('[ecx + 0x384]', '[ecx + 0x388]', '[ecx + 0x38c]', 'push 0x1d', 'call 0x4105b0'),
                '0x411350': ('call 0x4e0620', 'call 0x462bc0', '[0x52f3e4]', 'call 0x4105b0',
                            'sub dword ptr [esi + 0x338]', '[esi + 0x350]', '[esi + 0x34c]'),
                '0x4acc60': ('call 0x4b2270', 'call 0x4b1b20', 'ret 8'),
                '0x412130': ('fld qword ptr [0x55b4d0]', 'fcomp qword ptr [esi + 0x5a8]', 'test ah, 1', 'jne 0x41262a'),
                '0x40eab0': ('fld qword ptr [0x55b4d0]', 'fcomp qword ptr [ecx]', 'test ah, 1', 'jne 0x40eac5')}  # 실제 폭/호출/엄격 거리·목록과 다음 행동의 unordered 미도달 근거다.
    # 필수 명령이 원본 범위에 없으면 성공 기록을 만들지 않는다.
    for entry, items in expected.items():
        # 같은 함수의 모든 요구 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    constants = dict(crystal_type=struct.unpack_from('<I', mapped, 0x5412c0-base)[0],
                     disconnected_log=struct.unpack_from('<I', mapped, 0x52f3e4-base)[0], initial_distance=65536.0)
    if constants != dict(crystal_type=154, disconnected_log=29, initial_distance=65536.0):
        raise ValueError('원본 크리스털/로그/거리 상수 불일치')
    build_path = 'extracted/cpp-ai-resource-choice-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-resource-choice-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 프로세스를 실행하지 않고 실제 링크된 새 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    required = ('CountContained(core.pool,priestState.contained,actor,0x20,motionState.carrier.nuggetType)',
                'return anchor->GraphFor(sid.value);', 'core.neighbors.Center(sid)',
                'aiMovement->ToTarget(token,actor,target,0)', 'aiMovement->Home(token,actor)',
                'impl_->aiResourceChoice->CollectOrHome(token,actor)', 'impl_->aiResourceChoice->ReclassifyDisconnected(token)')
    # 연결 이름만으로 가짜 하위를 허용하지 않고 같은 실제 몸체 호출을 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: ' + item)
    factory = (ROOT / 'cpppj/src/o/RawAiFactoryProduction.cpp').read_text(encoding='utf-8-sig')
    if 'return CountContained(pool_,finder_,factory,0x10,0);' not in factory:
        raise ValueError('기존 생산과 같은 실제 내용물 총수 공유 누락')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions, constants=constants,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00411900 수집/귀환·00410a30 절삭/그래프 선택·004112b0 신전 중심',
                          '00410980/004109d0 포획 운반자 목록·00411350 끊긴 간헐천 재분류',
                          '004acc60 실제 내용물 총수/기존 생산 공유·같은 Session 그래프/이동 연결'],
                remaining=['00412130 전체 수집/포획 행동·00411cd0 사제 행동',
                           '건물/다리/가짜부족좌표 선정과 원래 순서 자동 AI 프레임/종료',
                           '목표15개+Edit/custom Fort·시나리오 자연 플레이/화면/음향·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 정밀도/예외와 CD10.72 미검증',
                             '손상된 물리 주소/음수 인덱스는 명시 경계', '명시 API bool은 본체 존재/호출이며 이동 도착 성공이 아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
