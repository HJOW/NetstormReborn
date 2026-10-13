#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초 실행의 Player 설정·Burning 변경영역 교정을 이전 전체 소스와 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from analyze_campaign_runtime15 import ROOT, HISTORY, audit_history

REPORT = ROOT / 'cpppj/run15-corrections.json'  # 원본 격자를 변경하지 않은 후속 교정 자료다.
SOURCE = 'cpppj/src/o/RawBurningFrame.cpp'  # SetRect 카메라·1픽셀 여백의 실제 원본 계산을 복원한다.
TEST = 'cpppj/tests/PlayerRegularTests.cpp'  # 잘못된 기본 연결번호0을 보호 TYPE의 실제 번호로 교체한다.
OLD_RECT = '''// SetRect의 dirty wrapper는 camera를 빼지 않고 현재 viewport로 양끝을 제한한다.
void RawBurningFrame::Invalidate(float left,float top,float right,float bottom) const {
    const auto& view=state_.viewport;
    SquidDisplayRect rect{std::clamp(FrameInteger(left),view.left,view.right),std::clamp(FrameInteger(top),view.top,view.bottom),
        std::clamp(FrameInteger(right),view.left,view.right),std::clamp(FrameInteger(bottom),view.top,view.bottom)};
'''  # 최초 초안의 전체 변경영역 조각이며 역변환에만 사용한다.
NEW_RECT = '''// SetRect는 양끝을1픽셀 확장하고 카메라를 뺀 넓은 값을 정수화한 뒤 viewport로 제한한다.
void RawBurningFrame::Invalidate(float left,float top,float right,float bottom) const {
    const auto& view=state_.viewport;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078; // 패치와CD의x87 감산 순서를 구별한다.
    // 패치는 여백→카메라, CD는 카메라→여백 순서로 넓은 좌표를 계산한다.
    const auto coordinate=[patch](float value,float camera,double edge) {
        return FrameInteger(patch ? (static_cast<double>(value)+edge)-camera : (static_cast<double>(value)-camera)+edge);
    };
    SquidDisplayRect rect{std::clamp(coordinate(left,state_.cameraX,-1),view.left,view.right),
        std::clamp(coordinate(top,state_.cameraY,-1),view.top,view.bottom),
        std::clamp(coordinate(right,state_.cameraX,1),view.left,view.right),
        std::clamp(coordinate(bottom,state_.cameraY,1),view.top,view.bottom)};
'''  # x87 fsub/fadd1→fsub camera→ftol→clip의 순서를 유지한다.
OLD_WORLD = '        const auto& assets=Assets(edition);netstorm::client::RawSurfaceWorld world(edition,assets.types,assets.frames,assets.shapes,{},97);auto core=world.Core();core.ownerMode.battle=true;\n'  # 실행 전 검사의 잘못된 기본값 설정이다.
NEW_WORLD = '''        const auto& assets=Assets(edition);BridgeConnectState bridge;bridge.bridgeType=82;bridge.islandType=94;bridge.stalagType=95;bridge.connectorType=155;bridge.noIslandType=157;bridge.battle=true;bridge.mission=true;
        netstorm::client::RawSurfaceWorld world(edition,assets.types,assets.frames,assets.shapes,bridge,97);auto core=world.Core();core.ownerMode.battle=true;
'''  # 기존 PlayerWorld 소비자가 쓰는 실제 TYPE 연결번호를 재사용한다.


def sha(path):
    """파일 전체 원문 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inverse(name, replacements):
    """좁은 교정만 되돌린 전체 원문이 기존 바이트와 동일한지 확인한다."""
    previous = (HISTORY / name).read_bytes()
    text = (ROOT / name).read_text(encoding='utf-8')
    # 지정 조각은 정확히 하나여야 하며 다른 소스는 역변환하지 않는다.
    for after, before in replacements:
        assert text.count(after) == 1, name
        text = text.replace(after, before, 1)
    restored = text.replace('\n', '\r\n').encode('utf-8') if b'\r\n' in previous else text.encode('utf-8')
    assert restored == previous, name


def native_setrect():
    """보호 PE의 실제 SetRect 명령과 상수를 읽기 전용으로 확인한다."""
    output = []
    instructions = {}
    # 두 보호 판본의 불연속 내보내기 범위 안에서 해당 SetRect만 해독한다.
    for edition, binary, address, size, one, camera in (
        ('originals', 'originals/Netstorm.exe', 0x487ba0, 0x109, 0x5012d0, 0x59a938),
        ('originalCD', 'originalCD/NETSTORM.EXE', 0x45ce80, 0x117, 0x502430, 0x565c50),
    ):
        pe = pefile.PE(str(ROOT / binary))
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        entries = list(decoder.disasm(pe.get_data(address - pe.OPTIONAL_HEADER.ImageBase, size), address))
        rendered = [f'{entry.address:08x} {entry.mnemonic} {entry.op_str}' for entry in entries]
        instructions[edition] = rendered
        # 실제 8개 좌표에 cameraX/Y 감산이 각각 두번씩 들어간다.
        assert sum(entry.mnemonic == 'fsub' and entry.op_str == f'dword ptr [0x{camera:x}]' for entry in entries) == 4
        assert sum(entry.mnemonic == 'fsub' and entry.op_str == f'dword ptr [0x{camera + 4:x}]' for entry in entries) == 4
        # 두판본의 실제 상수1 비트와각4번 감산/가산을 PE 원문으로 확인한다.
        assert pe.get_data(one - pe.OPTIONAL_HEADER.ImageBase, 4) == b'\x00\x00\x80\x3f'
        assert sum(entry.mnemonic == 'fsub' and entry.op_str == f'dword ptr [0x{one:x}]' for entry in entries) == 4
        assert sum(entry.mnemonic == 'fadd' and entry.op_str == f'dword ptr [0x{one:x}]' for entry in entries) == 4
        output.append(dict(edition=edition, binary_sha256=sha(ROOT / binary), instructions=rendered))
    return output


def historical_audits():
    """기존 감사가 현재 교정 전의 전체 파일을 읽어 당시 보고서를 재현하게 한다."""
    original_bytes, original_text = Path.read_bytes, Path.read_text
    mapping = {(ROOT / name).resolve(): HISTORY / name for name in (SOURCE, TEST)}

    def previous_bytes(path):
        """교정한 두파일만 당시 전체 바이트로 읽으며 그외 파일은 현재 원문을 읽는다."""
        return original_bytes(mapping.get(path.resolve(), path))

    def previous_text(path, *args, **kwargs):
        """기존 read_text의 인코딩·개행 규칙을 당시 원문에도 그대로 적용한다."""
        return original_text(mapping.get(path.resolve(), path), *args, **kwargs)

    Path.read_bytes, Path.read_text = previous_bytes, previous_text
    try:
        importlib.import_module('analyze_burningframe_draft_lineage').main(True)
        importlib.import_module('analyze_player_regular_native').verify()
    finally:
        Path.read_bytes, Path.read_text = original_bytes, original_text


def expected():
    """최초 전체 자료·정확 역변환·실제 명령어와 실패 로그를 연결한다."""
    audit_history()
    inverse(SOURCE, ((NEW_RECT, OLD_RECT), ('FrameInteger(double value)', 'FrameInteger(float value)')))
    inverse(TEST, ((NEW_WORLD, OLD_WORLD),))
    historical_audits()
    names = (SOURCE, TEST, 'cpppj/tests/fixtures/burningframe-x86.tsv', 'cpppj/tests/fixtures/player-regular-x86.tsv',
             'cpppj/burningframe.json', 'cpppj/player-regular.json',
             'extracted/run15-new-first-tests.log', 'extracted/run15-all-first-tests.log')
    return dict(schema=1, host='HJOW-Athlon', last_decompile_host='HJOW-Athlon', date='2026-10-14',
                new_native_inputs=0, whole_previous_source_inverse=True, whole_previous_test_inverse=True,
                native_setrect=native_setrect(), files={name: sha(ROOT / name) for name in names}, tool_sha256=sha(Path(__file__)),
                scope='Player 소비자에 기존 실제 TYPE 연결값을 공급한다. Burning SetRect의1픽셀 확장과 camera 감산을 실제 양판본 명령대로 복원한다. 기존 fixture/보고서/관찰1214 입력과 최초 실패로그는 보존하며 기대값을 변경하지 않는다.')


# 명시 CLI에서만 별도 교정 근거 생성 또는 고정된 현재 결과의 읽기 감사를 수행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    result = expected()
    if arguments.verify:
        assert json.loads(REPORT.read_text(encoding='utf-8')) == result
    else:
        REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('단위15 Player 실제 TYPE 설정/Burning SetRect 전체역변환/기존격자 불변 감사 통과')
