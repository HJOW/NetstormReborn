#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""목표11스테이지에서 실제 브리핑 버튼을 눌러 같은 raw 월드의 플레이 진입을 검사한다."""
import argparse
import json
import subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT,EXE
from cpp_world_smoke import states
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_fort_smoke import script_value
from cpp_menu_smoke import source
from taff import TaffArchive

MISSIONS=('tutorial1','tutorial2','tutorial3','tutorial4','tutorial5','tutorial6',
          'thewarbegins','masterofwhirligigs','demostormrages','guardmyback','dissolvedalliance') #원본 Early6/캠페인2/데모3이다.
ARCHIVE=TaffArchive(ROOT/'originals/netstorm.tarc') #시작 기대값은 C++ 보고서와 독립인 보호 원문에서 읽는다.
MAX_CLICKS=20 #잘못된 페이지 순환이 무한 창 실행으로 이어지지 않게 하는 검사 한계다.

def probe(mission,clicks,index,output):
    """관측한 실제 클릭 접두만 새 클론에서 재생하고 마지막 입력 다음 프레임을 읽는다."""
    directory=output/mission/str(index)
    directory.mkdir(parents=True,exist_ok=True)
    frame=4+2*len(clicks) #마지막 클릭은frame-1이다. UI 입력 소비 전의 이전 페이지를 읽지 않는다.
    steps=[(5+i*2,'click',label) for i,label in enumerate(clicks)] #직전 관측에서 보였던 버튼만 순서대로 클릭한다.
    steps.extend(((frame,'report','current'),(frame,'snapshot',str(directory/'current.bmp'))))
    script=directory/'steps.tsv'
    script.write_text(''.join(f'{number}\t{operation}\t{argument}\n' for number,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission',mission,'--window','--no-audio',
             '--frames',str(frame+2),'--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0',
             '--ui-script',str(script),'--ui-report',str(directory/'states.tsv')]
    startup=subprocess.STARTUPINFO() #별도 콘솔 창을 전면에 표시하지 않는다.
    startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow=0
    result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=55)
    (directory/'stdout.log').write_bytes(result.stdout)
    (directory/'stderr.log').write_bytes(result.stderr)
    if result.returncode:
        raise RuntimeError(str(dict(exitCode=result.returncode,stderr=result.stderr.decode('utf-8',errors='replace'))))
    current=states(directory/'states.tsv')['current']
    assert current['mission'][0].lower()==mission and current['raw_objects'] and int(current['raw_world'][2])>0,current
    return dict(frame=frame,phase=current['phase'],paused=current['paused'],page=current['page'],
                controls=current['controls'],mission=current['mission'],raw_world=current['raw_world'],
                priestOwners=[v[1] for v in current['objects'].values() if v[0]=='priest'],
                mission_start=current['mission_start'],mission_rules=current['mission_rules'],
                raw_session=current['raw_session'],raw_sp=current['raw_sp'],raw_brains=current['raw_brains'],raw_technology=current['raw_technology'],
                directory=directory.as_posix(),exitCode=result.returncode)

def check(mission,output):
    """MORE/OK/Play Mission은 현재 실제 보고서에서 확인한 경우에만 누른다."""
    script=source(ROOT/'originals',ARCHIVE,f'd/{mission}.english').decode('cp1252') #원본 미션의 시작 입력이다.
    money=int(script_value(script,'myStartMoney') or '0') #실제 원본은 atol 뒤 float로 쓴다. 이11개 입력은 정확한 정수다.
    technology=sorted(filter(None,(script_value(script,'myTech') or '').split(';'))) #타입 번호 순서와 독립인 이름 집합을 대조한다.
    clicks=[] #성공적으로 관측한 입력 접두이며 내부 페이지/게임 상태를 직접 쓰지 않는다.
    observations=[] #각 실행의 실제 프레임/컨트롤/월드 상태다.
    record=dict(mission=mission,passed=False,clicks=clicks,observations=observations)
    try:
        #각 접두는 새 프로세스에서 재생해 이전 프로세스의 숨은 상태를 빌리지 않는다.
        for index in range(MAX_CLICKS+1):
            current=probe(mission,clicks,index,output)
            observations.append(current)
            if current['phase']==['4'] and current['paused']==['0']:
                assert '1' in current['priestOwners'] and current['raw_world'][6:9]==['0','0','0'],current
                assert current['mission_start'][:3]==['1','1','0'],current
                assert current['raw_session']==['1','1'] and float(current['raw_sp'][1])==money,current
                assert sorted(current['raw_technology'])==technology,current
                assert current['raw_brains'][1]=='0',current #CLI MissionBegin은 이름이 데모여도 로컬 AI를 만들지 않는다.
                if mission in ('thewarbegins','masterofwhirligigs'):
                    assert int(current['raw_brains'][2])!=0,current
                record['expectedStart']=dict(money=money,technology=technology,demo=0)
                record['passed']=True
                break
            assert current['phase']==['3'] and current['paused']==['1'],current
            if index==MAX_CLICKS:
                raise RuntimeError('실제 브리핑 버튼20개 뒤에도 플레이 진입 없음')
            controls=current['controls'] #실제 표시된 버튼 이름을 그대로 사용한다.
            if 'MORE' in controls:
                clicks.append('MORE')
            elif 'Play Mission' in controls:
                clicks.append('Play Mission')
            elif 'OK' in controls:
                clicks.append('OK')
            else:
                raise RuntimeError('관측된 브리핑 버튼 미지원 '+str(list(controls)))
    except Exception as error:
        record['error']=str(error)
    return record

def main():
    """보호 설정을 복원하고11개 실제 진입 결과를 모두 보존한 뒤 실패를 반환한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'extracted/goal-stage-entry',help='새 관측 폴더')
    args=parser.parse_args()
    output=args.output.resolve() #child cwd와 무관하게 실제 입력/보고서 경로를 전달한다.
    if output.exists() and any(output.iterdir()):
        raise RuntimeError('기존 관측을 보존하려면 새로운 --output 폴더를 지정하십시오: '+str(output))
    output.mkdir(parents=True,exist_ok=True)
    before=snapshots(ROOT/'originals') #설정 예외를 포함한 전체 원본 디렉터리의 전후 스냅샷이다.
    records=[] #일부 실패해도 다른 목표 스테이지의 결과를 버리지 않는다.
    with preserve_game_settings(ROOT/'originals','d'):
        #실제 설정 파일을 공유하므로 창은 한 번에 하나만 시작한다.
        for mission in MISSIONS:
            record=check(mission,output)
            records.append(record)
            print(mission,'PASS' if record['passed'] else 'FAIL',record['clicks'],record.get('error',''),flush=True)
    unchanged=snapshots(ROOT/'originals')==before
    report=dict(scope='CLI --mission/실제 UI 버튼으로11개 플레이 진입. 사용자 메뉴 선택·자연 명령·승패·저장·완주/원본 전체 동일성은 미검증.',
                runs=records,originals_unchanged=unchanged)
    (output/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    if not unchanged or any(not record['passed'] for record in records):
        raise SystemExit(1)

if __name__=='__main__':main()
