#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제ESC해상도확인의No유지와Yes→SetSize2 OnExit정상조기종료를별도프로세스로검사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

from cpp_game_menu_smoke import (ROOT,GAME,save,sha,states,menu_items,choose,check_bar,
                                 protected_run,probe_steps,audio_values)
from cpp_menu_smoke import source
from taff import TaffArchive

WIDTH,HEIGHT=1024,768  # 실제ESCprobe와두확인분기모두같은화면비를사용한다.
FRAME_LIMIT,LATE_FRAME=1000,800  # Yes후이보고에도달하면조기종료검증실패다.


def section(text,name):
    """보호TELL의현재섹션원문만읽고게임명령을직접실행하지않는다."""
    match=re.search(r'(?im)^\['+re.escape(name)+r'\][ \t]*$',text)
    if not match:
        raise RuntimeError('보호Tell섹션없음: '+name)
    following=re.search(r'(?m)^\[',text[match.end():])  # 다음원문섹션경계다.
    return text[match.end():match.end()+following.start() if following else len(text)]


def original_contract():
    """현재보호스크립트의Yes/No/OnExit명령과해시를읽기전용으로확인한다."""
    root=ROOT/'originals'
    archive=TaffArchive(str(root/'netstorm.tarc'))
    raw=source(root,archive,'d/tell.english')  # 실제VFS와같이디스크가우선이다.
    text=raw.decode('cp1252').replace('\r','')
    first,second=section(text,'SETSIZE'),section(text,'SetSize2')
    buttons=dict(re.findall(r'(?im)^\$Button[ \t]*=[ \t]*([^,\n]+),([^\n]+)',first))
    assert buttons.get('Yes')=='Tell,SetSize2({param.1},{param.2})',buttons
    assert buttons.get('No')=='DoNothing,0',buttons
    exits=re.findall(r'(?im)^\$OnExit[ \t]*=([^\n]*)',second)
    assert exits==['test,QuitApp,99'],exits
    assert '<$Config,SCREENW={param.1}>' in second and '<$Config,SCREENH={param.2}>' in second,second
    # 원문바이트의해시를구하며줄끝정규화는섹션판독에만사용한다.
    return dict(path='d/tell.english',sha256=hashlib.sha256(raw).hexdigest(),yes=buttons['Yes'],no=buttons['No'],
                setSize2OnExitRaw=exits[0],expectedQuitAppParameter=99,screenBefore=[WIDTH,HEIGHT],requestedScreen=[800,600])


def plan(directory,labels,yes):
    """probe가반환한실제행라벨로같은Controls.Point와정수입력큐를통과한다."""
    steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),
           (25,'esc',''),(30,'report','bar'),(35,'click',labels['Options']),
           (42,'report','options'),(45,'click',labels['Resolution']),(52,'report','resolution'),
           (55,'click',labels['800 by 600']),(65,'report','confirm'),(65,'snapshot',str(directory/'confirm.bmp')),
           (70,'click','Yes' if yes else 'No')]
    if yes:
        steps.extend([(LATE_FRAME,'report','mustNotReachAfterYes'),(LATE_FRAME,'snapshot',str(directory/'must-not-reach-after-yes.bmp'))])
    else:
        steps.extend([(85,'report','rejected'),(85,'snapshot',str(directory/'rejected.bmp')),
                      (LATE_FRAME,'report','stillRunningAfterNo'),(LATE_FRAME,'snapshot',str(directory/'still-running-after-no.bmp'))])
    return steps


def process_observation(directory,result,expect_early_quit=False):
    """기존execute가저장한실제PID/명령/종료/원시stderr관측을읽는다."""
    identity=json.loads((directory/'running.json').read_text(encoding='utf-8'))
    assert identity['pid']>0 and identity['binarySha256']==result['binarySha256'],identity
    stderr=(directory/'stderr.log').read_text(encoding='utf-8',errors='replace')
    assert 'UI script ended before all steps ran' not in stderr,stderr
    audio_path=directory/'audio.tsv'  # 프레임제한도달시에만생성되는실제음향보고서다.
    audio=audio_values(audio_path)
    if audio_path.exists() or not expect_early_quit:
        assert audio.get('muteDepth')=='1',audio
    observation=dict(pid=identity['pid'],command=identity['command'],binarySha256=result['binarySha256'],
                returnCode=result['returnCode'],timedOut=result['timedOut'],seconds=result['seconds'],
                stdoutSha256=result['stdoutSha256'],stderrSha256=result['stderrSha256'],stderr=stderr,
                originalsUnchanged=result['originalsUnchanged'],audioReportObserved=audio_path.exists(),
                audioDeviceReady=audio.get('deviceReady'),audioMuteDepth=audio.get('muteDepth'))
    if not audio_path.exists():
        observation['audioUnobservedReason']='정상조기Quit은프레임제한용audio.tsv생성시점에도달하지않음'
    return observation


def check_confirmation(report):
    """실제ESC/Resolution하위/SETSIZE확인관측이분기선택보다먼저있었는지검사한다."""
    observed=states(report)
    check_bar(observed['bar'],True)
    assert observed['initial']['size']==[str(WIDTH),str(HEIGHT)],observed['initial']
    assert observed['confirm']['page'][0].casefold()=='setsize',observed['confirm']
    assert observed['confirm']['size']==[str(WIDTH),str(HEIGHT)],observed['confirm']
    assert {'Yes','No'}<=set(observed['confirm']['controls']),observed['confirm']['controls']
    assert observed['confirm']['controls']['Yes'][0]=='1' and observed['confirm']['controls']['No'][0]=='1',observed['confirm']['controls']
    target=choose(menu_items(report,'resolution'),text='800 by 600')
    assert target['visible'] and target['command']==0x02000095 and target['parameter']!=0,target
    return observed,target


def branch(out,labels,yes):
    """Yes와No를각각새실제프로세스와독립설정복원범위에서검증한다."""
    directory=out/('yes' if yes else 'no')
    report,result=protected_run(directory,WIDTH,HEIGHT,plan(directory,labels,yes),FRAME_LIMIT,capture_saved_options=yes)
    observed,target=check_confirmation(report)
    observation=process_observation(directory,result,expect_early_quit=yes)
    if yes:
        assert result.get('savedScreenSize')==[800,600],result.get('savedScreenSize')
        assert 'mustNotReachAfterYes' not in observed,observed['mustNotReachAfterYes']
        assert not (directory/'must-not-reach-after-yes.bmp').exists(),'Yes후금지된캡처도달'
        assert max(row['frame'] for row in observed.values())<LATE_FRAME,observed
        observation.update(normalEarlyQuit=True,forbiddenReportFrame=LATE_FRAME,frameLimit=FRAME_LIMIT,lastObservedFrame=max(row['frame'] for row in observed.values()),
                           savedScreenSize=result['savedScreenSize'],savedSizeVerifiedBeforeRestore=True)
    else:
        for label in ('rejected','stillRunningAfterNo'):  # No이후기존화면크기와미션실행이유지되어야한다.
            row=observed[label]
            assert row['phase']==['4'] and row['size']==[str(WIDTH),str(HEIGHT)] and row['paused']==['0'],(label,row)
        assert observed['stillRunningAfterNo']['frame']==LATE_FRAME,observed['stillRunningAfterNo']
        observation.update(applicationKeptRunning=True,sizePreserved=[WIDTH,HEIGHT],observedLateFrame=LATE_FRAME,frameLimit=FRAME_LIMIT)
    observation.update(passed=True,actualResolutionItem=target,confirmationObservedBeforeChoice=True)
    save(directory/'summary.json',observation)
    return observation


def main():
    """새출력경로에원본계약/probe/No/Yes의각증거를보존하며게임은명시실행시만연다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--only',choices=('no','yes'))
    parser.add_argument('--prepare-only',action='store_true',help='GUI없이원문계약과probe계획만작성')
    args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    result=dict(prepared=True,executed=False,toolSha256=sha(Path(__file__)),originalContract=original_contract(),runs={})
    save(out/'summary.json',result)
    if args.prepare_only:
        (out/'probe-steps.tsv').write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in probe_steps(out)),encoding='utf-8',newline='\n')
        print(json.dumps(result,ensure_ascii=False))
        return
    try:
        result['executed']=True
        probe=out/'probe'
        report,execution=protected_run(probe,WIDTH,HEIGHT,probe_steps(probe),44)
        observed=states(report)
        check_bar(observed['bar'],True);check_bar(observed['closed'],False)
        result['probe']=process_observation(probe,execution)
        items=menu_items(report,'bar')
        labels={text:choose(items,text=text)['label'] for text in ('Options','Resolution','800 by 600')}  # 실제같은StyleText행의라벨만사용한다.
        save(out/'actual-menu-items.json',items);save(out/'selected-actual-labels.json',labels)
        for yes in (False,True):  # No의계속실행증거를저장한뒤Yes를별도프로세스로실행한다.
            name='yes' if yes else 'no'
            if args.only and args.only!=name:
                continue
            result['runs'][name]=branch(out,labels,yes)
            assert result['runs'][name]['binarySha256']==result['probe']['binarySha256'],'관측도중실행파일이바뀜'
            save(out/'summary.json',result)
    except Exception as error:
        result['failed']=repr(error)
        save(out/'summary.json',result)
        raise
    result.update(passed=True,limitations=['QuitApp99원문은보호TELL에서독립확인하고실제Yes→정상Quit/늦은report미도달을인과적으로검증한다.이벤트전체바이트trace는별도native/제품검사범위다.',
                                          '원본left85전체HUD·픽셀정밀동일성·OS입력메시지발생·전체미션완주는검증범위밖이다.'])
    save(out/'summary.json',result)
    print(json.dumps(result,ensure_ascii=False))


if __name__=='__main__':  # 명시CLI호출전에는프로세스/게임/GUI를실행하지않는다.
    main()
