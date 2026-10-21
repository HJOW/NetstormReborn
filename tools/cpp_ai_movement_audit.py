"""원본 게임/native/C++ 검사 실행 없이 AI 이동·이름 근거와 최종 빌드를 기록한다."""
from pathlib import Path
import csv
import hashlib
import json
import re
import socket
import struct
import capstone
import pefile

ROOT = Path(__file__).resolve().parents[1]  # 저장소/보호 PE/이번 빌드의 읽기 기준이다.
REPORT = ROOT / 'cpppj/ai-movement-static-20261022.json'  # 과거 보고서와 구분하는 신규 기록이다.
ENTRIES = (0x411630, 0x411680, 0x4116d0, 0x4117e0, 0x4118a0, 0x41d840,
           0x40a850, 0x488fe0, 0x4b1a90, 0x4894e0, 0x489080, 0x494eb0,
           0x4ace20, 0x49af80, 0x454d30, 0x4a8980, 0x4de380)  # 복원한 이동/이름과 기존 실제 경로·의식의17개 근거다.
SOURCES = ('cpppj/src/o/RawAiMovement.h', 'cpppj/src/o/RawAiMovement.cpp', 'cpppj/src/o/RawSquidName.h',
           'cpppj/src/o/RawSquidName.cpp', 'cpppj/src/o/SquidNameVtables.inc', 'cpppj/src/o/RawCarrierMove.h',
           'cpppj/src/o/RawCarrierMove.cpp', 'cpppj/src/o/CMakeLists.txt', 'cpppj/src/client/RawCampaignSession.h',
           'cpppj/src/client/RawCampaignSession.cpp', 'cpppj/src/client/CampaignClientServices.cpp',
           'cpppj/src/client/RawObjectMenuInfo.cpp', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/AiMovementTests.cpp',
           'cpppj/tests/SquidNameTests.cpp', 'cpppj/tests/CarrierMoveTests.cpp', 'cpppj/tests/CampaignSessionTests.cpp')  # 실제 현재 게임/Client/검사 컴파일 입력이다.
TESTS = ('AiMovement_CurrentPathLookupAndRemovalArguments', 'AiMovement_ServerTargetBranchesAndNameOrder',
         'AiMovement_ClientTargetUsesActualBoardAndPacket', 'AiMovement_AuthorityRereadAfterSend',
         'AiMovement_PointStartAndFtolLogging', 'AiMovement_HomeBoundaryUnorderedAndCurrentTemple',
         'AiMovement_FootprintDistanceSignedAndUnordered', 'AiMovement_SelectedMissingEffectsKeepPrefix',
         'SquidName_BaseLanguageReadOrderAndSubstitution', 'SquidName_FactorySuffixAfterBaseAndCurrentLevel',
         'SquidName_CurrentTypeFieldAndSelectedBounds', 'SquidName_EntriesRomanSentinelAndCrtSuffix',
         'CarrierMove_SharedWriterKeepsRawBitsAndTail', 'CampaignSession_AiMovementUsesRealPathNamesAndStop')  # 새14개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(path):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def collect():
    """보호 PE 정밀 범위/상수·가상 표와 최종 빌드/링크·실제 연결을 읽기만 한다."""
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    if pe.FILE_HEADER.Machine != 0x14c:
        raise ValueError('보호 원본 x86 PE가 아닙니다')
    previous = json.loads((ROOT / 'cpppj/ai-dry-spots-static-20261022.json').read_text(encoding='utf-8'))
    if digest('originals/Netstorm.exe') != previous['original_sha256']:
        raise ValueError('직전 보호 원본 SHA와 불일치')
    base = pe.OPTIONAL_HEADER.ImageBase
    mapped = pe.get_memory_mapped_image()
    with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    functions = {}
    # 이번 실제 함수의 불연속 정밀 범위를 독립 해시/명령으로 보존한다.
    for entry in ENTRIES:
        blocks = []
        # 이웃 함수로 범위를 넓히지 않는다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            code = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(code).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(code, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x411630': ('push 0', 'call 0x488fe0', 'call 0x4b1a90'),
                '0x411680': ('push 1', 'push 0', 'call 0x488fe0', 'call 0x4b1a90'),
                '0x4116d0': ('call 0x4ade70', 'call 0x41d530', 'call 0x40a850', 'call 0x4e2dc0',
                            'call 0x4894e0', 'call 0x494eb0', 'call dword ptr [edx + 0x68]', 'call dword ptr [eax + 0x68]', 'call 0x4105b0'),
                '0x4117e0': ('call 0x40a850', 'call 0x4e2dc0', 'call 0x489080', 'call 0x4e49c0', 'call dword ptr [eax + 0x68]', 'call 0x4105b0'),
                '0x4118a0': ('call 0x41d840', 'fcomp dword ptr [0x501444]', 'test ah, 5', 'jnp 0x4118ee', 'call 0x4116d0'),
                '0x41d840': ('fstp dword ptr [esp + 0x1c]', 'test ah, 0x41', 'je 0x41d8b1'),
                '0x4ace20': ('call 0x49a840', 'jmp 0x49af80'), '0x49af80': ('call 0x4de380', 'call 0x4de9a0', 'call 0x441740'),
                '0x454d30': ('call 0x4ace20', 'call 0x4de9a0', 'call 0x4a8980', 'call 0x4a89c0')}  # 구현한 실제 폭/호출·unordered 분기다.
    # 보호 PE에 필수 명령이 없으면 성공으로 기록하지 않는다.
    for entry, items in expected.items():
        # 같은 함수 정밀 범위 안에서 모든 하위를 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<f', mapped, 0x501444-base)[0] != 2.0 or struct.unpack_from('<I', mapped, 0x52f3d8-base)[0] != 99:
        raise ValueError('원본 복귀float2/이동로그99 불일치')
    table = (ROOT / 'cpppj/src/o/SquidNameVtables.inc').read_text(encoding='utf-8')
    names = [(int(first, 16), int(second, 16)) for first, second in re.findall(r'\{0x([0-9a-f]+)U,0x([0-9a-f]+)U\}', table)]
    if len(names) != 83 or {entry for _, entry in names} != {0x4ace20, 0x454d30}:
        raise ValueError('실제83개 이름 가상 표 계약 불일치')
    # 생성 표의 모든+68을 원본 PE에서 독립 재조회한다.
    for address, entry in names:
        if struct.unpack_from('<I', mapped, address+0x68-base)[0] != entry:
            raise ValueError('보호 PE 이름 가상 표 불일치: ' + hex(address))
    build_path = 'extracted/cpp-ai-movement-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-movement-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
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
    if session.index('ConnectAiMovement();') < session.index('boardTarget=std::make_unique<RawBoardTarget>'):
        raise ValueError('실제 Motion/Board 설치보다 먼저 AI 이동이 연결됐습니다')
    if 'return StartMotion(request);' not in session or 'carrierTarget->StartTarget(actor,target,replace,helper)' not in session or 'sacrificeRequest->Begin(actor,target)' not in session:
        raise ValueError('같은 실제 경로/의식 연결 누락')
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21', edition='10.78',
                original_sha256=digest('originals/Netstorm.exe'), functions=functions, name_entries=[(hex(address), hex(entry)) for address, entry in names],
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00411630/00411680 조회·정지/004116d0·004117e0 대상·좌표명령/004118a0 복귀',
                          '0041d840 거리/공유40h writer/실제BFS·Host·의식 연결', '83개 실제+68/공통·Factory 이름/현재언어·설정·레벨문자표'],
                remaining=['사제·건물·다리·수집/포획 판단과 가짜부족좌표 선정', '원래 순서 자동 AI 프레임/종료',
                           '15개 미션+Edit/custom Fort·시나리오 자연 입력/플레이·화면/음향 동일성', 'Template/State7·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '명시API의bool은호출여부이며경로생성/도착성공이아님',
                             '일부 기존 실제 부모경로38/41/42·미확인 가상표는 명시 경계', '전체 x87 정밀도/예외·잘못된CRT서식·CD10.72 미검증'])


def main():
    """이번 신규 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']), name_entries=len(result['name_entries']), compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
