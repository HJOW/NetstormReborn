#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Client Help 조립의 소스·PE 가상 표/F1 인자·빌드와 과거 보고서 보존을 읽기 전용으로 확인한다."""
import argparse
import json
import struct
import pefile
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/quick-help-client-session.json'  # 실제 화면/제품 검사 실행 여부와 소스 연결을 구별하는 이번 보고서다.


def main():
    """게임/창/C++검사를 시작하지 않고 파일과 보호 PE의 실제 바이트만 읽는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    if not parser.parse_args().verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 실행/독립 관측을 새 연결의 검사 통과로 바꾸지 않는다.
    for group in ('sources', 'evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT / name) == expected, name
    assert record['unit'] == 257 and not record['newDecompile']
    assert record['clientVirtualDispatchConnected'] and record['clientHelpCommandConnected']
    assert record['newTestsCompiled'] and not record['testsExecuted'] and not record['gameExecuted']
    assert record['runtimeTestsDeferred'] and not record['goalComplete'] and record['newNativeInputs'] == 0
    assert record['build']['returnCode'] == record['build']['errors'] == record['build']['warnings'] == 0
    assert sha(ROOT / record['build']['log']) == record['build']['logSha256']
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
    base = pe.OPTIONAL_HEADER.ImageBase
    # PE 가상 표는 전용 창/HTML 부모/그림의 실제 Attach/Handle/scalar 경계를 구별한다.
    for address, slots in record['staticPeChecks']['vtables'].items():
        data = pe.get_data(int(address, 16) - base, len(slots) * 4)
        assert list(struct.unpack('<' + 'I' * len(slots), data)) == slots
    assert pe.get_data(0x4d8c47 - base, 24).hex() == record['staticPeChecks']['f1Bytes']
    assert pe.get_data(0x515c10 - base, 7) == b'F1Help\0'
    assert pe.get_dword_at_rva(0x54120c - base) == 109
    assert pe.get_dword_at_rva(0x5412dc - base) == 161
    menu = (ROOT / 'cpppj/src/client/RawMenuControls.cpp').read_text('utf-8')
    button = (ROOT / 'cpppj/src/client/RawButtonControls.cpp').read_text('utf-8')
    client = (ROOT / 'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    tests = (ROOT / 'cpppj/tests/RawMenuControlsTests.cpp').read_text('utf-8')
    # 조립이 실제 기존 처리기를 호출하는지 확인한다. 이 정적 조건은 제품 실행 검사를 대신하지 않는다.
    for marker in ('helpWindow->Attach', 'helpBody->Attach', 'common.stones.Attach', 'helpImage->Attach',
                   'DispatchQuickHelpWindow', 'QuickHelpRequest(window', 'OpenQuickHelpWindow',
                   'titleMarkup->Parse', 'helpBody->SetText', 'common.input.AddDialog',
                   'node->serial==serial', 's.common.stones.Draw', 's.helpImage->Draw',
                   's.helpHistory=QuickHelpHistory{};s.popupState.command=0', 's.helpLinks.erase(token)',
                   '0x61000003U) return "F1Help"', 'links.bytes[0]=0'):
        assert marker in menu, marker
    assert 'stoneHooks.measure=' in button and 'menus->MeasureText' in button
    event = client.split('void UberGump::Event(', 1)[1]
    assert event.index('missionRuntime_->tutorial->Event') < event.index('transitionCode==0x02000017U') < event.index('Menus()->HelpOpen()')
    assert 'PushCommand(0x02000017U,0,0x61000003)' in event
    assert '0x70000000U|token' in client and 'token&0x0fffffffU' in client
    assert 'world_ ? world_->Session().HelpTypeInfo(type) : ReadAssetQuickHelpType' in client
    assert 'KnowledgeOpen() || rawButtons_->Menus()->HelpOpen()' in client
    clear = client.split('void UberGump::ClearWorld()', 1)[1].split('bool UberGump::HasWorld()', 1)[0]
    assert clear.index('CloseHelp()') < clear.index('DisableProduction()') < clear.index('world_=nullptr')
    assert '기술 학습492a30 미연결' in client and '외부 OS 프로토콜 미복원' in client
    for name in record['newTests']:
        assert 'TEST_CASE(' + name + ')' in tests
    verify_pins(record)
    print(f"Client Help 소스 조립/PE 가상 표·F1Help/Release·{len(record['sourcePinChanges'])}핀 보존 확인; 실제 화면·C++검사는 이월")


if __name__ == '__main__':
    main()
