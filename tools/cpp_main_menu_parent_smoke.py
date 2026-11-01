#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 메인 부모의 팝업/HTML 보존·일반 선택 Kill·영상 복귀 또는45초 자동 데모를 확인한다(700)."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

def brief(scene):
    """검사 실패 시 대규모 월드 객체 대신 판정에 쓰는 실제 필드만 표시한다."""
    return {key:scene.get(key) for key in ('frame','phase','page','time','main_menu_parent','current_dialog','mission','mission_start','RawButtons')}

def main():
    """원본/설정을 매 실행 보존하고 현재 게임 보고만으로 부모/모달/기한을 판정한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mode',choices=('menus','auto'),required=True)
    parser.add_argument('--verify-existing',action='store_true',help='게임 실행 없이 이미 보존한 같은 원시 자료만 분석한다')
    parser.add_argument('--observed-exit',type=int,help='옛 도구의 exit0 선행 단언을 통과한 기록에 한해 관측 종료값을 명시한다')
    args=parser.parse_args()
    out=args.output.resolve()
    if args.verify_existing:
        assert out.is_dir() and not (out/'summary.json').exists(), '기존 원시 자료만 분석하며 완료 보고서는 덮지 않는다'
    else:
        out.mkdir(parents=True,exist_ok=False)
    plan,report=out/'steps.tsv',out/'states.tsv'
    auto=args.mode=='auto'
    if auto:
        steps=[(1,'pointer','0x80000000,400,300'),(20,'report','main'),(100,'report','waiting'),(400,'report','before'),(490,'report','demo'),(490,'snapshot-presented',str(out/'demo.bmp'))]
    else:
        steps=[(20,'report','main'),(24,'click','Help'),(30,'report','help'),(34,'click','General Help - F1'),(43,'report','general'),(47,'click','screen:1000,740'),(55,'report','generalClosed'),
               (60,'click','Help'),(66,'click','Version'),(74,'report','about'),(77,'esc',''),(85,'report','aboutClosed'),(90,'click','Options'),(98,'report','options'),(102,'click','Edge Scroll in Fullscreen'),(110,'report','toggled'),
               (114,'click','Options'),(125,'click','Resolution'),(135,'report','resolution'),(142,'click','screen:1000,740'),(150,'report','closed'),(155,'click','Campaign'),(166,'report','campaign'),(172,'esc',''),(188,'report','campaignClosed'),
               (194,'click','Intro'),(205,'report','movie'),(212,'esc',''),(232,'report','back'),(240,'click','Options'),(250,'report','optionsAgain'),(255,'click','screen:1000,740'),(265,'report','final'),(265,'snapshot-presented',str(out/'main.bmp'))]
    if not args.verify_existing:
        plan.write_text(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in steps),encoding='utf-8',newline='\n')
    exe=ROOT/'cpppj/build/bin/Release/Netstorm.exe'
    movie='' if auto else str(ROOT/'originalCD/MOVIE/englishintro2x.smk')
    settings=f'SCREENW=1024;SCREENH=768;maxFPS={10 if auto else 40};sleepPerLoop=1;autoDemo={int(auto)};tellTips=0;intro=1;WelcomeGiven=1;edgeScroll=0;introMovie={movie}'
    command=[str(exe),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','500' if auto else '275','--set',settings,'--ui-script',str(plan),'--ui-report',str(report)]
    if args.verify_existing:
        before=json.loads((out/'protection-before.json').read_text('utf-8'))
        after=json.loads((out/'protection-after.json').read_text('utf-8'))
        if (out/'process.json').exists():
            exit_code=json.loads((out/'process.json').read_text('utf-8'))['exit']
        else:
            assert args.observed_exit==0, '기존 exit0 선행 단언을 실제 확인한 값이 필요하다'
            exit_code=args.observed_exit
        assert before==after and exit_code==0,(before==after,exit_code)
    else:
        before=protection()
        (out/'protection-before.json').write_text(json.dumps(before,ensure_ascii=False),encoding='utf-8')
        (out/'run.json').write_text(json.dumps(dict(command=command,exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        with preserve_game_settings(ROOT/'originals','d',out/'settings-backup'):
            run=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=120)
        exit_code=run.returncode
        (out/'stdout.log').write_bytes(run.stdout)
        (out/'stderr.log').write_bytes(run.stderr)
        after=protection()
        (out/'protection-after.json').write_text(json.dumps(after,ensure_ascii=False),encoding='utf-8')
        (out/'process.json').write_text(json.dumps(dict(exit=exit_code,protectedUnchanged=before==after),indent=2)+'\n',encoding='utf-8')
        assert before==after and exit_code==0,(before==after,exit_code)
    run_metadata=json.loads((out/'run.json').read_text('utf-8'))
    observed=states(report)
    initial=observed['main']
    parent=initial['main_menu_parent'][0]
    assert parent!='0' and initial['current_dialog']==[parent] and initial['auto_demo']==[str(int(auto))],brief(initial)
    if auto:
        for label in ('waiting','before'):  # 실제 시간이 아직 기한 앞이며 같은 부모가 현재 모달이어야 한다.
            scene=observed[label]
            assert scene['main_menu_parent'][0]==parent and scene['current_dialog']==[parent],brief(scene)
            assert float(scene['time'][1])<float(scene['main_menu_parent'][2]),brief(scene)
        demo=observed['demo']
        assert demo['main_menu_parent']==['0'] and demo['mission'][0].lower()=='demostormrages' and demo['phase']==['3'] and demo['page']==['Demo'] and demo['mission_start'][2]=='1',brief(demo)  #StartDemo의 실제 초기 안내창/데모값을 확인한다. 안내창 OK 뒤의 진행을 합산하지 않는다.
        assert float(demo['time'][1])>=float(initial['main_menu_parent'][2]),brief(demo)
    else:
        for label in ('help','general','about','options','resolution','optionsAgain'):  # 팝업/HTML은 별도 현재 모달이며 뒤의 같은 메인 부모는 살아 있어야 한다.
            scene=observed[label]
            expected=parent if label!='optionsAgain' else observed['back']['main_menu_parent'][0]
            assert scene['main_menu_parent'][0]==expected and scene['current_dialog'] not in (['0'],[expected]),scene
        for label in ('generalClosed','aboutClosed','toggled','closed'):  # 일반 선택 이전에는 부모와 등록 수가 유지된다.
            scene=observed[label]
            assert scene['main_menu_parent'][0]==parent and scene['current_dialog']==[parent] and scene['RawButtons'][1]==initial['RawButtons'][1],scene
        assert observed['campaign']['main_menu_parent']==['0'] and observed['campaign']['page']==['UCampaign'],observed['campaign']
        assert observed['campaignClosed']['main_menu_parent'][0] not in ('0',parent),observed['campaignClosed']
        assert observed['movie']['main_menu_parent']==['0'] and observed['movie']['movie_stream'][3:5]==['1','0'],observed['movie']
        for label in ('back','final'):  # Native 영상 복귀가 새 실제 메인 부모와9버튼을 구성하고 정리 수를 회복한다.
            scene=observed[label]
            assert scene['main_menu_parent'][0]!='0' and scene['current_dialog']==[scene['main_menu_parent'][0]] and scene['page']==['main'] and len(scene['controls'])==9 and scene['RawButtons'][1]==initial['RawButtons'][1],scene
    result=dict(unit=700,mode=args.mode,gameExecutions=1,executionsThisInvocation=0 if args.verify_existing else 1,exit=exit_code,exitEvidence='earlier exit0 assertion before phase check' if args.verify_existing and not (out/'process.json').exists() else 'process.json',protectedUnchanged=True,exeSha256=run_metadata['exeSha256'],missionEntries=int(auto),physicalInput=False,
                observations={label:{field:value.get(field) for field in ('phase','page','time','main_menu_parent','current_dialog','RawButtons','mission')} for label,value in observed.items()})
    (out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({key:value for key,value in result.items() if key!='observations'},ensure_ascii=False))

if __name__=='__main__':
    main()
