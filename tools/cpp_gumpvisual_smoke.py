#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클론 돌 버튼의 외곽선/글자를 실제 원본 버튼 몸체와 독립 글꼴 해독으로 대조한다. 원본 게임 실행 없음."""
import argparse
import json
import shutil
from pathlib import Path
from PIL import Image
from cpp_menu_smoke import reports, run
from cpp_renderer_smoke import read_font, snapshots
from cpp_smoke_files import preserve_game_settings
from cpp_weather_smoke import fixture_root
from cpp_window_smoke import read_data, screen_palette
from decomp_gumpvisual_oracle import GumpOracle, CONTROLS
from taff import TaffArchive

# 검사 산출물과 기본 글꼴 이름이다. 자료 사본/캡처만 extracted에 만들고 원본은 읽기만 한다.
ROOT=Path(__file__).resolve().parent.parent
OUTPUT=ROOT/'extracted/cpp-gumpvisual-smoke'
FONT='!Arial.normal.14.700.chfnt'


def nearest(palette,rgb):
    """byte RGB 거리의 첫 최소 번호를 독립적으로 구한다. C++ 색 표/색 검색 결과를 기대값으로 읽지 않는다."""
    return min(range(256),key=lambda i:sum((palette[i][c]-rgb[c])**2 for c in range(3)))


def glyph_pixels(font,label,x,y,color):
    """원본 글리프의 투명/100번 잉크와 B+C 전진 폭으로 위치별 색 번호를 만든다."""
    result={}
    # 코드 페이지 글자를 독립 SHP 해독 결과로 출력한다.
    for code in label.encode('cp1252'):
        glyph=font['glyphs'][code]
        if glyph['frame']:
            left,top,_,_=glyph['frame'].rect
            # 불투명 픽셀만 합성한다. 색 표는 100번 잉크 이외를 검정 번호 0으로 매핑한다.
            for row,values in enumerate(glyph['rows']):
                # 각 열의 투명 여부와 잉크 번호를 원본 글리프에서 읽는다.
                for column,value in enumerate(values):
                    if value is not None:result[(x+left+column,y+top+row)]=color if value==100 else 0
        x+=glyph['draw_advance']
    return result


def expected_pixels(oracle,font,controls,pressed,colors):
    """실제 버튼 몸체의 선/위치를 관찰하고 독립 글리프로 덮는다. 질감/안쪽 명암은 완료 기준에서 제외한다."""
    border={};labels={};plans=0
    # 목록 행을 제외한 각 돌 버튼의 입력을 실제 10.78 몸체에 공급한다.
    for label,control in controls.items():
        enabled,menu,left,top,right,bottom,_=control
        if menu:continue
        width=sum(font['glyphs'][c]['advance'] for c in label.encode('cp1252'))
        case=(left,top,right,bottom-1,width,font['height'],0,int(label==pressed and enabled),int(not enabled),*colors)
        context_hex,x,y,selected,lines=oracle.run(case,CONTROLS[0]);plans+=1
        context=bytes.fromhex(context_hex)[selected*32:(selected+1)*32]
        flags=int.from_bytes(context[:4],'little');ink=int.from_bytes(context[8:12],'little')
        # 선 끝점을 포함한다. 원본이 호출한 순서와 DWORD 색을 그대로 사용한다.
        for line in (() if lines=='-' else lines.split('|')):
            x1,y1,x2,y2,color=map(int,line.split(','))
            # 수평/수직 선의 양 끝을 포함한 행 범위를 채운다.
            for py in range(min(y1,y2),max(y1,y2)+1):
                # 한 행 안의 실제 선 픽셀에 원본 출력 색을 기록한다.
                for px in range(min(x1,x2),max(x1,x2)+1):border[(px,py)]=color&255
        if flags&2:
            shadow=int.from_bytes(context[20:24],'little')
            sx=int.from_bytes(context[24:28],'little',signed=True);sy=int.from_bytes(context[28:32],'little',signed=True)
            labels.update(glyph_pixels(font,label,left+x+sx,top+y+sy,shadow&255))
        labels.update(glyph_pixels(font,label,left+x,top+y,ink&255))
    border.update(labels)
    return border,plans,len(labels)


def check(executable,root,name,disabled=False):
    """실제 마우스 누름/취소/대화상자/비활성 누름을 실행하고 같은 프레임의 픽셀을 대조한다."""
    archive=TaffArchive(root/'netstorm.tarc');palette=screen_palette(read_data(root,'d',archive,'gifcloud.col'))
    colors=(nearest(palette,(0,0,0)),nearest(palette,(255,255,255)),nearest(palette,(55,51,54)))
    font=read_font(read_data(root,'d',archive,FONT));oracle=GumpOracle('originals')
    stages=[(1,'normal',''),(2,'pressed','Campaign'),(3,'cancelled',''),(5,'dialog','')]
    steps=[(2,'down','Campaign'),(3,'outside',''),(4,'click','Multiplayer')]
    if disabled:steps.append((6,'down','Disabled'));stages.append((7,'disabled_ignored',''))
    # 프레임의 입력 처리가 끝난 다음 상태/화면을 함께 기록한다.
    for frame,label,_ in stages:steps.extend(((frame,'report',label),(frame,'snapshot',str(OUTPUT/f'{name}-{label}.bmp'))))
    script=OUTPUT/f'{name}-steps.tsv';report=OUTPUT/f'{name}-states.tsv'
    script.write_text(''.join(f'{f}\t{op}\t{arg}\n' for f,op,arg in sorted(steps,key=lambda item:item[0])),encoding='utf-8',newline='\n')
    with preserve_game_settings(root,'d'):
        run(executable,['--run',root,'--window','--frames','9','--set',
            'SCREENW=1024;SCREENH=768;windowScreenFlags=10;fontFaceName=Arial;autoDemo=0;tellTips=0;ascendancyPalette=0',
            '--ui-script',script,'--ui-report',report])
    observed=reports(report);pixels=plans=glyphs=0
    # 네 상태의 캡처를 원본 호출 결과/독립 글꼴과 대조한다.
    for _,label,pressed in stages:
        state=observed[label];expected,count,text_count=expected_pixels(oracle,font,state['controls'],pressed,colors)
        if disabled and label in ('dialog','disabled_ignored'):assert state['controls']['Disabled'][0]==0
        with Image.open(OUTPUT/f'{name}-{label}.bmp') as source:
            image=source.convert('RGB');actual=image.load()
            # 화면 안의 선/글자 픽셀만 RGB로 대조하며 질감은 검사 대상에 포함하지 않는다.
            for (x,y),index in expected.items():
                if 0<=x<image.width and 0<=y<image.height:
                    assert actual[x,y]==palette[index],(name,label,(x,y),actual[x,y],palette[index]);pixels+=1
        plans+=count;glyphs+=text_count
    if disabled:
        with Image.open(OUTPUT/f'{name}-dialog.bmp') as a,Image.open(OUTPUT/f'{name}-disabled_ignored.bmp') as b:
            assert a.tobytes()==b.tobytes(),'비활성 버튼 누름이 그림/배치를 바꿈'
    return dict(states=len(stages),native_button_plans=plans,compared_pixels=pixels,text_pixels=glyphs,edge_index=colors[2],disabled_ignored=disabled)


def prepare_probes(root,index):
    """검사 사본에만 구별 가능한 외곽선 색 번호와 비활성 돌 버튼을 만든다. 원본 UI의 기능으로 주장하지 않는다."""
    archive=TaffArchive(root/'netstorm.tarc');col=bytearray(read_data(root,'d',archive,'gifcloud.col'))
    # 목표 색의 중복을 제거해 실제 번호가 검사마다 달라지게 한다.
    for i in range(1,255):
        if col[8+i*3:11+i*3]==bytes((55,51,54)):col[8+i*3:11+i*3]=bytes((180,180,180))
    col[8+index*3:11+index*3]=bytes((55,51,54));col[8+91*3:11+91*3]=bytes((49,44,36))
    (root/'d/gifcloud.col').write_bytes(col)
    text=read_data(root,'d',archive,'tell.english').decode('cp1252');start=text.index('[NotImplemented]');end=text.index('\n[',start+1)
    text=text[:end]+'\n$Button=Disabled,Unsupported,0\n'+text[end:]
    (root/'d/tell.english').write_text(text,encoding='utf-8',newline='\n')


def main():
    """보존 원본과 검사 사본 두 팔레트를 순차 실행한다. 성공/실패 모두 설정을 복구하고 원본 해시를 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe');args=parser.parse_args()
    OUTPUT.mkdir(parents=True,exist_ok=True);source=ROOT/'originals';before=snapshots(source)
    result={'originals':check(args.exe,source,'originals')}
    # 검사 사본은 팔레트마다 새로 만들어 서로의 설정/스크립트가 섞이지 않게 한다.
    for index in (37,73):
        fixture=fixture_root(source)
        try:
            prepare_probes(fixture,index);result[f'probe{index}']=check(args.exe,fixture,f'probe{index}',True)
            assert result[f'probe{index}']['edge_index']==index
        finally:
            if fixture.resolve().parent!=(ROOT/'extracted').resolve():raise RuntimeError('검사 사본 정리 경로 오류')
            shutil.rmtree(fixture)
    assert snapshots(source)==before,'원본 파일/설정/존재 상태 변경'
    result['original_files_unchanged']=len(before)
    (OUTPUT/'report.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(result,ensure_ascii=False,indent=2))


if __name__=='__main__':main()
