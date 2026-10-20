#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Template9의 최초 원문/실제 공유 수명·같은State·빌드와 과거 JSON 보존을 읽기 전용으로 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/template-lifetime-session.json'  # State7 전체/제품 실행과 이번 명시 수명 연결을 구별한다.


def main():
    """게임/창/C++검사를 시작하지 않고 원문·소스 연결·빌드 기록만 읽는다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true')
    if not parser.parse_args().verify:parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 독립 근거/실제 빌드된 바이트를 각자의 사전으로 검증한다.
    for group in ('sources', 'evidence', 'binaries'):
        for name, expected in record[group].items():assert sha(ROOT / name) == expected, name
    native = json.loads((ROOT / 'cpppj/template-lifetime-v2-native.json').read_text('utf-8'))
    assert native['uniqueInputs'] == native['normalFunctionReturns'] == 240 and native['instructions'] == 15483
    assert native['entryCounts'] == {'004bfb00':108,'004bf690':108,'004bd2e0':24} and native['firstAttemptRejected'] and native['capturedAfterProductEdits']
    assert not native['gameExecuted'] and not native['osExecuted']
    # 초기 실패 관측과 별도 유효 관측을 각각 보존하고 제품 수정 뒤 재관측한 시점도 숨기지 않는다.
    for name in ('tools/decomp_template_lifetime.py', 'cpppj/tests/fixtures/template-lifetime-x86.tsv', 'cpppj/template-lifetime-native.json'):
        assert sha(ROOT / name) == sha(ROOT / 'extracted/template-lifetime-20261021-first-capture' / name.split('/')[-1])
    # 보정 실행기는 원본 PE만 읽으며 세 부모의 실제 진입 수를 전수 기록했다.
    for name in ('tools/decomp_template_lifetime_v2.py','cpppj/tests/fixtures/template-lifetime-v2-x86.tsv','cpppj/template-lifetime-v2-native.json'):
        assert sha(ROOT / name) == sha(ROOT / 'extracted/template-lifetime-20261021-corrected-capture' / name.split('/')[-1])
    rows = [line.split('\t') for line in (ROOT / 'cpppj/tests/fixtures/template-lifetime-v2-x86.tsv').read_text('utf-8').splitlines() if not line.startswith('#')]
    assert len(rows) == len({row[0] for row in rows}) == 240
    # 각 관측의 전체196BYTE와 소멸/free 경계 당시의 같은 폭을 검사한다.
    for row in rows:
        assert len(row) == 4
        before, after = bytes.fromhex(row[1]), bytes.fromhex(row[2]);assert len(before) == len(after) == 196 and before[:8] == after[:8] and before[-8:] == after[-8:]
        if row[3] != '-':
            for event in row[3].split(';'):assert event[0] in 'DF' and len(bytes.fromhex(event.split(',')[2])) == 196
    lifetime = (ROOT / 'cpppj/src/o/RawTemplateLifetime.cpp').read_text('utf-8')
    assert 'if (manager && !Read(header,4))' in lifetime and 'hooks.destroy(manager);hooks.release(manager)' in lifetime
    assert 'if (Read(header,0)) ReleaseTemplateHeader(header,hooks)' in lifetime
    storage = (ROOT / 'cpppj/src/client/CampaignTemplateStorage.cpp').read_text('utf-8')
    assert 'o::ClearTemplateHeaders(headers_,Effects())' in storage and 'images_.erase(handle)' in storage
    assert 'image.fort=o::FortTemplate{}' in storage and 'Read(Header(slot))' in storage
    assert 'fort.sections[at]' in storage and 'state={bytes,0' in storage
    assert 'image.retiredState=std::move(bytes);bytes.resize(size)' in storage and 'CommitStateGrowth' in storage
    state = (ROOT / 'cpppj/src/o/RawPlayerTemplateState.cpp').read_text('utf-8')
    assert 'auto& stream=states_(slot)' in state and state.index('stream.bytes=replacement.first(28)') < state.index('allocationComplete_(slot)')
    client = (ROOT / 'cpppj/src/client/UberGump.cpp').read_text('utf-8')
    initialize = client.split('void UberGump::InitializeMission()', 1)[1].split('void UberGump::LoadMissionWorld(', 1)[0]
    clear = client.split('void UberGump::ClearWorld()', 1)[1].split('bool UberGump::HasWorld()', 1)[0]
    assert 'templateStorage_->ClearAll()' in initialize and 'services.templateStorage=templateStorage_' in client
    assert 'templateStorage_->ClearAll()' not in clear and 'templateStorage_.reset()' not in clear
    session = (ROOT / 'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8')
    assert 'templateStorage(services.templateStorage ? services.templateStorage : std::make_shared<CampaignTemplateStorage>())' in session
    assert 'headers(templateStorage->Headers())' in session and 'templateStorage->Install(1,std::move(fort)' in session
    assert 'templateStorage->State(slot)' in session and 'templateStorage->Territory(slot)' in session
    assert all(name not in session for name in ('stateGrowth', 'stateBytes', 'stateStreams', 'stateNames', 'std::array<FortTemplate,9> templates'))
    login = (ROOT / 'cpppj/src/client/RawCampaignSessionLogin.inc').read_text('utf-8')
    assert 's.templateStorage->Install(owner,' in login and 's.templateStorage->Template(s.playerState.templateOwner)' in login
    assert 's.templates[' not in login and 's.stateBytes[' not in login
    assert record['build']['returnCode'] == record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT / record['build']['log']) == record['build']['logSha256']
    assert record['newTestsCompiled'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    assert record['sharedTemplateLifetimeConnected'] and record['sameStateSectionConnected'] and not record['wholeState7Restored'] and not record['wholeRawDataManagerRestored']
    assert record['pauseRequested'] and record['pauseAfterCommit']
    verify_pins(record)
    print(f"Template 최초240입력·소유/빌림·실제공유9칸/같은State·Release·{len(record['sourcePinChanges'])}핀 보존 확인; 제품 실행 이월")


if __name__ == '__main__':main()
