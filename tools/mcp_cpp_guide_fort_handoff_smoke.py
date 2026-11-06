#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Guide에서 AskEditFort/ReadFort로 State1 조기 반환과 다음 미션 설치를 관측한다."""
import argparse
import hashlib
import json
import os
import re
import threading
from pathlib import Path
from mcp_cpp_smoke import McpClient
from mcp_cpp_mission_abort_smoke import SAFE_BUTTONS,file_manifest

def main():
    """원문 앱 명령만 보내며 Native 번호/시계/승패/객체 상태는 쓰지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--game',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    game=args.game.resolve();out=args.output.resolve();root=Path(__file__).resolve().parents[1]
    for name in ('originals','originalCD'):  # 보호 원문에는 실제 저장/자료 출력을 보내지 않는다.
        protected=(root/name).resolve();assert all(path!=protected and protected not in path.parents for path in (game,out))
    out.mkdir(parents=True,exist_ok=False);(out/'tool-at-launch.py').write_bytes(Path(__file__).read_bytes());before=file_manifest(game)
    command=[str(args.exe.resolve()),'--run',str(game),'--mcp','--window','--no-audio','--set','intro=1;tellTips=0;autoDemo=0;currentLanguage=english;mayEditMissions=1']
    client=McpClient(command,dict(os.environ,NETSTORM_USER_DATA=str(out/'user-data')));watchdog=threading.Timer(180,client.process.kill);watchdog.start();actions=[];observations=[];result=dict(passed=False,exeSha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),command=command)
    def call(name,**arguments):
        """실제 MCP 오류를 전환 성공으로 바꾸지 않는다."""
        response=client.call(name,**arguments);assert not response.get('isError'),response;return response
    def observe(label):
        """완성된 보고만 고유 파일에 보존하고 실제 Native 상태/대여를 읽는다."""
        report=call('netstorm_report')['content'][0]['text'];name=f'{len(observations):03d}-{label}.tsv';(out/name).write_text(report,encoding='utf-8');rows={}
        for line in report.splitlines():  # 같은 키의 여러 버튼도 원문 순서로 보존한다.
            fields=line.split('\t');rows.setdefault(fields[0],[]).append(fields[1:])
        observations.append(dict(label=label,file=name,phase=rows.get('phase'),page=rows.get('page'),session=rows.get('raw_session'),fortPending=rows.get('raw_fort_pending'),mission=rows.get('mission'),state=rows.get('raw_local_state')));return rows
    def action(op,arg='',frames=1):
        """기존 명령/버튼을 실제 일반 입력 큐에 보내고 한두 프레임만 기다린다."""
        actions.append(dict(op=op,arg=arg));return call('netstorm_input',actions=[actions[-1]],wait_frames=frames,screenshot=False)
    def ready(label):
        """안전한 시작 안내만 닫고 실제 입력 가능한 Early1까지 기다린다."""
        for attempt in range(60):  # 사제 포획/희생·공격/승리 결과는 기다리지 않는다.
            call('netstorm_wait_frames',frames=5,screenshot=False);rows=observe(label+'-'+str(attempt))
            if rows.get('phase')==[['4']] and rows.get('paused')==[['0']] and 'raw_session' in rows:return rows
            labels={row[-1] for row in rows.get('raw_html_button',[]) if row};labels.update(row[0] for row in rows.get('control',[]) if row)
            choice=next((item for item in SAFE_BUTTONS if item in labels),None)
            if choice:action('click',choice,5)
        raise RuntimeError('실제 미션 입력 단계 미도달')
    try:
        client.request('initialize',dict(protocolVersion='2024-11-05',capabilities={},clientInfo=dict(name='guide-fort-handoff954',version='1')));client.notify('notifications/initialized');call('netstorm_wait_frames',frames=20,screenshot=False)
        for attempt in range(10):  # 첫 사용자 안내는 실제 버튼으로만 닫는다.
            rows=observe('main-'+str(attempt))
            if rows.get('page')==[['main']]:break
            labels={row[-1] for row in rows.get('raw_html_button',[]) if row};choice='OK' if 'OK' in labels else 'Main Menu' if 'Main Menu' in labels else None
            if choice:action('click',choice,5)
            else:call('netstorm_wait_frames',frames=10,screenshot=False)
        action('script-command','MissionBegin,tutorial1',5);initial=ready('initial');fort=initial['fort_names'][0][0];assert fort
        action('script-command','AskEditFort',2);observe('ask-edit')
        action('script-command','ReadFort,'+fort,2);observe('read-fort')
        for attempt in range(6):  # 짧은 설치 경계를 관측하며 미션 기한을 변경하지 않는다.
            call('netstorm_wait_frames',frames=1,screenshot=False);observe('handoff-'+str(attempt))
        ready('returned');result['passed']=True
    except Exception as error:result['error']=repr(error)
    finally:
        if client.process.poll() is None:
            try:call('netstorm_quit');client.process.wait(timeout=20)
            except Exception:client.process.kill();client.process.wait(timeout=10)
        watchdog.cancel();trace='\n'.join(client.stderr_lines);sequence=[int(match.group(1)) for line in trace.splitlines() if (match:=re.fullmatch(r'(?:State|로컬 미션 상태) (\d+)',line))];result.update(stateSequence=sequence,handoffObserved=sequence.count(1)==1 and sequence.count(7)==2 and sequence.count(14)==2,exitCode=client.process.returncode,dataUnchanged=file_manifest(game)==before,actions=actions,observations=observations);(out/'stderr.log').write_text('\n'.join(client.stderr_lines),encoding='utf-8');(out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    assert result['passed'] and result['handoffObserved'] and result['exitCode']==0 and result['dataUnchanged'],result.get('error',result)
    print(json.dumps(dict(passed=True,observations=len(observations),exitCode=0)))

if __name__=='__main__':main()
