"""게임/native/C++ 검사 실행 없이 배치 표적 점수의 PE 근거와 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 원문/소스/빌드 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-drop-targets-static-20261022.json'  # 이번 단위의 신규 기록이다.
ENTRIES = (0x412770, 0x4128f0, 0x412950, 0x48ece0, 0x4ac200, 0x4ac950,
           0x4ac550, 0x49acf0, 0x4acb80, 0x4adcb0, 0x46b6f0, 0x469590,
           0x41ce90, 0x4adea0, 0x4e49c0, 0x412a70)  # 새4개/기존 실제 하위11개/다음 별도 검색wrapper1개다.
SOURCES = ('cpppj/src/o/RawAiDropTargets.h', 'cpppj/src/o/RawAiDropTargets.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiDropTargetsTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/RawGunHit.h', 'cpppj/src/o/RawGunBlocker.cpp',
           'cpppj/src/o/RawGunTarget.cpp', 'cpppj/src/o/RawSquidNeighbors.cpp',
           'cpppj/src/o/RawAiCarrierActions.cpp')  # 같은 현재 TYPE/내용물/검색/방향과 컴파일 입력이다.
TESTS = ('AiDropTargets_NativeBasePriestBuildingGroupAndAltarBonuses',
         'AiDropTargets_RateIsAddedBeforeTruncationAndNonfinite', 'AiDropTargets_CaptiveBonusAndCurrentRateAfterLookup',
         'AiDropTargets_AllNativeBonusesAreCombined', 'AiDropTargets_ListBonusesDuplicateAndCurrentBacking',
         'AiDropTargets_FocusSavesHpAndUsesSignedHalf', 'AiDropTargets_EnemyOwnerFullDwordAndCurrentGetterEffects',
         'AiDropTargets_ListAdditionWrapsDword', 'AiDropTargets_PhysicalZeroCarrierAssertionContinues',
         'AiDropTargets_GunWritesResultBeforeScoreAndZero', 'AiDropTargets_BlockersSumAndNativeFirstEntryOutput',
         'AiDropTargets_BlockersNonpositivePreservesOutputAndWrapsTotal',
         'AiDropTargets_DirectionalFilterAfterScoringAndCurrentGlobal', 'AiDropTargets_FacingOddDirectionAndVerticalTie',
         'CampaignSession_AiDropTargetsUseActualContentsGunAndLifetime')  # 새15개 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """정밀 명령 범위·원본 상수/출력과 실제 최종 빌드/연결을 정적으로 읽는다."""
    previous = json.loads((ROOT / 'cpppj/ai-priest-actions-static-20261022.json').read_text(encoding='utf8'))
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
    # 기존 정밀 범위의 시작부터 해독해 명령 중간/이웃 함수 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 원본 범위도 각각 해시와 명령을 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {
        '0x412770': ('test edi, 0x4000000', 'mov dword ptr [esp + 0x10], 0xa', 'mov eax, 5',
                     'test edi, 0x200000', 'add dword ptr [esp + 0x10], 0xc8',
                     'test edi, 0x200', 'add dword ptr [esp + 0x10], 6', 'cmp eax, 6',
                     'add dword ptr [esp + 0x10], 7', '[0x5412e8]', 'add dword ptr [esp + 0x10], 8',
                     'test edi, 0x30000', 'push 0xaeb', 'call 0x4acb80',
                     'add dword ptr [esp + 0x10], 0x190', 'call 0x49acf0',
                     '0041284f fiadd dword ptr [esp + 0x10]', '00412853 call 0x4e49c0',
                     '[ebx + 0x374]', '[ebx + 0x380]', 'add edi, 0x64',
                     '004128b9 mov esi, dword ptr [ebp + 0x1a]', '004128be call 0x4adcb0',
                     '004128c6 sar eax, 1', 'add edi, 0x32',
                     'movzx eax, byte ptr [ebp + 0x22]', 'cmp eax, dword ptr [ebx + 0x264]', '004128e0 sar eax, 1'),
        '0x4128f0': ('call 0x46b6f0', '00412932 mov dword ptr [ecx], eax', '00412939 call 0x412770'),
        '0x412950': ('push 0x78', 'mov dword ptr [esp + 0x20], 0x1e', 'call 0x469590',
                     '004129cd call 0x412770', '[0x541290]', 'fsub dword ptr [0x5012d0]',
                     '00412a13 call 0x48ece0', '00412a21 cmp esi, ebx',
                     '00412a29 mov edx, dword ptr [eax]', '00412a31 mov dword ptr [ecx], edx',
                     '00412a33 add dword ptr [esp + 0x10], esi'),
        '0x48ece0': ('call 0x4ac200', 'test eax, 0x100000', 'call 0x4adea0', 'push 1',
                     'call 0x41ce90', 'cmp ecx, 2', 'cmp ecx, 4', 'cmp ecx, 6'),
        '0x49acf0': ('fld dword ptr [ecx + 0x48]',),
        '0x4ac200': ('[eax + 0xec]',),
        '0x4ac950': ('[eax + 0x9c]',),
        '0x412a70': ('call 0x46b540', 'call 0x412770')
    }  # 누락 가산점/합산 후 절삭/현재HP/출력 쓰기 위치의 실제 명령 근거다.
    # 필수 근거가 하나라도 다르면 성공 보고서를 기록하지 않는다.
    for entry, items in expected.items():
        # 지정 함수의 정밀 범위 안에서 각 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    constants = dict(blocker_offset=struct.unpack_from('<f', mapped, 0x5012d0-base)[0],
                     altar_type=struct.unpack_from('<I', mapped, 0x5412e8-base)[0],
                     directional_type=struct.unpack_from('<I', mapped, 0x541290-base)[0],
                     zero_sid_assertion=mapped[0x501390-base:].split(b'\0',1)[0].decode('ascii'))
    if constants != dict(blocker_offset=1.0, altar_type=164, directional_type=142, zero_sid_assertion='sid'):
        raise ValueError('원본 좌표 보정/TYPE/진단 문자열 불일치')
    build_path = 'extracted/cpp-ai-drop-targets-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-drop-targets-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사를 실행하지 않고 새 이름의 실제 링크만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    required = ('return FindContainedGenus(core.pool,priestState.contained,core.types,target,0x20,0x200000,0);',
                'return hitTypes.at(core.pool.Slot(target)[10]).damagePerSecond;',
                'return hp.MaxHitPoints(target);', 'return gunTarget->FindTarget(x,y,type,direction,owner);',
                'return gunBlocker->Collect(owner,{point.x,point.y},capacity);', 'core.neighbors.Center(target)',
                'AiDropTargetGlobals{runeState.altarType,constructionState.confirm.patternTypes[7]}',
                'impl_->aiDropTargets->Score(token,target)', 'impl_->aiDropTargets->Gun(token,type,{x,y},direction,result)',
                'impl_->aiDropTargets->Blockers(token,type,{x,y},direction,result)')  # 같은 실제 현재 자료와 검색 연결이다.
    # 각 현재 실제 하위와 public 명시 API 연결을 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: ' + item)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions, constants=constants,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00412770 전체 원본 점수와004128f0 실제 Gun 평가',
                          '00412950 실제 블로커 목록 총점/첫 SID 출력과0048ece0 방향 필터'],
                additional_read=dict(entry='00412a70', dependency='0046b540', implemented_in_this_unit=False),
                remaining=['00412a70 별도 검색/배치 스캔·선택·실행/다리/가짜부족좌표',
                           '원래 순서 자동 AI 프레임·종료/15개 미션+Edit/custom Fort·시나리오 자연 플레이·동일성'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 예외/확장 정밀도·CD10.72 미검증',
                             '손상된 본체/목록/물리 SID/TYPE는 명시 경계', '점수 조회는 배치 실행 성공이 아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
