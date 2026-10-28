#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 시나리오 명령 버튼으로 하단 채팅을 열고 실제 구분선 드래그·반납을 확인한다."""
import argparse
import shutil
from pathlib import Path
from cpp_bridge_ritual_runtime import ROOT, GAME, save, sha
from cpp_game_menu_smoke import protected_run, menu_items
from cpp_production_panel_smoke import repeated
from cpp_world_smoke import states

# 원본4:3 창·확장16:9 전체화면·확장16:10 창을 동시에 실행하지 않는다.
SIZES={'1024x768':(1024,768,False),'1280x720':(1280,720,True),'1280x800':(1280,800,False)}


def verify(report,run,width,height,fullscreen):
    """같은 실제 구분선·루트·지도·캡처를 읽고 원문25/화면높이-25 제한을 대조한다."""
    observed=states(report)
    dividers=repeated(report,'raw_hud_divider')
    rows={}
    for label in ('initial','shown','middleHeld','middleReleased','lowerHeld','lowerReleased','upperHeld','upperReleased','hiddenAgain','restored'):
        # 세 구분선 중 세로 이동축2를 가진 실제 채팅 구분선만 선택한다.
        selected=[row for row in dividers[label] if int(row[6])&2]
        assert len(selected)==1,(label,selected)
        rows[label]=selected[0]
        state=observed[label]
        assert state['phase']==['4'] and state['size']==[str(width),str(height)]
        assert bool(int(state['screen_mode'][0])&1)==fullscreen
        assert state['raw_world_modes']==['0','1'] and state['raw_session']==['1','1']
        assert state['mouse_cursor'][1]=='1' and float(state['raw_sp'][1])==500000
        assert state['raw_uber'][:6]==observed['initial']['raw_uber'][:6]
        assert state['raw_hud_owners']==observed['initial']['raw_hud_owners']
        assert selected[0][:3]==rows['initial'][:3] and int(selected[0][6])==26
        assert list(map(int,selected[0][9:11]))==[25,height-25]
    # 5픽셀 선 중앙의 anchor2는 방송의 pointer-2 제한 뒤 공통Move에서 다시 뺀다.
    expected={'initial':height,'shown':height-96,'middleHeld':height//2-4,'middleReleased':height//2-4,
              'lowerHeld':height-27,'lowerReleased':height-27,'upperHeld':23,'upperReleased':23,
              'hiddenAgain':height,'restored':23}
    for label,y in expected.items():
        # 원래 도킹 방송이 같은 지도 하단도 바꿔야 하며 검사에서 직접 치수를 쓰지 않는다.
        assert float(rows[label][3])==y,(label,rows[label])
        assert int(observed[label]['raw_uber'][6])==y,(label,observed[label]['raw_uber'])
        if label not in ('initial',):assert observed[label]['paused']==['0']
    for label in ('middleHeld','lowerHeld','upperHeld'):
        # 눌러 드래그하는 동안 실제100h 통지와 공통 캡처가 같은 토큰을 가리켜야 한다.
        assert int(rows[label][11])&0x100
        assert observed[label]['RawButtons'][3]==rows[label][0]
    for label in ('middleReleased','lowerReleased','upperReleased','hiddenAgain','restored'):
        # 뗌 뒤 notify와 공통 캡처가 함께 해제되고 이전 좌표가 되살아나지 않아야 한다.
        assert not int(rows[label][11])&0x100
        assert observed[label]['RawButtons'][3]=='0'
    for label in ('review','reviewAgain'):
        assert any(row[-1]=='Toggle Bottom Window' and int(row[3])==0x02000088 for row in repeated(report,'raw_html_button')[label])
        assert not any('CHEAT' in row['label'].upper() for row in menu_items(report,label) if row['visible'])
    assert observed['leave']['page'][0].casefold()=='abort'
    assert observed['main']['phase']==['0'] and observed['main']['state_hud'][0]=='0'
    return {'size':[width,height],'fullscreen':fullscreen,'root':observed['initial']['raw_uber'][0],
            'chatDivider':rows['initial'][0],'positions':expected,'captureReleased':True,
            'sameHudAndSp':True,'mainMenuReturned':True,'run':run}


def one(source,output,width,height,fullscreen):
    """원본과 별개인 새 사본의 안내에 원문 명령 버튼 한 줄만 추가한다."""
    output.mkdir(parents=True,exist_ok=False)
    data=output/'data'
    shutil.copytree(source,data)
    english=data/'d/TEST02.english'
    original=english.read_bytes()
    needle=b'$Button=Go!,DoNothing,0'
    assert original.count(needle)==1
    english.write_bytes(original.replace(needle,b'$Button=Toggle Bottom Window,ToggleBottomGump,2\r\n'+needle))
    save(output/'scenario-variant.json',{'sourceEnglishSha256':sha(source/'d/TEST02.english'),'englishSha256':sha(english),
         'fortSha256':sha(data/'d/TEST02.fort'),'sourceFortSha256':sha(source/'d/TEST02.fort'),
         'onlyChange':'A. Go! 앞 원문 ToggleBottomGump 인자2 버튼 한 줄'})
    x=width//2
    # 첫down만 큐에 넣고 드래그는 폴링 포인터를 옮긴다. 두번째down은 원본 anchor를 덮고 capture를 해제한다.
    steps=[(5,'report','initial'),(10,'click','Toggle Bottom Window'),(20,'report','shown'),(20,'snapshot',str(output/'shown.bmp')),
           (25,'down','hud:chat-divider'),(30,'pointer',f'0x10000,{x},{height//2}'),(35,'report','middleHeld'),
           (40,'up',f'screen:{x},{height//2}'),(45,'report','middleReleased'),(50,'down','hud:chat-divider'),
           (55,'pointer',f'0x10000,{x},{height-1}'),(60,'report','lowerHeld'),(65,'up',f'screen:{x},{height-1}'),
           (70,'report','lowerReleased'),(75,'down','hud:chat-divider'),(80,'pointer',f'0x10000,{x},10'),(85,'report','upperHeld'),
           (90,'up',f'screen:{x},10'),(95,'report','upperReleased'),(95,'snapshot',str(output/'upper.bmp')),
           (100,'esc',''),(105,'click','Game'),(110,'click','menu-command:02000091,0'),(115,'report','review'),
           (120,'click','Toggle Bottom Window'),(130,'report','hiddenAgain'),(135,'esc',''),(140,'click','Game'),
           (145,'click','menu-command:02000091,0'),(148,'report','reviewAgain'),(150,'click','Toggle Bottom Window'),
           (160,'report','restored'),(160,'snapshot',str(output/'restored.bmp')),(165,'esc',''),(170,'click','Game'),
           (175,'click','Leave Mission'),(185,'report','leave'),(195,'click','Main Menu'),(210,'report','main'),
           (210,'snapshot',str(output/'main.bmp'))]
    report,run=protected_run(output/'run',width,height,steps,220,data_root=data,mission='TEST02',fullscreen=fullscreen)
    result=verify(report,run,width,height,fullscreen)
    save(output/'summary.json',result)
    return result


def default_campaign(output):
    """같은 제품의 원래 캠페인에서 기본 View 채팅 숨김과 정상 Leave 수명을 대조한다."""
    steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),
           (25,'esc',''),(30,'click','View'),(36,'report','view'),(36,'snapshot',str(output/'view.bmp')),
           (45,'esc',''),(50,'esc',''),(55,'click','Game'),(60,'click','Leave Mission'),
           (70,'click','Main Menu'),(80,'report','mainMenu')]
    report,run=protected_run(output,1024,768,steps,85)
    observed=states(report)
    rows=repeated(report,'raw_hud_divider')['initial']
    chat=[row for row in rows if int(row[6])&2]
    assert len(chat)==1 and float(chat[0][3])==768
    items=[row for row in menu_items(report,'view') if row['visible']]
    assert not any(row['command']==0x02000088 or 'CHEAT' in row['label'].upper() for row in items)
    assert any(row['command']==0x020000ae for row in items)
    assert observed['initial']['raw_world_modes']==['0','1'] and observed['mainMenu']['phase']==['0']
    assert observed['mainMenu']['state_hud'][0]=='0'
    result={'mission':'thewarbegins','defaultChatHidden':True,'nativeViewConditionPreserved':True,
            'mainMenuReturned':True,'run':run}
    save(output/'summary.json',result)
    return result


def main():
    """보호 원본을 실행 사본으로 거부하고 지정 화면비를 같은 실행파일로 순차 관측한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--case',choices=SIZES,action='append')
    args=parser.parse_args()
    source,output=args.data_root.resolve(),args.output.resolve()
    assert all(source!=(ROOT/name).resolve() and (ROOT/name).resolve() not in source.parents for name in ('originals','originalCD'))
    output.mkdir(parents=True,exist_ok=False)
    binary=sha(GAME)
    results=[]
    for name in args.case or SIZES:
        # 실제 창은 동시에 실행하지 않으며 각 게임 뒤 같은 바이너리인지 확인한다.
        results.append(one(source,output/name,*SIZES[name]))
        assert sha(GAME)==binary
    default=default_campaign(output/'default-campaign')
    assert sha(GAME)==binary
    save(output/'summary.json',{'results':results,'defaultCampaign':default,'binarySha256':binary,'directWorldMutation':False})
    print('하단 표시·채팅 구분선 드래그/반납:',len(results),'개 통과')


if __name__=='__main__':
    main()
