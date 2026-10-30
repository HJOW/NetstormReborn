#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 배포 사본의 내부 Help/Technical Help 메뉴와 로컬 도우미만 확인한다."""
import argparse
import json
import os
import re
from pathlib import Path
from cpp_bridge_ritual_runtime import execute,save,sha
from cpp_world_smoke import states
from cpp_game_menu_smoke import menu_items,help_word,choose
from cpp_production_panel_smoke import repeated
from cpp_movie_runtime_smoke import verify_presentation
from cpp_solo_entry_smoke import BUTTONS


def main():
    """상태/승리 주입 없이 같은 실제 클릭과 F1 입력을 사용한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--game-dir',type=Path,required=True);parser.add_argument('--binary',type=Path,required=True);parser.add_argument('--probe',type=Path);parser.add_argument('--helper',type=Path);parser.add_argument('--external-link');parser.add_argument('--link-point');args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);data=args.game_dir.resolve();binary=args.binary.resolve()
    if data.name.lower() in ('originals','originalcd'): raise ValueError('보호 원본 대신 새 배포 사본이 필요합니다')
    enabled=args.probe is not None
    if (enabled or args.external_link) and args.helper is None: parser.error('실행 검사는 --helper가 함께 필요합니다')
    if args.external_link and (enabled or not args.link_point): parser.error('--external-link는 --probe와 별개이며 --link-point가 필요합니다')
    plan=[]
    for frame in range(3,57,5):  # 원래 초기 안내 버튼만 조건부 실제 입력으로 통과한다.
        for label in BUTTONS:  # 모든 일반 진입 도구에서 사용한 같은 실제 라벨이다.
            condition=r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)'
            plan.append((frame,'when',condition+';;click;;'+label))
    plan.extend([(60,'report','ready'),(70,'esc',''),(75,'report','bar'),(75,'snapshot',str(out/'bar.bmp'))])
    settings='SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0;intro=1'
    proof=out/'received.txt';expected=None
    if enabled:
        probe=json.loads(args.probe.read_text(encoding='utf-8'));about=probe['aboutLabel'];helper=args.helper.resolve();argument=out/'manual.hlp';argument.write_bytes(b'Local test input only.\n')
        # 새 로컬 도우미 경로만 지정한다. 브라우저/외부 사이트/사용자 실행 파일은 선택하지 않는다.
        if ' ' in str(helper) or ' ' in str(argument): raise ValueError('이번 실제 메뉴 설정은 명시 ASCII 무공백 도우미 경로를 사용합니다')
        settings+=';techHelpExe='+str(helper)+';techHelpFile='+str(argument);expected=str(helper)+' '+str(argument)
        plan.extend([(80,'click',about),(87,'report','about'),(87,'snapshot',str(out/'about.bmp')),(95,'click','menu-command:2000049,0'),(110,'report','technicalClosed')]) #숫자는 실제 보이는 항목의 누를 곳을 찾는 조회이며 명령을 직접 주입하지 않는다.
    else: plan.append((95,'esc',''))
    plan.extend([(120,'key','f1'),(135,'report','help'),(135,'snapshot',str(out/'help.bmp'))])
    if args.external_link:
        helper=args.helper.resolve();scheme,separator,body=args.external_link.partition(':');assert separator and scheme=='http'
        if ' ' in str(helper): raise ValueError('이번 프로토콜 도우미는 ASCII 무공백 경로가 필요합니다')
        settings+=';techHelpExe=;techHelpFile=;autoOpen=;http:='+str(helper)+' %1';expected=str(helper)+'   '+body
        plan.extend([(142,'click','screen:'+args.link_point),(155,'report','after-link'),(165,'outside',''),(168,'up-if','screen:0,0'),(175,'report','helpClosed')]) #관측한 링크의 실제 화면 위치를 클릭한다. 명령/링크 토큰을 직접 주입하지 않는다.
    else: plan.extend([(145,'outside',''),(148,'up-if','screen:0,0'),(160,'report','helpClosed')])
    plan.extend([(180,'report','final'),(180,'snapshot',str(out/'final.bmp')),(180,'snapshot-output',str(out/'final-output.bmp'))])
    script=out/'steps.tsv';script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in plan),encoding='utf-8',newline='\n')
    command=[str(binary),'--run',str(data),'--mission','Tutorial1','--window','--no-audio','--frames','182','--set',settings,'--ui-script',str(script),'--ui-report',str(out/'states.tsv')]
    previous=os.environ.get('NETSTORM_HELP_PROOF_PATH')  # 현재 도구/자식 환경만 변경하고 원래 값을 복구한다.
    try:
        if enabled or args.external_link: os.environ['NETSTORM_HELP_PROOF_PATH']=str(proof)
        run=execute(command,out,binary,180);save(out/'run.json',run)
    finally:
        if previous is None: os.environ.pop('NETSTORM_HELP_PROOF_PATH',None)
        else: os.environ['NETSTORM_HELP_PROOF_PATH']=previous
    assert run['returnCode']==0 and not run['timedOut'],run
    report=out/'states.tsv';observed=states(report);items=menu_items(report,'bar');about=help_word(report,'bar',items)
    technical=[item for item in items if item['command']==0x02000049];assert bool(technical)==enabled,(enabled,technical)
    help_rows=repeated(report,'raw_help');assert help_rows.get('help') and not help_rows.get('helpClosed'),help_rows
    for label in ('ready','final'):  # 같은 실제 Session/미션/비정지 상태를 구별한다.
        current=observed[label];assert current['phase']==['4'] and current['paused']==['0'] and current['raw_session']==['1','1'];assert current['mission']==['Tutorial1','Tutorial','3']
    assert float(observed['final']['time'][0])>float(observed['ready']['time'][0]);assert observed['ready']['raw_sp'][1]==observed['final']['raw_sp'][1]
    child=None
    if enabled or args.external_link:
        pid,value=proof.read_text(encoding='utf-8').split('\n',1);assert value==expected,(value,expected);child={'pid':int(pid),'proofSha256':sha(proof),'commandMatched':True,'childExitCodeCaptured':False}
    if enabled:
        displayed=[row for row in menu_items(report,'about') if row['command']==0x02000049 and row['visible']];assert displayed
    presentation=verify_presentation(out/'final.bmp',out/'final-output.bmp',observed['final'],out);assert presentation['same']
    summary={'passed':True,'enabled':enabled,'externalLink':args.external_link,'linkPoint':args.link_point,'run':run,'gameSha256':sha(binary),'aboutLabel':about,'technicalLabel':technical[0]['label'] if enabled else 'Technical Help','helper':child,'presentation':presentation,'mission':observed['final']['mission'],'timeBefore':observed['ready']['time'][0],'timeAfter':observed['final']['time'][0],'internalHelpOpenedAndClosed':True,'scope':'실제 ESC/About/Technical Help/HTML 링크 클릭과 F1/바깥 클릭·같은 Session/창RGB. 프로토콜은 로컬 도우미만 지정. 브라우저/외부 사이트/포획/희생/특정 공격/승리 주입 없음.'}
    save(out/'summary.json',summary);print(json.dumps({'passed':True,'enabled':enabled,'seconds':run['seconds'],'aboutLabel':about}))


if __name__=='__main__':
    main()
