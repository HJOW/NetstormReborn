#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론의 현대 Windows 전체화면/창 전환·실제 출력·설정 재시작을 분리 자산에서 짧게 관찰한다."""
import argparse
import hashlib
import json
import platform
import shutil
import sys
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states


def main():
    """원본은 복사/해시 조회만 하고 실제 클론의 메뉴 입력과 관측 자료를 새 폴더에 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--case',choices=('menu','restart','stage','wide','nine','ten','window-override','adjust','timeout'),required=True)
    parser.add_argument('--label')
    args=parser.parse_args();root=args.output.resolve();root.mkdir(exist_ok=True)
    directory=root/(args.label or args.case);directory.mkdir(exist_ok=False);data=root/'game-data'
    before=snapshots(ROOT/'originals')
    if not data.exists():
        shutil.copytree(ROOT/'originals',data,ignore=lambda path,names:[name for name in names if name.lower().endswith('.fort') and name.lower()!='test01.fort'])
    steps=[(3,'click-if','OK'),(5,'click-if','OK'),(10,'report','startup')];frames=45
    setting='maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;currentLanguage=english';extra=[]
    if args.case=='menu':
        extra=['--window'];setting+=';SCREENW=1024;SCREENH=768;startInFullScreen=1;workingFullScreenFlags=111'
        steps += [(12,'click','Options'),(16,'click','Direct Draw / Full Screen'),(25,'report','full'),(25,'snapshot',str(directory/'logical-full.bmp')),(25,'snapshot-output',str(directory/'output-full.bmp')),
            (30,'click','Options'),(34,'click','Resolution >'),(38,'click','800 by 600'),(48,'report','full-resized'),(48,'snapshot-output',str(directory/'output-resized.bmp')),
            (54,'click','Options'),(58,'click','Window Mode'),(68,'report','window'),(68,'snapshot-output',str(directory/'output-window.bmp')),
            (74,'click','Options'),(78,'click','Direct Draw / Full Screen'),(88,'report','full-again'),(100,'click','Quit')];frames=120
    elif args.case=='restart':
        steps += [(15,'snapshot-output',str(directory/'output-restart.bmp')),(25,'click','Quit')]
    elif args.case=='stage':
        extra=['--mission','TheWarBegins'];setting+=';SCREENW=1024;SCREENH=768;startInFullScreen=1'
        steps=[(8,'report','briefing'),(15,'click','Play Mission'),(35,'report','stage'),(35,'snapshot',str(directory/'logical-stage.bmp')),(35,'snapshot-output',str(directory/'output-stage.bmp')),(36,'require',r'page\tmission'),(120,'report','stage-later')];frames=130
    elif args.case in ('wide','nine','ten'):
        resolutions={'wide':'SCREENW=1366;SCREENH=768','nine':'SCREENW=1280;SCREENH=720','ten':'SCREENW=1280;SCREENH=800'} # 실제 DIB 행·정확16:9·16:10의 분리 입력이다.
        setting+=';startInFullScreen=1;'+resolutions[args.case]
        steps += [(15,'snapshot',str(directory/'logical.bmp')),(15,'snapshot-output',str(directory/'output.bmp')),(25,'click','Quit')]
    elif args.case=='window-override':
        extra=['--window'];setting+=';startInFullScreen=1';steps += [(25,'click','Quit')]
    else:
        setting+=';SCREENW=1024;SCREENH=768;startInFullScreen=1;workingFullScreenFlags=111'
        steps += [(12,'click','Options'),(16,'click','Adjust Direct Draw'),(24,'report','adjust'),(28,'click','Safe & Jumpy Mouse'),(34,'report','mouse-explain'),(38,'click','Turn Off'),(45,'report','preview'),(50,'click','Cancel'),(58,'report','cancelled'),
            (62,'click','Options'),(66,'click','Adjust Direct Draw'),(72,'click','Safe & Jumpy Mouse'),(78,'click','Turn Off'),(85,'report','preview-again'),(90,'click','OK'),(98,'report','accepted'),(102,'click','OK'),(110,'click','Quit')];frames=125
        if args.case=='timeout':
            steps=steps[:10]+[(700,'report','timed-out'),(710,'click','Quit')];frames=725 # 첫 미리보기만 남겨 실제15초 타이머를 기다린다.
    # 옛 DirectDraw 실패 표식은 의도한 검사 입력이며 원본 파일에는 쓰지 않는다.
    marker=data/'fullscreenStateFile.dat';marker.write_bytes(b'legacy-directdraw-marker')
    script=directory/'steps.tsv';script.write_text(''.join(f'{at}\t{operation}\t{argument}\n' for at,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(GAME),'--run',str(data),'--no-audio','--frames',str(frames),'--set',setting,'--ui-script',str(script),'--ui-report',str(directory/'states.tsv'),*extra]
    result=execute(command,directory,GAME,90)
    result.update(os=platform.platform(),host=platform.node(),originalsUnchanged=before==snapshots(ROOT/'originals'),legacyMarkerRemoved=not marker.exists(),actualStageCompletion=False)
    observed=states(directory/'states.tsv') if (directory/'states.tsv').exists() else {}
    result['screens']={label:{key:scene.get(key) for key in ('page','screen_mode','screen_window','screen_output','screen_cursor_clip','mission')} for label,scene in observed.items()}
    options=data/'d/options.cfg';result['savedOptionsSha256']=hashlib.sha256(options.read_bytes()).hexdigest()
    save(directory/'result.json',result);print(json.dumps(result,ensure_ascii=False))
    return result['returnCode'] or int(not result['originalsUnchanged'])


if __name__=='__main__':
    sys.exit(main())
