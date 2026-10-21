"""원본/native/C++ 검사 실행 없이 별도 검색·배치 평가의 근거와 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 보호 원문/현재 소스/빌드 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-drop-evaluation-static-20261022.json'  # 과거 기록과 구분한 이번 보고서다.
ENTRIES = (0x46b540, 0x46b360, 0x4b22d0, 0x4b1b80, 0x4b1bd0,
           0x412a70, 0x412640, 0x4126c0, 0x412ab0, 0x412b90,
           0x46af80, 0x469440, 0x41d530, 0x41d400, 0x422230,
           0x41d7a0, 0x41ce90, 0x4105b0, 0x4e49c0)  # 새10몸체와 기존 실제 의존9개의 정밀 범위다.
SOURCES = ('cpppj/src/o/RawGunGroundTarget.h', 'cpppj/src/o/RawGunGroundTarget.cpp',
           'cpppj/src/o/RawAiDropEvaluation.h', 'cpppj/src/o/RawAiDropEvaluation.cpp',
           'cpppj/src/o/RawAiDropTargets.h', 'cpppj/src/o/RawAiDropTargets.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/GunGroundTargetTests.cpp', 'cpppj/tests/AiDropEvaluationTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp', 'cpppj/src/o/RawGunTarget.cpp',
           'cpppj/src/o/RawSquidFinder.cpp', 'cpppj/src/o/RawSquidNeighbors.cpp',
           'cpppj/src/o/RawEnergySupply.cpp', 'cpppj/src/o/RawAiDrySpots.cpp')  # 실제 공유 자료와 최종 컴파일 입력이다.
TESTS = ('GunGroundTarget_AirborneAndCommonTargetFilters',
         'GunGroundTarget_WalkerUsesActualDependChainAndCurrentType', 'GunGroundTarget_ExcludedSourcePrecedesDependencyLookup',
         'GunGroundTarget_DependencyDiagnosticAndCurrentHead', 'GunGroundTarget_DependencyFirstMatchAndCycleBoundary',
         'GunGroundTarget_ActualHashStrictSquaredRange', 'GunGroundTarget_ActualHashKeepsFirstEqualCandidate',
         'GunGroundTarget_CurrentAllianceAndDiagnosticContinuation', 'GunGroundTarget_SnapBiasAndAsymmetricSourceFootprint',
         'GunGroundTarget_NoRangeDrawAndRejectedWalkerDoesNotWin', 'AiDropEvaluation_DryEmptySignedCountPreservesIndex',
         'AiDropEvaluation_DryNonzeroManhattanFirstTieAndNan', 'AiDropEvaluation_DryComparesExtendedThenStoresFloat',
         'AiDropEvaluation_SupplyStrictRadiusAndStoredScore', 'AiDropEvaluation_SupplyCstrlenAndCurrentSignedLimit',
         'AiDropEvaluation_DispatchFlagsPrecedeGroupsAndPreserveOutput', 'AiDropEvaluation_DispatchGroundGunAndSilentGroupSix',
         'AiDropEvaluation_UnknownGroupDiagnosticAndTypeBounds', 'AiDropEvaluation_GroundResultWrittenBeforeScoreFailure',
         'AiDropEvaluation_PlacementGateCanonicalArguments', 'AiDropEvaluation_CurrentOwnerBetweenPlacementScoreAndEnergy',
         'AiDropEvaluation_PositiveTieAndSignedImprovement', 'AiDropEvaluation_WorseScoreKeepsTargetSideEffect',
         'AiDropEvaluation_EnergyFailureUsesActualDryBackingAndCurrentLog', 'AiDropEvaluation_SelectedMissingHooksPreservePrefix',
         'CampaignSession_AiDropEvaluationUsesActualManaGroundPlacementAndLifetime')  # 새26개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """보호 PE 정밀 범위·현재 전역/가상 표와 실제 최종 빌드/연결을 정적으로 읽는다."""
    previous = json.loads((ROOT / 'cpppj/ai-drop-targets-static-20261022.json').read_text(encoding='utf8'))
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
        # 불연속 범위도 각각 해시와 전체 명령을 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {
        '0x46b540': ('call 0x49a900', 'fmul dword ptr [0x500ee4]', 'call 0x49ace0',
                     '0046b5d1 fst dword ptr [esp + 0x78]', '0046b5d5 call 0x4e49c0',
                     'push 5', 'call 0x422230', 'fmul dword ptr [esp + 0x78]',
                     'call 0x48f790', 'test al, 0x10', 'call 0x46b360',
                     'call 0x469440', 'fcom dword ptr [esp + 0x78]', 'test ah, 5',
                     '0046b6c1 fstp dword ptr [esp + 0x78]', 'call 0x4b1810'),
        '0x46b360': ('test eax, 0x120000', 'test eax, 0x10000', '[0x541178]',
                     'mov dword ptr [esp + 0xc], 0x508f14', 'call 0x4b22d0', 'call 0x46af80'),
        '0x4b22d0': ('[0x5e4794]', '[0x594fc8]', '[0x540bc4]', 'push 0x158',
                     'movzx eax, word ptr [esi + 6]', 'call 0x4b1b80'),
        '0x4b1b80': ('cmp byte ptr [eax + 0xa], 0x46', 'call dword ptr [edx]'),
        '0x4b1bd0': ('[0x541074]',),
        '0x412a70': ('call 0x46b540', 'call 0x412770'),
        '0x412640': ('[ecx + 0xae0]', '[ecx + 0xae4]', 'fabs', 'fsubr dword ptr [0x501480]',
                     '00412693 fst dword ptr [esp + 0x14]', '00412697 fcomp st(1)',
                     '004126a2 mov dword ptr [edi], edx', '004126a4 fld dword ptr [esp + 0x14]'),
        '0x4126c0': ('call 0x412640', '004126e8 fst dword ptr [esp + 0x10]',
                     'fld dword ptr [0x52f488]', 'fadd st(0), st(0)', '[0x541174]',
                     'push -1', 'call 0x4730c0', 'cmp eax, dword ptr [esi + 0xad8]',
                     'fld dword ptr [esp + 0x10]', 'call 0x4e49c0', 'idiv esi'),
        '0x412ab0': ('call 0x4126c0', 'call 0x412950', 'call 0x412a70', 'call 0x4128f0', 'push 0x1d'),
        '0x412b90': ('call 0x49b510', 'call 0x412ab0', '00412c10 cmp ebp, eax', 'jg 0x412c24',
                     'jle 0x412cc1', 'call 0x4734d0', '00412c52 mov dword ptr [eax + 4], edi',
                     '00412c55 mov dword ptr [eax], ebx', '00412c60 mov dword ptr [eax], ebp',
                     'call 0x40f1c0', 'call 0x4105b0'),
        '0x422230': ('call 0x41d7a0', 'call 0x4e49c0'),
        '0x41d7a0': ('fadd dword ptr [0x502310]',),
        '0x4105b0': ('call 0x4de490', '[eax + 0xb00]')
    }  # 원본 범위·필터·저장float·현재문자열·출력순서의 근거다.
    # 필수 근거가 다르면 성공 보고서를 기록하지 않는다.
    for entry, items in expected.items():
        # 같은 정밀 함수 범위 안의 필수 명령을 모두 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    predicate = struct.unpack_from('<I', mapped, 0x508f14-base)[0]
    raw_thunk = mapped[predicate-base:predicate-base+8]
    thunk = []
    # 실제 가상 표가 지정한 진입점부터 ret까지 읽는다.기존 정밀 목록에 없는 thunk를 별도로 기록한다.
    for item in decoder.disasm(raw_thunk, predicate):
        thunk.append(f'{item.address:08x} {item.mnemonic} {item.op_str}')
        if item.mnemonic == 'ret':
            raw_thunk = raw_thunk[:item.address-predicate+item.size]
            break
    if predicate != 0x453740 or thunk != ['00453740 call 0x4b1bd0', '00453745 ret 4']:
        raise ValueError('실제 Depend 가상 필터 thunk 불일치')
    constants = dict(distance_value=struct.unpack_from('<f', mapped, 0x501480-base)[0],
                     initial_mana_radius=struct.unpack_from('<f', mapped, 0x52f488-base)[0],
                     snap_bias_bits=hex(struct.unpack_from('<I', mapped, 0x502310-base)[0]),
                     half=struct.unpack_from('<f', mapped, 0x500ee4-base)[0],
                     probe_type=struct.unpack_from('<I', mapped, 0x541174-base)[0],
                     excluded_type=struct.unpack_from('<I', mapped, 0x541178-base)[0],
                     depend_type=struct.unpack_from('<I', mapped, 0x541074-base)[0])
    if constants != dict(distance_value=1000.0, initial_mana_radius=22.0, snap_bias_bits='0x3f7fff58',
                         half=0.5, probe_type=71, excluded_type=72, depend_type=1):
        raise ValueError('원본 점수/반경/보정/TYPE 상수 불일치')
    build_path = 'extracted/cpp-ai-drop-evaluation-20261022-build2.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-drop-evaluation-20261022-build2-result.json').read_text(encoding='utf-8-sig'))
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
    required = ('GunGroundTargetGlobals ground{aiGroundExcludedType,flyerDependType,gunState.checking,core.ownerMode.challenge,assetState.boss,',
                'core.pool,core.hash,core.types,gunTypes,*gunTarget,gunTargetState,std::move(ground)',
                'return aiGroundTarget->Find(x,y,type,owner);', 'energySupply->Collect(text,x,y,type,owner,theme);',
                'return MayPlaceGameplay(query);', 'return energySupply->Evaluate(text,x,y,type,owner);',
                'return aiDrySpots->Add(token,x,y,type);', 'AiDropEvaluationGlobals{cursorManaGlobals.radiusLimit,aiManaProbeType}',
                'impl_->aiDropEvaluation->Evaluate(token,type,{x,y},direction,target)',
                'impl_->aiDropEvaluation->Consider(token,type,{x,y},direction,bestPoint,bestScore,target)')  # 같은 실제 하위/현재 전역과 명시 API다.
    # 현재 실제 마나/부족좌표/전체 배치와 별도 검색 연결을 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: ' + item)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions, constants=constants,
                depend_predicate=dict(vtable='0x508f14', entry=hex(predicate), sha256=hashlib.sha256(raw_thunk).hexdigest(), instructions=thunk),
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path), previous_failure=dict(
                    result=json.loads((ROOT / 'extracted/cpp-ai-drop-evaluation-20261022-build1-result.json').read_text(encoding='utf-8-sig')),
                    log_sha256=digest('extracted/cpp-ai-drop-evaluation-20261022-build1.log'))),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['0046b540/0046b360 실제 지상검색과004b22d0/004b1b80/004b1bd0 현재Depend필터',
                          '00412a70/00412640/004126c0/00412ab0/00412b90 실제점수/마나/배치후보승인'],
                remaining=['00412ce0 다리주변후보/00412e10 스캔·00413100 TYPE선택/004162e0 배치실행',
                           '다리/가짜부족좌표·원래자동AI프레임/종료·15개미션+Edit/custom Fort·시나리오 자연플레이동일성'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 예외/확장 정밀도·CD10.72 미검증',
                             '손상된본체/목록/체인·TYPE·물리범위밖좌표/사거리는명시경계', '명시후보승인은배치실행성공이아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
