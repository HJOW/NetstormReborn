#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 전체화면의 F12가 모달 앞에서 작동하고 창 모드에서는 보존되는지 실제 메뉴에서 확인한다."""
import argparse
import json
from pathlib import Path
import shutil
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states


def main():
    """별도 자산 사본/설정을 사용하고 실제 입력·현재 화면 모드·기존 모달을 관측한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    data = out/'game'
    before = snapshots(ROOT/'originals')
    shutil.copytree(ROOT/'originals',data,ignore=lambda directory,names:[name for name in names if name.lower().endswith('.fort') and name.lower()!='test01.fort'])
    steps = [(3,'click-if','OK'),(5,'click-if','OK'),(10,'report','startup'),
             (14,'click','Options'),(18,'click','Adjust Direct Draw'),(24,'report','modal'),
             (28,'key','f12'),(34,'report','toggled'),(38,'keyup','f12'),(44,'report','released'),
             (48,'key','ctrl+shift+alt+f12'),(54,'report','modified'),(58,'snapshot-presented',str(out/'modal.bmp')),
             (62,'click','OK'),(68,'click','Options'),(72,'click','Window Mode'),(80,'report','window'),
             (84,'key','f12'),(90,'report','windowAfterKey'),(96,'click','Quit')]
    script = out/'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    command = [str(GAME),'--run',str(data),'--no-audio','--frames','106','--set','SCREENW=1024;SCREENH=768;startInFullScreen=1;workingFullScreenFlags=111;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;currentLanguage=english','--ui-script',str(script),'--ui-report',str(out/'states.tsv')]
    result = execute(command,out,GAME,60)
    result['originalsUnchanged'] = before == snapshots(ROOT/'originals')
    observed = states(out/'states.tsv') if (out/'states.tsv').exists() else {}
    result['observations'] = {name:{key:scene.get(key) for key in ('page','phase','paused','screen_mode','screen_window','screen_output','raw_html_window','raw_html_button','raw_html_text')} for name,scene in observed.items()}
    save(out/'result.json',result)
    save(out/'observations.json',result['observations'])
    try:
        assert result['returnCode']==0 and not result['timedOut'] and result['originalsUnchanged']
        mode = {name:int(scene['screen_mode'][0]) for name,scene in observed.items()}
        assert mode['startup'] & 1 and mode['modal']==mode['startup']
        assert mode['toggled']==(mode['modal']^0x40),mode
        assert mode['released']==mode['toggled'] and mode['modified']==mode['modal'],mode
        assert not mode['window']&1 and mode['windowAfterKey']==mode['window'],mode
        for label in ('modal','toggled','released','modified'):  # F12는 확인 Tell이나 기존 모달 닫기를 추가하지 않는다.
            assert 'AdjustDirectDraw' in '\t'.join(observed[label].get('page',[])),(label,observed[label].get('page'))
        result['observerPassed'] = True
    except Exception as error:
        result['observerPassed'] = False
        result['observerError'] = repr(error)
    save(out/'result.json',result)
    print(json.dumps({key:result[key] for key in ('returnCode','seconds','originalsUnchanged','observerPassed')},ensure_ascii=False))
    return int(not result['observerPassed'])


if __name__ == '__main__':
    raise SystemExit(main())
