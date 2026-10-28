#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 캠페인 안내의 F8 재진입·뗌·수정키·정지와 같은 모달 수명을 검증한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT,GAME,execute,save,sha
from cpp_game_menu_smoke import protected_run,muted_device_environment
from cpp_renderer_smoke import snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_world_smoke import states

# 원래4:3 창·확장16:9 전체화면·16:10 창을 같은 제품으로 순서대로 확인한다.
SIZES={'1024x768':(1024,768,False),'1280x720':(1280,720,True),'1280x800':(1280,800,False)}


def verify(report,run,width,height,fullscreen):
    """같은 HTML 토큰/serial·Pause·현재 테마와 실제 Guide/Session을 대조한다."""
    observed=states(report)
    groups={'briefing':('briefF8','briefRelease','briefShift'),'review':('reviewF8','reviewAlt','reviewShift','reviewRelease'),
            'pausedReview':('pausedReviewF8',)}
    for baseline,labels in groups.items():
        # 이미 열린 안내는 키를 반복해도 새 창/자기 정지 깊이를 만들지 않아야 한다.
        initial=observed[baseline]
        assert initial['phase']==['3'] and initial['paused']==['1'] and initial['html_window_state'][0]=='1'
        for label in labels:
            # 텍스트가 같아도 다른 창을 만들면 실패이며 현재 물리 루트/serial까지 대조한다.
            row=observed[label]
            assert row['raw_html_root'][:2]==initial['raw_html_root'][:2],(label,row['raw_html_root'],initial['raw_html_root'])
            assert row['html_window_state'][0]=='1' and row['paused']==['1']
            assert row['raw_view_keys'][2]==initial['raw_view_keys'][2]
    plain=('initial','released','control','controlShift','continued','themeFirst','themeReleased','themeSecond','resumed','continuedAlt')
    for label in plain:
        # 단일 Play Mission으로 돌아온 장면에는 중복 안내/잠금이 남지 않아야 한다.
        row=observed[label]
        assert row['phase']==['4'] and row['paused']==['0'] and row['html_window_state'][0]=='0',(label,row['phase'],row['paused'])
    mode=int(observed['initial']['raw_view_keys'][2]);assert mode in (0,1)
    for label in ('themeFirst','themeReleased','pausedTheme'):
        # Shift F8의 테마 Tell은 일반 정지 관문보다 앞이지만 열린 모달보다 뒤다.
        assert int(observed[label]['raw_view_keys'][2])==int(not mode)
    for label in ('themeSecond','pausedThemeBack'):assert int(observed[label]['raw_view_keys'][2])==mode # 두 번 전환한 뒤에는 원래 현재 설정으로 돌아온다.
    for label in ('paused','pausedTheme','pausedThemeBack','afterPausedReview'):
        # Guide 안내의 자기 정지가 해제돼도 사용자가 별도로 잡은 정지는 유지한다.
        assert observed[label]['paused']==['1'],(label,observed[label]['paused'])
    assert observed['altReview']['html_window_state'][0]=='1' and observed['altReview']['paused']==['1']
    for label,row in observed.items():
        # 같은 해상도/모드이며 실제 월드가 있는 관측은 같은 HUD와 Session을 유지한다.
        assert row['size']==[str(width),str(height)] and bool(int(row['screen_mode'][0])&1)==fullscreen
        if label not in ('leave','main'):
            assert row['raw_session']==['1','1'] and row['raw_uber'][:3]==observed['initial']['raw_uber'][:3]
    assert observed['leave']['page'][0].casefold()=='abort'
    assert observed['main']['phase']==['0'] and observed['main']['state_hud'][0]=='0'
    return {'size':[width,height],'fullscreen':fullscreen,'modalRootAndSerialPreserved':True,'releaseAndControlIgnored':True,
            'shiftF8ThemeWhilePaused':True,'altF8Review':True,'manualPausePreserved':True,'singleClickResume':True,
            'sameHudAndSession':True,'mainMenuReturned':True,'run':run}


def one(output,width,height,fullscreen):
    """기본 캠페인1-1의 실제 키/안내 버튼/ESC만 사용하며 직접 상태/명령을 쓰지 않는다."""
    output.mkdir(parents=True,exist_ok=False)
    steps=[(5,'report','briefing'),(10,'key','f8'),(18,'report','briefF8'),(20,'keyup','f8'),(26,'report','briefRelease'),
           (30,'key','shift+f8'),(38,'report','briefShift'),(40,'click','Play Mission'),(50,'report','initial'),
           (55,'keyup','f8'),(60,'report','released'),(65,'key','ctrl+f8'),(70,'report','control'),
           (75,'key','ctrl+shift+f8'),(80,'report','controlShift'),(85,'key','f8'),(95,'report','review'),
           (100,'key','f8'),(110,'report','reviewF8'),(115,'key','alt+f8'),(125,'report','reviewAlt'),
           (130,'key','shift+f8'),(140,'report','reviewShift'),(145,'keyup','f8'),(150,'report','reviewRelease'),
           (150,'snapshot',str(output/'review.bmp')),(155,'click','Play Mission'),(165,'report','continued'),
           (170,'key','shift+f8'),(180,'report','themeFirst'),(185,'keyup','shift+f8'),(190,'report','themeReleased'),
           (195,'key','shift+f8'),(205,'report','themeSecond'),(210,'key','shift+f9'),(220,'report','paused'),
           (225,'key','shift+f8'),(235,'report','pausedTheme'),(240,'key','shift+f8'),(250,'report','pausedThemeBack'),
           (255,'key','f8'),(265,'report','pausedReview'),(270,'key','f8'),(280,'report','pausedReviewF8'),
           (285,'click','Play Mission'),(295,'report','afterPausedReview'),(300,'key','shift+f9'),(310,'report','resumed'),
           (315,'key','alt+f8'),(325,'report','altReview'),(330,'click','Play Mission'),(340,'report','continuedAlt'),
           (345,'esc',''),(350,'click','Game'),(355,'click','Leave Mission'),(365,'report','leave'),
           (375,'click','Main Menu'),(385,'report','main'),(385,'snapshot',str(output/'main.bmp'))]
    report,run=protected_run(output/'run',width,height,steps,395,fullscreen=fullscreen)
    result=verify(report,run,width,height,fullscreen);save(output/'summary.json',result)
    return result


def editor(source,output,width,height,fullscreen):
    """새 자료 사본의 실제 Edit 메뉴로 들어가 F8 mode0→1과 뗌 보존을 확인한다."""
    output.mkdir(parents=True,exist_ok=False);data=output/'data';shutil.copytree(source,data)
    before=snapshots(data);original=snapshots(ROOT/'originals')  # 실행 사본과 보호 원본을 별도로 비교한다.
    steps=[(3,'click-if','OK'),(5,'click-if','OK'),(10,'click','Edit'),(18,'click','TEST02'),
           (55,'report','editor'),(60,'key','f8'),(70,'report','afterF8'),(75,'keyup','f8'),
           (85,'report','released'),(90,'snapshot',str(output/'editor.bmp'))]
    script=output/'steps.tsv';script.write_bytes(''.join(f'{frame}\t{op}\t{arg}\n' for frame,op,arg in steps).encode('utf8'))
    command=[str(GAME),'--run',str(data),'--mute-audio','--frames','100','--set',
             f'SCREENW={width};SCREENH={height};maxFPS=60;autoDemo=0;tellTips=0;sleepPerLoop=8;music=0;playWind=0',
             '--ui-script',str(script),'--ui-report',str(output/'states.tsv')]
    if fullscreen:command[command.index('--set')+1]+=';startInFullScreen=1'
    else:command.append('--window')
    with preserve_game_settings(data,'d',output/'settings-backup'),muted_device_environment():
        run=execute(command,output,GAME,90)
    save(output/'run.json',run);assert run['returnCode']==0 and not run['timedOut'],run
    observed=states(output/'states.tsv');initial=observed['editor']
    assert initial['raw_input'][1]=='0'
    for label in ('afterF8','released'):
        # 현재 선택/세션/HUD는 보존하고 같은 native 강조 모드와 커서만 바뀐다.
        row=observed[label]
        assert row['raw_world_modes']==['1','1'] and row['raw_session']==['1','1'] and row['paused']==['0']
        assert row['raw_input'][1]=='1' and row['raw_input'][0]==initial['raw_input'][0]
        assert row['mouse_cursor'][0]=='3' and row['raw_uber'][:3]==initial['raw_uber'][:3]
        assert row['size']==[str(width),str(height)] and bool(int(row['screen_mode'][0])&1)==fullscreen
    assert snapshots(data)==before and snapshots(ROOT/'originals')==original
    result={'size':[width,height],'fullscreen':fullscreen,'modeBefore':0,'modeAfter':1,'sameSelectedSid':True,
            'releasePreserved':True,'dataUnchanged':True,'originalsUnchanged':True,'run':run}
    save(output/'summary.json',result);return result


def main():
    """세 실제 창을 순차 실행하고 실행파일 SHA·보호 원본 설정 복원을 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--case',choices=SIZES,action='append');parser.add_argument('--editor-data-root',type=Path)
    args=parser.parse_args()
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False);binary=sha(GAME);results=[]  # 순차 수락한 현재 제품 관측이다.
    for name in args.case or SIZES:
        # 하나의 게임이 끝난 뒤 다음 창을 실행하며 도중 제품 변경을 거부한다.
        results.append(one(output/name,*SIZES[name]));assert sha(GAME)==binary
    editors=[]  # Editor는 보호 파일을 직접 열어 쓰지 않고 지정 원본의 새 사본만 실행한다.
    if args.editor_data_root:
        for name in args.case or SIZES:
            # 일반 미션 창이 모두 끝난 뒤 Editor 창도 한 번에 하나씩 실행한다.
            editors.append(editor(args.editor_data_root.resolve(),output/('editor-'+name),*SIZES[name]));assert sha(GAME)==binary
    save(output/'summary.json',{'results':results,'editors':editors,'binarySha256':binary,'directWorldMutation':False})
    print('F8 모달/수정키/정지/단일 복귀:',len(results),'개 통과')


if __name__=='__main__':main()
