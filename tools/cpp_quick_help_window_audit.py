#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전용 Help 전체 창/HTML 부모·빈 요청의 최초98관측과 빌드/과거 보고서 보존을 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT=ROOT/'cpppj/quick-help-window-session.json'  # 실제 Client 연결/플레이 검증과 창의 처리기 복원을 구별한다.


def main():
    """읽기 전용으로 근거/현재 소스/빌드·핀 역변환만 검사하며 게임/C++검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 몸체와 독립 원문은 각자의 전체 바이트로 확인한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    for suffix,count,instructions in (('',84,162878),('-paths',14,90207)):
        native=json.loads((ROOT/f'cpppj/quick-help-window{suffix}-native.json').read_text('utf-8'))
        for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
        rows=[line.split('\t') for line in (ROOT/f'cpppj/tests/fixtures/quick-help-window{suffix}-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==len({(row[0],row[1]) for row in rows})==native['inputs']==native['normalReturns']==count
        assert native['instructions']==instructions and not native['gameExecuted'] and not native['osExecuted']
        # 주 처리기/빈 요청 수정 전에 동결한 도구/원문/JSON이 그대로인지 확인한다.
        for name in (f'decomp_quick_help_window{suffix.replace("-","_")}.py',f'quick-help-window{suffix}-x86.tsv',f'quick-help-window{suffix}-native.json'):
            current=ROOT/('tools' if name.endswith('.py') else 'cpppj/tests/fixtures' if name.endswith('.tsv') else 'cpppj')/name
            assert sha(current)==sha(ROOT/'extracted/quick-help-window-20261021-first-capture'/name)
    source=(ROOT/'cpppj/src/client/QuickHelpWindow.cpp').read_text('utf-8');body=(ROOT/'cpppj/src/client/HtmlBodyGump.cpp').read_text('utf-8');request=(ROOT/'cpppj/src/client/QuickHelpRequest.cpp').read_text('utf-8')
    ctor=source.split('std::uint32_t QuickHelpWindow::Construct(',1)[1].split('void QuickHelpWindow::Reset(',1)[0]
    assert ctor.index('Need(hooks_.constructBase)')<ctor.index('Need(hooks_.scripted)')<ctor.index('Allocate(0xc4)')<ctor.index('Need(hooks_.nearestColor)')<ctor.index('Allocate(0x40a4)')<ctor.index('"Quick Help Spec"')<ctor.index('state_.id=Node(token).serial')
    assert 'learnType ? "Sacrifice" : "OK"' in ctor and 'data->push_back' in ctor and '0x03e40000' in ctor
    assert 'static_cast<double>(count)*10' in source and 'Larger(width(Node(token).backButton)' in source
    show=source.split('void QuickHelpWindow::Show(',1)[1]
    assert show.index('Reset(token)')<show.index('Need(hooks_.render)')<show.index('Need(hooks_.substitute)')<show.index('Need(hooks_.helpFrame)')<show.index('Need(hooks_.bodyText)')
    assert 'state_.battle ? "pyrosphereHelp" : state_.challenge ? "challengeHelp" : "introHelp"' in show
    assert 'auto& output=Body(token).body' in show and 'std::string_view(output.data(),text.size())' in show
    assert 'Node(token).body[0]=Node(token).links[0]=0' in body and 'if (!Need(parser.hasOutput)())' in body
    assert 'Need(hooks_.slider)(Node(token).text,1)' in body and 'Need(hooks_.scrollable)(Node(token).text,1)' in body
    assert 'if (!stem.empty()) { RemoveSuffix' in request and 'kTopicBytes=80' in request
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    assert not record['clientVirtualDispatchConnected'] and not record['clientHelpCommandConnected']
    verify_pins(record)
    print(f"Help 전체 창: 최초84+14입력·실제 생성/배치/본문/HTML 부모·빈 요청·Release·{len(record['sourcePinChanges'])}핀 보존 확인, Client 연결/제품 실행 이월")


if __name__=='__main__':main()
