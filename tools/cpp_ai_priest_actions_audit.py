"""게임/native/C++ 검사 실행 없이 사제 AI의 원본 근거와 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 보호 자료/빌드 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-priest-actions-static-20261022.json'  # 과거 기록과 구분하는 이번 보고서다.
ENTRIES = (0x411cd0, 0x41d7a0, 0x40e800, 0x491b30, 0x448a90,
           0x40e850, 0x40ead0, 0x40eab0, 0x425ed0, 0x4ad710,
           0x40f660, 0x40f700, 0x411b40, 0x4108f0, 0x44c420,
           0x4116d0, 0x4117e0, 0x4118a0, 0x411900)  # 이번5함수와 기존 실제 의존14함수의 정밀 범위다.
SOURCES = ('cpppj/src/o/RawAiPriestActions.h', 'cpppj/src/o/RawAiPriestActions.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiPriestActionsTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/RawAiBuildSearch.cpp', 'cpppj/src/o/RawAiBuildChoice.cpp',
           'cpppj/src/o/RawAiMovement.cpp', 'cpppj/src/o/RawAiResourceChoice.cpp',
           'cpppj/src/o/RawSalvageCommand.cpp', 'cpppj/src/o/RawPathAnimation.cpp')  # 같은 현재 연결/컴파일 입력이다.
TESTS = ('AiPriestActions_SearchPrefixPrecedesActorAndDeadlines',
         'AiPriestActions_OrderedReadinessAndEarlyGates', 'AiPriestActions_RitualUsesStoredSelfAndStops',
         'AiPriestActions_AltarRequeriesSelfAndDaisBeforeSacrifice', 'AiPriestActions_DaisStrictDistanceAndCurrentTarget',
         'AiPriestActions_LowFundsSelectionAndSavedHpFallback', 'AiPriestActions_NoTypeValidTargetOnlyClears',
         'AiPriestActions_DeployUsesRawPositionAndPriorTimer', 'AiPriestActions_MissingDeployPreservesPrefix',
         'AiPriestActions_SearchDeadlineIsOrdered', 'AiPriestActions_CompletedSearchConsumesSnapsAndMoves',
         'AiPriestActions_NewSearchSnapshotsArgumentsAndCurrentClock', 'AiPriestActions_CurrentSearchProgressAfterSelect',
         'AiPriestActions_DaisReclaimHighestSignedHpAndRequestOrder', 'AiPriestActions_ReclaimFallbackUsesCurrentTempleAndFunds',
         'AiPriestActions_ReclaimTimerNonzeroAndNan', 'AiPriestActions_MissingSalvagePreservesTargetRequest',
         'AiPriestActions_PointSnapAndValidityNativeBoundaries', 'AiPriestActions_AltarArgumentWordAndPhysicalZero',
         'AiPriestActions_RitualUsesActualFrameCode',
         'CampaignSession_AiPriestActionsUseActualSearchMovementAndLifetime')  # 새21개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """보호 PE의 정밀 범위·상수와 최종 소스/빌드/링크/실제 연결을 읽기만 한다."""
    previous = json.loads((ROOT / 'cpppj/ai-build-choice-static-20261022.json').read_text(encoding='utf8'))
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
    expected = {
        '0x411cd0': ('00411cf5 call 0x40f700', '00411d15 test edi, edi',
                     'fcomp qword ptr [esi + 0x5a8]', '00411d2b test ah, 1', '00411d2e jne 0x4120ce',
                     'call 0x411ca0', 'call 0x425ed0', 'call 0x411630', 'call 0x4adcb0',
                     'call 0x491b30', 'call 0x448a90', 'call 0x41d840', 'call 0x411b40',
                     'cmp dword ptr [esi + 0x2d8], 0x190', 'call 0x40e850',
                     'fadd qword ptr [0x5012d8]', 'call 0x4108f0', 'call 0x40ead0',
                     '00412055 call 0x4116d0', '0041205b call 0x44c420',
                     'fadd qword ptr [0x501450]', 'fstp qword ptr [esi + 0x5e8]',
                     '004120ae call 0x40f660', '004120b7 call 0x40f700',
                     'fadd qword ptr [0x501448]', 'call 0x4117e0'),
        '0x41d7a0': ('fadd dword ptr [0x502310]', 'call 0x4e49c0', 'fild dword ptr [esp + 4]'),
        '0x40e800': ('fild dword ptr [0x531928]', 'fstp dword ptr [esp]', 'test ah, 5'),
        '0x491b30': ('call 0x4ad710', 'cmp eax, 0xa', 'cmp eax, 0xe'),
        '0x448a90': ('[0x5412e8]', 'call 0x4b1fa0', 'movzx eax, word ptr [eax + ecx + 0xc]', 'sub eax, dword ptr [esp + 0x10]'),
        '0x40e850': ('fucompp', 'test ah, 0x44', 'jp 0x40e877'),
        '0x40ead0': ('fld qword ptr [0x500460]', 'fucompp', 'test ah, 0x44', 'jnp 0x40eae7'),
        '0x40eab0': ('fld qword ptr [0x55b4d0]', 'test ah, 1', 'jne 0x40eac5'),
        '0x425ed0': ('push 0x493', 'call 0x4afb40'),
        '0x4ad710': ('mov esi, dword ptr [ecx + 0x24]', 'movsx eax, byte ptr [ecx + esi*4]', 'sub eax, 0x41')
    }  # 원본 순서/현재 인자와 double·float·WORD·signed BYTE 폭의 필수 근거다.
    # 필수 원본 명령이 없으면 성공 보고서를 만들지 않는다.
    for entry, items in expected.items():
        # 같은 정밀 함수 범위 안에서 모든 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    constants = dict(snap_bias_bits=hex(struct.unpack_from('<I', mapped, 0x502310-base)[0]),
                     snap_bias=struct.unpack_from('<f', mapped, 0x502310-base)[0],
                     dais_distance=struct.unpack_from('<f', mapped, 0x501444-base)[0],
                     build_delay=struct.unpack_from('<d', mapped, 0x5012d8-base)[0],
                     search_delay=struct.unpack_from('<d', mapped, 0x501448-base)[0],
                     salvage_delay=struct.unpack_from('<d', mapped, 0x501450-base)[0],
                     zero_timer=struct.unpack_from('<d', mapped, 0x500460-base)[0],
                     altar_type=struct.unpack_from('<I', mapped, 0x5412e8-base)[0],
                     dais_type=struct.unpack_from('<I', mapped, 0x5412ec-base)[0])
    if constants != dict(snap_bias_bits='0x3f7fff58', snap_bias=0.9999899864196777, dais_distance=2.0,
                         build_delay=20.0, search_delay=3.0, salvage_delay=10.0, zero_timer=0.0, altar_type=164, dais_type=165):
        raise ValueError('원본 좌표 보정/거리/기한/TYPE 상수 불일치')
    build_path = 'extracted/cpp-ai-priest-actions-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-priest-actions-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
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
    required = ('priest.scan=[this](auto token,auto budgeted) { aiBuildSearch->Scan(token,budgeted); }',
                'return core.processes.FindEvent(actor,0x493)!=nullptr;', 'return aiMovement->MovementForm(actor);',
                'return AiPriestInRitual(core.pool,core.frames,actor);', 'return world.Find(x,y,type);',
                'return AiAltarContainsEnemy(core.pool,dais,enemy,runeState.altarType,',
                'return AiDistanceToFootprint(point,core.neighbors.Footprint(dais));',
                'return aiBuildChoice->Select(token);', 'aiMovement->ToTarget(token,actor,target,sacrifice);',
                'aiResourceChoice->CollectOrHome(token,actor);', 'aiCollectors->DeployWithNotice(token,type,point.x,point.y,0,0);',
                'aiBuildSearch->Initialize(token,request.type,request.bounds,request.avoid,request.owner);',
                'aiMovement->ToPoint(token,actor,point,0);', 'return aiBuildChoice->EnemyLowFunds(token);',
                'salvageCommand->Request(target.value);',
                'AiPriestActionGlobals{core.processState.now,cursorManaGlobals.mapLimit,runeState.daisType}',
                'impl_->aiPriestActions->Act(token,priest);')  # 같은 실제 하위/현재 전역과 명시 API 연결을 확인한다.
    # 각 현재 실제 하위가 Session에서 사제 행동에 연결됐는지 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: ' + item)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions, constants=constants,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00411cd0 전체 사제 행동/0041d7a0 보정/0040e800 유효 좌표',
                          '00491b30 실제 frame 의식/00448a90 실제 제단 argument 조회',
                          '같은 실제 탐색/이동/의식/배치/수집·귀환/54h 회수 요청 연결'],
                remaining=['건물/다리/가짜부족좌표/원래 순서 자동 AI 프레임·종료',
                           '15개 미션+Edit/custom Fort·시나리오 자연 플레이/화면/음향·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 예외/확장 정밀도·CD10.72 미검증',
                             '손상된 본체/목록/물리 SID/프레임은 명시 경계', 'ManageAiPriest의true는 명시 몸체 호출이며 배치 성공이 아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
