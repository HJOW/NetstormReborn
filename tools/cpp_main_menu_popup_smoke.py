#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""메인 Help/Options의 실제 버튼·하위 메뉴·설정 반전과 모달 정리를 검사한다(697)."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def menu_rows(path):
    """각 보고에서 중복 키인 실제 메뉴 행을 모두 보존한다."""
    result = {}
    for block in re.split(r'(?m)^frame\t', path.read_text(encoding='utf-8'))[1:]:  # 실제 관측 블록을 프레임 라벨별로 나눈다.
        header, body = block.split('\n', 1)
        result[header.split('\t', 1)[1]] = [line.split('\t')[1:] for line in body.splitlines() if line.startswith('raw_menu_item\t')]
    return result


def main():
    """미션 진입 없이 한 메인 세션에서 팝업 선택·반복·회수를 확인하고 보호 설정을 복원한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    plan, report = out/'steps.tsv', out/'states.tsv'
    steps = [(20,'report','main'), (24,'click','Help'), (30,'report','help'), (30,'snapshot-presented',str(out/'help.bmp')),
             (34,'click','General Help - F1'), (43,'report','general'), (43,'snapshot-presented',str(out/'general.bmp')),
             (47,'click','screen:1000,740'), (55,'report','generalClosed'), (60,'click','Help'), (66,'click','Version'),
             (74,'report','about'), (77,'esc',''), (85,'report','aboutClosed'), (90,'click','Options'),
             (98,'report','options'), (98,'snapshot-presented',str(out/'options.bmp')), (102,'click','Edge Scroll in Fullscreen'),
             (110,'report','toggled'), (114,'click','Options'), (122,'report','optionsToggled'), (125,'click','Resolution'),
             (135,'report','resolution'), (135,'snapshot-presented',str(out/'resolution.bmp')), (142,'click','screen:1000,740'),
             (150,'report','closed'), (155,'click','Help'), (160,'report','helpAgain'), (165,'click','screen:1000,740'),
             (173,'report','closedAgain'), (178,'click','Options'), (185,'report','optionsAgain'), (190,'click','Edge Scroll in Fullscreen'), (198,'report','restored')]
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps), encoding='utf-8', newline='\n')
    exe = ROOT/'cpppj/build/bin/Release/Netstorm.exe'
    command = [str(exe),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','210','--set',
               'SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;intro=1;WelcomeGiven=1;edgeScroll=0;techHelpExe=;techHelpFile=',
               '--ui-script',str(plan),'--ui-report',str(report)]
    before = protection()
    (out/'protection-before.json').write_text(json.dumps(before,ensure_ascii=False),encoding='utf-8')
    (out/'run.json').write_text(json.dumps(dict(command=command,exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    with preserve_game_settings(ROOT/'originals','d',out/'settings-backup'):
        run = subprocess.run(command,cwd=ROOT,capture_output=True,timeout=120)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after = protection()
    (out/'protection-after.json').write_text(json.dumps(after,ensure_ascii=False),encoding='utf-8')
    assert before == after and run.returncode == 0, (before == after,run.returncode)
    observed, menus = states(report), menu_rows(report)
    for label in ('help','options','optionsToggled','resolution','helpAgain','optionsAgain'):  # 같은 메인 화면에 실제 모달 팝업이 있어야 한다.
        assert observed[label]['phase'] == ['0'] and observed[label]['page'] == ['main'] and observed[label]['current_dialog'] != ['0'], label
    help_items = [row for row in menus['help'] if row[-2] == '1']
    assert [int(row[3]) for row in help_items] == [0x02000017,0x02000049,0x02000095], help_items
    assert int(help_items[1][2]) & 0x200, help_items  # 설정되지 않은 Technical Help는 원문처럼 비활성이다.
    assert observed['general']['raw_help'][0] != '0', observed['general']
    assert observed['about']['page'] == ['About'], observed['about']
    visible = [row[-1] for row in menus['options'] if row[-2] == '1']
    assert any('Pass Server Diagnostic' in name for name in visible) and not any('Counterclockwise' in name for name in visible), visible  # 실제 State3는 원문 fort 모드를 세워 이 분기를 선택한다.
    edge = lambda label: next(row for row in menus[label] if 'Edge Scroll' in row[-1] and row[-2] == '1')
    assert int(edge('options')[2]) & 4 == 0 and int(edge('optionsToggled')[2]) & 4, (edge('options'),edge('optionsToggled'))
    assert any(row[-1] == '1280 by 720' and row[-2] == '1' for row in menus['resolution']), menus['resolution']
    counts = {label:int(observed[label]['RawButtons'][1]) for label in ('main','generalClosed','aboutClosed','toggled','closed','closedAgain','restored')}
    assert len(set(counts.values())) == 1, counts
    for label in counts:  # 닫힌 뒤 호스트 메인과 모달/등록표가 같은 값으로 돌아와야 한다.
        assert observed[label]['phase'] == ['0'] and observed[label]['current_dialog'] == [observed[label].get('main_menu_parent',['0'])[0]], label  #700 이후 실제 메인 부모가 현재 모달이며 이전 제품의 필드 부재는0이다.
    result = dict(unit=697,exit=run.returncode,protectedUnchanged=True,gameExecutions=1,missionEntries=0,
                  exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),helpCommands=[int(row[3]) for row in help_items],registryCounts=counts,
                  nativeOptions=True,nativeHelp=True,compatibilityResolutionSubmenu=True,configToggle=True,physicalInput=False)
    (out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False))


if __name__ == '__main__':
    main()
