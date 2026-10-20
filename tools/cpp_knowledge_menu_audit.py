#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 지식 창의 첫 PE 관측·정렬 이동·실제 UI 연결과 과거 결과 보존을 감사한다."""
import argparse
import json
import subprocess
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT=ROOT/'cpppj/knowledge-menu-session.json'  # 현재 연결과 후속 C++/실제 플레이 검증을 구별한다.
BASELINE='bf68f0d4'  # 기존 Factory 정렬 몸체가 있던 이번 작업 시작 커밋이다.


def relocated_sort():
    """기존 비교/정렬의 본문이 공용 함수로 같은 내용으로 옮겨졌는지 코드 바이트를 역대조한다."""
    old=subprocess.check_output(['git','show',f'{BASELINE}:cpppj/src/client/RawFactoryMenu.cpp'],cwd=ROOT).decode('utf-8').replace('\r\n','\n')
    first=old.index('int RawFactoryMenu::Compare(');start=old.rfind('// ',0,first);last=old.index('void BindFactoryMenuControls(',first);end=old.rfind('// ',first,last)
    expected=old[start:end].replace('int RawFactoryMenu::Compare(const Knowledge& first,const Knowledge& second) const','int CompareKnowledgeRecords(const KnowledgeRecord& first,const KnowledgeRecord& second,std::span<const o::RiftTypeRecord> types)')
    expected=expected.replace('Type(first.type).flags2,b=Type(second.type).flags2','types[first.type].flags2,b=types[second.type].flags2').replace('void RawFactoryMenu::Sort(std::span<Knowledge> entries) const','void SortKnowledgeRecords(std::span<KnowledgeRecord> entries,std::span<const o::RiftTypeRecord> types)')
    expected=expected.replace('return Compare(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)]);','return CompareKnowledgeRecords(entries[static_cast<std::size_t>(a)],entries[static_cast<std::size_t>(b)],types);')
    current=(ROOT/'cpppj/src/client/KnowledgeSort.cpp').read_text('utf-8');current=current[current.index(old[start:first]):].removesuffix('}\n')
    current=current.replace('    if (first.type>=types.size() || second.type>=types.size()) throw std::out_of_range("지식 정렬 TYPE 범위");\n','')
    assert current==expected,'공용 정렬 본문의 기계적 이동 차이'
    return True


def main():
    """현재 파일·원본·빌드·핀 보존만 읽고 게임/OS/제품 검사 실행 파일은 시작하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 연결과 동결된 원본 근거를 각자의 전체 바이트로 확인한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/knowledge-menu-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/knowledge-menu-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==len({row[0] for row in rows})==native['inputs']==native['normalReturns']==25
    assert native['instructions']==241740 and not native['gameExecuted'] and not native['osExecuted']
    assert sha(ROOT/'cpppj/tests/fixtures/knowledge-menu-x86.tsv')==record['firstCapture']['fixtureSha256']
    assert relocated_sort() and record['sharedSortRelocation']
    client=(ROOT/'cpppj/src/client/UberGump.cpp').read_text('utf-8');session=(ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8');menus=(ROOT/'cpppj/src/client/RawMenuControls.cpp').read_text('utf-8')
    assert 'KnowledgeScriptCode(action.command)' in client and 'client_.Input().PushCommand(kShowTechnologyCommand,0,0)' in client
    assert 'OpenKnowledge(world_->Session()' in client and 'rawButtons_->Menus()->KnowledgeOpen()' in client
    event=client.split('void UberGump::Event(InputEvent event)',1)[1].split('void UberGump::Tick()',1)[0]
    assert event.index('missionRuntime_->tutorial->Event(missionEvent)')<event.index('if (transitionCode==kShowTechnologyCommand)')
    assert 'hooks.currentTechnology=[current] { return current->player->CurrentTechnology(); }' in session
    assert 'current->priests->Carrier().FindContained' in session and 'current->production->Contains(type)' in session
    assert 'session.ShowKnowledge(state,std::move(hooks))' in menus and 's.menu->AttachCurrent(x,y,s.common.create.collection.root)' in menus
    assert 's.menu->SetWidth(token,value)' in menus and 's.controls->AddText(text,command,value,true,enabled,false)' in menus
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"지식 창: 최초25입력·공용 원본 정렬/실제 세션·메뉴/명령 큐·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 제품 실행 이월")


if __name__=='__main__':
    main()
