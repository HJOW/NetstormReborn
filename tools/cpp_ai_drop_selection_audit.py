"""게임/native/C++ 검사 실행 없이 AI 배치 TYPE 선택의 원본 근거와 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 보호 원문/현재 소스·빌드 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-drop-selection-static-20261022.json'  # 과거 기록과 구분한 이번 보고서다.
ENTRIES = (0x413100, 0x425830, 0x40e9d0, 0x4acac0, 0x4ac9f0, 0x4acc60,
           0x4acb80, 0x48fc90, 0x4558c0, 0x40f390, 0x4105b0, 0x4162e0)  # 이번3개/기존 실제 하위8개/다음 전체행동1개다.
SOURCES = ('cpppj/src/o/RawAiDropSelection.h', 'cpppj/src/o/RawAiDropSelection.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiDropSelectionTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/RawCarrierPreDestroy.cpp', 'cpppj/src/o/RawContainedFinder.cpp',
           'cpppj/src/o/RawAiCarrierActions.cpp', 'cpppj/src/o/RawAiDropScan.cpp',
           'cpppj/src/o/SpStore.cpp')  # 실제 현재 기술/후보/난수와 최종 컴파일 입력이다.
TESTS = ('AiDropSelection_EmptyTechnologyOnlyWritesNativePrefix', 'AiDropSelection_CachesActualTechnologyAndCount',
         'AiDropSelection_CurrentDirectionalTypesAndRotation', 'AiDropSelection_SignedOddRotationAndOrdinalAdvance',
         'AiDropSelection_OrdinalWrapAndSignedRemainder', 'AiDropSelection_RepeatByteConsumesRandomBeforeOtherGates',
         'AiDropSelection_RepeatCompactsAndRechecksSameIndex', 'AiDropSelection_RepeatProbabilityBoundaries',
         'AiDropSelection_RepeatUsesCurrentTypeOwnerAndCanonicalFlags', 'AiDropSelection_BlockerPriorityOverridesRepeat',
         'AiDropSelection_CurrentBlockerRepeatKeepsPriorityFlag', 'AiDropSelection_BlockerAbsentAndRotationBudget',
         'AiDropSelection_ExactZeroScoreResetAndSignedFirstAttempt', 'AiDropSelection_CarrierAndOrderedSupplyCooldown',
         'AiDropSelection_MissingNthPreservesPartialWrites',
         'CampaignSession_AiDropSelectionUsesActualProductsScanAndLifetime')  # 새16개 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """정밀 명령·현재 TYPE/로그 상수와 실제 최종 빌드/연결을 정적으로 읽는다."""
    previous = json.loads((ROOT / 'cpppj/ai-drop-scan-static-20261022.json').read_text(encoding='utf8'))
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
    # 기존 정밀 범위 시작부터 해독해 명령 중간/이웃 함수 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 전체 명령과 해시를 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {
        '0x413100': ('[esi + 0xaf8]', '[esi + 0xafc]', '[esi + 0x3ec]', 'call 0x48fc90',
                     'push 8', 'call 0x4acc60', '00413175 push 0x64', 'call 0x4558c0',
                     'call 0x40e9d0', 'cmp edi, 0x50', 'cmp edi, 0x1e', 'call 0x4acb80',
                     '00413230 mov cl, byte ptr [esi + 0x260]', 'call 0x49b510', 'call 0x40f390',
                     '0041328b add ebx, 2', '00413296 dec ebx', 'call 0x425830', 'cmp eax, 6',
                     'add eax, 2', '004132c4 inc eax', 'idiv dword ptr [esp + 0x10]',
                     'call 0x4acac0', '00413314 mov dword ptr [esi + 0x570], ebp',
                     '00413323 jne 0x41332b', '00413325 mov dword ptr [esi + 0x3e8], ebp',
                     'setle cl', 'test eax, 0x30000', 'test ah, 8',
                     'fld qword ptr [0x55b4d0]', 'fcomp qword ptr [esi + 0x5b8]',
                     '00413389 test ah, 1', '0041338c je 0x41339e'),
        '0x425830': ('[0x541264]', '[0x54125c]', '[0x541288]', '[0x541290]'),
        '0x40e9d0': ('[ecx + 8]', 'cmp dword ptr [ecx], esi'),
        '0x4acac0': ('call 0x4b2270', 'movzx ebp, word ptr [edx + ebp + 0x12]',
                     'movzx ebp, word ptr [edx + ebp + 0x14]', 'dec ebx', 'call 0x4b1b20',
                     'movzx eax, word ptr [eax + edx + 0x12]'),
        '0x4162e0': ('call 0x413100', 'call 0x412e10', 'call 0x4108f0')
    }  # 캐시/확률·회전예산/현재후보/정확한0·signed·unordered정리의명령근거다.
    # 필수 근거가 다르면 성공 보고서를 기록하지 않는다.
    for entry, items in expected.items():
        # 지정 정밀 범위 안의 각 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    directions = [struct.unpack_from('<I', mapped, address-base)[0] for address in (0x541264,0x54125c,0x541288,0x541290)]
    drop_log = struct.unpack_from('<I', mapped, 0x52f3d4-base)[0]
    if directions != [131,129,140,142] or drop_log != 29:
        raise ValueError('현재 네 방향 TYPE/배치 로그 초기값 불일치')
    build_path = 'extracted/cpp-ai-drop-selection-20261022-build2.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-drop-selection-20261022-build2-result.json').read_text(encoding='utf-8-sig'))
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
    required = ('return CountContained(core.pool,contentFinder,parent,8,0);',
                'return FindContainedGenus(core.pool,contentFinder,core.types,parent,8,0x4000000,0);',
                'priests->Carrier().FindContained(parent,8,0,index)', 'static_cast<std::uint32_t>(raw[18])|(static_cast<std::uint32_t>(raw[19])<<8)',
                'selection.mayPlace=[this](const CanonPlacementQuery& query) { PrepareConstruction();return MayPlaceGameplay(query); }',
                'selection.remove=[this](auto token,auto index) { aiDropScan->Remove(token,index); }',
                'std::span<const std::uint32_t>(constructionState.confirm.patternTypes).subspan(4)',
                'std::uint32_t aiDropLog{29}', 'impl_->aiDropSelection->Select(token)')  # 같은 실제기술/전체배치·압축/네전역/로그의연결이다.
    # 각 현재 실제 하위와 명시 API 연결을 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: ' + item)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                directional_types=directions, initial_drop_log=drop_log, selection_attempts='cached_count+2',
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path), initial_build=dict(
                    result=json.loads((ROOT / 'extracted/cpp-ai-drop-selection-20261022-build1-result.json').read_text(encoding='utf-8-sig')),
                    log_sha256=digest('extracted/cpp-ai-drop-selection-20261022-build1.log'),
                    rebuild_reason='직전목록개수의진입cache를정밀명령과일치시킴')),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00413100 전체 배치TYPE선택/00425830 현재 네 방향TYPE/0040e9d0 직전목록첫일치',
                          '004acac0은기존실제4ac9f0뒤현재TYPE WORD조회로공유/같은실제후보압축·배치·공용난수'],
                additional_read=dict(entry='004162e0', implemented_in_this_unit=False),
                remaining=['004162e0 실제배치실행/004133e0·00415fe0 가짜부족좌표/다리·원래자동AI프레임·종료',
                           '15개미션+Edit/custom Fort·시나리오수명/자연플레이·표시/음향동일성'],
                limitations=['작성/컴파일/정적확인이며실행통과가아님', '전체x87예외/확장정밀도·CD10.72미검증',
                             '손상본체/목록/TYPE/30후보밖index·signed나눗셈오류는명시경계', '명시Select의true는몸체호출이며TYPE비영이아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
