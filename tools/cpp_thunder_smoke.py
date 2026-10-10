#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 창에서 천둥 생성·정지 중 종료·메뉴 복귀/재진입과 창 모드의 팔레트 불변을 검사한다. 원본 게임은 실행하지 않는다."""
import argparse
import json
from pathlib import Path
from PIL import Image
from cpp_audio_smoke import ROOT,OUTPUT,run,snapshots
from cpp_world_smoke import states
from cpp_weather_smoke import image_colors
from cpp_window_smoke import read_data,screen_palette
from taff import TaffArchive


def check(executable,root,name,silent=False):
    """브리핑에서 네 곡을 순환하며 같은 프레임의 천둥 프로세스와 두 프레임 뒤 종료를 관찰한다."""
    steps=[(3,'report','initial')]
    # 브리핑 정지 중에도 실제 SceneMusic::Next와 Kernel의 프레임 진행을 호출한다.
    for i in range(4):
        frame=4+i*5
        steps.extend(((frame,'music-next',''),(frame,'report',f'cycle{i}'),
                      (frame,'snapshot',str(OUTPUT/f'{name}-cycle{i}.bmp')),(frame+3,'report',f'ended{i}')))
    steps.extend(((25,'click','Play Mission'),(27,'click','Game'),(29,'click','Leave Battle'),
                  (31,'click','Leave Honorably'),(33,'report','menu'),
                  (35,'music-next',''),(36,'music-next',''),(37,'music-next',''),(37,'report','menu-thunder'),
                  (39,'click','Campaign'),(41,'click','Struggle For Freedom'),(43,'click','1 The War Begins!'),
                  (47,'report','reload')))
    values=run(executable,root,name,49,extra=['--mission','thewarbegins'],script=steps,
               settings='ascendancyPalette=0'+(';sound=0;music=0' if silent else ''))
    observed=states(OUTPUT/f'{name}-states.tsv');previous=observed['initial'];thunders=0
    palette=screen_palette(read_data(root,'d',TaffArchive(str(root/'netstorm.tarc')),'gifcloud.col'))
    assert previous['paused']==['1'] and previous['thunder_processes']==['0'],previous
    # 천둥 곡에서만 즉시 등록되고 짧은 창 모드 수명 뒤 제거된다. 게임 시각은 그대로지만 실시간은 증가한다.
    for i in range(4):
        current=observed[f'cycle{i}'];ended=observed[f'ended{i}'];index=int(current['weather'][0])
        assert index==(int(previous['weather'][0])+1)%4,current
        assert current['thunder_processes']==[str(int(index==2))],current
        assert ended['thunder_processes']==['0'],ended
        assert current['paused']==['1'] and ended['time'][0]==current['time'][0],current
        assert float(ended['time'][1])>float(current['time'][1]),ended
        image=Image.open(OUTPUT/f'{name}-cycle{i}.bmp').convert('RGB')
        assert image_colors(image)<=set(palette),'창 모드 번개가 화면 팔레트를 바꿈'
        thunders+=int(index==2);previous=current
    assert thunders==1
    assert observed['menu']['thunder_processes']==['0'] and observed['menu-thunder']['thunder_processes']==['0']
    assert observed['menu-thunder']['weather'][0]=='2','메뉴에서 부모가 없는 천둥 곡 요청 누락'
    assert observed['reload']['paused']==['1'] and observed['reload']['thunder_processes']==['0']
    assert values['workerFailed']=='0'
    return dict(states=len(observed),screenshots=4,thunder_cycles=thunders,muted=True,sound_music_disabled=silent)


def main():
    """두 실제 클론 창 시나리오를 실행하고 원본 파일/설정 보호 결과를 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--root',type=Path,default=ROOT/'originals');args=parser.parse_args()
    before=snapshots(args.root);report={}
    try:
        # 실제 장치를 음소거한 실행과 소리/음악 옵션을 모두 끈 실행을 각각 검사한다.
        for name,silent in (('thunder',False),('thunder-disabled',True)):report[name]=check(args.exe,args.root,name,silent)
    finally:assert snapshots(args.root)==before,'보호 원본 파일 변경'
    report['original_files_unchanged']=len(before);print(json.dumps(report,ensure_ascii=False,indent=2))


if __name__=='__main__':main()
