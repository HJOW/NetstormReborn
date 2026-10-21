"""게임/native/C++ 검사 실행 없이 전체 AI 수집·포획과 최종 빌드를 정적으로 기록한다."""
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
REPORT = ROOT / 'cpppj/ai-carrier-actions-static-20261022.json'  # 과거 기록을 덮어쓰지 않는 신규 보고서다.
ENTRIES = (0x412130, 0x4acb80, 0x4487e0, 0x4adcb0, 0x40eab0, 0x40ea40, 0x492280,
           0x411ca0, 0x410260, 0x427320, 0x427070, 0x426150, 0x49a840, 0x4af170)  # 이번3개 복원 함수와 기존 실제 하위/초기화14개 근거다.
SOURCES = ('cpppj/src/o/RawAiCarrierActions.h', 'cpppj/src/o/RawAiCarrierActions.cpp',
           'cpppj/src/client/RawCampaignSession.h', 'cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt', 'cpppj/tests/CMakeLists.txt', 'cpppj/tests/AiCarrierActionsTests.cpp',
           'cpppj/tests/CampaignSessionTests.cpp', 'cpppj/src/o/RawAiMovement.cpp',
           'cpppj/src/o/RawAiResourceChoice.cpp', 'cpppj/src/o/RawCarrierDrop.cpp',
           'cpppj/src/o/RawContainedFinder.cpp', 'cpppj/src/o/RawBoardTarget.h')  # 실제 현재 컴파일/공유 하위 입력이다.
TESTS = ('AiCarrierActions_PrefixAndEmptyListOrder', 'AiCarrierActions_PrisonerQueuePriorityAndCollapse',
         'AiCarrierActions_ThinkTimerOrderedAndUnordered', 'AiCarrierActions_RoundRobinAndFreeStableRemoval',
         'AiCarrierActions_ReturningDiagnosticRereadsState', 'AiCarrierActions_ServerCaptureCrystalAndCurrentEnemy',
         'AiCarrierActions_CaptureAuthorityAfterCrystalLookup', 'AiCarrierActions_FlyingCaptureAndGraphFallback',
         'AiCarrierActions_CaptureShortCircuitAndTimerEffects', 'AiCarrierActions_DeliveryAndCollectionCurrentQueue',
         'AiCarrierActions_ActivePathProtectionAndMissingTarget', 'AiCarrierActions_GeyserPathRetentionAndCancellation',
         'AiCarrierActions_SelectedMissingEffectAndSignedCursor', 'AiCarrierActions_ContainedGenusWordKindAndFilterOrder',
         'AiCarrierActions_DaisPointCurrentDiagnosticAndFallback',
         'CampaignSession_AiCarrierActionsUseActualWorldContentsAndLifetime')  # 새16개 작성·컴파일 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 파일 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """보호 PE/정밀 명령과 최종 빌드·검사 링크·실제 Session 연결을 읽기만 한다."""
    previous = json.loads((ROOT / 'cpppj/ai-resource-choice-static-20261022.json').read_text(encoding='utf8'))
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
    # 기존 정밀 함수의 시작부터 해독해 명령 중간 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 원문 해시와 명령을 보존한다.
        for item in facts[entry]['ranges'].split(';'):
            first, last = (int(value, 16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first), last=hex(last), sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw, first)]))
        functions[hex(entry)] = blocks
    listings = {entry: '\n'.join(line for block in blocks for line in block['instructions']) for entry, blocks in functions.items()}
    expected = {'0x412130': ('call 0x4adcb0', 'call 0x4acb80', 'call 0x4487e0', 'call 0x4117e0',
                            'fcomp qword ptr [esi + 0x5a8]', 'test ah, 1', 'jne 0x41262a', 'idiv ecx',
                            'call 0x492280', 'call 0x411ca0', 'call 0x40eab0', 'call 0x410980',
                            'call 0x410260', 'call 0x4acc60', 'call 0x4ac9f0', 'call dword ptr [edx + 0x10]',
                            'call 0x427320', 'call 0x4116d0', 'call 0x4109d0', 'call 0x411900',
                            'call 0x426150', 'call 0x411350', 'call 0x411680'),
                '0x4acb80': ('movzx edx, word ptr', 'call 0x49a840', 'and eax, edi', 'cmp eax, edi',
                            'movzx ecx, word ptr', 'test ebx, ecx', 'dec ebp', 'call 0x4b1b20'),
                '0x4487e0': ('call 0x4e0620', 'test eax, 0x400000', 'fsub dword ptr [0x501908]',
                            '[0x545370]', '[0x545374]'),
                '0x40eab0': ('fcomp qword ptr [ecx]', 'test ah, 1', 'jne 0x40eac5'),
                '0x427320': ('push 0', 'call 0x427070'),
                '0x426150': ('call 0x488fe0', 'call 0x4b1a90', 'call 0x4ae2d0', '[eax + 0x186cc]', '[eax + 0x186d0]'),
                '0x49a840': ('mov eax, dword ptr [esp + 4]', 'imul eax, eax, 0x1f4'),
                '0x4af170': ('mov dword ptr [0x545374], 0xc61c4000', 'mov dword ptr [0x545370], 0xc61c4000')}  # 실제 흐름/폭/순서와 월드 초기값 근거다.
    # 필수 원본 명령이 없으면 성공 보고서를 만들지 않는다.
    for entry, items in expected.items():
        # 같은 함수 범위의 모든 필수 명령을 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: ' + entry + ' ' + item)
    if struct.unpack_from('<f', mapped, 0x501908-base)[0] != 3.0 or 'byte ptr' in listings['0x40ea40']:
        raise ValueError('다이스float3/물리유효성의 free 비트 비조회 계약 불일치')
    result = json.loads((ROOT / 'extracted/cpp-ai-carrier-actions-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    build_path = 'extracted/cpp-ai-carrier-actions-20261022-build1.log'
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
    required = ('FindContainedGenus(core.pool,priestState.contained,core.types,actor,0x20,0x200000,0)',
                'DaisApproachPoint(core.pool,core.types,gunState.checking,boardTargetState.invalid,services.ai.assertion,dais)',
                'return AiEnemyAvailable(owner);', 'return AiCarrierTargets(actor);', 'return AiCarrierTargets(sid);',
                'PrepareCarrierDrop();carrierDrop->Request(actor,Sid{})', 'world.Destroy(crystal,0)',
                'aiResourceChoice->RememberCaptureCarrier(token,actor)', 'impl_->aiCarrierActions->Act(token)')
    # 실제 하위 공유와 새 명시 API를 확인한다.
    for item in required:
        if item not in session:
            raise ValueError('같은 실제 Session 하위 누락: ' + item)
    return dict(hostname=socket.gethostname(), last_full_decompile_host='HJOW-Athlon', last_full_decompile_date='2026-10-21',
                edition='10.78', original_sha256=digest('originals/Netstorm.exe'), functions=functions,
                sources={name: digest(name) for name in SOURCES}, build=dict(result=result, log_sha256=digest(build_path)),
                binaries={name: digest(name) for name in (binary_path, 'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS), execution=dict(cpp_tests=0, ctest=0, game=0, original_native=0),
                restored=['00412130 전체 수집/포획/귀환/경로 유지·취소', '004acb80 내용물 genus/004487e0 다이스 접근점',
                          '같은 실제 내용물/권한/경로/삭제/요청/간헐천·포획 목록 연결', '현재 적 참여/경로 TargetA/B 공통 하위 공유'],
                remaining=['00411cd0 사제/004162e0 건물/00417bd0 다리/00415fe0 가짜부족좌표 행동',
                           '원래 순서 자동 AI 프레임/종료', '15개 미션+Edit/custom Fort·시나리오 자연 플레이/화면/음향·전환/저장 수명'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님', '전체 x87 정밀도/예외·CD10.72 미검증',
                             '손상된 물리 주소/음수 인덱스/IDIV overflow는 명시 경계', '명시API bool은 본체 존재/호출이며 포획 성공이 아님'])


def main():
    """이번 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)), functions=len(result['functions']),
                          compiled_tests=len(TESTS), execution=result['execution']), ensure_ascii=False))


if __name__ == '__main__':
    main()
