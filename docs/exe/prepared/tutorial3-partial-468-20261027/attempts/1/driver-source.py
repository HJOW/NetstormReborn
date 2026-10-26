#!/usr/bin/env python3
"""원본 상태를 쓰지 않고 별도 자산 복사본의 Tutorial3 실제 입력/원시만 관찰한다."""
import hashlib
import json
import sys
from pathlib import Path

# 현재 clone 실행과 설정 보존은 기존 검사 장치를 재사용한다.
ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'tools'))
from cpp_bridge_ritual_runtime import execute,GAME
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states


def prepare_one(out):
    """첫 시도는 실제 브리핑·골렘·간헐천/적섬 연결 입력만 준비한다."""
    rows=[]
    def add(frame,operation,argument):
        """허용 입력 또는 읽기 전용 관찰 행을 기록한다."""
        assert operation in ('click','right','esc','when','report','snapshot')
        rows.append((frame,operation,argument))
    def when(frame,condition,operation,argument):
        """현재 report만 읽고 같은 실제 입력 큐로 보낼 사건을 제한한다."""
        assert operation in ('click','right','esc');add(frame,'when',condition+';;'+operation+';;'+argument)
    mission=r'page\tmission\n'
    add(4,'report','initial');add(4,'snapshot',str(out/'initial.bmp'))
    when(25,r'page\tA\.\n','click','MORE');when(35,r'page\tA1\.\n','click','OK')
    when(60,mission+r'&&!object\t[0-9]+\tsunWalker\t1\t','click','production:133')
    when(75,mission+r'&&raw_cursor\t0\t[0-9]+\t133\t[1-9][0-9]*\t','click','placement-near:128,96')
    for frame in range(100,501,40):
        when(frame,mission+r'&&mission_gate\t3\t65\t','click','production:82')
        when(frame+10,mission+r'&&mission_gate\t3\t65\t&&raw_cursor\t0\t[0-9]+\t82\t[1-9][0-9]*\t','click','placement-reach:103,106')
    when(560,mission+r'&&object\t[0-9]+\tsunWalker\t1\t','click','world:sunWalker')
    when(575,mission+r'&&raw_input\t[1-9][0-9]*\t(2|5)\t','click','cell:103,106')
    for frame in range(600,2301,100):
        for offset,page,label in ((0,'B.','MORE'),(5,'B1.','OK'),(10,'C.','OK'),(15,'D.','MORE'),(20,'D1.','OK')):
            when(frame+offset,'page\\t'+page.replace('.','\\.')+'\\n','click',label)
        when(frame+30,mission+r'&&mission_gate\t3\t66\t','click','production:82')
        when(frame+40,mission+r'&&mission_gate\t3\t66\t&&raw_cursor\t0\t[0-9]+\t82\t[1-9][0-9]*\t','click','placement-reach:130,129')
    for frame in (90,200,400,550,590,800,1000,1200,1400,1600,1800,2000,2200,2350):
        add(frame,'report','f'+str(frame))
    for frame in (90,550,1200,1800,2350):add(frame,'snapshot',str(out/('f'+str(frame)+'.bmp')))
    rows.sort(key=lambda row:row[0]);return rows,2404


def main():
    """최초 준비/실제 실행/실패 원시와 원본·복사본·실행 파일 불변을 보존한다."""
    number=sys.argv[1];out=ROOT/('extracted/cpp-tutorial3-468-attempt'+number);assert out.is_dir();data=ROOT/'extracted/cpp-tutorial3-468-data'
    identity=hashlib.sha256(GAME.read_bytes()).hexdigest();assert identity=='6bfa1865fcc6bee83a82577e740f55e4b6723fd9e03f7c5f9021ab43a4d88567',identity
    assert number=='1';rows,limit=prepare_one(out);script=out/'steps.tsv';assert not script.exists();script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in rows),encoding='utf8',newline='\n')
    settings='SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;useProductTimers=1'
    command=[str(GAME),'--run',str(data),'--mission','Tutorial3','--window','--no-audio','--frames',str(limit),'--set',settings,'--ui-script',str(script),'--ui-report',str(out/'states.tsv')]
    profile=dict(command=command,originalGameExecuted=False,testStateInjection=False,testSpHpOwnerInjection=False,testBridgePatterns=False,initialProgressInjected=False,gameSha256=identity,inputSha256=hashlib.sha256(script.read_bytes()).hexdigest(),allowedOperations=['click','right','esc','when','report','snapshot'])
    (out/'prepared.json').write_text(json.dumps(profile,ensure_ascii=False,indent=2)+'\n',encoding='utf8');original=snapshots(ROOT/'originals');before=snapshots(data)
    with preserve_game_settings(data,'d',out/'settings-backup'):result=execute(command,out,GAME,360)
    result.update(profile=profile,originalsUnchanged=snapshots(ROOT/'originals')==original,dataUnchanged=snapshots(data)==before,binaryUnchanged=hashlib.sha256(GAME.read_bytes()).hexdigest()==identity)
    if (out/'states.tsv').exists():
        current=states(out/'states.tsv');result['observations']={name:{key:value for key,value in state.items() if key in ('frame','mission','page','mission_gate','raw_input','raw_cursor','raw_sp','mission_progress')} for name,state in current.items()}
    (out/'execution.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8');print(json.dumps(result,ensure_ascii=False),flush=True)
    if result['returnCode'] or result['timedOut'] or not all(result[k] for k in ('originalsUnchanged','dataUnchanged','binaryUnchanged')):raise SystemExit(1)


if __name__=='__main__':main()
