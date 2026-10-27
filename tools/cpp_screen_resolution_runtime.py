#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""메인/ESC의 실제 해상도 행·원본 확인/저장/종료와 새 프로세스 재시작을 분리 자산에서 관측한다."""
import argparse
import json
import platform
import re
import shutil
import subprocess
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,GAME,execute,save,sha
from cpp_world_smoke import states


def menu_rows(path):
    """보고서의 같은 실제 frame별 메뉴 행을 모두 유지한다. 일반 단일 레코드 판독기와 구별한다."""
    result={}
    for block in re.split(r'(?m)^frame\t',path.read_text('utf-8'))[1:]: # 중간 필드의 frame 부분 문자열은 자르지 않는다.
        header,body=block.split('\n',1);label=header.split('\t',1)[1]
        result[label]=[line.split('\t')[1:] for line in body.splitlines() if line.startswith('raw_menu_item\t')]
    return result


def main():
    """실제 표시 메뉴/경고 버튼을 클릭하고 저장된 암호화 Config는 기존 읽기 전용 CLI로 조회한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--case',choices=('main-no','main-yes','main-restart','main-nine-yes','nine-restart','mission-no','mission-yes','wide-restart','main-classic-yes','classic-restart'),required=True)
    args=parser.parse_args();root=args.output.resolve();root.mkdir(parents=True,exist_ok=True);directory=root/args.case;directory.mkdir(exist_ok=False);data=root/'game-data'
    if not data.exists():
        shutil.copytree(ROOT/'originals',data)
    mission=args.case.startswith('mission');restart=args.case.endswith('restart');yes=args.case in ('main-yes','main-nine-yes','mission-yes','main-classic-yes')
    target={'main-yes':'1280 by 800','main-no':'1280 by 800','main-restart':'1280 by 720','main-nine-yes':'1280 by 720','nine-restart':'800 by 600','mission-no':'1280 by 720','mission-yes':'1366 by 768','wide-restart':'640 by 480','main-classic-yes':'800 by 600','classic-restart':'1280 by 800'}[args.case]
    saved_before={key:subprocess.check_output([str(GAME),'--config-get',str(data),key],cwd=ROOT).decode('utf-8').strip() for key in ('SCREENW','SCREENH')} # CLI의 시작 크기 덮어쓰기와 별도로 기존 저장 설정을 기록한다.
    steps=[(3,'click-if','OK'),(5,'click-if','OK'),(10,'report','startup')]
    if mission:
        steps += [(8,'click','Play Mission'),(22,'report','stage'),(26,'esc',''),(30,'click','Options'),(34,'click','Resolution'),(38,'report','choices'),(42,'click',target),(46,'report','warning')];confirm=50
    else:
        steps += [(12,'click','Options'),(16,'click','Resolution >'),(20,'report','choices'),(24,'click',target),(28,'report','warning')];confirm=32
    # 같은 실제 보고 시각에 논리/실제 출력 캡처를 보존하며 메뉴 명령 자체는 직접 호출하지 않는다.
    for frame,label in [(38 if mission else 20,'choices'),(46 if mission else 28,'warning')]:
        steps += [(frame,'snapshot',str(directory/(label+'.bmp'))),(frame,'snapshot-output',str(directory/(label+'-output.bmp')))]
    steps += [(confirm,'click','Yes' if yes else 'No')]
    if yes:
        steps += [(confirm+24,'report','unexpected-alive')];frames=confirm+30
    else:
        steps += [(confirm+10,'report','cancelled')]
        if not mission:
            steps += [(confirm+15,'click','Quit')]
        frames=confirm+25
    steps.sort(key=lambda step:step[0]);script=directory/'steps.tsv';script.write_text(''.join(f'{f}\t{o}\t{a}\n' for f,o,a in steps),encoding='utf-8',newline='\n')
    setting='startInFullScreen=1;workingFullScreenFlags=11;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;edgeScroll=1;currentLanguage=english'
    if not restart:
        setting+=';SCREENW=1280;SCREENH=800' if mission else ';SCREENW=1024;SCREENH=768'
    command=[str(GAME),'--run',str(data),'--no-audio','--frames',str(frames),'--set',setting,'--ui-script',str(script),'--ui-report',str(directory/'states.tsv')]
    if mission:
        command += ['--mission','TheWarBegins']
    result=execute(command,directory,GAME,90);observed=states(directory/'states.tsv') if (directory/'states.tsv').exists() else {};rows=menu_rows(directory/'states.tsv') if observed else {}
    # Config 파일은 암호화되어 있으므로 원본과 같은 기존 C++ 설정 reader만 사용한다. 이 CLI는 게임/창을 시작하지 않는다.
    saved={key:subprocess.check_output([str(GAME),'--config-get',str(data),key],cwd=ROOT).decode('utf-8').strip() for key in ('SCREENW','SCREENH')}
    result.update(case=args.case,target=target,yes=yes,os=platform.platform(),host=platform.node(),stageCompletion=False,savedBefore=saved_before,saved=saved,savedOptionsSha256=sha(data/'d/options.cfg'),
        observations={label:{key:scene.get(key) for key in ('page','phase','size','screen_mode','game_menu_bar','current_dialog','raw_camera_frame')} for label,scene in observed.items()},
        resolutionRows={label:[r for r in values if re.fullmatch(r'\d+ by \d+',r[-1])] for label,values in rows.items()})
    save(directory/'result.json',result);print(json.dumps(result,ensure_ascii=False))
    return result['returnCode'] or int(result['timedOut'])


if __name__=='__main__':
    raise SystemExit(main())
