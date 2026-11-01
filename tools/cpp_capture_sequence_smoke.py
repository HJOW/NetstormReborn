#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 메뉴의 BeginCapture→20프레임→Stop·공통안내/메모리/Unicode BMP를 검사한다(695)."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from PIL import Image
from cpp_deck_reactivation_smoke import ROOT,protection
from cpp_smoke_files import preserve_game_settings
from cpp_product_credits_smoke import chunks
from cpp_world_smoke import states


def main():
    """미션이나 전투 없이 현재 메뉴의 실제 Capture 버튼과 프레임만 사용한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    out=parser.parse_args().output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    prefix=out/'프레임 😀'
    plan,report=out/'steps.tsv',out/'states.tsv'
    steps=((20,'report','main'),(25,'kernel-tell','BeginCapture'),(35,'report','begin'),(40,'click','Begin Capture'),(45,'report','capturing'),(46,'snapshot-presented',str(out/'during.bmp')),(50,'kernel-tell','BeginCapture'),(55,'report','ending'),(60,'click','Begin Capture'),(65,'report','saved'),(66,'snapshot-presented',str(out/'notice.bmp')),(75,'click','OK'),(85,'report','main_return'))
    plan.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    before=protection()
    (out/'protection-before.json').write_text(json.dumps(before,ensure_ascii=False),encoding='utf-8')
    exe=ROOT/'cpppj/build/bin/Release/Netstorm.exe'
    command=[str(exe),'--run',str(ROOT/'originals'),'--window','--no-audio','--frames','89','--set',f'SCREENW=1024;SCREENH=768;maxFPS=40;autoDemo=0;tellTips=0;sleepPerLoop=1;intro=1;WelcomeGiven=1;GifSequenceName={prefix}','--ui-script',str(plan),'--ui-report',str(report)]
    (out/'run.json').write_text(json.dumps(dict(command=command,exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest()),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    with preserve_game_settings(ROOT/'originals','d',out/'settings-backup'):
        run=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=120)
    (out/'stdout.log').write_bytes(run.stdout)
    (out/'stderr.log').write_bytes(run.stderr)
    after=protection()
    (out/'protection-after.json').write_text(json.dumps(after,ensure_ascii=False),encoding='utf-8')
    assert before==after and run.returncode==0,(before==after,run.returncode)
    observed,raw=states(report),chunks(report)
    assert observed['begin']['page'][0].lower()=='begincapture' and observed['ending']['page'][0].lower()=='begincapture'
    assert observed['main']['screen_capture']==['0','0','0','0']
    assert observed['capturing']['screen_capture']==['6','1','1','6'],observed['capturing']['screen_capture']
    assert observed['ending']['screen_capture']==['16','1','1','16'],observed['ending']['screen_capture']
    assert observed['saved']['screen_capture']==['20','0','1','0'] and observed['saved']['page']==['capturewritten'],observed['saved']
    assert observed['main_return']['phase']==['0'] and observed['main_return']['current_dialog']==['0']
    assert observed['main_return']['screen_capture']==['20','0','1','0']
    names=sorted(out.glob('프레임 😀*.bmp'))
    assert [path.name for path in names]==[f'프레임 😀{index:03d}.bmp' for index in range(20)]
    files=[]
    for path in names:  # 실제 새 파일의 전체 길이/색표/논리 크기를 확인한다.
        assert path.stat().st_size==1078+1024*768
        with Image.open(path) as bitmap:
            assert bitmap.mode=='P' and bitmap.size==(1024,768)
        files.append(dict(name=path.name,bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    with Image.open(names[6]) as saved,Image.open(out/'during.bmp') as shown:
        assert saved.convert('RGB').tobytes()==shown.convert('RGB').tobytes(),'프레임46의실제수집파일과같은표시시점RGB가다름'
    assert 'Wrote ' in raw['saved'] and '000.bmp' in raw['saved'] and '020.bmp' in raw['saved'],raw['saved']
    assert '[U+1F600]' in raw['saved'],'원문8비트글꼴이없는이모지의식별표시누락'
    result=dict(exit=run.returncode,runtimeProduct=695,version='11.01',exeSha256=hashlib.sha256(exe.read_bytes()).hexdigest(),protectedUnchanged=True,resolution=[1024,768],captureFirstFrame=40,captureStopFrame=60,capturedFrames=20,framesReleased=True,actualBmpFiles=files,pixelComparedFrame=46,pixelComparedFile=names[6].name,reportLastName='020.bmp',actualLastName='019.bmp',mainReturned=True,settingsRestored=True,limits='앱내검사입력과명시kernel-tell을사용한메뉴전용실행이다. 자연단축키/물리입력·미션/포획/희생/특정적공격/승리·MP/PCM/소리녹화·전체34검사가아니다.')
    (out/'summary.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:value for key,value in result.items() if key!='actualBmpFiles'},ensure_ascii=False))


if __name__=='__main__':
    main()
