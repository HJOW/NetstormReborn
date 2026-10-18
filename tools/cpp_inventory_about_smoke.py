#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 건설/연구 뒤 생산 항목 정보 메뉴의 Form/비용/Help와 닫기·재열기·후속 생산을 관찰한다."""
import argparse,json,subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT,EXE
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated
from cpp_priest_construction_smoke import asset_fields


def main():
    """판정/비용을 주입하지 않고 현재 실제 UI 버튼과 생산 항목만 입력한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve()
    if out.exists() and any(out.iterdir()):raise RuntimeError('새 관측 경로 필요')
    out.mkdir(parents=True,exist_ok=True)
    steps=[(5,'click','Play Mission'),(7,'right','world:priest'),(35,'click','Construct'),(65,'click','Workshop'),(95,'click',' Build Sun Workshop for 800'),
           (102,'up','cell:107,103'),(110,'click','cell:107,103'),(120,'report','placedWorkshop'),(1050,'report','completedWorkshop'),
           (1060,'right','world:sunFactory'),(1070,'click','Put Knowledge into Production'),(1080,'click','Sun Cannon'),(1100,'report','researched'),
           (1102,'right','production:128'),(1103,'report','aboutBefore'),(1103,'snapshot',str(out/'about-before.bmp')),(1104,'esc',''),(1105,'report','closedBefore'),
           (1106,'right','production:128'),(1107,'report','reopened'),(1108,'esc',''),(1109,'report','closedAgain'),
           (1110,'click','production:128'),(1120,'report','taken'),(1130,'up','cell:94,109'),(1140,'report','preparedCannon'),(1150,'click','cell:94,109'),(1160,'report','placedCannon'),
           (1600,'right','production:128'),(1610,'report','aboutAfter'),(1610,'snapshot',str(out/'about-after.bmp')),(1620,'esc',''),(1640,'report','closedAfter'),
           (2400,'report','after'),(2400,'snapshot',str(out/'after.bmp'))]
    script=out/'steps.tsv';report=out/'states.tsv';script.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--no-audio','--frames','2403','--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0','--ui-script',str(script),'--ui-report',str(report)]
    before=snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals','d'):
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=105);(out/'stdout.log').write_bytes(result.stdout);(out/'stderr.log').write_bytes(result.stderr)
    unchanged=snapshots(ROOT/'originals')==before
    if result.returncode or not unchanged:raise RuntimeError(dict(exitCode=result.returncode,unchanged=unchanged,stderr=result.stderr.decode('utf-8',errors='replace')))
    observed=states(report);menus=repeated(report,'raw_menu');items=repeated(report,'raw_menu_item');bindings=repeated(report,'raw_menu_object');production=repeated(report,'raw_production_item');headings=repeated(report,'raw_menu_heading');forms=[]
    # 각 실행의 실제 단일 메뉴와 같은 내용물 Form을 직접 읽는다.
    for label in ('aboutBefore','reopened','aboutAfter'):
        current=observed[label];assert current['phase']==['4'] and current['paused']==['0'] and current['raw_menus'][1]=='0',current['raw_menus']
        assert len(menus[label])==1 and int(menus[label][0][3])&0x40,menus[label]
        content=[row for row in production[label] if row[2]=='128'];assert len(content)==1,content
        assert len(bindings[label])==1 and bindings[label][0][0]==menus[label][0][0] and bindings[label][0][1]==content[0][1] and int(bindings[label][0][2])!=0,bindings[label]
        forms.append(bindings[label][0]);assert any('Cost: ' in row[-1] and '400' in row[-1] for row in headings[label]),headings[label]
        helpItems=[row for row in items[label] if int(row[3])==0x02000017];assert len(helpItems)==1 and helpItems[0][4]=='128' and helpItems[0][5]=='1',helpItems
        assert not any(int(row[3])==0x0200005d for row in items[label]),items[label]
    # ESC는 원본 지연 삭제를 거쳐 메뉴/Form을 정리하고 월드 플레이를 유지해야 한다.
    for label in ('closedBefore','closedAgain','closedAfter','after'):
        assert not menus[label] and not bindings[label] and observed[label]['phase']==['4'] and observed[label]['paused']==['0'],label
    assert observed['aboutBefore']['raw_sp']==observed['researched']['raw_sp'] and observed['reopened']['raw_sp']==observed['researched']['raw_sp']
    source=[row for row in production['researched'] if row[2]=='128'];assert len(source)==1 and observed['taken']['raw_cursor'][3]==source[0][1] and observed['taken']['raw_cursor'][0]=='0'
    assert observed['preparedCannon']['raw_cursor'][8:]==['1','1','1'] and observed['placedCannon']['raw_sp'][1]=='1800' and observed['after']['raw_sp'][1]=='1800'
    cannons={sid:row for sid,row in observed['placedCannon']['raw_objects'].items() if row[:2]==['128','1']};fields=asset_fields(report);assert cannons and all(fields['after'][sid]==[0,128,600] for sid in cannons),fields['after']
    summary=dict(passed=True,originalsUnchanged=unchanged,forms=forms,cannons=cannons,menus={label:items[label] for label in ('aboutBefore','reopened','aboutAfter')},sp=[observed['researched']['raw_sp'][1],observed['after']['raw_sp'][1]],limits=['실제 기본 생산 내용물의 About/비용/Help 항목·Form/ESC/재열기와 후속 생산·완공 관측이다. Help 명령 실행·실제 회수·SP 부족 창·전체 미션 완주는 별도다.'])
    (out/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(summary,ensure_ascii=False))


if __name__=='__main__':main()
