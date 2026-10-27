#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 파일을 보존하며 실제 미니맵의 클릭/경계 드래그/뗌/모달/미션 이탈을 관측한다."""
import argparse
import json
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated


def main():
    """같은 실제 입력 큐로 화면을 조작하며 카메라나 bitmap 상태를 직접 쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--width', type=int, default=1024)
    parser.add_argument('--height', type=int, default=768)
    parser.add_argument('--data-root', type=Path, default=ROOT/'originals', help='실행 자료 사본 경로. 생략하면 기존 originals를 사용한다.')
    args = parser.parse_args();out = args.output.resolve();out.mkdir(parents=True, exist_ok=False)
    data_root=args.data_root.resolve()  # 같은 미니맵 입력을 보호 원본과 분리한 사본에서도 검증한다.
    steps = [(5, 'report', 'briefing'), (6, 'click', 'minimap:15,15'), (8, 'report', 'briefingIgnored'), (10, 'click', 'Play Mission'),
             (20, 'report', 'initial'), (20, 'snapshot', str(out/'initial.bmp')),
             (25, 'down', 'minimap:40,37'), (28, 'report', 'held'),
             (30, 'down', 'minimap:90,37'), (46, 'report', 'rightDrag'), (46, 'snapshot', str(out/'right-drag.bmp')),
             (48, 'up', 'minimap:90,37'), (51, 'report', 'released'),
             (55, 'down', 'minimap:40,37'), (57, 'down', 'minimap:40,-5'), (76, 'report', 'topDrag'),
             (78, 'up', 'minimap:40,-5'), (81, 'report', 'topReleased'), (81, 'snapshot', str(out/'top-released.bmp')),
             (85, 'esc', ''), (88, 'report', 'menu'), (90, 'click', 'Game'), (94, 'report', 'gameMenu'),
             (96, 'click', 'Leave Mission'), (103, 'report', 'leaveConfirm'),
             (106, 'click', 'Main Menu'), (119, 'report', 'mainMenu'), (119, 'snapshot', str(out/'main-menu.bmp'))]  # 실제가로막대의Game→미션이탈→원본ABORT확인창을통해종료한다.
    script = out/'steps.tsv';report = out/'states.tsv'
    script.write_text(''.join(f'{frame}\t{op}\t{argument}\n' for frame,op,argument in steps), encoding='utf-8', newline='\n')
    command = [str(GAME), '--run', str(data_root), '--mission', 'thewarbegins', '--window', '--no-audio', '--frames', '124',
               '--set', f'SCREENW={args.width};SCREENH={args.height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8', '--ui-script', str(script), '--ui-report', str(report)]
    before = snapshots(data_root);original_before=before if data_root==ROOT/'originals' else snapshots(ROOT/'originals')  # 실행 사본과 보호 원본의 전체 해시를 구별한다.
    save(out/'originals-before.json', original_before);save(out/'data-before.json',before)
    with preserve_game_settings(data_root, 'd',out/'settings-backup'):
        result = execute(command, out, GAME, 90)
    result.update(originalsUnchanged=snapshots(ROOT/'originals')==original_before,dataUnchanged=snapshots(data_root)==before,dataRoot=str(data_root));save(out/'run.json', result)
    if result['returnCode'] or result['timedOut'] or not result['originalsUnchanged'] or not result['dataUnchanged']:
        raise RuntimeError(result)
    observed = states(report)
    assert observed['briefing']['camera'] == observed['briefingIgnored']['camera'], observed['briefingIgnored']
    summary = {}
    for label,local in (('held',(40,37)),('rightDrag',(90,37)),('topDrag',(40,-5))):  # 현재crop의497f20요청과프레임말미499fe0의원본세계한계를구별한다.
        row = observed[label];mini = [int(value) for value in row['minimap']]
        assert mini[1:5] == [0,args.height-75,80,75] and mini[7] == 1, mini
        requested = [(mini[5]+local[0])*32-args.width//2,(mini[6]+local[1])*22-args.height//2]
        expected = [max(0,min(requested[0],256*16-args.width)),max(0,min(requested[1],256*11-args.height))]  # 표준256칸미션의현재적용카메라는복원된Present후한계를따른다.
        assert [int(value) for value in row['camera']] == expected, (label,mini,row['camera'],expected)
        assert int(row['RawButtons'][3]) != 0, row['RawButtons']
        summary[label] = dict(crop=mini[5:7],requestedCamera=requested,camera=expected,bitmap=mini[11],frames=mini[8])
    for label in ('released','topReleased'):  # 뗌은같은실제capture와Idle을해제한다.
        assert observed[label]['minimap'][7] == '0' and observed[label]['RawButtons'][3] == '0', observed[label]
    assert int(observed['rightDrag']['minimap'][5]) == 48, observed['rightDrag']['minimap']
    assert int(observed['topDrag']['minimap'][6]) == 0 and int(observed['topReleased']['camera'][1]) == 0, observed['topReleased']  # 음수요청은원본Present가0으로제한하며뗀뒤에도유지한다.
    assert observed['menu']['paused'] == ['0'] and observed['menu']['popup'] == [''] and observed['menu']['game_menu_bar'][1] == '1', observed['menu']
    leave_buttons=repeated(report,'raw_html_button')['leaveConfirm']  # 실제Native ABORT의표시버튼을읽으며제거된임시control행을요구하지않는다.
    assert observed['leaveConfirm']['page'][0].casefold() == 'abort' and observed['leaveConfirm']['paused']==['1'] and any(row[10]=='1' and row[11]=='Main Menu' for row in leave_buttons), observed['leaveConfirm']
    assert 'minimap' not in observed['mainMenu'] and observed['mainMenu']['phase'] == ['0'], observed['mainMenu']
    summary.update(passed=True, viewport=[args.width,args.height], originalsUnchanged=True,dataUnchanged=True,dataRoot=str(data_root),
                   limitations=['실제기본HUD의crop/카메라/큐경로이며전체dock드래그와원본viewport배치는후속이다.', '모드2/5유닛이동및미션전체완주는이검사에포함하지않는다.'])
    save(out/'summary.json', summary);print(json.dumps(summary, ensure_ascii=False))


if __name__ == '__main__':  # 명시CLI요청에서만클론창을연다.
    main()
