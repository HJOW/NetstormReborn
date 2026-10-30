#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""별도 배포 사본에서 Config 입력줄 요청과 실제 재실행 뒤 Chat 서식 적용을 검사한다."""
import argparse
import json
import subprocess
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,execute,save,sha
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated
from cpp_movie_runtime_smoke import verify_presentation

SIZES=((1024,768),(1280,800),(1280,720))  # 창4:3 뒤 같은 설정을16:10/16:9의 새 프로세스로 읽는다.


def review(steps,frame,label):
    """실제 ESC→Game→Review Briefing 항목에서 준비된 원문 Config 버튼을 클릭한다."""
    steps.extend(((frame,'esc',''),(frame+4,'click','Game'),(frame+8,'click','menu-command:02000091,0'),(frame+13,'click',label)))


def typed(steps,frame,text):
    """일반 WM_CHAR와 같은 문자 사건만 한 프레임씩 넣는다."""
    for index,character in enumerate(text): steps.append((frame+index,'key',character))  # 완성된 버퍼를 직접 쓰지 않는다.


def capture(steps,frame,label,directory):
    """같은 실제 상태와 논리 프레임/실제 창 전체 출력을 함께 기록한다."""
    steps.extend(((frame,'report',label),(frame,'snapshot',str(directory/(label+'.bmp'))),(frame,'snapshot-output',str(directory/(label+'-output.bmp')))))


def run(product,directory,width,height,index):
    """첫 실행은 실제 설정 클릭/저장을, 새 실행은 저장값의 실제 화면 사용을 확인한다."""
    first=index==0;directory.mkdir(parents=True,exist_ok=False);steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','playing'),(25,'key','enter'),(30,'report','shown')]
    if first:
        review(steps,35,'Request One');steps.extend(((56,'report','one'),(60,'key','enter'),(66,'report','restored-one')))
        review(steps,70,'Request Zero');steps.append((91,'report','zero'))
        review(steps,95,'Request Minus');steps.extend(((116,'report','minus'),(120,'key','enter'),(126,'report','restored-minus')))
        review(steps,130,'Reparse Config');steps.extend(((151,'report','reparsed'),(155,'key','enter'),(161,'report','restored-reparse')))
        review(steps,165,'Request Zero');steps.extend(((186,'report','zero-final'),(190,'key','f3'),(193,'key','enter')))
        typed(steps,200,'.front ~g');steps.extend(((211,'report','format-typed'),(213,'key','enter'),(221,'report','format-set')))
        message='614-first';typed(steps,230,message);steps.append((242,'key','enter'));capture(steps,254,'styled',directory);frames=260
    else:
        message='614-restart-'+str(width)+'x'+str(height);typed(steps,38,message);steps.append((40+len(message),'report','message-typed'));steps.append((43+len(message),'key','enter'));capture(steps,57+len(message),'styled',directory);frames=64+len(message)
        if index==1:
            # 원문 시작ID -1의 키를 실제 Config로 저장하되 현재 manager를 임의 재초기화하지 않는다.
            frame=frames+3;review(steps,frame,'Startup Green');steps.extend(((frame+22,'key','f3'),(frame+25,'key','enter')))
            typed(steps,frame+32,'614-after-config');steps.append((frame+52,'key','enter'));capture(steps,frame+62,'after-config',directory);frames=frame+68
    steps.sort(key=lambda item:item[0]);plan=directory/'steps.tsv';plan.write_text(''.join(str(frame)+'\t'+operation+'\t'+argument+'\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    binary=product/'Netstorm.exe';game=product/'game';command=[str(binary),'--run',str(game),'--mission','TheWarBegins','--window','--no-audio','--frames',str(frames),'--set',f'SCREENW={width};SCREENH={height};maxFPS=40;sleepPerLoop=1;autoDemo=0;tellTips=0','--ui-script',str(plan),'--ui-report',str(directory/'states.tsv')]
    result=execute(command,directory,binary,150);save(directory/'run.json',result);assert result['returnCode']==0 and not result['timedOut']
    observed=states(directory/'states.tsv');windows=repeated(directory/'states.tsv','raw_chat_window');globals_=repeated(directory/'states.tsv','raw_chat_globals');inputs=repeated(directory/'states.tsv','raw_chat_input')
    expected={'shown':True}
    if first: expected.update(one=False,**{'restored-one':True,'zero':True,'minus':False,'restored-minus':True,'reparsed':False,'restored-reparse':True,'zero-final':True})
    root=windows['shown'][0][0];console=windows['shown'][0][3]
    for label,visible in expected.items():  # 실제 같은 root/Console과 프레임말 요청 소비를 대조한다.
        rows=windows[label];assert len(rows)==1 and rows[0][0]==root and rows[0][3]==console
        assert bool(int(rows[0][4])&1)==visible,(label,rows)
        assert globals_[label]==[[root,'0']],(label,globals_[label])
        assert observed[label]['phase']==['4'] and observed[label]['paused']==['0'] and observed[label]['raw_session']==['1','1']
    if first: assert bytes.fromhex(inputs['format-typed'][0][1])==b'.front ~g'
    else: assert bytes.fromhex(inputs['message-typed'][0][1])==message.encode('ascii')
    fragments=repeated(directory/'states.tsv','raw_chat_fragment');messages=[];presentations={}
    expected_messages=[('styled',message,246 if index==1 else 134)]  # 첫 재시작은1키만 저장돼 있어 원문 시작ID -1에서는 기본246이다.
    if index==1: expected_messages.append(('after-config','614-after-config',246))
    for label,text,foreground in expected_messages:  # 현재 manager 보존과 다음 새 프로세스의 -1키 재적재를 구별한다.
        matches=[row for row in fragments[label] if text.encode('ascii') in bytes.fromhex(row[3])];assert matches,(text,fragments[label])
        assert all(row[6]==str(foreground) for row in matches),(text,matches)
        presentation=verify_presentation(directory/(label+'.bmp'),directory/(label+'-output.bmp'),observed[label],directory);assert presentation['same'];presentations[label]=presentation
        messages.append({'label':label,'text':text,'foreground':foreground,'fragments':matches})
    config={}
    keys=['inputLineOff','1chatFormat2']+(['-1chatFormat2'] if index else [])
    for key in keys:  # 같은 EXE의 별도 프로세스 Config reader를 사용하며 암호화 파일을 문자열 검색하지 않는다.
        command=[str(binary),'--config-get',str(game),key];read=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
        (directory/('config-'+key+'.stdout.log')).write_bytes(read.stdout);(directory/('config-'+key+'.stderr.log')).write_bytes(read.stderr)
        assert read.returncode==0;config[key]=read.stdout.decode('utf-8').strip()
    expected_config={'inputLineOff':'0','1chatFormat2':'~g'}
    if index: expected_config['-1chatFormat2']='~g'
    assert config==expected_config,config
    summary={'passed':True,'processIndex':index,'firstProcess':first,'size':[width,height],'messages':messages,'inputVisibleStates':expected,'sameRoot':root,'sameConsole':console,'frameRequestsConsumed':True,'savedConfig':config,'optionsSha256':sha(game/'d/options.cfg'),'presentations':presentations,'run':result,'scope':'실제 설정 클릭/입력/로컬Chat과 저장/재실행이다. 시작ID -1과 미션ID1의 서로 다른 키를 원문대로 유지한다. 공격/포획/희생/승리·신규MP/PCM/전체화면은 실행하지 않는다.'}
    save(directory/'summary.json',summary);return summary


def main():
    """원본 실행을 거부하고 준비한 새 배포 사본에서만3회를 순차 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--product',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();product=args.product.resolve();output=args.output.resolve()
    for protected in (ROOT/'originals',ROOT/'originalCD'):  # 원본 자체와 그 하위 경로는 실행/설정 저장 대상으로 거부한다.
        assert product!=protected.resolve() and protected.resolve() not in product.parents
    assert (product/'Netstorm.exe').is_file() and (product/'game/d/thewarbegins.english').is_file();output.mkdir(parents=True,exist_ok=False);results=[]
    for index,(width,height) in enumerate(SIZES): results.append(run(product,output/str(index+1),width,height,index))  # 하나의 실제 창이 정상 종료한 뒤에만 다음 프로세스를 시작한다.
    save(output/'summary.json',{'passed':True,'binarySha256':sha(product/'Netstorm.exe'),'runs':results,'capturesSacrificesSpecificEnemyAttacksOrVictoriesReproduced':False,'networkExecuted':False,'sourceScope':'같은 배포 사본/브리핑Config 버튼5개. 원본 화면 픽셀 비교/자연완주/전체C++/전체34의 근거는 아니다.'})


if __name__=='__main__':
    main()
