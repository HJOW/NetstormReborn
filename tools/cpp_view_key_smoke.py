#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기본 캠페인의 실제 F2/F3·수정키·정지·F8/모달과 메인 복귀를 순차 확인한다."""
import argparse
from pathlib import Path
from cpp_bridge_ritual_runtime import GAME,save,sha
from cpp_game_menu_smoke import protected_run,menu_items
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

# 원본4:3 창과 확장16:9 전체화면·16:10 창을 같은 제품으로 순서대로 관측한다.
SIZES={'1024x768':(1024,768,False),'1280x720':(1280,720,True),'1280x800':(1280,800,False)}


def verify(report,run,width,height,fullscreen):
    """실제 Client 표시 전역·원본 HUD·정지·현재 HTML과 선택 후 수명을 읽는다."""
    observed=states(report)
    expectedHide={'initial':0,'hidden':1,'hiddenReleased':1,'shown':0,'chatShown':0,'chatReleased':0,
                  'chatCtrl':0,'chatBoth':0,'chatHidden':0,'paused':0,'pausedF2':0,'pausedF3':0,
                  'themeFirst':0,'themeSecond':0,'resumed':0,'review':0,'modalF2':0,'modalF3':0,
                  'continued':0,'chatRestored':0,'hiddenFinal':0,'view':0}
    for label,hide in expectedHide.items():
        # 실제 같은 전역과 소유자가 키 입력 중 새 HUD/Session 사본으로 바뀌지 않아야 한다.
        row=observed[label]
        assert row['raw_view_keys'][0]==str(hide),(label,row['raw_view_keys'])
        assert row['size']==[str(width),str(height)] and bool(int(row['screen_mode'][0])&1)==fullscreen
        assert row['raw_uber'][:6]==observed['initial']['raw_uber'][:6]
        assert row['raw_hud_owners']==observed['initial']['raw_hud_owners'] and row['raw_session']==['1','1']
    expectedChat={label:height for label in expectedHide}
    expectedChat.update({label:height-96 for label in ('chatShown','chatReleased','chatCtrl','chatBoth','chatRestored')})
    for label,y in expectedChat.items():
        # F3는 원래 큐 인자0/2를 거친 동일 구분선과 지도 하단을 바꾼다.
        dividers=[row for row in repeated(report,'raw_hud_divider')[label] if int(row[6])&2]
        assert len(dividers)==1 and float(dividers[0][3])==y,(label,dividers)
        assert int(observed[label]['raw_uber'][6])==y
    for label in expectedHide:
        # Shift-F3의 설정 호출은 정지 앞이며 일반 F2/F3는 정지 중 표시를 바꾸지 않는다.
        paused=label in ('paused','pausedF2','pausedF3','themeFirst','themeSecond','review','modalF2','modalF3')
        assert observed[label]['paused']==[str(int(paused))],(label,observed[label]['paused'])
    initialMode=int(observed['initial']['raw_view_keys'][2])
    assert initialMode in (0,1),initialMode
    assert int(observed['themeFirst']['raw_view_keys'][2])==int(not initialMode)
    assert int(observed['themeSecond']['raw_view_keys'][2])==initialMode
    assert int(observed['review']['html_window_state'][0])==1
    assert observed['review']['raw_html_body'][-1]==observed['briefing']['raw_html_body'][-1]
    for label in ('modalF2','modalF3'):
        # 실제 F8이 연 같은 HTML 루트가 현재 모달 키 소비 뒤에도 살아 있어야 한다.
        assert observed[label]['raw_html_root'][0]==observed['review']['raw_html_root'][0]
    items=[row for row in menu_items(report,'view') if row['visible']]
    assert not any(row['command']==0x02000088 or 'CHEAT' in row['label'].upper() for row in items)
    assert observed['leave']['page'][0].casefold()=='abort'
    assert observed['main']['phase']==['0'] and observed['main']['state_hud'][0]=='0'
    return {'size':[width,height],'fullscreen':fullscreen,'nativeF2F3':True,'releaseAndModifiers':True,
            'pausedKeysBlocked':True,'themeTellWhilePaused':True,'f8ActualReview':True,
            'modalKeysBlocked':True,'missionViewChatHidden':True,'sameHudAndSession':True,'mainMenuReturned':True,'run':run}


def one(output,width,height,fullscreen):
    """일반 캠페인1-1에서 키와 정상 UI 버튼만 보내며 직접 명령/월드 쓰기를 하지 않는다."""
    output.mkdir(parents=True,exist_ok=False)
    steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),
           (25,'key','f2'),(30,'report','hidden'),(30,'snapshot',str(output/'hidden.bmp')),
           (35,'keyup','f2'),(40,'report','hiddenReleased'),(45,'key','alt+f2'),(50,'report','shown'),
           (50,'snapshot',str(output/'shown.bmp')),(55,'key','f3'),(60,'report','chatShown'),
           (65,'keyup','f3'),(70,'report','chatReleased'),(75,'key','ctrl+f3'),(80,'report','chatCtrl'),
           (85,'key','ctrl+shift+f3'),(90,'report','chatBoth'),(95,'key','alt+f3'),(100,'report','chatHidden'),
           (105,'key','shift+f9'),(110,'report','paused'),(115,'key','f2'),(120,'report','pausedF2'),
           (125,'key','f3'),(130,'report','pausedF3'),(135,'key','shift+f3'),(145,'report','themeFirst'),
           (150,'key','shift+f3'),(160,'report','themeSecond'),(165,'key','shift+f9'),(170,'report','resumed'),
           (175,'key','f8'),(180,'report','review'),(180,'snapshot',str(output/'review.bmp')),
           (185,'key','f2'),(190,'report','modalF2'),(195,'key','f3'),(200,'report','modalF3'),
           (215,'click','Play Mission'),(220,'report','continued'),(225,'key','f3'),(230,'report','chatRestored'),
           (235,'key','f3'),(240,'report','hiddenFinal'),(245,'esc',''),(250,'click','View'),(255,'report','view'),
           (260,'esc',''),(265,'esc',''),(270,'click','Game'),(275,'click','Leave Mission'),(285,'report','leave'),
           (295,'click','Main Menu'),(305,'report','main'),(305,'snapshot',str(output/'main.bmp'))]
    report,run=protected_run(output/'run',width,height,steps,315,fullscreen=fullscreen)
    result=verify(report,run,width,height,fullscreen);save(output/'summary.json',result)
    return result


def main():
    """각 실제 게임이 끝난 뒤 다음 창을 열며 같은 바이너리 SHA와 보호 설정 복원을 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--case',action='append',choices=SIZES)
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    binary=sha(GAME);results=[]  # 실행 전 제품과 순서대로 수락한 결과다.
    for name in args.case or SIZES:
        # 동시에 두 게임을 실행하지 않으며 단계 사이에서 제품 변경을 거부한다.
        results.append(one(output/name,*SIZES[name]));assert sha(GAME)==binary
    save(output/'summary.json',{'results':results,'binarySha256':binary,'directWorldMutation':False,'f7BannerPathDeferred':True})
    print('F2/F3 표시·수정키·정지·F8 실제 안내:',len(results),'개 통과')


if __name__=='__main__':main()
