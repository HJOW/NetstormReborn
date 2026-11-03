#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사본 cpppj의 시작/메인 메뉴6프레임만 확인한다. 실제 실행 전 AGENTS 확인 규칙을 따른다."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time
from PIL import Image
from cpp_loading_screen_smoke import ROOT,SOURCE,CLONE,SETTINGS,prepare_copy,snapshot

def main():
    """최신 실행파일과 자산의 새 사본만 실행하며 앱 내부 보고서/출력 BMP를 수집한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve();assert out.is_relative_to(ROOT/'extracted');out.mkdir(parents=True,exist_ok=False)
    before=snapshot(SOURCE);SETTINGS.update(SCREENW='1024',SCREENH='768',autoDemo='0',tellTips='0',WelcomeGiven='1');command,exe,game,settings=prepare_copy(out/'copy',False,before,CLONE);shutil.copy2(CLONE.parent/'NetstormSmacker.dll',exe.parent/'NetstormSmacker.dll')
    plan=out/'steps.tsv';plan.write_text('1\treport\tfirst\n2\treport\tsecond\n4\treport\tmenu\n4\tsnapshot-presented\t'+str(out/'menu.bmp')+'\n',encoding='utf-8');command+=['--frames','6','--set','autoDemo=0;tellTips=0;WelcomeGiven=1;maxFPS=40;sleepPerLoop=1','--ui-script',str(plan),'--ui-report',str(out/'states.tsv')]
    start=time.monotonic();timed_out=False;code=None
    with (out/'stdout.log').open('wb') as stdout,(out/'stderr.log').open('wb') as stderr:
        try:code=subprocess.run(command,cwd=game,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW,timeout=50,check=False).returncode
        except subprocess.TimeoutExpired:timed_out=True  # subprocess.run은이자기자식만종료/대기하며실패자료를남긴다.
    after=snapshot(SOURCE);assert before==after
    image_info=None
    if (out/'menu.bmp').exists():
        with Image.open(out/'menu.bmp') as image:  # 앱내부BMP만읽으며물리화면/다른앱을캡처하지않는다.
            rgb=image.convert('RGB');colors=rgb.getcolors(maxcolors=1<<24);image_info=dict(size=list(rgb.size),colors=len(colors),sha256=hashlib.sha256((out/'menu.bmp').read_bytes()).hexdigest())
    state=(out/'states.tsv').read_text(encoding='utf-8') if (out/'states.tsv').exists() else '';stderr=(out/'stderr.log').read_text(encoding='utf-8',errors='replace')
    result=dict(unit=794,command=command,exitCode=code,timedOut=timed_out,elapsed=time.monotonic()-start,exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),resolution=[1024,768],framesRequested=6,menuReportAtFrame4='frame\t4\tmenu' in state,observedState42='State 42' in stderr,observedState41='State 41' in stderr,applicationBitmap=image_info,originalsUnchanged=before==after,originalFiles=len(before),settings=settings,gameExecutions=1,originalGameExecutions=0,physicalInput=0,physicalCapture=0,prohibitedGameplay=0,multiplayer=0,pcm=0)
    (out/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({key:result[key] for key in ('exitCode','timedOut','elapsed','menuReportAtFrame4','applicationBitmap','originalsUnchanged')},ensure_ascii=False))
    assert code==0 and not timed_out and result['menuReportAtFrame4'] and image_info and image_info['size']==[1024,768] and image_info['colors']>20

if __name__=='__main__':main()
