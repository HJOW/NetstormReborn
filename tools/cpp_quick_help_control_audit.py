#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Quick Help 제어의 최초 PE 관측·공유 팝업 연결·빌드·과거 JSON 보존을 읽기 전용으로 감사한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT=ROOT/'cpppj/quick-help-control-session.json'  # 현재 구현/컴파일과 이월한 실제 화면 검증을 구별한다.


def main():
    """전체 해시/현재 연결과 핀 역변환을 확인하며 제품·게임·C++검사를 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 코드와 독립 원문은 각자의 전체 바이트로 확인한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    native=json.loads((ROOT/'cpppj/quick-help-control-native.json').read_text('utf-8'))
    for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
    rows=[line.split('\t') for line in (ROOT/'cpppj/tests/fixtures/quick-help-control-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==len({(row[0],row[1]) for row in rows})==native['inputs']==native['normalReturns']==150
    assert native['instructions']==35367 and not native['gameExecuted'] and not native['osExecuted']
    assert {kind:sum(row[0]==kind for row in rows) for kind in ('O','H','E','Z','P')}=={'O':16,'H':20,'E':109,'Z':1,'P':4}
    # 최초 도구/기대값/보고서가 제품 작성 전에 보존한 사본과 같은지 읽는다.
    for name in ('decomp_quick_help_control.py','quick-help-control-x86.tsv','quick-help-control-native.json'):
        current=ROOT/('tools' if name.endswith('.py') else 'cpppj/tests/fixtures' if name.endswith('.tsv') else 'cpppj')/name
        assert sha(current)==sha(ROOT/'extracted/quick-help-control-20261021-first-capture'/name)
    source=(ROOT/'cpppj/src/client/QuickHelpControl.cpp').read_text('utf-8');menus=(ROOT/'cpppj/src/client/RawMenuControls.cpp').read_text('utf-8')
    assert 'QuickHelpPopupState popupState;' in menus and 's.popupState.Mark(command)' in menus and 'return impl_->popupState' in menus
    assert 'popupCommand' not in menus and 'popupArgument' not in menus
    opener=source.split('std::uint32_t OpenQuickHelpWindow(',1)[1].split('// 마우스 좌표',1)[0]
    assert opener.index('Need(hooks.closePopups)()')<opener.index('Need(hooks.lookup)(state.helpId)')<opener.index('Need(hooks.noModal)()')
    assert 'Node(hooks.node,help).depth-76U' in opener and 'mode==0 ? kRequest : kHelp' in opener
    back=source.split('const auto topic=history.Back();',1)[1].split('return close();',1)[0]
    assert back.index('const auto previous=popup.command')<back.index('Need(hooks.kill)()')<back.index('popup.Argument()')<back.index('Need(hooks.queue)(previous,argument,argument)')
    assert 'Node(hooks.node,token).serial==first' in source and 'Need(hooks.learn)(window.learnType);return close();' in source
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    assert not record['dedicatedWindowConnected'] and not record['derivedBodyConnected'] and not record['clientHelpCommandConnected']
    verify_pins(record)
    print(f"Quick Help 제어: 최초150입력·현재 팝업/이력·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 전용 본문/창·제품 실행은 후속")


if __name__=='__main__':main()
