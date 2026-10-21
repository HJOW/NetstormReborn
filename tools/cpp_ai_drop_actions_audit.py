"""게임/native/C++ 검사 실행 없이 AI 전체 배치 행동의 원본 근거와 최종 빌드를 기록한다."""
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
REPORT = ROOT / 'cpppj/ai-drop-actions-static-20261022.json'  # 과거 기록과 구분한 신규 보고서다.
ENTRIES = (0x4162e0,0x411a10,0x410580,0x4103c0,0x413100,0x412e10,0x410280,
           0x4108f0,0x415fe0,0x4133e0,0x40f390,0x40e800,0x4558c0,0x49b510,0x4105b0,0x417c70)  # 이번2/기존실제하위13/다음caller1이다.
SOURCES = ('cpppj/src/o/RawAiDropActions.h','cpppj/src/o/RawAiDropActions.cpp',
           'cpppj/src/client/RawCampaignSession.h','cpppj/src/client/RawCampaignSession.cpp',
           'cpppj/src/o/CMakeLists.txt','cpppj/tests/CMakeLists.txt',
           'cpppj/tests/AiDropActionsTests.cpp','cpppj/tests/CampaignSessionTests.cpp',
           'cpppj/src/o/RawAiDropSelection.cpp','cpppj/src/o/RawAiDropScan.cpp',
           'cpppj/src/o/RawAiDeployment.cpp','cpppj/src/o/RawAiCollectors.cpp',
           'cpppj/src/o/RawAiStrategySetup.cpp','cpppj/src/o/RawAiFakeDrySpots.cpp',
           'cpppj/src/o/RawAiDrySpots.cpp','cpppj/src/o/SpStore.cpp')  # 같은 실제 선택·스캔·생산/비용/배치와 최종 컴파일 입력이다.
TESTS = ('AiDropActions_SelectionZeroStopsAtNativeBoundary','AiDropActions_LostAbilityDoesNotForceSelection',
         'AiDropActions_ScanSnapshotAndCompletionBeforeGates','AiDropActions_OrderedQwordDeadlines',
         'AiDropActions_SignedWalletMinimum','AiDropActions_FirstAttemptAndPartialScan',
         'AiDropActions_CurrentPendingBitOnly','AiDropActions_PriestProgressAndStrictTarget',
         'AiDropActions_NoChoiceAssertionUsesCurrentType','AiDropActions_CannotAffordPreservesPriorState',
         'AiDropActions_RandomRejectionsUseActualCompression','AiDropActions_AllChoicesInvalidKeepFocus',
         'AiDropActions_CurrentQueriesLogsAndDeployArguments','AiDropActions_FocusUsesLowByteOnly',
         'AiDropActions_RemoveEffectControlsStopping','AiDropActions_AffordMutationClearsWithoutRandom',
         'AiDropActions_MissingEffectsPreservePartialWrites','AiDropActions_DisabledLogStillReadsPhysicalTarget',
         'CampaignSession_AiDropActionsUseActualSelectionScanAndWallet')  # 새19개 이름이며 실행 통과 수가 아니다.


def digest(name):
    """현재 전체 파일 바이트의 SHA256을 읽는다."""
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()


def collect():
    """원본 정밀 명령/관문·호출과 최종 빌드/같은 실제 연결을 정적으로 확인한다."""
    previous = json.loads((ROOT / 'cpppj/ai-fake-dry-spots-static-20261022.json').read_text(encoding='utf8'))
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
    # 정밀 범위 시작에서 해독해 이웃 함수/명령 중간 진입을 피한다.
    for entry in ENTRIES:
        blocks = []
        # 불연속 범위도 각각 전체 해시와 명령을 기록한다.
        for item in facts[entry]['ranges'].split(';'):
            first,last = (int(value,16) for value in item.split('-'))
            raw = mapped[first-base:last-base+1]
            blocks.append(dict(first=hex(first),last=hex(last),sha256=hashlib.sha256(raw).hexdigest(),
                               instructions=[f'{part.address:08x} {part.mnemonic} {part.op_str}' for part in decoder.disasm(raw,first)]))
        functions[hex(entry)] = blocks
    listings = {entry:'\n'.join(line for block in blocks for line in block['instructions']) for entry,blocks in functions.items()}
    expected = {
        '0x4162e0': ('call 0x4103c0','call 0x413100','lea edx, [esi + 0x3e8]','call 0x412e10',
                    '0041637a mov dword ptr [esi + 0x3ec], 1','fld qword ptr [0x55b4d0]',
                    'fcomp qword ptr [esi + 0x5a8]','004163b3 test ah, 1','fcomp qword ptr [esi + 0x5e8]',
                    'cmp dword ptr [esi + 0x2d8], 0x4b0','004163dd jl 0x4166d2','call 0x410580',
                    'cmp eax, dword ptr [esi + 0x5a0]','0041641d jl 0x4166d2','call 0x40e800',
                    'call 0x410280','00416475 je 0x41662f','0041647b mov dword ptr [esi + 0x3f4], 0',
                    '00416485 mov dword ptr [esi + 0x3fc], 0','call 0x4558c0','call 0x411a10',
                    '004164c7 fld dword ptr [esi + edi*8 + 0x40c]',
                    '00416549 mov eax, dword ptr [esi + edi*4 + 0x4f8]',
                    '00416559 mov edx, dword ptr [eax + ebp + 0x12]',
                    '0041655d mov ecx, dword ptr [eax + ebp + 0xe]',
                    '00416563 movzx ebp, byte ptr [eax + 0xa]',
                    '004165ad mov ecx, dword ptr [esi + 0x3e4]',
                    '004165ba mov eax, dword ptr [esi + edi*8 + 0x408]','call 0x4108f0',
                    'test byte ptr [esi + 0x248], 0x40','004165de mov dword ptr [esi + 0x3f4], 1',
                    '004165ef mov dword ptr [esi + 0x3fc], edx','004165fe mov dword ptr [esi + 0x3f8], eax',
                    'call 0x40f390','00416618 je 0x416490','push 0xf8b','call 0x415fe0','call 0x4133e0'),
        '0x411a10': ('mov al, byte ptr [ecx + 0x260]','push 1','push 0','call 0x49a840','call 0x49b510','ret 0xc'),
        '0x410580': ('call 0x4103c0','0x15','test'),
        '0x417c70': ('call 0x415a10','call 0x4136b0','call 0x411cd0','call 0x4162e0','call 0x417bd0','call 0x412130','call 0x4138c0')
    }  # ordered기한/1200·signed/현재인자/후보난수/실제표적raw/압축뒤성공의 원문 근거다.
    # 원문 근거가 다르면 성공 보고서를 기록하지 않는다.
    for entry,items in expected.items():
        # 각 지정 정밀 함수 범위 안에서만 확인한다.
        for item in items:
            if item not in listings[entry]:
                raise ValueError('원본 명령 누락: '+entry+' '+item)
    drop_log = struct.unpack_from('<I',mapped,0x52f3d4-base)[0]
    if drop_log != 29:
        raise ValueError('원본 배치 로그 초기값 불일치')
    build_path = 'extracted/cpp-ai-drop-actions-20261022-build1.log'
    result = json.loads((ROOT / 'extracted/cpp-ai-drop-actions-20261022-build1-result.json').read_text(encoding='utf-8-sig'))
    log = (ROOT / build_path).read_text(encoding='utf-8-sig')
    if result['exit_code'] or re.search(r'\b(?:warning|error) [A-Z]+\d+',log,re.I):
        raise ValueError('최종 빌드 종료/경고/오류를 확인해야 합니다')
    binary_path = 'cpppj/build/bin/Release/netstorm_tests.exe'
    binary = (ROOT / binary_path).read_bytes()
    # 검사를 실행하지 않고 새 이름의 실제 링크만 확인한다.
    for name in TESTS:
        if name.encode('ascii') not in binary:
            raise ValueError('새 검사 이름 누락: '+name)
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text(encoding='utf-8-sig')
    required = ('drops.product=[this](auto token,auto type) { return aiStrategySetup->FindProduct(token,type); }',
                'drops.select=[this](auto token) { aiDropSelection->Select(token); }',
                'drops.scan=[this](auto token,auto type,auto direction) { return aiDropScan->Scan(token,type,direction); }',
                'drops.canAfford=[this](auto token,auto type) { return aiDeployment->CanAfford(token,type); }',
                'drops.mayPlace=[this](const CanonPlacementQuery& query) { PrepareConstruction();return MayPlaceGameplay(query); }',
                'aiCollectors->DeployWithNotice(token,type,point.x,point.y,argument,direction)',
                'drops.fake=[this](auto token) { aiFakeDrySpots->Extend(token); }',
                'drops.seed=[this](auto token) { aiDrySpots->Seed(token,random); }',
                'drops.remove=[this](auto token,auto index) { aiDropScan->Remove(token,index); }',
                'AiDropActionGlobals{core.processState.now,cursorManaGlobals.mapLimit,gunState.checking}',
                'impl_->aiDropActions->Run(token)')  # 별도 생산/구매 성공/좌표 사본 없는 실제 연결이다.
    # 실제 하위 연결을 빠뜨린 단순 성공을 허용하지 않는다.
    for item in required:
        if item not in session:
            raise ValueError('실제 Session 연결 누락: '+item)
    return dict(hostname=socket.gethostname(),last_full_decompile_host='HJOW-Athlon',last_full_decompile_date='2026-10-21',
                edition='10.78',original_sha256=digest('originals/Netstorm.exe'),functions=functions,initial_drop_log=drop_log,
                sources={name:digest(name) for name in SOURCES},build=dict(result=result,log_sha256=digest(build_path)),
                binaries={name:digest(name) for name in (binary_path,'cpppj/build/bin/Release/NetstormCpp.exe')},
                compiled_tests=list(TESTS),execution=dict(cpp_tests=0,ctest=0,game=0,original_native=0),
                restored=['004162e0 전체 배치행동/00411a10 원래 전체 배치 query wrapper',
                          '같은 실제 기술/선택·스캔/비용·배치/부족좌표·압축·공용 난수/원래로그'],
                additional_read=dict(entry='00417c70',implemented_in_this_unit=False),
                remaining=['00417bd0 다리 전략/00417c70 원래 전체 순서·자동AI프레임·종료',
                           '목표15개+Edit/custom Fort·시나리오 수명/자연 플레이·표시·음향 동일성'],
                limitations=['작성/컴파일/정적 확인이며 실행 통과가 아님','전체 x87 확장 정밀도/예외·CD10.72 미검증',
                             '손상 본체/TYPE/SID/30후보 밖 index는 명시 물리 오류',
                             'Session 검사는 실제 선택·스캔·1199 지갑 관문까지이며 실제 완주/전투·배치 실행 증거가 아님',
                             '명시 Manage API의 true는 본체 호출이며 실제 배치 성공이 아님'])


def main():
    """이번 신규 정적 보고서만 UTF-8로 기록하고 실행0 범위를 요약한다."""
    result = collect()
    REPORT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print(json.dumps(dict(report=str(REPORT.relative_to(ROOT)),functions=len(result['functions']),
                          compiled_tests=len(TESTS),execution=result['execution']),ensure_ascii=False))


if __name__ == '__main__':
    main()
