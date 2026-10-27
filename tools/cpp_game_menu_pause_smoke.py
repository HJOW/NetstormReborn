#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Options 행으로 Pause/정지 중 ESC/재개와 같은 메시지 표의 수명을 검사한다."""
import argparse
import json
from pathlib import Path

from cpp_game_menu_smoke import protected_run, probe_steps, menu_items, choose, audio_values
from cpp_bridge_ritual_runtime import save, sha
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated


def main():
    """실제 관측한 행 라벨을 클릭하며 정지 깊이나 상태 메시지를 직접 변경하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--data-root',type=Path,help='별도 실행 자료 사본. 생략하면 기존 originals를 사용한다.')
    args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    probe=out/'probe'  # 실제 같은 바이너리의 메뉴 원문을 먼저 읽는 실행이다.
    report,first=protected_run(probe,1024,768,probe_steps(probe),44,data_root=args.data_root)
    items=menu_items(report,'bar')  # 숨긴 Options 하위의 실제 원문 라벨/명령을 그대로 읽는다.
    options=choose(items,text='Options')['label']
    pause=choose(items,command=0x020000cf)['label']  # 보호 명령 표의 Pause ID다.
    run=out/'scenario'  # 별도 프로세스/설정 복원과 현재 상태 자료를 보존한다.
    steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),
           (25,'esc',''),(30,'click',options),(35,'report','options'),(40,'click',pause),
           (48,'report','paused'),(48,'snapshot',str(run/'paused.bmp')),
           (58,'esc',''),(64,'click',options),(70,'report','pausedBar'),
           (70,'snapshot',str(run/'paused-bar.bmp')),(76,'click',pause),
           (84,'report','resumed'),(84,'snapshot',str(run/'resumed.bmp'))]
    report,result=protected_run(run,1024,768,steps,96,data_root=args.data_root)
    observed=states(report)
    assert observed['initial']['paused']==['0'],observed['initial']['paused']
    assert observed['paused']['paused']==['1'] and observed['pausedBar']['paused']==['1'],observed['pausedBar']['paused']
    assert observed['pausedBar']['game_menu_bar'][1]=='1',observed['pausedBar']['game_menu_bar']
    assert observed['resumed']['paused']==['0'],observed['resumed']['paused']
    warnings=repeated(report,'warning')  # 실제20칸 표/노드의 영구 기한과 원문만 관측한다.
    for label in ('paused','pausedBar'):  # 정지 중 프레임과 메뉴 재열기가 같은 안내를 보존한다.
        rows=[row for row in warnings[label] if 'Game Paused!' in row[-1]]
        assert len(rows)==1 and float(rows[0][6])==0,rows
    assert not [row for row in warnings['resumed'] if 'Game Paused!' in row[-1]],warnings['resumed']
    audio=audio_values(run/'audio.tsv')
    assert audio.get('deviceReady')=='1' and audio.get('muteDepth')=='1',audio
    summary=dict(passed=True,originalsUnchanged=first['originalsUnchanged'] and result['originalsUnchanged'],
                 binarySha256=result['binarySha256'],toolSha256=sha(Path(__file__)),
                 pausedMenuReopened=True,permanentMessageRemoved=True,audioMuteDepth=audio['muteDepth'])
    save(out/'summary.json',summary)
    print(json.dumps(summary,ensure_ascii=False))


if __name__=='__main__':  # 명시 CLI 실행에서만 클론 창을 연다.
    main()
