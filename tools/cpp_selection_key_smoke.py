#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 캠페인1-1의 선택 키·정지·H/F4/F5 왕복을 앱 내 입력으로 짧게 확인한다(704)."""
import argparse
import json
import re
from pathlib import Path
from cpp_bridge_ritual_runtime import save
from cpp_deck_reactivation_smoke import protection
from cpp_game_menu_smoke import protected_run
from cpp_world_smoke import states
from cpp_solo_entry_smoke import BUTTONS

def brief(scene):
    """실제 보고에서 판정에 필요한 필드만 남긴다."""
    return {key:scene.get(key) for key in ('phase','selected','camera','raw_camera_frame','paused','time','raw_input')}

def main():
    """단일 짧은 게임의 원시 기록을 보존하며 기존 기록 재분석은 다시 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify-existing',action='store_true');args=parser.parse_args();out=args.output.resolve()
    if args.verify_existing: assert out.is_dir() and not (out/'summary.json').exists()
    else:
        out.mkdir(parents=True,exist_ok=False);before=protection();save(out/'protection-before.json',before)
        steps=[(5,'report','briefing'),(10,'click','Play Mission'),(20,'report','initial'),(25,'key','P'),(30,'report','priestSelected'),(35,'key','P'),(40,'report','priestRepeat'),(45,'key','N'),(50,'report','next'),(55,'key','ctrl+f5'),(60,'report','ctrlNext'),(65,'key','shift+f9'),(70,'report','paused'),(74,'key','N'),(77,'key','P'),(80,'key','R'),(83,'key','H'),(86,'key','f4'),(89,'key','f5'),(92,'key','ctrl+f5'),(96,'report','pausedKeys'),(100,'key','shift+f9'),(105,'report','resumed'),(110,'key','f5'),(120,'report','priestView'),(125,'key','ctrl+f4'),(135,'report','blockedHome'),(140,'key','shift+f4'),(150,'report','blockedShift'),(155,'key','H'),(165,'report','home'),(170,'key','f5'),(180,'report','viewAgain'),(185,'key','f4'),(195,'report','homeF4'),(200,'keyup','f4'),(205,'report','releasedHome'),(210,'snapshot-presented',str(out/'home.bmp'))]
        steps=[(frame+220,operation,argument) for frame,operation,argument in steps if frame>=20]  #로딩 중 고정 버튼을 누르지 않고 실제 안내 종료 뒤 키 관측을 시작한다.
        for frame in range(12,220,8):  #실제 표시된 Dialog/Briefing 버튼만 같은 일반 입력 경로로 누른다.
            for label in BUTTONS:
                condition=r'phase\t[13]\n&&(?:control\t'+re.escape(label)+r'\t|raw_html_button[^\n]*\t'+re.escape(label)+r'\n)';steps.append((frame,'when',condition+';;click;;'+label))
        steps.append((5,'report','startup'));steps.sort(key=lambda row:row[0])
        try: protected_run(out/'run',1024,768,steps,435,mission='thewarbegins')
        finally: save(out/'protection-after.json',protection())
    before=json.loads((out/'protection-before.json').read_text('utf-8'));after=json.loads((out/'protection-after.json').read_text('utf-8'));run=json.loads((out/'run/run.json').read_text('utf-8'));assert before==after and run['returnCode']==0 and not run['timedOut'],run
    observed=states(out/'run/states.tsv');labels=('initial','priestSelected','priestRepeat','next','ctrlNext','paused','pausedKeys','resumed','priestView','blockedHome','blockedShift','home','viewAgain','homeF4','releasedHome')
    for label in labels:  # 같은 실제 Mission/World에서만 키와 카메라 상태를 비교한다.
        assert observed[label]['phase']==['4'],(label,brief(observed[label]))
    for label in ('priestSelected','priestRepeat','next','ctrlNext'): assert int(observed[label]['selected'][0])!=0,(label,brief(observed[label]))  # 실제 참여/다음 유닛 선택의 최종 SID만 관측한다.
    paused,keys=observed['paused'],observed['pausedKeys'];assert paused['paused']==keys['paused']==['1'] and paused['selected']==keys['selected'] and paused['raw_camera_frame'][:4]==keys['raw_camera_frame'][:4] and paused['time'][0]==keys['time'][0],(brief(paused),brief(keys))
    assert observed['resumed']['paused']==['0']
    for label in ('blockedHome','blockedShift','viewAgain'): assert observed[label]['raw_camera_frame'][:4]==observed['priestView']['raw_camera_frame'][:4],(label,brief(observed[label]))  # 금지 수정키는 Home으로 직접 우회하지 않고 일반 F5 왕복이 같은 위치에 도착한다.
    for label in ('homeF4','releasedHome'): assert observed[label]['raw_camera_frame'][:4]==observed['home']['raw_camera_frame'][:4],(label,brief(observed[label]))  # H/F4 일반 큐와 키 뗌의 관측 결과다.
    summary=dict(unit=704,passed=True,processes=1,exeSha256=run['binarySha256'],returnCode=0,protectionUnchanged=True,observations={label:brief(observed[label]) for label in labels},scope='앱 내 키 입력/실제 캠페인1-1의 짧은 관측이다. 물리 키·전체 픽셀/PCM·자연 완주·포획/희생/특정 적 공격/승리·공중3종 전용 행동·MP/통신 검사가 아니다.')
    save(out/'summary.json',summary);print(json.dumps({key:value for key,value in summary.items() if key!='observations'},ensure_ascii=False))

if __name__=='__main__':
    main()
