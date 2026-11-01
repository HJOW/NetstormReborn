#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""같은 원문 Credits 창에서 제품11.01의 추가 항목과 기존 두 항목 왕복을 확인한다(685)."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from cpp_deck_reactivation_smoke import ROOT, protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def chunks(path):
    """각 보고 구간의 원시 텍스트를 보존해 줄별 중복 위젯을 잃지 않는다."""
    result={}
    current=None
    for line in path.read_text('utf-8').splitlines():  # 프레임 경계를 기준으로 실제 보고를 분리한다.
        fields=line.split('\t')
        if fields[0]=='frame':
            current=fields[2]
            result[current]=[]
        elif current is not None:
            result[current].append(line)
    return {label:'\n'.join(lines) for label,lines in result.items()}


def main():
    """메인 메뉴/세 Credits만 열고 미션·MP·원격 주소를 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--width',type=int,default=1024)
    parser.add_argument('--height',type=int,default=768)
    args=parser.parse_args()
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    plan,report=out/'steps.tsv',out/'states.tsv'
    steps=((20,'report','main'),(30,'click','Credits'),(45,'report','credits'),
           (55,'click','Netstorm 11.01 Patch Credits'),(70,'report','patch'),(71,'snapshot-presented',str(out/'patch.bmp')),
           (85,'click','Back'),(100,'report','returned1'),(110,'click','Netstorm 10.72 Patch Credits'),(125,'report','oldpatch'),
           (140,'click','Back'),(155,'report','returned2'),(165,'click','Netstorm Original Credits'),(180,'report','original'),
           (195,'click','Back'),(210,'report','returned3'),(220,'click','Netstorm 11.01 Patch Credits'),(235,'report','patch_again'),
           (245,'click','Cancel'),(260,'report','main_return'),(270,'click','Credits'),(285,'report','returned4'),(300,'click','Cancel'),(315,'report','main_final'))
    plan.write_text(''.join(f'{frame}\t{operation}\t{value}\n' for frame,operation,value in steps),encoding='utf-8',newline='\n')
    before=protection()
    (out/'protection-before.json').write_text(json.dumps(before,ensure_ascii=False),encoding='utf-8')
    exe=ROOT/'cpppj/build/bin/Release/Netstorm.exe'
    command=[str(exe),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','318',
             '--set',f'SCREENW={args.width};SCREENH={args.height};maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;intro=1;WelcomeGiven=1',
             '--ui-script',str(plan),'--ui-report',str(report)]
    with preserve_game_settings(ROOT/'originals','d',out/'settings-backup'):
        run=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=120)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after=protection()
    (out/'protection-after.json').write_text(json.dumps(after,ensure_ascii=False),encoding='utf-8')
    assert before==after and run.returncode==0,run.returncode
    observed,raw=states(report),chunks(report)
    for label in ('credits','returned1','returned2','returned3','returned4'):  # 실제 세 목록 항목과 Cancel의 보존/왕복을 확인한다.
        labels=[line.split('\t')[-1] for line in raw[label].splitlines() if line.startswith('raw_menu_item\t')] # 실제 메뉴 행의 순서/개수를 읽는다.
        assert labels==['Netstorm 10.72 Patch Credits','Netstorm Original Credits','Netstorm 11.01 Patch Credits'],(label,labels)
        # 원문 Tell 조회는 ASCII 대소문자를 무시하므로 진단 페이지 이름도 같은 규칙으로 비교한다.
        assert observed[label]['page'][0].lower()=='credits' and int(observed[label]['current_dialog'][0])!=0
    for text in ('NetStorm','11.01','Patch Credits','HJOW','AI','v11.01'):  # 원문 공장이 만든 추가 창의 실제 글자를 읽는다.
        assert text in raw['patch'],text
    assert observed['patch']['page'][0].lower()=='patch1101.credits',observed['patch']['page']
    assert observed['oldpatch']['page'][0].lower()=='creditsnew' and observed['original']['page'][0].lower()=='creditsold'
    assert observed['patch_again']['page'][0].lower()=='patch1101.credits'
    assert observed['main_return']['phase']==observed['main_final']['phase']==['0'] and observed['main_return']['current_dialog']==observed['main_final']['current_dialog']==['0']
    result=dict(exit=run.returncode,resolution=[args.width,args.height],exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),protectedUnchanged=True,
                pages={label:value['page'] for label,value in observed.items()},menuItemCount=len(labels),menuItemLabels=labels,newCreditsTextVerified=True,originalTwoCreditsPreserved=True,
                image=str(out/'patch.bmp'),limits='실제 앱 내 검사 입력으로 세 Credits와 메인 복귀를 확인했다. 원문 게임/전체34/MP/미션·포획/희생/특정 적 공격·승리/물리 입력·PCM 검사가 아니다.')
    (out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(result,ensure_ascii=False))


if __name__=='__main__':
    main()
