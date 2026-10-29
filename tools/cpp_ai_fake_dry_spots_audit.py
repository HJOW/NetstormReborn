"""실제 게임/native/C++ 검사 실행 없이 전체 가짜 부족좌표의 원본 근거와 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 같은 저장소의 보호 원문/현재 소스·빌드 기준이다.
REPORT = ROOT / 'cpppj/ai-fake-dry-spots-static-20261022.json'  # 이전 보고서를 덮어쓰지 않는 이번 기록이다.
ENTRIES = (0x415fe0,0x40f260,0x40f1c0,0x412640,0x4adea0,0x415bb0,0x40e800,
           0x41cfc0,0x4133e0,0x410540,0x4105b0,0x4162e0)  # 이번1/기존실제하위10/다음caller1이다.
SOURCES = ('cpppj/src/o/RawAiFakeDrySpots.h','cpppj/src/o/RawAiFakeDrySpots.cpp',
           'cpppj/src/client/RawCampaignSession.h','cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiFakeDrySpotsTests.cpp','cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/RawAiDrySpots.cpp','cpppj/src/o/RawAiDropEvaluation.cpp',
           'cpppj/src/o/RawAiDryGeometry.cpp','cpppj/src/o/RawAiPriestActions.cpp',
           'cpppj/src/o/RawSquidNeighbors.cpp')  # 같은 실제 메모리/거리/경로/등록·난수와 최종 컴파일 입력이다.
TESTS = ('AiFakeDrySpots_ZeroTypeSkipsFullAndAllEffects','AiFakeDrySpots_FullRemovalPrecedesMissingTemple',
         'AiFakeDrySpots_TempleUsesRawPointAndIgnoresDuplicate','AiFakeDrySpots_CachedSupplyListAndFirstMaximum',
         'AiFakeDrySpots_CurrentDryPointerAfterCenter','AiFakeDrySpots_RadiusOrderedGateUsesOriginalCenter',
         'AiFakeDrySpots_InvalidCandidateHasNoDiscardLog','AiFakeDrySpots_NegativeCountFallsBackWithoutLookup',
         'AiFakeDrySpots_AddFailureStillUpdatesStrategyAndMarks','AiFakeDrySpots_SeedAfterMarkKeepsOriginalLogPoint',
         'AiFakeDrySpots_CurrentTypeAndDebugAfterCandidate','AiFakeDrySpots_SelectedMissingPreservesPartialWrites',
         'CampaignSession_AiFakeDrySpotsUseActualStateAndLifetime')  # 새13개 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """정밀 범위 명령/실제 상수·현재 연결과 최종 빌드를 정적으로 확인한다."""
    previous = json.loads((ROOT / 'cpppj/ai-dry-geometry-static-20261022.json').read_text(encoding='utf8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf8') as stream:
        facts = {int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    functions = {}
    # 기존 정밀 범위의 시작에서 해독해 명령 중간/이웃 함수 진입을 피한다.
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
        '0x415fe0': ('00415fe7 mov eax, dword ptr [esi + 0x280]','00415ff1 je 0x4162da',
                    '00416003 cmp eax, ecx','0041600c jl 0x416016','call 0x40f260',
                    '00416022 jne 0x416066','00416024 mov eax, dword ptr [esi + 0x294]',
                    '00416074 jle 0x41624f','0041607a mov ecx, dword ptr [esi + 0x2fc]',
                    '00416081 mov ebx, dword ptr [esi + 0x2f4]','00416090 mov edi, dword ptr [ebx]',
                    'call 0x412640','004160bd jne 0x4160cd','call 0x4adea0',
                    '0041610c mov edx, dword ptr [esi + 0xae4]','fsub dword ptr [0x5012d0]',
                    '0041612f mov ecx, dword ptr [esp + 0x1c]','0041614e mov edx, dword ptr [esp + 0x1c]',
                    '00416153 push ecx','00416154 push edx','call 0x415bb0','call 0x40e800',
                    'call 0x41cfc0','fmul dword ptr [0x501784]','004161a7 jne 0x416227',
                    '004161c0 call 0x40f1c0','004161c5 cmp dword ptr [esi + 0x2d4], 4',
                    '004161d4 mov dword ptr [esi + 0xaf0], eax','004161da mov dword ptr [esi + 0xaf4], ecx',
                    '004161ec mov dword ptr [esi + 0x3cc], 0','004161f6 mov dword ptr [esi + 0x3d4], edx',
                    '004161fc mov dword ptr [esi + 0x3d0], edx','call 0x410540',
                    '00416260 jp 0x4162a1','004162a3 call 0x4133e0','004162ae je 0x4162d9',
                    '004162b4 call 0x4e49c0','004162be call 0x4e49c0'),
        '0x4162e0': ('call 0x415fe0','call 0x4133e0')
    }  # 선행 제거/정수SID·cache/중심Y 중복/ordered 거리·전략 쓰기/보충·로그 순서의 근거다.
    # 필수 명령이 보호 PE에 없으면 성공 보고서를 기록하지 않는다.
    for entry,items in expected.items():
        # 지정된 실제 함수 범위 안의 각 명령만 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: '+entry+' '+item)
    factor = struct.unpack_from('<f',mapped,0x501784-base)[0]
    if mapped[0x501784-base:0x501784-base+4].hex() != '3333333f':
        raise ValueError('원본 float0.7 상수 불일치')
    build_path = 'extracted/cpp-ai-fake-dry-spots-20261022-build2.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-fake-dry-spots-20261022-build2-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+',log,re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 실제 링크된 새 이름만 확인하며 바이너리를 실행하지 않는다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: '+name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    required = ('return aiDropEvaluation->DryDistance(token,point,index);',
                'const auto center=core.neighbors.Center(sid);return RawPathPoint{center[0],center[1]};',
                'return aiDryGeometry->Candidate(source,target);','return aiDrySpots->Add(token,point.x,point.y,type);',
                'fake.remove=[this](auto token,auto index) { aiDrySpots->Remove(token,index); }',
                'fake.seed=[this](auto token) { aiDrySpots->Seed(token,random); }',
                'fake.mark=[this](RawPathPoint point) { aiDryGeometry->Mark(point); }',
                'AiFakeDryGlobals{cursorManaGlobals.radiusLimit,cursorManaGlobals.mapLimit,aiDrySpotLog}',
                'impl_->aiFakeDrySpots->Extend(token)')  # 같은 실제 하위/전역/명시 API 연결이다.
    # 같은 월드와 연결되지 않은 대체 성공을 허용하지 않는다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: '+item)
    return dict(hostname=socket.gethostname(),last_full_decompile_host='HJOW-Athlon',last_full_decompile_date='2026-10-21',
                edition='10.78',original_sha256=digest('originals/Netstorm.exe'),functions=functions,radius_factor=factor,
                sources={name:digest(name) for name in SOURCES},build=dict(result=result,log_sha256=digest(build_path),
                    initial_build=dict(result=json.loads((ROOT / 'extracted/cpp-ai-fake-dry-spots-20261022-build1-result.json').read_text(encoding='utf-8-sig')),
                                       log_sha256=digest('extracted/cpp-ai-fake-dry-spots-20261022-build1.log'),
                                       rebuild_reason='현재 Dry pointer 검사에서 선택 전 index 유효성과 중심 뒤 count 축소를 명시함')),
                binaries={name:digest(name) for name in (binary_path,'cpppj/build/bin/Release/Netstorm.exe')},
                compiled_tests=list(TESTS),execution=dict(cpp_tests=0,ctest=0,game=0,original_native=0),
                restored=['00415fe0 전체 가짜 부족좌표 갱신/같은 실제 점수·중심·양방향 탐색/목록·공용 난수·표식/원래 로그'],
                additional_read=dict(entry='004162e0',implemented_in_this_unit=False),
                remaining=['004162e0 실제 배치행동/다리·원래 자동AI프레임·종료',
                           '목표15개 미션+Edit/custom Fort·시나리오 수명/자연 플레이·표시·음향 동일성'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님','전체 x87 확장 정밀도/예외·CD10.72 미검증',
                             '손상 본체/목록/SID/index의 물리 범위는 명시 오류',
                             '원래 양방향 탐색은 반복 예산이 없고 경계 밖/퇴화 선분에서 정지할 수 있음',
                             '명시 Extend API의 true는 본체 호출이며 등록 성공이 아님'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)),functions=len(result['functions']),
                          compiled_tests=len(TESTS),execution=result['execution']),ensure_ascii=False))


if __name__ == '__main__':
    main()
