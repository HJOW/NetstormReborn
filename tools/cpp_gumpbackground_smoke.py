#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 돌 버튼의 질감·명암·외곽선·글자를 독립 SHP/글꼴 해독과 실제 원본 몸체 출력으로 대조한다."""
import argparse
import json
import shutil
import struct
from pathlib import Path
from PIL import Image
from cpp_gumpvisual_smoke import FONT, nearest, expected_pixels, prepare_probes
from cpp_menu_smoke import reports, run
from cpp_renderer_smoke import read_font, snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_weather_smoke import fixture_root
from cpp_window_smoke import read_data, screen_palette
from decomp_gumpbackground_oracle import BackgroundOracle
from decomp_gumpvisual_oracle import GumpOracle, CONTROLS
from shp import TYPE_LOAD_ORDER, read_blocks, read_clusters, decode_frame
from taff import TaffArchive

# 원본은 읽기만 하고 검사 사본·캡처·보고서는 extracted에 만든다.
ROOT=Path(__file__).resolve().parent.parent
OUTPUT=ROOT/'extracted/cpp-gumpbackground-smoke'


def texture_source(root,archive):
    """원본 fortGump A00의 SHP 메타 폭/높이·기준점·불투명 픽셀을 독립 Python 판독기로 읽는다."""
    data=read_data(root,'d',archive,'_shapes.shp');block=read_blocks(data)[TYPE_LOAD_ORDER.index('fortGump')]
    clusters=read_clusters(read_data(root,'d',archive,'fortGump.type').decode('cp1252'))
    frame=block.frames[next(i for i,c in enumerate(clusters) if c[0]=='A00')]
    width,height=struct.unpack_from('<hh',data,frame.offset-12)
    return width,height,frame.rect,decode_frame(data,frame)


def background_pixels(oracle,controls,pressed,size,texture,maps):
    """실제 배경/반복 몸체가 호출한 clip/map/draw만 독립 SHP 픽셀로 합성한다. C++ 결과는 읽지 않는다."""
    result={};plans=0;width,height,rect,rows=texture
    # 목록 행을 제외하고 모든 돌 버튼에 같은 원본 배경 자식 계약을 적용한다.
    for label,control in controls.items():
        enabled,menu,left,top,right,bottom,_=control
        if menu:continue
        flags=0x3e00000|(0x40000 if enabled and label==pressed else 0)
        events=oracle.draw((left+1,top+1,right-1,bottom-2,0,0,*size,flags,width,height),CONTROLS[0]);plans+=1
        clip=(0,0,*size);shade=0
        # 원본 VFX 호출 순서를 유지하여 모서리의 마지막 테두리 색도 확인한다.
        for event in events.split('|'):
            fields=event.split(',');kind=fields[0];args=list(map(int,fields[1:]))
            if kind=='c':clip=tuple(args)
            elif kind=='m':shade=args[0]
            elif kind=='d':
                x,y,shade=args;map_=maps[shade-1] if shade else None
                # 실제 클립에 들어오는 불투명 SHP 행만 합성한다.
                for row,values in enumerate(rows):
                    py=y+rect[1]+row
                    if not max(0,clip[1])<=py<min(size[1],clip[3]):continue
                    # 화면 및 clip의 right/bottom은 포함하지 않는다.
                    for column,index in enumerate(values):
                        px=x+rect[0]+column
                        if index is not None and max(0,clip[0])<=px<min(size[0],clip[2]):result[px,py]=map_[index] if map_ else index
    return result,plans


def check(executable,root,name,sizes,disabled=False):
    """세 화면 크기·실제 눌림/취소·대화상자·비활성 버튼을 필요 조합으로 검사한다."""
    archive=TaffArchive(root/'netstorm.tarc');palette=screen_palette(read_data(root,'d',archive,'gifcloud.col'))
    logical=[r|(g<<8)|(b<<16) for r,g,b in palette]
    background=BackgroundOracle('originals');maps=tuple(bytes.fromhex(v) for v in background.maps(logical,CONTROLS[0]))
    font=read_font(read_data(root,'d',archive,FONT));parent=GumpOracle('originals');texture=texture_source(root,archive)
    colors=(nearest(palette,(0,0,0)),nearest(palette,(255,255,255)),nearest(palette,(55,51,54)))
    result=dict(states=0,native_background_plans=0,native_parent_plans=0,compared_pixels=0,background_pixels=0,edge_index=colors[2],texture_metrics=texture[:2])
    # 해상도마다 새 프로세스를 사용하여 화면 모드 적용 뒤 표/격자도 검사한다.
    for size in sizes:
        prefix=f'{name}-{size[0]}';stages=[(1,'normal',''),(2,'pressed','Campaign'),(3,'cancelled',''),(5,'dialog','')]
        steps=[(2,'down','Campaign'),(3,'outside',''),(4,'click','Multiplayer')]
        if disabled:steps.append((6,'down','Disabled'));stages.append((7,'disabled_ignored',''))
        # 같은 프레임의 상태와 실제 8비트 DIB 캡처를 함께 보관한다.
        for frame,label,_ in stages:steps.extend(((frame,'report',label),(frame,'snapshot',str(OUTPUT/f'{prefix}-{label}.bmp'))))
        script=OUTPUT/f'{prefix}-steps.tsv';report=OUTPUT/f'{prefix}-states.tsv'
        script.write_text(''.join(f'{f}\t{op}\t{arg}\n' for f,op,arg in sorted(steps,key=lambda v:v[0])),encoding='utf-8',newline='\n')
        with preserve_game_settings(root,'d'):
            run(executable,['--run',root,'--window','--frames','9','--set',
                f'SCREENW={size[0]};SCREENH={size[1]};windowScreenFlags=10;fontFaceName=Arial;autoDemo=0;tellTips=0;ascendancyPalette=0',
                '--ui-script',script,'--ui-report',report])
        observed=reports(report)
        # 독립 질감 합성 위에 기존 전체 부모 함수의 선/글자를 덮고 모든 해당 픽셀을 비교한다.
        for _,label,pressed in stages:
            state=observed[label];pixels,count=background_pixels(background,state['controls'],pressed,size,texture,maps)
            result['background_pixels']+=len(pixels);result['native_background_plans']+=count
            foreground,count,_=expected_pixels(parent,font,state['controls'],pressed,colors);pixels.update(foreground);result['native_parent_plans']+=count
            with Image.open(OUTPUT/f'{prefix}-{label}.bmp') as source:
                image=source.convert('RGB');actual=image.load();assert image.size==size
                # 투명 유지/자식이 덮지 않는 부모 모서리는 이 대조 영역에 포함하지 않는다.
                for (x,y),index in pixels.items():
                    if 0<=x<size[0] and 0<=y<size[1]:
                        assert actual[x,y]==palette[index],(prefix,label,(x,y),actual[x,y],palette[index]);result['compared_pixels']+=1
            result['states']+=1
        if disabled:
            assert observed['disabled_ignored']['controls']['Disabled'][0]==0
            with Image.open(OUTPUT/f'{prefix}-dialog.bmp') as a,Image.open(OUTPUT/f'{prefix}-disabled_ignored.bmp') as b:
                assert a.tobytes()==b.tobytes(),'비활성 버튼 눌림이 화면을 변경함'
        print(f'{prefix}: 돌 배경/부모/글자 대조 통과',flush=True)
    return result


def main():
    """원본 및 구별 가능한 두 사본을 순차 실행하고 설정/원본 해시를 항상 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe');args=parser.parse_args()
    OUTPUT.mkdir(parents=True,exist_ok=True);source=ROOT/'originals';before=snapshots(source)
    result={'originals':check(args.exe,source,'originals',[(1024,768),(800,600)])}
    # RGB와 외곽선 번호를 바꾸는 것은 검사 사본에만 한정한다.
    for index,size in ((37,(1024,768)),(73,(640,480))):
        fixture=fixture_root(source)
        try:
            prepare_probes(fixture,index);result[f'probe{index}']=check(args.exe,fixture,f'probe{index}',[size],True)
            assert result[f'probe{index}']['edge_index']==index
        finally:
            if fixture.resolve().parent!=(ROOT/'extracted').resolve():raise RuntimeError('검사 사본 정리 경로 오류')
            shutil.rmtree(fixture)
    assert snapshots(source)==before,'원본/설정 바이트 또는 존재 상태 변경'
    result['original_files_unchanged']=len(before)
    (OUTPUT/'report.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(result,ensure_ascii=False,indent=2))


if __name__=='__main__':main()
