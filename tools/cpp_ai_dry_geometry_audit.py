"""게임/native/C++ 검사 실행 없이 가짜좌표 양방향 탐색의 원본 근거와 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 보호 원문/현재 입력/빌드 기준이다.
REPORT = ROOT / 'cpppj/ai-dry-geometry-static-20261022.json'  # 과거 기록과 구분한 신규 보고서다.
ENTRIES = (0x415bb0,0x41cfc0,0x414d30,0x410540,0x4ac220,0x4af530,0x4b28f0,0x4b2b50,0x415fe0)  # 이번4/기존실제하위·지도근거4/다음caller1이다.
SOURCES = ('cpppj/src/o/RawAiDryGeometry.h','cpppj/src/o/RawAiDryGeometry.cpp',
           'cpppj/src/client/RawCampaignSession.h','cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiDryGeometryTests.cpp','cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/SquidHash.cpp','cpppj/src/o/RawBuildingSpace.cpp')  # 같은 실제 지도/표식 수명과 최종 컴파일 입력이다.
TESTS = ('AiDryGeometry_DistanceFloatOperands','AiDryGeometry_HorizontalVerticalTie',
         'AiDryGeometry_RoundsCellAfterBias','AiDryGeometry_ResetsBothRunsAndChoosesRight',
         'AiDryGeometry_IncludesBoundsBeforeMoving','AiDryGeometry_OutOfGridAvoidsSurface',
         'AiDryGeometry_DebugTraceOrderAndCurrentKind','AiDryGeometry_MarkPartialWrites',
         'AiDryGeometry_ClearMatchesWordTypeAndDead','AiDryGeometry_ClearCachesNextBeforeDelete',
         'AiDryGeometry_ClearUsesCurrentMarkerType','AiDryGeometry_MissingSelectedSurface',
         'CampaignSession_AiDryGeometryUsesActualSurface')  # 새13개 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 전체 파일 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """원본 정밀 명령/편향·TYPE 전역과 최종 빌드·같은 실제 연결을 정적으로 확인한다."""
    previous = json.loads((ROOT / 'cpppj/ai-dry-seed-static-20261022.json').read_text(encoding='utf8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf8') as stream:
        facts = {int(row['entry'],16): row for row in csv.DictReader(stream,delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    functions = {}
    # 정밀 범위 시작부터 해독해 이웃 함수/명령 중간 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 전체 해시와 명령을 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first,last = (int(value,16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first),last=hex(last),sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw,first)]))
        functions[hex(entry)] = blocks
    listings = {entry:'\n'.join(line for block in blocks for line in block['instructions']) for entry,blocks in functions.items()}
    expected = {
        '0x415bb0': ('call 0x414d30','call 0x41cfc0','00415c41 fstp dword ptr [esp + 0x10]',
                    '00415c4f fdiv dword ptr [esp + 0x10]','00415c75 fstp dword ptr [esp + 0x18]',
                    '00415c87 fstp dword ptr [esp + 0x14]','fmul dword ptr [0x500ee4]',
                    '00415d69 jge 0x415fb0','00415d8a jp 0x415e4f','fadd dword ptr [0x500edc]',
                    'call 0x4e49c0','mov edx, dword ptr [0x542514]',
                    '00415e4b inc dword ptr [esp + 0x10]','00415f27 inc dword ptr [esp + 0x54]',
                    '00415f93 jl 0x415d64','00415fa3 mov dword ptr [eax], ebp','00415fd0 mov dword ptr [eax], edi'),
        '0x41cfc0': ('fsub dword ptr [esp + 8]','fsub dword ptr [esp + 4]','fsqrt'),
        '0x414d30': ('mov dword ptr [esp + 0xc], 1','call 0x40eb40','00414d8f inc esi',
                    'test byte ptr [ecx], 1','mov ecx, dword ptr [0x5411f8]',
                    'movzx edx, byte ptr [eax + edi + 0xa]','cmp edx, ecx',
                    'cmp word ptr [eax + 0xc], 0x5d','call dword ptr [edx + 0x10]','jmp 0x414d58'),
        '0x410540': ('push 2','call 0x4af530','push 0x5d','call 0x4ac220','call dword ptr [edx + 0x90]'),
        '0x4ac220': ('movzx edx, byte ptr [ecx + 0xa]','mov word ptr [ecx + 0xc], ax',
                    'cmp edx, dword ptr [0x5411a0]','call 0x4214a0'),
        '0x4b28f0': ('mov eax, dword ptr [esi + 0xc]','mov dword ptr [0x5c84bc], eax'),
        '0x4b2b50': ('mov ecx, 0x542508','jmp 0x4b28f0'),
        '0x415fe0': ('call 0x415bb0','call 0x410540')
    }  # 4연속/동률·현재 경계/전체DWORD TYPE 비교·실제 지도 identity의 명령 근거다.
    # 필수 원문 명령이 다르면 성공 보고서를 기록하지 않는다.
    for entry,items in expected.items():
        # 지정 함수 범위 안의 각 명령만 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: '+entry+' '+item)
    bias = struct.unpack_from('<f',mapped,0x500edc-base)[0]
    marker = struct.unpack_from('<I',mapped,0x5411f8-base)[0]
    debug = struct.unpack_from('<I',mapped,0x52f3dc-base)[0]
    if mapped[0x500edc-base:0x500edc-base+4].hex() != '72f97f3f' or marker != 104 or debug != 99:
        raise ValueError('원본 편향/표식/디버그 초기값 불일치')
    build_path = 'extracted/cpp-ai-dry-geometry-20261022-build3.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-dry-geometry-20261022-build3-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+',log,re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 실제 새 이름의 링크만 확인하고 검사 바이너리를 실행하지 않는다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: '+name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    required = ('geometry.surface=[this](auto x,auto y) { return core.hash.Cell(0,x,y); }',
                'geometry.create=[this](auto type,auto flags) { return world.Create(type,flags); }',
                'geometry.pop=[this](Sid sid,RawPathPoint point,auto flags) { world.Pop(sid,point.x,point.y,flags); }',
                'geometry.destroy=[this](Sid sid,auto flags) { world.Destroy(sid,flags); }',
                'geometry.word=[this](Sid sid,auto word) { auto raw=core.pool.AllocatedBytes(sid);raw[12]=static_cast<std::uint8_t>(word);raw[13]=static_cast<std::uint8_t>(word>>8);if (raw[10]==core.bridgeState.bridgeType) core.building.NotifySurface(sid); };',
                'AiDryGeometryGlobals{aiDropScanBounds,aiDrySpotLog,aiDryMarkerType}',
                'impl_->aiDryGeometry->Candidate(source,target)')  # 같은 실제 지도/수명/현재 전역 연결이다.
    # 누락된 실제 하위를 단순 성공으로 취급하지 않는다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: '+item)
    source = (ROOT / 'cpppj/src/o/RawAiDryGeometry.cpp').read_text(encoding='utf8')
    if 'raw[10]==globals_.markerType' not in source:
        raise ValueError('표식 TYPE의 전체 DWORD 비교 누락')
    return dict(hostname=socket.gethostname(),last_full_decompile_host='HJOW-Athlon',last_full_decompile_date='2026-10-21',
                edition='10.78',original_sha256=digest('originals/Netstorm.exe'),functions=functions,
                cell_bias=bias,initial_marker_type=marker,initial_debug_kind=debug,
                sources={name:digest(name) for name in SOURCES},build=dict(result=result,log_sha256=digest(build_path),
                    initial_build=dict(result=json.loads((ROOT / 'extracted/cpp-ai-dry-geometry-20261022-build1-result.json').read_text(encoding='utf-8-sig')),
                                       log_sha256=digest('extracted/cpp-ai-dry-geometry-20261022-build1.log'),
                                       rebuild_reason='표식 TYPE 전역의 하위BYTE 변환을 제거하고 원본 전체DWORD 비교/상위비트 검사를 적용'),
                    second_build=dict(result=json.loads((ROOT / 'extracted/cpp-ai-dry-geometry-20261022-build2-result.json').read_text(encoding='utf-8-sig')),
                                      log_sha256=digest('extracted/cpp-ai-dry-geometry-20261022-build2.log'),
                                      rebuild_reason='실제kind WORD 쓰기의 다리 통지도 원본 전체DWORD 비교와 일치시킴')),
                binaries={name:digest(name) for name in (binary_path,'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS),execution=dict(cpp_tests=0,ctest=0,game=0,original_native=0),
                restored=['00415bb0 전체 양방향 탐색/0041cfc0 거리/00414d30 표식 삭제/00410540 실제 표식 생성·Pop'],
                additional_read=dict(entry='00415fe0',implemented_in_this_unit=False),
                remaining=['00415fe0 전체 가짜좌표/004162e0 실제 배치행동/다리·원래 자동AI프레임·종료',
                           '15개 미션+Edit/custom Fort·시나리오 수명/자연 플레이·표시·음향 동일성'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님','전체 x87 확장 정밀도/예외·CD10.72 미검증',
                             '원본처럼 반복 예산이 없어 경계 밖/퇴화 선분에서는 진행이 정지할 수 있음',
                             '현재542514는같은해시0배열/고정5c84bc와분리교체하는추가환경은미검증'])


def main():
    """이번 신규 정적 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)),functions=len(result['functions']),
                          compiled_tests=len(TESTS),execution=result['execution']),ensure_ascii=False))


if __name__ == '__main__':
    main()
