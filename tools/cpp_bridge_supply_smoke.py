#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제1-1 진입 뒤 자동 다리 공급과6초 품질 회복을 같은 GUI 루프에서 확인한다."""
import argparse,json,subprocess
from pathlib import Path
from cpp_campaign_entry_smoke import ROOT,EXE
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated

# 직접 생성/시간 주입 없이 실제 브리핑 버튼과 프레임 보고만 사용한다.
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve()
    if out.exists() and any(out.iterdir()):raise RuntimeError('새 관측 경로 필요')
    out.mkdir(parents=True,exist_ok=True);script=out/'steps.tsv';report=out/'states.tsv'
    steps=[(5,'click','Play Mission'),(10,'report','early'),(10,'snapshot',str(out/'early.bmp')),(1100,'report','mature'),(1100,'snapshot',str(out/'mature.bmp')),
           (1110,'click','production:82'),(1120,'report','taken'),(1130,'up','cell:109,97'),(1140,'report','prepared'),(1150,'click','cell:109,97'),(1160,'report','placed'),(1160,'snapshot',str(out/'placed.bmp')),(1600,'report','refilled'),(1600,'snapshot',str(out/'refilled.bmp'))]
    script.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in steps),encoding='utf-8',newline='\n')
    command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--no-audio','--frames','1603','--set','SCREENW=1024;SCREENH=768;maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0','--ui-script',str(script),'--ui-report',str(report)]
    before=snapshots(ROOT/'originals')
    with preserve_game_settings(ROOT/'originals','d'):
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=90);(out/'stdout.log').write_bytes(result.stdout);(out/'stderr.log').write_bytes(result.stderr)
    unchanged=snapshots(ROOT/'originals')==before
    if result.returncode or not unchanged:raise RuntimeError(dict(exitCode=result.returncode,unchanged=unchanged,stderr=result.stderr.decode('utf-8',errors='replace')))
    observed=states(report);rows=repeated(report,'raw_production_bridge');early=rows['early'];mature=rows['mature'];actual=observed['mature']['raw_production_maintenance']
    assert len(early)==1 and early[0][1:3]==['0','1'],early
    assert len(mature)==int(actual[5]) and all(row[2]=='2' for row in mature),mature
    assert actual[0]=='0' and observed['mature']['raw_sp'][1]=='3000',actual
    assert observed['taken']['raw_cursor'][0]=='0' and observed['taken']['raw_cursor'][2]=='82',observed['taken']['raw_cursor']
    assert observed['prepared']['raw_cursor'][8:]==['1','1','1'],observed['prepared']['raw_cursor']
    server={sid:row for sid,row in observed['placed']['raw_objects'].items() if sid>=15000 and row[:2]==['82','1']};assert server,observed['placed']['raw_cursor']
    refilled=rows['refilled'];assert len(refilled)==len(mature) and all(row[2]=='2' for row in refilled) and observed['refilled']['raw_sp'][1]=='3000',refilled
    summary=dict(passed=True,originalsUnchanged=unchanged,early=early,mature=mature,maintenance=actual,taken=observed['taken']['raw_cursor'],prepared=observed['prepared']['raw_cursor'],serverBridges=server,refilled=refilled,limits=['실제 미션의 자동 공급/품질 회복·아이콘 클릭·배치·재공급이며 공격·미션 완주 증거는 별도'])
    (out/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(summary,ensure_ascii=False))

# 명시 CLI 요청에서만 클론 창을 실행한다.
if __name__=='__main__':main()
