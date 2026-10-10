#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 창에서 네 날씨 팔레트·같은 프레임 색 표·해상도 변경 뒤 색 보존을 확인한다. 원본 게임은 실행하지 않는다.

DirectSound 검사는 모두 무음이다. 설정 파일은 성공/실패 모두 복구하고 다른 원본 파일의 바이트/존재 상태도 확인한다.
현재 보존 자료에는 네 날씨 COL이 없으므로 원본 자료를 검사 폴더에 복사하고 합성 COL을 그 사본에만 만든다.
이 검사는 연결/순서를 검증하며 원본 날씨의 RGB 재현을 주장하지 않는다.
"""
import argparse
import json
import shutil
import tempfile
from pathlib import Path
from PIL import Image
from cpp_audio_smoke import ROOT, OUTPUT, run, snapshots
from cpp_world_smoke import states
from cpp_window_smoke import read_data, screen_palette
from taff import TaffArchive

# 원본 곡 순서의 팔레트 파일과 RGB 색이다. 둘 다 원본 00469c80/004a4850의 표 순서를 유지한다.
PALETTES=('windy.col','rainy.col','thundery.col','sunny.col')
WEATHER_RGB=((255,255,22),(22,22,255),(255,22,22),(181,140,111))


def tints(palette):
    """byte 범위 RGB 제곱 거리로 첫 최소 번호를 찾는다. 이 범위는 원본 x87 반올림 차이가 없다."""
    return [min(range(256),key=lambda i:sum((palette[i][c]-color[c])**2 for c in range(3))) for color in WEATHER_RGB]


def fixture_root(source):
    """원본을 수정하지 않는 검사 전용 사본을 만들고 비어 있는 날씨 이름에 합성 팔레트를 준비한다."""
    OUTPUT.mkdir(parents=True,exist_ok=True)
    destination=Path(tempfile.mkdtemp(prefix='cpp-weather-data-',dir=ROOT/'extracted'))
    # 실제 클론 초기화에 필요한 자산/글꼴/소리만 복사한다. 원본 실행 파일은 리소스 읽기용이며 실행하지 않는다.
    for name in ('d','sound','music'):
        shutil.copytree(source/name,destination/name)
    for name in ('Netstorm.exe','netstorm.tarc'):
        shutil.copy2(source/name,destination/name)
    base=read_data(source,'d',TaffArchive(str(source/'netstorm.tarc')),'gifcloud.col')
    if len(base)!=776:raise RuntimeError('합성 팔레트는 RGB COL을 요구함')
    # XOR 색 변환은 검사 전용이다. 원본 날씨의 색을 추정하는 알고리즘으로 사용하지 않는다.
    for index,name in enumerate(PALETTES):
        mask=1<<(index+3)
        colors=bytes(value^(mask if channel%3==index%3 else mask*2) for channel,value in enumerate(base[8:]))
        (destination/'d'/name).write_bytes(base[:8]+colors)
    return destination


def image_colors(image):
    """화면의 RGB 픽셀을 집합으로 만든다. Pillow의 버전별 getdata 경고에 의존하지 않는다."""
    raw=image.convert('RGB').tobytes()
    return set(zip(raw[0::3],raw[1::3],raw[2::3]))


def check(executable,root,name,setting,palettes,initial,resize=False):
    """실제 음악 순환을 네 번 호출하여 팔레트 로드 전 tint와 로드 후 tints/픽셀을 확인한다."""
    labels=['initial',*[f'cycle{i}' for i in range(4)]];steps=[]
    # 브리핑에서 게임 시계가 멈춰도 원소 음악·날씨 순환 경로는 살아 있어야 한다.
    for i,label in enumerate(labels):
        frame=3+i*2
        if i:steps.append((frame,'music-next',''))
        steps.extend(((frame,'report',label),(frame,'snapshot',str(OUTPUT/f'{name}-{label}.bmp'))))
    frames=13
    if resize:
        steps.extend(((13,'click','Play Mission'),(15,'report','world'),
                      (17,'click','Game'),(19,'click','Leave Battle'),(21,'click','Leave Honorably'),(23,'report','menu'),
                      (25,'click','Options'),(27,'click','Resolution >'),(29,'click','800 by 600'),(30,'report','800'),
                      (30,'snapshot',str(OUTPUT/f'{name}-800.bmp')),
                      (33,'click','Options'),(35,'click','Resolution >'),(37,'click','640 by 480'),(38,'report','640'),
                      (38,'snapshot',str(OUTPUT/f'{name}-640.bmp')),
                      (41,'click','Campaign'),(43,'click','Struggle For Freedom'),(45,'click','1 The War Begins!'),
                      (48,'report','reload'),(48,'snapshot',str(OUTPUT/f'{name}-reload.bmp'))))
        frames=50
    enabled=setting==1
    result=run(executable,root,name,frames,extra=['--mission','thewarbegins'],script=steps,
               settings=f'ascendancyPalette={setting}'+(';sound=0;music=0' if name.endswith('silent') else ''))
    assert result['workerFailed']=='0' and result['deviceReady']=='1',(name,'음악 스레드/장치 실패',result)
    if name.endswith('silent'):assert result['soundEnabled']=='0' and result['musicOption']=='0' and result['musicActive']=='0'
    observed=states(OUTPUT/f'{name}-states.tsv');before=initial
    previous_index=None;selected=initial
    # 각 보고와 같은 프레임에 저장한 RGB 화면을 검사 COL 256색과 직접 비교한다.
    for label in labels:
        state=observed[label];index,tint,dirty,*actual_tints=map(int,state['weather'])
        assert state['paused']==['1'], (name,label,'브리핑 정지 해제')
        if previous_index is not None:assert index==(previous_index+1)%4,(name,label,index,previous_index)
        selected=palettes[index] if enabled else initial
        assert actual_tints==tints(selected),(name,label,actual_tints,tints(selected))
        expected_tint=tints(selected)[index] if label=='initial' else (tints(before)[index] if enabled else observed['initial']['weather'][1])
        assert tint==int(expected_tint) and dirty==int(enabled),(name,label,state['weather'],expected_tint)
        with Image.open(OUTPUT/f'{name}-{label}.bmp') as image:
            pixels=image_colors(image)
            assert pixels<=set(selected) and len(pixels)>25,(name,label,'화면 팔레트 불일치',len(pixels-set(selected)))
        previous_index=index;before=selected
    if resize:
        assert observed['world']['paused']==['0'] and observed['menu']['phase']==['0']
        # 메뉴 곡으로 돌아가도 현재 날씨 팔레트를 유지한다. 새 Screen의 SetMode가 검정으로 덮으면 픽셀 검사에서 실패한다.
        for label,size in (('800',(800,600)),('640',(640,480))):
            assert observed[label]['size']==list(map(str,size))
            with Image.open(OUTPUT/f'{name}-{label}.bmp') as image:
                pixels=image_colors(image)
                assert image.size==size and pixels<=set(selected) and len(pixels)>25,(name,label,'해상도 변경 후 팔레트 손실')
        index,tint,dirty,*actual_tints=map(int,observed['reload']['weather'])
        assert actual_tints==tints(palettes[index]) and tint==actual_tints[index] and dirty==1
        with Image.open(OUTPUT/f'{name}-reload.bmp') as image:
            assert image_colors(image)<=set(palettes[index]),'미션 재진입 팔레트 불일치'
    return dict(states=len(observed),all_four_palettes=enabled,option=setting,
                sound_and_music_off=name.endswith('silent'),resize_and_reload=resize,
                worker_failed=result['workerFailed'],device_ready=result['deviceReady'])


def main():
    """원본 10.78의 검사 사본으로 설정 1/0/2 및 소리/음악 끔의 창 검사를 순서대로 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--root',type=Path,default=ROOT/'originals');args=parser.parse_args()
    source=args.root;before=snapshots(source);root=fixture_root(source);archive=TaffArchive(str(root/'netstorm.tarc'))
    palettes=[screen_palette(read_data(root,'d',archive,name)) for name in PALETTES]
    initial=screen_palette(read_data(root,'d',archive,'gifcloud.col'));report={}
    try:
        # 원본 설정은 값이 정확히 1일 때만 켜진다. 소리/음악을 꺼도 날씨 팔레트는 바뀐다.
        for name,setting,resize in (('weather-on',1,True),('weather-off',0,False),('weather-two',2,False),('weather-silent',1,False)):
            report[name]=check(args.exe,root,name,setting,palettes,initial,resize)
    finally:
        assert snapshots(source)==before,'원본 파일의 바이트/존재 상태 변경'
    report['original_files_unchanged']=len(before)
    report['synthetic_palette_fixture']=str(root)
    print(json.dumps(report,ensure_ascii=False,indent=2))


if __name__=='__main__':main()
