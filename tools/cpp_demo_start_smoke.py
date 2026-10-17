#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""메인 메뉴의 실제 Demo/세 목록을 눌러 StartDemo와 같은 미션 Start 저장소를 확인한다."""
import argparse
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT,EXE
from cpp_fort_smoke import script_value
from cpp_menu_smoke import source
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from taff import TaffArchive

DEMOS=(('DemoStormRages','The Storm Rages'),('GuardMyBack','Demo of "Guard My Back"'),('DissolvedAlliance','Demo of "Dissolved Alliance"')) #원본 tell.DEMOVILLE의 순서/라벨이다.


def run(mission,label,output,archive):
    """상태를 쓰지 않고 실제 버튼 두 번과 원본15초 안내의 자연 timeout만 사용한다."""
    directory=output/mission
    directory.mkdir(parents=True)
    steps=[(2,'report','menu'),(3,'click','Demo'),(4,'report','choices'),(5,'click',label),(8,'report','started'),(8,'snapshot',str(directory/'started.bmp')),
           (1100,'report','after'),(1100,'snapshot',str(directory/'after.bmp'))]
    script=directory/'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','1102','--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=16','--ui-script',str(script),'--ui-report',str(directory/'states.tsv')]
    startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
    result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=50)
    (directory/'stdout.log').write_bytes(result.stdout);(directory/'stderr.log').write_bytes(result.stderr)
    assert result.returncode==0,dict(exitCode=result.returncode,stderr=result.stderr.decode('utf-8',errors='replace'))
    observed=states(directory/'states.tsv');initial=observed['started'];after=observed['after']
    assert 'Demo' in observed['menu']['controls'] and label in observed['choices']['controls'],observed
    assert initial['mission'][0].lower()==mission.lower() and initial['mission_start'][:3]==['1','1','1'],initial
    assert initial['page']==['Demo'] and initial['phase']==['3'] and initial['paused']==['1'],initial
    assert int(initial['raw_brains'][1])!=0 and after['raw_brains']==initial['raw_brains'],(initial,after)
    original=source(ROOT/'originals',archive,f'd/{mission}.english').decode('cp1252')
    money=int(script_value(original,'myStartMoney') or '0') #직접 플레이 SP는 AI 내부 지갑의 시작 금액과 별도다.
    technology=set(filter(None,(script_value(original,'myTech') or '').split(';'))) #Start가 먼저 실제 Player에 추가하는 기술이다.
    aiTech=script_value(original,'ai1Tech') or script_value(original,'aiTech') or 'sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher'
    technology.update(filter(None,aiTech.split(';'))) #실제 AI 초기 갱신은 이전 기술을 지우지 않고 추가한다.
    assert float(initial['raw_sp'][1])==money and sorted(initial['raw_technology'])==sorted(technology),initial
    assert after['phase']==['4'] and after['paused']==['0'] and after['mission_start']==initial['mission_start'],after
    return dict(mission=mission,label=label,passed=True,command=command,started=dict(start=initial['mission_start'],sp=initial['raw_sp'],technology=initial['raw_technology'],brains=initial['raw_brains']),after=dict(phase=after['phase'],paused=after['paused'],time=after['time']),directory=directory.as_posix())


def main():
    """실제 설정을 보존하고 세 데모를 순차 실행해 모든 결과/실패와 원본 불변 여부를 남긴다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    output=args.output.resolve()
    if output.exists() and any(output.iterdir()):raise RuntimeError('새 관측 경로가 필요합니다: '+str(output))
    output.mkdir(parents=True,exist_ok=True);before=snapshots(ROOT/'originals');archive=TaffArchive(ROOT/'originals/netstorm.tarc');records=[]
    with preserve_game_settings(ROOT/'originals','d'):
        #같은 options.cfg를 공유하므로 실제 창은 한 번에 하나만 실행한다.
        for mission,label in DEMOS:
            try:record=run(mission,label,output,archive)
            except Exception as error:record=dict(mission=mission,passed=False,error=str(error))
            records.append(record);print(mission,'PASS' if record['passed'] else 'FAIL',record.get('error',''),flush=True)
    report=dict(scope='실제 메인 메뉴→StartDemo→시작 자금/기술/AI/원본15초 안내 timeout. 전략AI/전투 완주/승패/자동재시작은 미검증.',runs=records,originals_unchanged=snapshots(ROOT/'originals')==before)
    (output/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    if not report['originals_unchanged'] or any(not record['passed'] for record in records):raise SystemExit(1)


if __name__=='__main__':main()
