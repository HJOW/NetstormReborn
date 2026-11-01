#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""메인 Intro 표시/실제 클릭→Native 영상 상태→스킵→메인 복귀를 확인한다(699). 스테이지에는 진입하지 않는다."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def main():
    """명시 영상 경로 또는 빈 경로를 시작 옵션에 공급하고 원본/설정을 매번 보존한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--movie',type=Path)
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    plan,report=out/'steps.tsv',out/'states.tsv'
    steps=[(20,'report','main')]
    if args.movie:
        steps.extend([(24,'click','Intro'),(34,'report','movie1'),(34,'snapshot-presented',str(out/'movie1.bmp')),(42,'esc',''),(60,'report','back1'),
                      (64,'click','Intro'),(75,'report','movie2'),(82,'esc',''),(105,'report','back2'),(110,'snapshot-presented',str(out/'back2.bmp'))])
    else:
        steps.extend([(22,'snapshot-presented',str(out/'main.bmp')),(24,'click','Options'),(33,'report','options'),(36,'click','screen:1000,740'),(44,'report','back')])
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    exe=ROOT/'cpppj/build/bin/Release/Netstorm.exe'
    movie=str(args.movie.resolve()) if args.movie else ''
    settings='SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;intro=1;WelcomeGiven=1;introMovie='+movie
    command=[str(exe),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','120' if args.movie else '50','--set',settings,'--ui-script',str(plan),'--ui-report',str(report)]
    before=protection()
    (out/'protection-before.json').write_text(json.dumps(before,ensure_ascii=False),encoding='utf-8')
    (out/'run.json').write_text(json.dumps(dict(command=command,exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    with preserve_game_settings(ROOT/'originals','d',out/'settings-backup'):
        run=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=120)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after=protection()
    (out/'protection-after.json').write_text(json.dumps(after,ensure_ascii=False),encoding='utf-8')
    assert before==after and run.returncode==0,(before==after,run.returncode)
    observed=states(report)
    initial=observed['main']
    assert initial['page']==['main'] and initial['intro_available']==[str(int(bool(args.movie)))], initial
    assert ('Intro' in initial['controls'])==bool(args.movie),initial['controls']
    assert len(initial['controls'])==(9 if args.movie else 8),initial['controls']
    if args.movie:
        for label in ('movie1','movie2'):  # 실제 decoder/모달/표시 프레임이 있어야 하며 이름표 예약만으로 재생 성공을 세지 않는다.
            scene=observed[label]
            assert int(scene['movie_runtime'][0]) and int(scene['movie_runtime'][1])==1 and int(scene['movie_stream'][2])>0 and scene['movie_stream'][3:5]==['1','0'],scene  # runtime의 displayed는 이미 반납한 영상 누계다. 재생 중인 노드의 실제 표시 횟수를 읽는다.
            assert scene['movie_update_depth']==['1'] and scene['current_dialog']!=['0'],scene
            assert Path(scene['movie_source'][1]).resolve()==args.movie.resolve(),scene['movie_source']
        for label in ('back1','back2'):  # 두 번 모두 같은 제품의 실제 Native39/40/41→3 복귀와 자원 반납을 확인한다.
            scene=observed[label]
            assert scene['page']==['main'] and scene['phase']==['0'] and scene['movie_runtime'][:2]==['0','0'] and scene['movie_update_depth']==['0'] and scene['current_dialog']==[scene.get('main_menu_parent',['0'])[0]],scene  #700부터는 실제 메인 부모가 모달이며 이전 제품의 필드 부재는 기존0 계약이다.
            assert scene['RawButtons'][1]==initial['RawButtons'][1] and len(scene['controls'])==9,(initial['RawButtons'],scene['RawButtons'])
    else:
        assert observed['options']['current_dialog']!=['0'] and observed['back']['current_dialog']==[observed['back'].get('main_menu_parent',['0'])[0]] and observed['back']['RawButtons'][1]==initial['RawButtons'][1]  #실제 메인 부모 복귀를 일반 모달0과 구별한다.
    result=dict(unit=699,gameExecutions=1,exit=run.returncode,protectedUnchanged=True,exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),introAvailable=bool(args.movie),
                buttons=len(initial['controls']),movieCycles=2 if args.movie else 0,missionEntries=0,physicalInput=False,rawStates={key:{field:value.get(field) for field in ('page','current_dialog','movie_runtime','movie_update_depth','RawButtons')} for key,value in observed.items()})
    (out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False))


if __name__=='__main__':
    main()
