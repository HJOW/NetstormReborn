"""게임/native/C++ 검사 실행 없이 AI 배치 스캔의 원본 근거와 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 보호 자료/현재 소스·빌드 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-drop-scan-static-20261022.json'  # 과거 기록과 구분한 신규 보고서다.
ENTRIES = (0x412ce0, 0x412e10, 0x40f390, 0x4fc530, 0x40e880, 0x40e8f0,
           0x4ad680, 0x4558c0, 0x4e49c0, 0x40f130, 0x412b90, 0x4162e0)  # 새3개/CRT1개/기존 하위7개/다음 전체행동인 실제caller1개다.
SOURCES = ('cpppj/src/o/RawAiDropScan.h', 'cpppj/src/o/RawAiDropScan.cpp',
           'cpppj/src/o/RawAiDropEvaluation.h', 'cpppj/src/o/RawAiDropEvaluation.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiDropScanTests.cpp', 'cpppj/tests/AiDropEvaluationTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp', 'cpppj/src/o/RawAiBuildSearch.cpp',
           'cpppj/src/o/SpStore.cpp', 'cpppj/src/o/RawPathAnimation.cpp')  # 실제 현재 지도/난수/프레임·승인 연결과 컴파일 입력이다.
TESTS = ('AiDropScan_BridgeJPreferredSideAndSharedRandom', 'AiDropScan_BridgeJWholeByteFallbackAndFailureNoRandom',
         'AiDropScan_BridgeKAndPhysicalRowBoundary', 'AiDropScan_BridgeCurrentTypeAndActualFrameCode',
         'AiDropScan_CursorPreincrementAndInclusiveCompletion', 'AiDropScan_EmptyMapNormalAndSupplyBudgets',
         'AiDropScan_BridgeBudgetConsumesSharedRandomOnRejectedCandidates', 'AiDropScan_DirectedAllianceAndOwnFootprintBudget',
         'AiDropScan_NonbridgeEligibilityGatesAndMaskedSpot', 'AiDropScan_BoundsAndFlagsArePerCallSnapshot',
         'AiDropScan_ImprovementResetsAndEqualScoreAppends', 'AiDropScan_FullThirtyChoicesRandomReplacement',
         'AiDropScan_RejectedCandidateKeepsCurrentStoredEffects', 'AiDropScan_MissingConsiderKeepsCursorAndBridgeRandom',
         'AiDropScan_RemoveStableCompactionAndUntouchedTail', 'AiDropScan_RemoveDiagnosticContinuationAndPrefix',
         'AiDropEvaluation_StoredScoreReadsCurrentBodyAtNativeSites',
         'CampaignSession_AiDropScanUsesActualMapsStateAndLifetime')  # 새18개 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """보호 PE의 정밀 범위·CRT/전역·예산과 최종 소스/빌드/실제 연결을 읽기만 한다."""
    previous = json.loads((ROOT / 'cpppj/ai-drop-evaluation-static-20261022.json').read_text(encoding='utf8'))
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
    # 정밀 범위 시작부터 해독해 명령 중간/이웃 함수 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 전체 명령/해시를 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {
        '0x412ce0': ('[0x5411a0]', 'call 0x4ad680', 'cmp eax, 0x4a', 'cmp eax, 0x4b',
                     'cmp byte ptr [esi - 0x100], 0', 'cmp byte ptr [esi + 0x100], 0',
                     'mov bl, byte ptr [eax - 3]', 'mov bl, byte ptr [eax + 3]', 'push 3',
                     '00412d57 mov dword ptr [ebx], eax', '00412d59 call 0x4558c0',
                     '00412df4 mov dword ptr [edx], ecx', '00412df6 call 0x4558c0'),
        '0x412e10': ('[0x544d14]', '[0x544d18]', '[0x544d1c]', '[0x544d20]',
                     'mov ebp, 0x3e8', 'mov ebp, 0x1f4', '00412eaa inc ecx',
                     'mov dword ptr [esi + 0xaf8], ecx', 'jg 0x4130de', '[0x5c84bc]',
                     'call 0x412ce0', 'sub ebp, 5', 'test ebx, 0x4040800',
                     'and cl, 0x3f', 'cmp cl, 2', '[esi + 0x2c8]', '[edi*4 + 0x595200]',
                     'call 0x40f130', 'sub ebp, 0xa', 'call 0x412b90',
                     '0041302a cmp dword ptr [ebx], edi', 'mov dword ptr [esi + 0x570], 0',
                     'cmp eax, 0x1e', 'call 0x4558c0', '[esi + eax*8 + 0x408]',
                     '[esi + eax*4 + 0x4f8]', 'inc dword ptr [esi + 0x570]', 'sub ebp, 0x7d',
                     '004130c6 dec ebp', '004130e3 mov dword ptr [esi + 0xafc], ecx'),
        '0x40f390': ('push 0x1ac', 'call 0x4e0620', '0040f3b4 mov edx, dword ptr [esi + 0x194]',
                     '0040f3bd mov dword ptr [esi + 0x194], edx', '[esi + edx*4 + 0x11c]', '[esi + edx*8 + 0x2c]'),
        '0x4fc530': ('0x43760000', '0x41200000', '[0x544d14]', '[0x544d18]',
                     '[0x544d1c]', '[0x544d20]', 'call 0x40e880', 'call 0x40e8f0'),
        '0x4ad680': ('mov edi, dword ptr [esi + 0x24]', 'movsx eax, byte ptr [ecx + edi*4]'),
        '0x4162e0': ('00416358 lea edx, [esi + 0x3e8]', '00416363 call 0x412e10'),
        '0x412b90': ('00412c00 call 0x412ab0', '00412c0e mov eax, dword ptr [eax]',
                     '00412c10 cmp ebp, eax', '00412c52 mov dword ptr [eax + 4], edi',
                     '00412c55 mov dword ptr [eax], ebx', '00412c60 mov dword ptr [eax], ebp')
    }  # 원본 전역 초기화/선증가/전체BYTE·방향별동맹/예산/현재점수와출력의근거다.
    # 필수 원본 근거가 다르면 성공 보고서를 기록하지 않는다.
    for entry, items in expected.items():
        # 같은 정밀 범위 안에서 각 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    ctor = struct.unpack_from('<I', mapped, 0x52801c-base)[0]
    initial = list(struct.unpack_from('<4f', mapped, 0x544d14-base))
    if ctor != 0x4fc530 or initial != [0.0,0.0,0.0,0.0]:
        raise ValueError('원본 CRT 포인터/BSS 초기 바이트 불일치')
    build_path = 'extracted/cpp-ai-drop-scan-20261022-build2.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-drop-scan-20261022-build2-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+', log, re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사 실행 없이 실제 링크된 새 이름만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: ' + name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    required = ('AiBuildSearchBounds aiDropScanBounds{10,10,246,246}', 'return aiBuildSearch->FreeFootprint(type,x,y);',
                'return aiDropEvaluation->ConsiderStored(token,type,point,direction,bestPoint,target);',
                'core.spots,core.hash.Entries(0),alliances,random,AiDropScanGlobals{core.bridgeState.bridgeType,aiDropScanBounds}',
                'impl_->aiDropScan->Scan(token,type,direction)', 'impl_->aiDropScan->Remove(token,index)')  # 같은 현재 지도/공용난수/실제점수 포인터 연결이다.
    # 각 실제 하위/전역과 명시 API 연결을 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: ' + item)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                scan_bounds=dict(crt_table='0x528014[2]', initializer=hex(ctor), pe_uninitialized=initial, initialized=[10.0,10.0,246.0,246.0]),
                budgets=dict(normal=1000, supply=500, surface=5, allied=10, consider=125, iteration=1, choices=30),
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path), initial_build=dict(
                    result=json.loads((ROOT / 'extracted/cpp-ai-drop-scan-20261022-build1-result.json').read_text(encoding='utf-8-sig')),
                    log_sha256=digest('extracted/cpp-ai-drop-scan-20261022-build1.log'),
                    rebuild_reason='시설flags snapshot 검사 강화 후 최종 링크 재확인')),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00412ce0 다리 주변 후보/00412e10 예산 스캔/0040f390 현재 후보 압축',
                          '실제 유일 caller의 같은3e8 점수 직접조회/쓰기와 현재원본 지도·공용난수 연결'],
                additional_read=dict(entry='004162e0', implemented_in_this_unit=False),
                remaining=['00413100 TYPE선택/004162e0 배치실행/다리·가짜부족좌표/원래자동AI프레임·종료',
                           '15개미션+Edit/custom Fort·시나리오 수명/자연플레이·화면/음향동일성'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 예외/확장 정밀도·CD10.72 미검증',
                             '손상된본체/지도밖물리주소/30후보배열밖index는명시경계', '스캔true는미완료이며배치성공이아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
