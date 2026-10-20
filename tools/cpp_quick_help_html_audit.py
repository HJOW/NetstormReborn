#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전용 Help HTML의 최초86관측·실제 TYPE/공용 요구 에너지·빌드와 과거 JSON 보존을 감사한다."""
import argparse
import json
import hashlib
import subprocess
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT=ROOT/'cpppj/quick-help-html-session.json'  # 본문/정보 계층과 실제 전용 창/게임 검증은 구별한다.


def request_layout():
    """실제 요청의80바이트 주제와256바이트 서식 영역을 LEA/현재 ESP로 재확인한다."""
    import sys
    sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
    import pefile
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
    expected={0x4c5c00:'81ec50010000',0x4c5c21:'8d54240c',0x4c5cb3:'8d54240c',0x4c5cb7:'52',0x4c5cb8:'8d442460'}
    # push 뒤 lea의 ESP는4만큼 작으므로 목적지0x60은 이전 ESP+0x5c다.
    for address,raw in expected.items():assert bytes(image[address-base:address-base+len(raw)//2]).hex()==raw,hex(address)
    assert 0x5c-0x0c==80 and 0x150-80==256
    return {'frameBytes':336,'topicBytes':80,'formatBytes':256,'instructions':{f'{address:08x}':raw for address,raw in expected.items()}}


def main():
    """현재 전체 바이트/핀 역변환만 확인하며 C++검사·게임·OS/창은 시작하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify:parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 코드와 보호 원문을 각자의 전체 바이트로 대조한다.
    for group in ('sources','evidence'):
        for name,expected in record[group].items():assert sha(ROOT/name)==expected,name
    for suffix,count,instructions in (('',83,5939980),('-nested',3,43371)):
        native=json.loads((ROOT/f'cpppj/quick-help-html{suffix}-native.json').read_text('utf-8'))
        for name,expected in native['sha256'].items():assert sha(ROOT/name)==expected,name
        rows=[line.split('\t') for line in (ROOT/f'cpppj/tests/fixtures/quick-help-html{suffix}-x86.tsv').read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==native['inputs']==native['normalReturns']==count and native['instructions']==instructions
        assert not native['gameExecuted'] and not native['osExecuted']
        # 제품 변경/중첩 cursor 수정 전에 동결한 최초 사본이 그대로인지 확인한다.
        for name in (f'decomp_quick_help_html{suffix.replace("-","_")}.py',f'quick-help-html{suffix}-x86.tsv',f'quick-help-html{suffix}-native.json'):
            path=ROOT/('tools' if name.endswith('.py') else 'cpppj/tests/fixtures' if name.endswith('.tsv') else 'cpppj')/name
            assert sha(path)==sha(ROOT/'extracted/quick-help-html-20261021-first-capture'/name)
    assert record['requestBufferLayout']==request_layout()
    energy=json.loads((ROOT/'cpppj/type-required-energy.json').read_text('utf-8'))
    # 기존 후처리의 원본/도구/fixture/제품 몸체는 보존하고 이미 다른 역사 CMake 핀은 성공으로 바꾸지 않는다.
    for name,expected in energy['sha256'].items():
        if name=='cpppj/src/client/CMakeLists.txt':
            historical=record['knownExistingEnergyCmakePin'];assert expected==historical['expected']
            baseline=subprocess.check_output(['git','show',f"{record['baseline']}:{name}"],cwd=ROOT)
            assert hashlib.sha256(baseline).hexdigest()==historical['baseline']!=expected
        else:assert sha(ROOT/name)==expected,name
    assert energy['observations']==energy['native_calls']==2352 and energy['normal_return'] and energy['os_calls']==0
    assert 'kTopicBytes=80' in (ROOT/'cpppj/src/client/QuickHelpRequest.cpp').read_text('utf-8')
    session=(ROOT/'cpppj/src/client/RawCampaignSession.cpp').read_text('utf-8');source=(ROOT/'cpppj/src/client/QuickHelpHtml.cpp').read_text('utf-8')
    assert 'hooks.mana=BindAiTypeRequiredEnergy(current->assets.game,current->core.types)' in session
    assert 'factory.mana=BindAiTypeRequiredEnergy(current->assets.game,current->core.types)' in session
    info=session.split('QuickHelpTypeInfo RawCampaignSession::HelpTypeInfo(',1)[1].split('// 원본 사제/',1)[0]
    assert 'impl_->gunTypes[type].range' in info and 'impl_->hitTypes[type].damagePerSecond' in info and 'BindAiTypeRequiredEnergy(impl_->assets.game,impl_->core.types)(type)' in info
    assert 'if (links_) links_->cursor=cursor' in source and 'state.inputOffset=entry' in source and 'infoMode_=0' in source and 'infoMode_=1' in source
    ctor=source.split('QuickHelpHtml::QuickHelpHtml(',1)[1].split('// 번역은',1)[0]
    assert ctor.index('state_=HtmlTextParserState{}')<ctor.index('if (type_) static_cast<void>(Need(hooks_.type)())')<ctor.index('state_.shown=0')
    assert record['build']['returnCode']==0 and record['build']['warnings']==record['build']['errors']==0
    assert sha(ROOT/record['build']['log'])==record['build']['logSha256']
    assert record['newTestsCompiled'] and record['runtimeTestsDeferred'] and not record['testsExecuted'] and not record['gameExecuted'] and not record['goalComplete']
    assert not record['dedicatedWindowConnected'] and not record['clientHelpCommandConnected']
    verify_pins(record)
    print(f"Help HTML: 최초83+3입력·같은 TYPE/후처리 마나/중첩 링크·Release·{len(record['sourcePinChanges'])}핀 보존 확인, 전용 창/제품 실행 이월")


if __name__=='__main__':main()
