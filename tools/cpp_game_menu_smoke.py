#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 ESC 가로 메뉴를 먼저 관측하고 같은 행 라벨/Controls.Point로 두 화면비를 검사한다."""
import argparse
import json
import os
from contextlib import contextmanager
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, GAME, execute, save, sha
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states
from cpp_production_panel_smoke import repeated

SIZES=((1024,768),(1280,720))  # 첫 화면비의 작은 core가 끝난 뒤 두번째 확장을 실행한다.
COMMANDS={'review':0x02000091,'restart':0x0200008d,'leave':0x0200008c,'tell':0x02000095,
          'sound':0x020000e6,'music':0x020000e7,'hide':0x020000f1,'home':0x020000ae,'priest':0x020000b8}  # 보호PE의실제일반명령이다.
KEYS=(('Q',0),('W',1),('A',2),('S',3),('Z',4),('X',5))  # 실제생산큐의여섯슬롯을검사한다.


@contextmanager
def muted_device_environment():
    """기존무음환경을보관하고장치를열어음소거하는실행동안만NO_AUDIO를제거한다."""
    previous=os.environ.pop('NETSTORM_CPP_NO_AUDIO',None)  # 기존호출자의존재/값을복원한다.
    try:
        yield
    finally:
        if previous is None:
            os.environ.pop('NETSTORM_CPP_NO_AUDIO',None)
        else:
            os.environ['NETSTORM_CPP_NO_AUDIO']=previous


def menu_items(report,label):
    """같은실제보고서의반복항목을버리지않고숫자필드와라벨을읽는다."""
    output=[]  # 각실제메뉴의항목기록이다.
    for row in repeated(report,'raw_menu_item')[label]:  # 현재원시항목만해석하고별도메뉴기대목록은만들지않는다.
        if len(row)!=7:
            raise RuntimeError('raw_menu_item 보고폭 변경: '+repr(row))
        output.append(dict(token=int(row[0]),index=int(row[1]),flags=int(row[2]),command=int(row[3]),parameter=int(row[4]),visible=bool(int(row[5])),label=row[6]))
    return output


def choose(items,*,command=None,parameter=None,text=None):
    """원시명령또는실제문자열로현재항목을고르고같은라벨을입력에사용한다."""
    matches=[row for row in items if (command is None or row['command']==command) and
             (parameter is None or row['parameter']==parameter) and
             (text is None or row['label'].strip().casefold()==text.casefold())]  # 원문항목만선택한다.
    if not matches:
        raise RuntimeError('실제 메뉴 항목 없음: '+repr((command,parameter,text)))
    labels={row['label'] for row in matches}  # Game/View에있는같은Review라벨은하나의실제문자열이다.
    if len(labels)!=1:
        raise RuntimeError('명령별 실제 라벨 모호: '+repr(matches))
    return matches[0]


def help_word(report,label,items):
    """실제가로루트토큰의나머지열에서config.helpWord라벨을그대로얻는다."""
    horizontal={int(row[0]) for row in repeated(report,'raw_menu')[label] if int(row[4])&0x40000}  # 실제menuFlags의가로메뉴비트다.
    roots=[token for token in horizontal if any(row['token']==token and row['visible'] for row in items)]  # 표시되는같은루트항목이다.
    if len(roots)!=1:
        raise RuntimeError('실제 표시 가로 메뉴 루트 모호: '+repr(roots))
    remaining=[row for row in items if row['token']==roots[0] and row['label'].strip().casefold() not in ('game','view','options','players','edit','cheat')]  # 원문조건으로포함한알려진다른열만제외한다.
    if len(remaining)!=1:
        raise RuntimeError('실제 helpWord 열 모호: '+repr(remaining))
    return remaining[0]['label']


def audio_values(path):
    """실제종료직전장치/음소거관측만읽는다."""
    if not path.exists():
        return {}
    return dict(line.split('\t',1) for line in path.read_text(encoding='utf-8').splitlines() if '\t' in line)


def protected_run(directory,width,height,steps,frames):
    """기존execute의PID/SHA/종료관측을쓰고실패에도보호설정을복원한다."""
    directory.mkdir(parents=True,exist_ok=False)
    script,report=directory/'steps.tsv',directory/'states.tsv'  # 이번실행의별도입력/현재상태다.
    script.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in steps),encoding='utf-8',newline='\n')
    command=[str(GAME),'--run',str(ROOT/'originals'),'--mission','thewarbegins','--window','--mute-audio','--frames',str(frames),
             '--set',f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;useProductTimers=0;sound=1;music=0;playWind=0;soundVolume=3;musicVolume=3',
             '--ui-script',str(script),'--ui-report',str(report),'--audio-report',str(directory/'audio.tsv')]
    before=snapshots(ROOT/'originals')  # 허용설정도포함한전체이름/바이트목록이다.
    save(directory/'originals-before.json',before)
    result=dict(command=command,binarySha256=sha(GAME),scriptSha256=sha(script))  # 실행전실패도검토가능하게남긴다.
    try:
        with preserve_game_settings(ROOT/'originals','d'),muted_device_environment():
            result.update(execute(command,directory,GAME,max(90,frames/60*4+30)))
    except Exception as error:
        result['exception']=repr(error)
        raise
    finally:
        result['originalsUnchanged']=snapshots(ROOT/'originals')==before
        save(directory/'run.json',result)
    if result['returnCode'] or result['timedOut'] or not result['originalsUnchanged']:
        raise RuntimeError(result)
    return report,result


def probe_steps(directory):
    """고정좌표없이최초실제ESC막대와모든실제하위항목을읽는다."""
    return [(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),(25,'esc',''),
            (30,'report','bar'),(30,'snapshot',str(directory/'bar.bmp')),(35,'esc',''),(40,'report','closed')]


def plan(directory,items,extended,volumes_available,actual_help_word):
    """probe의원시라벨로만실제클릭계획을작성하고명령/Tell직접주입을쓰지않는다."""
    labels={key:choose(items,command=COMMANDS[key])['label'] for key in ('review','restart','leave')}  # 하위실제행라벨이다.
    for key in ('Game','View','Options','Resolution'):  # 상위/하위제목도같은원시항목에서읽는다.
        labels[key]=choose(items,text=key)['label']
    labels['About']=actual_help_word  # 실제config.helpWord열이며About이라는라벨을합성하지않는다.
    labels['version']=choose(items,text='Version')['label']
    steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),(20,'snapshot',str(directory/'initial.bmp')),
           (25,'esc',''),(30,'report','opened'),(30,'snapshot',str(directory/'opened.bmp')),
           (32,'key','X'),(36,'report','missingSlot'),(40,'esc',''),(44,'report','closed'),
           (55,'esc',''),(60,'click',labels['Game']),(64,'report','game'),(70,'click',labels['review']),
           (78,'report','review'),(78,'snapshot',str(directory/'review.bmp')),(85,'click','Play Mission'),(92,'report','reviewClosed'),
           (100,'esc',''),(105,'click',labels['Game']),(110,'click',labels['leave']),(118,'report','leaveConfirm'),
           (118,'snapshot',str(directory/'leave-confirm.bmp')),(125,'click','Continue Mission'),(132,'report','leaveCancelled'),
           (140,'esc',''),(145,'click',labels['View']),(152,'report','view'),(152,'snapshot',str(directory/'view.bmp')),
           (160,'esc',''),(166,'esc',''),(172,'click',labels['About']),(177,'click',labels['version']),
           (185,'report','version'),(185,'snapshot',str(directory/'version.bmp')),(192,'click','OK'),(200,'report','versionClosed'),
           (208,'esc',''),(214,'click',labels['Game']),(220,'click',labels['restart']),(235,'report','restarted'),
           (235,'snapshot',str(directory/'restarted.bmp')),(242,'click','Play Mission'),(252,'report','restartPlayed')]  # Leave의원문취소는No가아닌Continue Mission이다.
    if not extended:
        return steps,258,labels
    steps.extend([(265,'esc',''),(271,'click',labels['Options']),(278,'report','options'),(278,'snapshot',str(directory/'options.bmp'))])
    frame=285  # 실제선택후다음막대를열기까지관측간격을둔다.
    if volumes_available:
        for key,title,volume in (('sound','Sound Effect Volume',2),('music','Music Volume',4)):  # 두실제음량하위의서로다른숫자인자를선택한다.
            labels[key+'Title']=choose(items,text=title)['label']
            labels[key+'Volume']=choose(items,command=COMMANDS[key],parameter=volume)['label']
            steps.extend([(frame,'click',labels[key+'Title']),(frame+6,'report',key+'Submenu'),(frame+10,'click',labels[key+'Volume']),
                          (frame+18,'report',key+'Applied'),(frame+25,'esc',''),(frame+31,'click',labels['Options'])])
            frame+=40
    labels['resolution800']=choose(items,text='800 by 600')['label']
    steps.extend([(frame,'click',labels['Resolution']),(frame+6,'report','resolutionSubmenu'),(frame+12,'click',labels['resolution800']),
                  (frame+20,'report','resolutionConfirm'),(frame+20,'snapshot',str(directory/'resolution-confirm.bmp')),
                  (frame+28,'click','No'),(frame+36,'report','resolutionCancelled')])
    # 재시작이후충분히공급한원문현재슬롯을기대입력으로삼고소비/직접조각생성은하지않는다.
    steps.extend([(1300,'report','mature'),(1300,'snapshot',str(directory/'mature.bmp'))])
    for index,(key,slot) in enumerate(KEYS):  # 매번취득전실제막대를열어그때Pause가추가되지않는지관측한다.
        at=1310+index*30
        if index:
            steps.append((at,'esc',''))  # 앞취득커서를실제ESC로취소하고현재팝업을닫는다.
        steps.extend([(at+5,'esc',''),(at+10,'report','keyBar'+key),(at+15,'key',key),(at+21,'report','key'+key)])
    steps.extend([(1510,'esc',''),(1518,'esc',''),(1523,'report','outsideBefore'),(1528,'click','world:priest'),
                  (1535,'report','outsideAfter'),(1535,'snapshot',str(directory/'outside-after.bmp'))])
    return sorted(steps,key=lambda row:row[0]),1540,labels


def check_bar(row,opened):
    """동일serial유효조회와정지관측으로막대상태를확인한다."""
    assert row['game_menu_bar'][1]==str(int(opened)),row.get('game_menu_bar')
    assert row['paused']==['0'],row['paused']
    assert row['popup']==[''],row['popup']
    if opened:
        assert int(row['game_menu_bar'][0])!=0,row['game_menu_bar']


def check_core(report,width,height):
    """두비율공통의실제ESC/명령/확인취소/재시작결과를판정한다."""
    observed=states(report)  # 최종보고값을입력이나예상상태로주입하지않는다.
    for label in ('initial','opened','closed','reviewClosed','leaveCancelled','view','versionClosed','restartPlayed'):  # 막대만의열림이자동정지를만들지않는다.
        assert observed[label]['paused']==['0'],(label,observed[label]['paused'])
        assert observed[label]['size']==[str(width),str(height)],(label,observed[label]['size'])
    check_bar(observed['opened'],True)
    check_bar(observed['closed'],False)
    assert observed['review']['page'][0].casefold()=='a.',observed['review']
    assert observed['leaveConfirm']['page'][0].casefold()=='abort',observed['leaveConfirm']
    assert {'Main Menu','Replay Mission','Continue Mission'}<=set(observed['leaveConfirm']['controls']),observed['leaveConfirm']['controls']
    assert observed['leaveCancelled']['phase']==['4'],observed['leaveCancelled']
    assert observed['version']['page'][0].casefold()=='about' and 'OK' in observed['version']['controls'],observed['version']
    assert observed['restarted']['phase']==['3'] and 'Play Mission' in observed['restarted']['controls'],observed['restarted']
    assert observed['restartPlayed']['mission'][0].casefold()=='thewarbegins' and observed['restartPlayed']['mission_start'][0]=='1',observed['restartPlayed']
    contents=repeated(report,'raw_production_content')  # 실제동일공급장부에서아직없는슬롯을확인한다.
    early={int(row[4]) for row in contents['missingSlot'] if int(row[2])&0x40}
    assert 5 not in early and observed['missingSlot']['raw_cursor'][0]!='0',(early,observed['missingSlot']['raw_cursor'])
    visible=[row['label'] for row in menu_items(report,'game') if row['visible']]  # 실제표시된Game행과숨긴다른하위는구별한다.
    assert any(row['visible'] and row['command']==COMMANDS['review'] for row in menu_items(report,'game')),visible
    assert any(row['visible'] and row['command']==COMMANDS['leave'] for row in menu_items(report,'game')),visible
    return observed,dict(corePassed=True,viewport=[width,height],earlyMissingSlot=5,earlySlots=sorted(early),visibleGameLabels=visible,
                         viewLabels=[row['label'] for row in menu_items(report,'view') if row['visible']])


def check_extended(report,observed,width,height,volumes_available):
    """같은실제숫자인자/해상도취소/열린막대의생산키와월드전달만추가검증한다."""
    summary=dict(volumeVerified=volumes_available)  # 장치실패는음량검증성공으로포장하지않는다.
    if volumes_available:
        assert observed['soundApplied']['soundVolume']==['2'],observed['soundApplied']['soundVolume']
        assert observed['musicApplied']['musicVolume']==['4'],observed['musicApplied']['musicVolume']
        for name,value in (('sound',2),('music',4)):  # 실제열린하위의원문숫자인자와명령을검증한다.
            assert any(row['visible'] and row['command']==COMMANDS[name] and row['parameter']==value for row in menu_items(report,name+'Submenu')),name
    else:
        summary['volumeUnverifiedReason']='실제 음향 장치가 준비되지 않아 원문 음량 하위가 비활성화됨'
    assert observed['resolutionConfirm']['page'][0].casefold()=='setsize',observed['resolutionConfirm']
    assert {'Yes','No'}<=set(observed['resolutionConfirm']['controls']),observed['resolutionConfirm']['controls']
    assert observed['resolutionCancelled']['phase']==['4'] and observed['resolutionCancelled']['size']==[str(width),str(height)],observed['resolutionCancelled']
    target=choose(menu_items(report,'resolutionSubmenu'),command=COMMANDS['tell'],text='800 by 600')
    assert target['visible'] and target['parameter']!=0,target
    parameters=repeated(report,'raw_menu_parameter').get('resolutionSubmenu',[])  # 차후실제문자열토큰보고가있으면원문인자까지직접대조한다.
    matched=[row for row in parameters if row and int(row[0])==target['parameter']]
    summary['resolutionTellParameterToken']=target['parameter']
    summary['resolutionTellParameterVerified']=bool(matched)
    if matched:
        assert matched[0][1].casefold()=='setsize(800,600)',matched
    else:
        summary['resolutionTellParameterLimitation']='현재 보고서는 토큰만 제공하여 원문 문자열 인자 자체는 미관측; SETSIZE 페이지/No/기존 크기 보존만 검증'
    contents=repeated(report,'raw_production_content')
    slots={int(row[4]):int(row[0]) for row in contents['mature'] if int(row[2])&0x40}
    assert set(range(6))<=set(slots),slots
    selections=[]  # 실제취득원본SID를입력슬롯과대조한다.
    for key,slot in KEYS:  # 하위Take직접호출없이문자→큐→Gump소비를거친현재커서를읽는다.
        check_bar(observed['keyBar'+key],True)
        cursor=observed['key'+key]['raw_cursor']
        assert observed['key'+key]['paused']==['0'] and cursor[0]=='0' and int(cursor[2])==82 and int(cursor[3])==slots[slot],(key,slot,cursor,slots)
        selections.append(dict(key=key,slot=slot,sid=int(cursor[3])))
    check_bar(observed['outsideBefore'],True)
    assert observed['outsideAfter']['game_menu_bar'][1]=='0' and observed['outsideAfter']['paused']==['0'],observed['outsideAfter']
    owner=int(observed['outsideAfter']['raw_session'][0])
    priests={sid for sid,row in observed['outsideAfter']['objects'].items() if row[0]=='priest' and int(row[1])==owner}
    assert int(observed['outsideAfter']['selected'][0]) in priests,(priests,observed['outsideAfter']['selected'])
    summary.update(extendedPassed=True,resolutionNoPreservedSize=True,selections=selections,outsideWorldSelection=observed['outsideAfter']['selected'])
    return summary


def one_size(out,width,height,extended):
    """한비율의probe와실제계획을각각독립프로세스/PID/설정보존으로실행한다."""
    directory=out/f'{width}x{height}'
    directory.mkdir(parents=True,exist_ok=False)
    probe=directory/'probe'
    report,probe_result=protected_run(probe,width,height,probe_steps(probe),44)
    observed=states(report)
    check_bar(observed['bar'],True)
    check_bar(observed['closed'],False)
    items=menu_items(report,'bar')  # 숨김하위도생성된원문행이므로여기서만라벨을얻는다.
    save(directory/'actual-menu-items.json',items)
    available=not (choose(items,text='Sound Effect Volume')['flags']&0x200 or choose(items,text='Music Volume')['flags']&0x200)
    run_dir=directory/'scenario'
    steps,frames,labels=plan(run_dir,items,extended,available,help_word(report,'bar',items))
    save(directory/'selected-actual-labels.json',labels)
    report,result=protected_run(run_dir,width,height,steps,frames)
    observed,summary=check_core(report,width,height)
    if extended:
        summary.update(check_extended(report,observed,width,height,available))
    audio=audio_values(run_dir/'audio.tsv')
    assert audio.get('muteDepth')=='1',audio
    summary.update(originalsUnchanged=probe_result['originalsUnchanged'] and result['originalsUnchanged'],
                   audioDeviceReady=audio.get('deviceReady'),audioMuteDepth=audio.get('muteDepth'),
                   binarySha256=result['binarySha256'],labelsFromActualRawMenuItems=True,
                   limitations=['현재 fixed HUD의 viewport를 사용하며 원본left85 전체dock/픽셀정밀동일성을판정하지않는다.',
                                'WM_CHAR/마우스사건은실제큐에주입한다. OS키메시지발생자체와전체미션완주는범위밖이다.',
                                'View하위는실제열림/현재행을관측한다. 모든View명령/Players/NSInfo/전체Editor완성을주장하지않는다.'])
    save(directory/'summary.json',summary)
    return summary


def main():
    """첫1024core결과를먼저보존하고그다음1280확장을실행하며실패자료도유지한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--only',choices=('1024x768','1280x720'))
    parser.add_argument('--prepare-only',action='store_true',help='GUI없이probe입력계획과범위기록만작성')
    args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    sizes=[size for size in SIZES if args.only is None or f'{size[0]}x{size[1]}'==args.only]  # 지정비율만재검증할수있다.
    result=dict(prepared=True,executed=False,viewports=sizes,toolSha256=sha(Path(__file__)),runs=[])
    save(out/'summary.json',result)
    if args.prepare_only:
        for width,height in sizes:  # 원시라벨은실행한probe에서만얻으므로가짜메뉴라벨계획은만들지않는다.
            path=out/f'{width}x{height}-probe.tsv'
            path.write_text(''.join(f'{frame}\t{operation}\t{argument}\n' for frame,operation,argument in probe_steps(out)),encoding='utf-8',newline='\n')
        print(json.dumps(result,ensure_ascii=False))
        return
    try:
        for width,height in sizes:  # core의성공요약이저장된뒤에만wide확장에진입한다.
            result['executed']=True
            result['runs'].append(one_size(out,width,height,width==1280))
            save(out/'summary.json',result)
    except Exception as error:
        result['failed']=repr(error)
        save(out/'summary.json',result)
        raise
    result['passed']=True
    save(out/'summary.json',result)
    print(json.dumps(result,ensure_ascii=False))


if __name__=='__main__':  # 명시CLI실행전에는게임/GUI/프로세스를시작하지않는다.
    main()
