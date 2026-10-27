#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 편집 맵의 제네레이터 메뉴로 Meltdown과 미션/편집 모드의 숨김 조건을 관측한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,GAME,execute,save
from cpp_game_menu_smoke import menu_items
from cpp_production_panel_smoke import repeated
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def one(output,source,size,mode):
    """새 사본에서 실제 우클릭과 ESC만 조작하며 숨긴 명령을 강제하지 않는다."""
    output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data)
    if mode=='editor':
        steps=[(3,'click-if','OK'),(5,'click-if','OK'),(10,'click','Edit'),(18,'click','breakingthrough'),(55,'report','initial'),(60,'key','right'),(74,'keyup','right'),(80,'right','cell:173,123'),(90,'report','generator-menu'),(90,'snapshot',str(output/'menu.bmp')),(100,'esc',''),(105,'report','escaped'),(110,'esc',''),(115,'report','final')]
    else:
        steps=[(5,'report','briefing'),(10,'click','Go!'),(20,'report','initial'),(25,'right','world:windBattery'),(35,'report','generator-menu'),(35,'snapshot',str(output/'menu.bmp')),(40,'esc',''),(45,'report','escaped'),(50,'esc',''),(55,'report','final')]
    plan=output/'steps.tsv';report=output/'states.tsv';plan.write_text(''.join(f'{frame}\t{op}\t{value}\n' for frame,op,value in steps),'utf-8')
    command=[str(GAME),'--run',str(data),'--window','--no-audio','--frames','125' if mode=='editor' else '65','--set',f'SCREENW={size[0]};SCREENH={size[1]};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0','--ui-script',str(plan),'--ui-report',str(report)]
    if mode=='mission':command.extend(['--mission','TEST02'])
    original=snapshots(ROOT/'originals');before=snapshots(data);run={}
    try:
        with preserve_game_settings(data,'d',output/'settings-backup'):run=execute(command,output,GAME,120)
    finally:
        run.update(originalsUnchanged=original==snapshots(ROOT/'originals'),dataUnchanged=before==snapshots(data));save(output/'run.json',run)
    assert run['returnCode']==0 and not run['timedOut'] and run['originalsUnchanged'] and run['dataUnchanged'],run
    observed=states(report);roots=[row for row in repeated(report,'raw_menu_object')['generator-menu'] if int(row[1])];assert len(roots)==1,roots
    sid=int(roots[0][1]);parent=observed['generator-menu']['objects'][sid];assert parent[0]=='windBattery',parent
    assert parent[1]==('2' if mode=='editor' else '1'),parent
    items=menu_items(report,'generator-menu');assert not any(row['command']==0x030000f8 for row in items),items
    assert sid in observed['final']['objects'] and observed['final']['objects'][sid][12]==parent[12]
    assert observed['generator-menu']['mouse_cursor'][1]=='1' and observed['final']['mouse_cursor'][1]=='1'
    assert observed['final']['raw_menus'][:3]==['0','0','0']
    result=dict(mode=mode,size=size,parentSid=sid,parentBefore=parent,parentAfter=observed['final']['objects'][sid],meltdownHidden=True,menu=items,run=run,attackFlyersNotReproduced=True,effectNotForced=True)
    save(output/'summary.json',result);return result


def main():
    """원본 보호 경로를 거부하고 두 화면비의 새 사본만 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--data-root',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--mode',choices=('editor','mission'),required=True);parser.add_argument('--size',default='both',choices=('both','1280x720','1024x768'));args=parser.parse_args()
    source=args.data_root.resolve();output=args.output.resolve();assert all(source!=(ROOT/name).resolve() and (ROOT/name).resolve() not in source.parents for name in ('originals','originalCD'))
    output.mkdir(parents=True,exist_ok=False);sizes=((1280,720),(1024,768)) if args.size=='both' else (tuple(map(int,args.size.split('x'))),)
    results=[]
    for size in sizes:results.append(one(output/f'{size[0]}x{size[1]}',source,size,args.mode))  # 앞 실행 실패를 다음 성공으로 덮지 않는다.
    save(output/'summary.json',dict(results=results));print('실제 Meltdown 검사 통과',len(results))


if __name__=='__main__':main()
