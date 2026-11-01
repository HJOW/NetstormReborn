#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 메인 메뉴에서 RecheckCD 일반 입력이 현재 serial을 조회·Kill하고 0으로 유지하는지 검사한다(696)."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from PIL import Image
from cpp_deck_reactivation_smoke import ROOT,protection
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states


def main():
    """미션이나 전투 없이 메인 메뉴의 실제 등록 표에 이름표 명령을 세 번 넣는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    out=parser.parse_args().output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    plan,report=out/'steps.tsv',out/'states.tsv'
    steps=((20,'report','main'),(22,'snapshot-presented',str(out/'before.bmp')),(25,'command','RecheckCD'),(30,'report','first'),(32,'snapshot-presented',str(out/'first.bmp')),
           (35,'command','RecheckCD,0,1'),(40,'report','second'),(45,'command','recheckcd'),(50,'report','third'),(52,'snapshot-presented',str(out/'third.bmp')),
           (55,'kernel-tell','BeginCapture'),(62,'report','dialog'),(63,'snapshot-presented',str(out/'dialog.bmp')),(64,'test-recheck-serial','current-dialog'),(65,'report','armed'),
           (66,'command','RecheckCD'),(72,'report','killed'),(73,'snapshot-presented',str(out/'killed.bmp')))  # 앞 세 입력은 최초 serial 0의 무효과, 뒤 입력은 열린 모달의 실제 serial을 넣은 Kill 경로다.
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    before=protection()
    (out/'protection-before.json').write_text(json.dumps(before,ensure_ascii=False),encoding='utf-8')
    exe=ROOT/'cpppj/build/bin/Release/Netstorm.exe'
    command=[str(exe),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','80','--set','SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;intro=1;WelcomeGiven=1','--ui-script',str(plan),'--ui-report',str(report)]
    (out/'run.json').write_text(json.dumps(dict(command=command,exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    with preserve_game_settings(ROOT/'originals','d',out/'settings-backup'):
        run=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=120)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after=protection()
    (out/'protection-after.json').write_text(json.dumps(after,ensure_ascii=False),encoding='utf-8')
    assert before==after and run.returncode==0,(before==after,run.returncode)
    observed=states(report)
    labels=('main','first','second','third','dialog','armed','killed')
    counts={label:int(observed[label]['RawButtons'][1]) for label in labels}  # 실제 공통 등록 표의 현재 항목 수다.
    serials={label:observed[label]['recheck_cd'][0] for label in labels}  # 005c85ec의 현재 값이다.
    assert counts['main']==counts['first']==counts['second']==counts['third'],counts  # 모든 Gump는 0이 아닌 고유 serial을 받으므로 serial 0 조회는 대상을 찾지 못한다.
    assert [serials[label] for label in ('main','first','second','third')]==['0']*4,serials
    assert observed['dialog']['current_dialog']!=['0'] and observed['dialog']['page']==['BeginCapture'] and observed['dialog']['phase']==['1'],observed['dialog']  # 실제 모달이 열렸다(메인 컨트롤이 교체되므로 등록 항목 수의 증감은 단언하지 않는다).
    assert serials['dialog']=='0' and serials['armed']!='0',serials  # 검사 전용 연산이 열린 모달의 실제 serial을 넣었다.
    assert serials['killed']=='0',serials  # Kill 뒤 항상 0이다.
    assert counts['killed']==counts['main'] and observed['killed']['current_dialog']==['0'] and observed['killed']['page']==['main'] and observed['killed']['phase']==['0'],observed['killed']  # 모달이 공통 Kill/프레임 정리로 반납되고 호스트가 메인으로 돌아갔다.
    images={}
    for name in ('before','first','third','dialog','killed'):  # 실제 표시 RGB 전체의 변화를 기록한다.
        with Image.open(out/f'{name}.bmp') as bitmap:
            images[name]=hashlib.sha256(bitmap.convert('RGB').tobytes()).hexdigest()
    assert images['before']==images['first']==images['third'],images  # 최초 serial 0 입력은 화면을 바꾸지 않는다.
    assert images['dialog']!=images['before'],images  # 모달이 실제로 표시됐다.
    result=dict(exit=run.returncode,runtimeProduct=696,version='11.01',exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),protectedUnchanged=True,resolution=[1024,768],
                registryCounts=counts,recheckSerial=serials,currentDialog={label:observed[label]['current_dialog'][0] for label in ('dialog','armed','killed')},displaySha256=images,
                dialogClosedByRecheck=counts['killed']==counts['main'] and serials['killed']=='0',zeroSerialNoEffect=counts['main']==counts['third'] and images['before']==images['third'])
    (out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(result,ensure_ascii=False))


if __name__=='__main__':
    main()
