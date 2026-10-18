#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 클론 템플 화면을 동결한 원본 호출 순서와 독립 SHP 픽셀 합성으로 대조한다."""
import argparse,json,re,struct,subprocess
from pathlib import Path
from PIL import Image
from cpp_smoke_files import preserve_game_settings
from cpp_renderer_smoke import snapshots
from cpp_menu_smoke import source
from cpp_world_smoke import states
from cpp_window_smoke import screen_palette
from shp import TYPE_LOAD_ORDER,read_blocks,decode_frame
from taff import TaffArchive

ROOT=Path(__file__).resolve().parents[1] # 현재 보호 원본과 클론의 공통 기준이다.
EXE=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe' # 실제 제품 실행 파일이다.


def compare(directory,name):
    """제품의 프레임 순서 계산을 사용하지 않고 해당 원본 관측 행을 읽는다."""
    archive=TaffArchive(ROOT/'originals/netstorm.tarc');data=source(ROOT/'originals',archive,'d/_shapes.shp');block=read_blocks(data)[TYPE_LOAD_ORDER.index(name)];palette=screen_palette(source(ROOT/'originals',archive,'d/gifcloud.col'))
    native={}
    # 제품 전에 동결한 같은 프레임/WORD 상태의 원본 반환과 제출 순서다.
    for line in (ROOT/'cpppj/tests/fixtures/temple-draw-x86.tsv').read_text('utf-8').splitlines():
        if not line or line.startswith('#'):continue
        kind,frame,word,base,trace,raw=line.split('\t');native[kind,int(frame),int(word)]=(int(base),trace)
    text=source(ROOT/'originals',archive,'d/'+name+'.type').decode('cp1252')
    # 실제 로더의 binary32 속성×16/11→0방향 절삭과 일반SpriteDraw의 TYPE 기준점 차감이다.
    def offset(axis,factor):
        """보호 .type 입력에서 원본 타입 기준점만 계산한다. 레인(.5,.8)은 실제 로더에서도(8,8)이다."""
        value=re.search('hotFootRatio'+axis+r'\s*=\s*([-+.0-9]+)',text)
        ratio=struct.unpack('<f',struct.pack('<f',float(value[1]) if value else 0))[0]
        return int(ratio*factor)
    hot_x,hot_y=offset('X',16),offset('Y',11)
    records=[]
    # 같은 루프에서 찍은 실제 raw 보고와BMP만 대조한다.
    for label,state in states(directory/'states.tsv').items():
        sid,temple=next((sid,row) for sid,row in state['objects'].items() if row[0]==name and row[1]=='1');frame=int(temple[7]);word=0 if int(temple[11])==0 else 1;base,trace=native[name,frame,word]
        camera_x,camera_y=map(int,state['camera']);x=round(float(temple[5])*16)-camera_x;y=round(float(temple[6])*11)-camera_y;x-=hot_x;y-=hot_y
        layers=([base] if base!=0xffffffff else [])+[int(item[1:]) for item in trace.split(',') if item.startswith('F')];expected={};base_pixels=set();other_pixels=set()
        # 기존 Python 압축 해제와 불투명 덮기로 원본 순서의 각 픽셀을 만든다.
        for number in layers:
            shape=block.frames[number]
            if shape.special:continue # 원본 픽셀 소비자처럼 이미지 아닌 특수 레코드는 출력하지 않는다.
            for dy,row in enumerate(decode_frame(data,shape)):
                for dx,color in enumerate(row):
                    point=x+shape.rect[0]+dx,y+shape.rect[1]+dy
                    if color is not None and 0<=point[0]<1024 and 28<=point[1]<768:
                        expected[point]=palette[color];(base_pixels if number==base else other_pixels).add(point)
        actual=Image.open(directory/(label+'.bmp')).convert('RGB');assert actual.size==(1024,768)
        differences=[(point,rgb,actual.getpixel(point)) for point,rgb in expected.items() if rgb!=actual.getpixel(point)]
        records.append(dict(label=label,sid=sid,frame=frame,word=word,layers=layers,comparedPixels=len(expected),baseOnlyPixels=len(base_pixels-other_pixels),mismatches=len(differences),firstMismatches=differences[:8]))
    return records


def main():
    """보호 설정 복구와 전체 원본 스냅샷 비교를 포함한 실제 창 검사를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--name',choices=('windVortex','rainVortex'),default='windVortex');parser.add_argument('--compare-only',action='store_true');parser.add_argument('--expect-pass',action='store_true');args=parser.parse_args();directory=args.output.resolve();directory.mkdir(parents=True,exist_ok=True);before=snapshots(ROOT/'originals')
    if not args.compare_only:
        rows=[(5,'click','MORE'),(7,'click','OK')] if args.name=='windVortex' else [(5,'click','Play Mission')]
        # 네 애니메이션 시점의 보고와 화면은 같은 이벤트 루프에서 기록한다.
        for frame in (12,22,32,42):rows.extend(((frame,'report',f'temple{frame}'),(frame,'snapshot',str(directory/f'temple{frame}.bmp'))))
        script=directory/'steps.tsv';script.write_text(''.join(f'{frame}\t{op}\t{value}\n' for frame,op,value in rows),'utf-8',newline='\n');command=[str(EXE),'--run',str(ROOT/'originals'),'--mission','tutorial5' if args.name=='windVortex' else 'thewarbegins','--window','--no-audio','--frames','45','--set','SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0','--ui-script',str(script),'--ui-report',str(directory/'states.tsv')]
        startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
        with preserve_game_settings(ROOT/'originals','d'):result=subprocess.run(command,cwd=ROOT,capture_output=True,startupinfo=startup,timeout=80)
        (directory/'stdout.log').write_bytes(result.stdout);(directory/'stderr.log').write_bytes(result.stderr);assert result.returncode==0,result.stderr.decode('utf-8',errors='replace')
    unchanged=snapshots(ROOT/'originals')==before;records=compare(directory,args.name);passed=unchanged and len(records)==4 and len({row['frame'] for row in records})>1 and all(row['mismatches']==0 for row in records)
    value=dict(passed=passed,originalsUnchanged=unchanged,name=args.name,scope='실제 창의 일반 살아있는 템플 본체 픽셀. 그림자/화면전체/상승음/실전 완주 검증은 별도다.',captures=records);(directory/'pixels.json').write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(value,ensure_ascii=False));assert unchanged
    if args.expect_pass:assert passed,'원본 템플 합성과 실제 창 픽셀 불일치'


if __name__=='__main__':main()
