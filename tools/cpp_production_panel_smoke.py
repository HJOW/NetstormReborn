#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 건설/연구 뒤 원본 생산 항목의 클릭/Take/배치를 같은 UI로 관측한다."""
import argparse,json,subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT,EXE
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states

# 실제 반복 행을 덮어쓰지 않고 보고 지점별로 읽는다.
def repeated(report,key):
    result={}
    for block in report.read_text('utf-8').split('frame\t')[1:]:
        header,body=block.split('\n',1);_,label=header.split('\t',1)
        result[label]=[line.split('\t')[1:] for line in body.splitlines() if line.startswith(key+'\t')]
    return result

# 이미 검증한 게임 동작을 같은 원본 UI 명령으로 실행하고 결과만 읽는다.
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve()
    if out.exists() and any(out.iterdir()):raise RuntimeError('새 관측 경로 필요')
    out.mkdir(parents=True,exist_ok=True)
    steps=[(5,'click','Play Mission'),(7,'right','world:priest'),(35,'click','Construct'),(65,'click','Workshop'),(95,'click',' Build Sun Workshop for 800'),
           (102,'up','cell:107,103'),(105,'report','preparedWorkshop'),(110,'click','cell:107,103'),(120,'report','placedWorkshop'),
           (1050,'report','completedWorkshop'),(1060,'right','world:sunFactory'),(1070,'click','Put Knowledge into Production'),(1080,'click','Sun Cannon'),
           (1100,'report','researched'),(1100,'snapshot',str(out/'production.bmp')),
           (1110,'click','production:128'),(1120,'report','taken'),(1120,'snapshot',str(out/'taken.bmp')),
           (1130,'up','cell:94,109'),(1140,'report','preparedCannon'),(1150,'click','cell:94,109'),(1160,'report','placedCannon'),(1160,'snapshot',str(out/'placed.bmp')),
           (2400,'report','after'),(2400,'snapshot',str(out/'after.bmp')),
           (2405,'key','right'),(2425,'keyup','right'),(2426,'report','scrolled'),(2440,'report','stopped')]
    script=out/'steps.tsv';report=out/'states.tsv';script.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--no-audio','--frames','2443','--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8','--ui-script',str(script),'--ui-report',str(report)]
    before=snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals','d'):
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=90)
        (out/'stdout.log').write_bytes(result.stdout);(out/'stderr.log').write_bytes(result.stderr)
    unchanged=snapshots(ROOT/'originals')==before
    if result.returncode or not unchanged:raise RuntimeError(dict(exitCode=result.returncode,unchanged=unchanged,stderr=result.stderr.decode('utf-8',errors='replace')))
    observed=states(report);items=repeated(report,'raw_production_item');researched=observed['researched'];taken=observed['taken'];placed=observed['placedCannon']
    source=[row for row in items['researched'] if int(row[2])==128];assert len(source)==1,source
    assert int(taken['raw_cursor'][3])==int(source[0][1]) and int(taken['raw_cursor'][2])==128 and taken['raw_cursor'][0]=='0',taken['raw_cursor']
    assert observed['preparedCannon']['raw_cursor'][8:]==['1','1','1'],observed['preparedCannon']['raw_cursor']
    assert float(placed['raw_sp'][1])<float(researched['raw_sp'][1]),(researched['raw_sp'],placed['raw_sp'])
    cannons={sid:row for sid,row in placed['raw_objects'].items() if row[:2]==['128','1']};assert cannons,cannons
    # 생산 루트가 있는 상태에서 좌표0의 키 사건도 실제 월드로 전달하고, 키를 떼면 카메라가 멈춰야 한다.
    camera=[observed[label]['camera'] for label in ('after','scrolled','stopped')]
    assert int(camera[1][0])>int(camera[0][0]) and camera[1][1]==camera[0][1] and camera[2]==camera[1],camera
    summary=dict(passed=True,originalsUnchanged=unchanged,content=source[0],taken=taken['raw_cursor'],prepared=observed['preparedCannon']['raw_cursor'],sp=[researched['raw_sp'][1],placed['raw_sp'][1]],cannons=cannons,camera=camera,limits=['한 지점의 실제 UI 건설/연구/생산/Take/배치와 키 누름/해제 관측이며 공격·미션 완주 판정은 별도'])
    (out/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(summary,ensure_ascii=False))

# 명시 CLI 요청에서만 클론 창을 실행한다.
if __name__=='__main__':main()
