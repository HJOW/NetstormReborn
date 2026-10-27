#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원래 BreakingThrough의 자기/다른 소유자 워크샾·신전 메뉴를 실제 입력으로 확인한다."""
import argparse
import re
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import save
from cpp_game_menu_smoke import menu_items,protected_run
from cpp_production_panel_smoke import repeated
from cpp_solo_entry_smoke import BUTTONS
from cpp_world_smoke import states

SIZES=((1280,720),(1024,768))  # 기본4:3과확장16:9에서같은원본배치를사용한다.
CASES=(('own-workshop','rainFactory',1,'world:rainFactory','left',8),
       ('own-temple','rainVortex',1,'world:rainVortex','left',8),
       ('other-workshop','windFactory',2,'cell:180,118','right',14),
       ('other-temple','windVortex',2,'cell:180,125','right',14))  # 다른소유자는owner1전용world:도구대신원래몸체의칸을클릭한다.


def one(output,data,width,height,case):
    """실제브리핑/카메라/우클릭뒤현재대상·소유자전용항목·닫기를검사한다."""
    name,kind,owner,point,direction,duration=case;steps=[]
    for frame in range(12,72,8):  # 현재존재하는안내버튼만정상경로로넘긴다.
        for label in BUTTONS:  # 이미게임이시작되면브리핑버튼을추가클릭하지않는다.
            condition=r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)';steps.append((frame,'when',condition+';;click;;'+label))
    steps.extend([(76,'report','initial'),(80,'key',direction),(80+duration,'keyup',direction),(98,'report','position'),(100,'right',point),(110,'report','menu'),(110,'snapshot',str(output/'menu.bmp')),(120,'outside',''),(130,'report','closed')])
    report,run=protected_run(output,width,height,steps,140,data_root=data,mission='breakingthrough');result=verify(report,run,width,height,case);save(output/'summary.json',result);return result


def verify(report,run,width,height,case):
    """저장된 원시 보고서에도 같은 대상/항목/레벨 조건을 적용하며 게임은 실행하지 않는다."""
    name,kind,owner,point,direction,duration=case;observed=states(report);roots=[row for row in repeated(report,'raw_menu_object')['menu'] if int(row[1])];assert len(roots)==1,roots;root=roots[0];sid=int(root[1]);target=observed['menu']['objects'][sid]
    assert target[:2]==[kind,str(owner)] and observed['menu']['raw_session']==['1','1'] and observed['menu']['raw_world_modes']==['0','1'],target
    items=[row for row in menu_items(report,'menu') if row['visible'] and row['token']==int(root[0])];labels=[row['label'] for row in items];assert items and not any('CHEAT' in text.upper() for text in labels),labels
    own=owner==1;level=None # 워크샾의실제상위레벨비트는Upgrade표시에만사용한다.
    if 'workshop' in name:
        assert ('View Current Production' in labels)==own and ('Put Knowledge into Production' in labels)==own,labels
        fields=[row for row in repeated(report,'raw_asset_fields')['menu'] if int(row[0])==sid];assert len(fields)==1;level=int(fields[0][1])>>5 # 이열은raw[11]이며raw_object의frame열과구별한다. 원본가상80/레벨은이상위3비트를읽는다.
        assert any(text.startswith('Upgrade costs') for text in labels)==(own and level<3),labels
    else:
        assert ('View Netstorm Knowledge' in labels)==own,labels
    assert any(text.startswith('Salvage') for text in labels)==own,labels
    assert observed['closed']['raw_menus'][:3]==['0','0','0'] and observed['closed']['objects'][sid][:7]==target[:7]
    assert observed['menu']['mouse_cursor'][1]=='1' and observed['menu']['paused']==['0']
    headings=[row[-1] for row in repeated(report,'raw_menu_heading')['menu'] if row[0]==root[0]];return dict(name=name,size=[width,height],kind=kind,owner=owner,sid=sid,target=target,level=level,items=items,headings=headings,run=run)


def main():
    """원본과관측자료를바꾸지않도록새입력사본과새결과폴더만사용한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--data-root',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();source=args.data_root.resolve();output=args.output.resolve();assert source!=output and source not in output.parents;output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data);rows=[]
    for width,height in SIZES:  # 각사례는새게임프로세스이며보존설정/원본불변검사를함께수행한다.
        for case in CASES:  # 원래존재하는네객체만선택하고owner/HP/프레임을조작하지않는다.
            rows.append(one(output/f'{case[0]}-{width}x{height}',data,width,height,case))
    save(output/'summary.json',dict(results=rows,explicitObjectMutation=False,attackFlyersNotTested=True));print('실제 워크샾/신전 소유자별 일반 메뉴8사례 통과')


if __name__=='__main__':main()
