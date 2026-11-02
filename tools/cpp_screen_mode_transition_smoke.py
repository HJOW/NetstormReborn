#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""분리 자산에서 실제 화면 변경 실패 복구·조정 취소/확인/15초 만료와 저장 재시작을 관측한다."""
import argparse
import json
from pathlib import Path
import re
import shutil
from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_renderer_smoke import snapshots
from cpp_world_smoke import states
from nscfg import decode_file


def read_options(path):
    """실제 종료 설정의 화면 관련 정수 두 개만 독립 복호화기로 읽는다."""
    text=decode_file(str(path))
    result={}
    for key in ('startInFullScreen','workingFullScreenFlags'):  # 마지막 확인 모드와 시작 모드는 별도 설정이다.
        found=re.search(r'(?im)^\s*'+key+r'\s*=\s*"?(-?\d+)"?',text)
        if not found:
            raise RuntimeError('화면 설정 부재: '+key)
        result[key]=int(found[1])
    return result


def restart_game(data,out,before):
    """저장된 같은 자산만 재실행하며 시작 안내를 닫은 뒤 창 모드와 마지막 확인값을 관측한다."""
    out.mkdir()
    script=out/'steps.tsv'
    script.write_text('3\tclick-if\tOK\n5\tclick-if\tOK\n10\treport\tstartup\n20\tclick\tQuit\n',encoding='utf-8')
    expected=read_options(data/'d/options.cfg')
    again=execute([str(GAME),'--run',str(data),'--no-audio','--frames','30','--ui-script',str(script),'--ui-report',str(out/'states.tsv')],out,GAME,30)
    startup=states(out/'states.tsv').get('startup',{})
    again['originalsUnchanged']=before==snapshots(ROOT/'originals')
    again['observerPassed']=again['returnCode']==0 and not again['timedOut'] and int(startup['screen_mode'][0])==138 and expected==dict(startInFullScreen=0,workingFullScreenFlags=171) and read_options(data/'d/options.cfg')==expected and again['originalsUnchanged']
    again['startup']=startup.get('screen_mode')
    save(out/'result.json',again)
    print(json.dumps({key:again[key] for key in ('returnCode','seconds','observerPassed')},ensure_ascii=False))
    return int(not again['observerPassed'])


def main():
    """메뉴만 실행하며 명시 잘못된 모드 입력과 실제 조정 UI의 결과를 구별한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--restart-data',type=Path) #이미 통과한 조정 경로를 반복하지 않고 해당 사본의 재시작 입력만 보완한다.
    args=parser.parse_args()
    out=args.output.resolve()
    before=snapshots(ROOT/'originals')
    if args.restart_data:
        return restart_game(args.restart_data.resolve(),out,before)
    out.mkdir(parents=True,exist_ok=False)
    data=out/'game'
    shutil.copytree(ROOT/'originals',data,ignore=lambda directory,names:[name for name in names if name.lower().endswith('.fort') and name.lower()!='test01.fort'])
    steps=[(3,'click-if','OK'),(5,'click-if','OK'),(10,'report','startup'),
           (14,'command','ScreenModeAbsChange,0,17'),(20,'report','recovered'),  # 검사 입력: 구름 없는 시차는 실제 Screen 합법성 검사에서 거부된다.
           (24,'command','ScreenModeAbsChange,0,235'),(28,'report','same'),
           (32,'click','Options'),(36,'click','Adjust Direct Draw'),(42,'report','adjust'),
           (46,'click','Safe & Jumpy Mouse'),(52,'click','Turn Off'),(58,'report','preview'),
           (62,'click','Cancel'),(68,'report','cancelled'),
           (72,'click','Options'),(76,'click','Adjust Direct Draw'),(82,'click','Safe & Jumpy Mouse'),
           (88,'click','Turn Off'),(94,'report','preview2'),(98,'click','OK'),(104,'report','accepted'),
           (108,'click','OK'),(114,'click','Options'),(118,'click','Adjust Direct Draw'),
           (124,'click','Safe & Jumpy Mouse'),(130,'click','Turn On'),(136,'report','preview3'),
           (760,'report','timedout'),(770,'click','Options'),(774,'click','Window Mode'),
           (780,'report','window'),(784,'click','Quit')]
    script=out/'steps.tsv'
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    settings='SCREENW=1024;SCREENH=768;startInFullScreen=1;workingFullScreenFlags=111;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;currentLanguage=english'
    command=[str(GAME),'--run',str(data),'--no-audio','--frames','790','--set',settings,'--ui-script',str(script),'--ui-report',str(out/'states.tsv')]
    result=execute(command,out,GAME,60)
    observed=states(out/'states.tsv') if (out/'states.tsv').exists() else {}
    result.update(exeSha256=sha(GAME),originalsUnchanged=before==snapshots(ROOT/'originals'),newGameProcesses=1,scope='메뉴/명시 잘못된 모드 입력; 스테이지/물리 입력/PCM 없음')
    result['observations']={label:{key:scene.get(key) for key in ('page','screen_mode','screen_transition','screen_window','screen_output')} for label,scene in observed.items()}
    save(out/'result.json',result)
    try:
        assert result['returnCode']==0 and not result['timedOut'] and result['originalsUnchanged']
        modes={label:int(scene['screen_mode'][0]) for label,scene in observed.items()}
        expected={'startup':235,'recovered':235,'same':235,'adjust':235,'preview':171,'cancelled':235,'preview2':171,'accepted':171,'preview3':235,'timedout':171,'window':138}
        assert modes==expected,(modes,expected)
        refresh={label:int(scene['screen_transition'][0]) for label,scene in observed.items()}
        start=refresh['startup']
        for label,delta in (('recovered',1),('same',2),('adjust',2),('preview',3),('cancelled',4),('preview2',5),('accepted',5),('preview3',6),('timedout',7),('window',8)):
            assert refresh[label]==(start+delta)&0xffffffff,(label,refresh,start,delta)
        assert observed['preview']['page']==['DirectDrawVerify'] and observed['preview3']['page']==['DirectDrawVerify']
        assert observed['accepted']['page']==['AdjustDirectDraw']
        result['savedOptions']=read_options(data/'d/options.cfg')
        assert result['savedOptions']==dict(startInFullScreen=0,workingFullScreenFlags=171)
        result['observerPassed']=True
    except Exception as error:
        result['observerPassed']=False
        result['observerError']=repr(error)
    save(out/'result.json',result)
    print(json.dumps({key:result[key] for key in ('returnCode','seconds','originalsUnchanged','observerPassed')},ensure_ascii=False))
    if not result['observerPassed']:
        return 1
    return restart_game(data,out/'restart',before)


if __name__=='__main__':
    raise SystemExit(main())
