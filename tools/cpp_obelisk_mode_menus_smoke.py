#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원래 캠페인 비석의 편집/일반 메뉴와 별도 special 관문을 두 화면비에서 확인한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,GAME,execute,save
from cpp_game_menu_smoke import menu_items,protected_run
from cpp_production_panel_smoke import repeated
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

SIZES=((1280,720),(1024,768))  # 원본4:3과확장16:9의현재같은제품을순차실행한다.


def editor(output,source,width,height):
    """실제 Edit에서는 Turn To Spell을 표시하고 Dig Up은 별도 관문에 따라 숨긴다."""
    output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data)
    steps=[(3,'click-if','OK'),(5,'click-if','OK'),(10,'click','Edit'),(18,'click','dissolvedalliance'),(55,'report','editor'),(60,'key','left'),(68,'keyup','left'),(75,'report','position'),(80,'right','cell:88,119'),(90,'report','menu'),(90,'snapshot',str(output/'menu.bmp')),(100,'outside',''),(115,'report','closed'),(115,'snapshot',str(output/'closed.bmp'))]
    plan=output/'steps.tsv';plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),'utf-8',newline='\n')
    command=[str(GAME),'--run',str(data),'--window','--no-audio','--frames','130','--set',f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0','--ui-script',str(plan),'--ui-report',str(output/'states.tsv')]
    with preserve_game_settings(data,'d',output/'settings-backup'):run=execute(command,output,GAME,120)
    save(output/'run.json',run);assert run['returnCode']==0 and not run['timedOut'],run
    report=output/'states.tsv';observed=states(report);before=observed['menu'];after=observed['closed'];assert before['raw_world_modes']==after['raw_world_modes']==['1','1']
    roots=[row for row in repeated(report,'raw_menu_object')['menu'] if int(row[1])];assert len(roots)==1,roots;old=int(roots[0][1]);assert before['objects'][old][0]=='buried'
    commands=[row for row in menu_items(report,'menu') if row['visible']];assert not any(row['command']==0x0200002d for row in commands),commands
    convert=[row for row in commands if row['command']==0x030000ce];assert len(convert)==1 and convert[0]['parameter']==old,convert
    assert any(row['label']=='Destroy' for row in commands) and any(row['label']=='CHEAT Set Owner' for row in commands)
    assert after['objects'][old][:5]==before['objects'][old][:5] and after['raw_menus'][:3]==['0','0','0'];result=dict(mode='editor',size=[width,height],sid=old,before=before['objects'][old],after=after['objects'][old],commands=commands,run=run);save(output/'summary.json',result);return result


def normal(output,source,width,height):
    """같은캠페인의일반모드에서는비석명령을임의로노출시키지않는다."""
    data=output.parent/(output.name+'-source');shutil.copytree(source,data)
    steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),(25,'key','left'),(33,'keyup','left'),(45,'right','cell:88,119'),(53,'report','menu'),(53,'snapshot',str(output/'menu.bmp')),(65,'outside',''),(75,'report','closed')]
    report,run=protected_run(output,width,height,steps,84,data_root=data,mission='dissolvedalliance');observed=states(report);roots=[row for row in repeated(report,'raw_menu_object')['menu'] if int(row[1])];assert len(roots)==1,roots;sid=int(roots[0][1]);assert observed['menu']['objects'][sid][0]=='buried' and observed['menu']['raw_world_modes']==['0','1']
    commands=[row for row in menu_items(report,'menu') if row['visible']];assert commands and not any(row['command'] in (0x0200002d,0x030000ce) for row in commands);assert not any('CHEAT' in row['label'].upper() for row in commands);assert observed['closed']['raw_menus'][:3]==['0','0','0'] # CHEAT 항목은 Edit 편집에서만 나타날 수 있다는 사용자 기준을 확인한다.
    result=dict(mode='normal',size=[width,height],sid=sid,commands=commands,run=run);save(output/'summary.json',result);return result


def main():
    """새출력폴더와데이터사본만쓰며원본/이전관측자료는수정하지않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--data-root',type=Path,required=True);args=parser.parse_args();output=args.output.resolve();source=args.data_root.resolve();assert source!=output and source not in output.parents;output.mkdir(parents=True,exist_ok=False);rows=[]
    for width,height in SIZES:  # 각명령은새세션과별도데이터사본에서실행한다.
        rows.append(editor(output/f'editor-{width}x{height}',source,width,height));rows.append(normal(output/f'normal-{width}x{height}',source,width,height))
    save(output/'summary.json',dict(results=rows,attackFlyersNotTested=True,explicitObjectMutation=False,digUpInvocations=0));print('실제 비석 편집/일반 메뉴 관문 두 화면비4사례 통과')


if __name__=='__main__':main()
