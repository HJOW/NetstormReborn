#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 제네레이터의 실제 가상 표와 원본 일반 Draw의 기준점을 격리 관찰한다."""
import argparse
import collections
import csv
import json
import re
import socket
import struct
from decomp_spritedraw_oracle import SpriteDrawOracle, TYPES, VTABLE, DATA, SHAPES, RAW, ENTRY, SPECS, plan
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle, ROOT, CONTROLS, digest, fbits, STACK, STOP,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)
from shp import load_type_texts

REPORT=ROOT/'cpppj/generator-draw-native.json'  # 현재 PC의 독립 원본 관측 기록이다.


class GeneratorOracle(SpriteDrawOracle):
    """가상 frame/Draw를 대체하지 않고 실제4acfe0/41c100까지 실행한다."""
    def __init__(self):
        """기존 일반 Draw의 출력 경계와 검토한 짧은 실제 함수 범위를 연결한다."""
        ProcessOracle.__init__(self,'originals')
        self.d=SPECS['originals']
        self.extra=[ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv']
        wanted=set(json.loads((ROOT/'tools/ghidra/spritedraw-functions.json').read_text('utf-8'))['editions']['originals'])
        self.allowed=[]
        self.entry_names={}
        self.returns=collections.Counter()
        self.boundaries=collections.Counter()
        # 마지막 전체 디컴파일의 실제 함수 범위에서 일반 Draw 관련 몸체만 선택한다.
        with self.extra[1].open(encoding='utf-8') as stream:
            # 짧은 두 가상 함수는 별도로 검토한 바이트 범위를 아래에서 추가한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if f"{int(row['entry'],16):08x}" in wanted:
                    self.entry_names[int(row['entry'],16)]=f"{int(row['entry'],16):08x}"
                    # 불연속 몸체를 주소 사이의 코드까지 넓히지 않는다.
                    for part in row['ranges'].split(';'):
                        begin,end=(int(value,16) for value in part.split('-'))
                        self.allowed.append((begin,end+1))
        assert set(self.entry_names.values())==wanted
        self.mu.mem_map(DATA,0x90000)
        self.mu.mem_map(SHAPES,0x100000)
        self.allowed.extend(((0x4acfe0,0x4acff4),(0x41c100,0x41c103)))
        self.virtuals={offset:self.u32(0x501ea0+offset) for offset in (0x78,0xa4)}
        self.virtual_hits=collections.Counter()  # 실제 가상 함수 진입 횟수를 출력 경계와 별도로 센다.

    def on_instruction(self,mu,address,size,data):
        """실제 두 가상 몸체 진입을 세고 기존 허용 범위/출력 경계를 적용한다."""
        if address in self.virtuals.values():
            self.virtual_hits[hex(address)]+=1
        super().on_instruction(mu,address,size,data)

    def observe(self,case,control):
        """두 x87에서 읽기 전용 입력과 ABI 정상 반환을 검사한다."""
        mu,d=self.mu,self.d
        self.case=case
        self.events=[]
        self.entries=collections.Counter()
        self.calls=collections.Counter()
        mu.mem_write(DATA,bytes(0x90000))
        mu.mem_write(SHAPES,bytes(0x100000))
        self.put(d['table'],TYPES)
        self.put(d['pool'],RAW-5*50)
        self.put(ENTRY+8,RAW)
        self.put(ENTRY+15,case['tail'],1)
        self.put(RAW,VTABLE)
        self.put(RAW+10,70,1)
        self.put(RAW+40,2,1)
        self.put(RAW+34,1,1)
        # 순수 입력인 월드/화면 쌍을 준비하고 출력 기준점은 원본에서만 관찰한다.
        for world_offset,screen_offset,key,scale in ((14,22,'world_x',16),(18,24,'world_y',11)):
            self.put(RAW+world_offset,case[key])
            world=struct.unpack('<f',struct.pack('<I',case[key]))[0]
            self.put(RAW+screen_offset,int(world*scale+.5),2)
        # 실제 가상 함수 주소를 첫 명령 전에 설치한다. 실행 중 입력 변경은 허용하지 않는다.
        for offset,target in self.virtuals.items():
            self.put(VTABLE+offset,target)
        typ=TYPES+70*500
        self.put(typ+0xdc,SHAPES+70*0x1000)
        self.put(typ+0x114,2)
        self.put(typ+0x128,0)
        self.put(typ+0x1dc,case['hot_x'])
        self.put(typ+0x1e0,case['hot_y'])
        # 선택/표식/소유자 색 교체를 요청하지 않는 실제 일반 본체 입력이다.
        for address,value in ((d['camera'][0],case['camera_x']),(d['camera'][1],case['camera_y']),
            (d['mode'],0),(d['wide'],90),(d['unfiltered'],90),(d['marker'][0],0),(d['pinned'],1),
            (d['alias'],90),(d['player'],1),(d['selected'],0),(d['owner_maps']+70*4,0)):
            self.put(address,value)
        before=bytes(mu.mem_read(DATA,0x90000))
        headers=bytes(mu.mem_read(SHAPES,0x100000))
        self.writes=[(STACK-0xf000,STACK+0x1000)]
        # 보존 레지스터와 실제 thiscall 스택을 구성한다.
        for register,value in REGISTERS.items():
            mu.reg_write(register,value)
        self.put(STACK,STOP)
        mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.reg_write(UC_X86_REG_ECX,ENTRY)
        mu.reg_write(UC_X86_REG_EFLAGS,2)
        mu.reg_write(UC_X86_REG_FPCW,control)
        mu.reg_write(UC_X86_REG_FPSW,0)
        mu.emu_start(d['root'],STOP,timeout=30000000,count=2000000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert all(mu.reg_read(register)==value for register,value in REGISTERS.items())
        assert mu.reg_read(UC_X86_REG_FPCW)==control and not mu.reg_read(UC_X86_REG_FPSW)&0x3800
        assert bytes(mu.mem_read(DATA,0x90000))==before and bytes(mu.mem_read(SHAPES,0x100000))==headers
        self.returns[f'{d["root"]:08x}']+=1
        return list(self.events)


def main():
    """원본 TYPE 입력과 실제 출력 명령을 기록하거나 기록된 출처를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if options.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 보존한 관측과 현재 출처를 분리해서 확인한다.
        for name,value in report['files'].items():
            assert digest(ROOT/name)==value,name
        assert report['inputs']==24 and report['normalReturns']==48
        assert report['virtuals']=={'0x78':'0x4acfe0','0xa4':'0x41c100'}
        assert report['virtualHits']=={'0x4acfe0':24,'0x41c100':24}
        assert len(report['observations'])==24 and all(row['hotspot']==[8,4] for row in report['observations'])
        print('제네레이터 실제 가상 함수와 기준점 24입력/48정상 반환 출처 통과')
        return
    assert not REPORT.exists()
    oracle=GeneratorOracle()
    texts=load_type_texts(str(ROOT/'originals'))
    observations=[]
    # 세 원본 자산의 서로 다른 이름과 같은 기준점을 읽는다.
    for name in ('windbattery','rainbattery','thunderbattery'):
        text=texts[name]
        ratios=[float(re.search(r'hotFootRatio'+axis+r'\s*=\s*([\d.]+)',text).group(1)) for axis in ('X','Y')]
        hot=[int(ratios[0]*16),int(ratios[1]*11)]
        # 실전의 두 다리 끝 좌표와 두 카메라를 명시 입력으로 공급한다.
        for x,y in ((131,99),(137,100)):
            # 화면 밖 좌표도 root의 출력 인수 그대로 기록한다.
            for camera_x,camera_y in ((0,0),(1534,845)):
                # 본체는 실제 baseFrame, 그림자는 현재 프레임의 다음 레이어를 사용한다.
                for tail in (0,4):
                    case=plan()[0]
                    case.update(tail=tail,extra=2,frame=0,world_x=fbits(x),world_y=fbits(y),camera_x=camera_x,camera_y=camera_y,
                        hot_x=hot[0],hot_y=hot[1],frame_count=2,owner_map=0,marker_enabled=0)
                    values=[oracle.observe(case,control) for control in CONTROLS]
                    assert values[0]==values[1]
                    observations.append(dict(asset=name,world=[x,y],camera=[camera_x,camera_y],hotspot=hot,tail=tail,events=values[0]))
    files={ROOT/name for name in ('tools/decomp_generator_draw_probe.py','tools/decomp_spritedraw_oracle.py','tools/decomp_process_oracle.py',
        'tools/decomp_drawsort_oracle.py','tools/ghidra/spritedraw-functions.json','tools/shp.py','tools/taff.py','originals/Netstorm.exe','originals/netstorm.tarc')}
    files.update(oracle.exports+oracle.extra)
    record=dict(host=socket.gethostname(),lastDecompile='VM-W11-Codex / 2026-10-19',inputs=len(observations),normalReturns=sum(oracle.returns.values()),
        virtuals={hex(key):hex(value) for key,value in oracle.virtuals.items()},virtualHits=dict(oracle.virtual_hits),observations=observations,
        limits='타입70의 격리 슬롯에 실제 세 TYPE 기준점과 baseFrame을 공급한다. 일반 Draw와 실제 가상 함수까지 실행하며 VFX 출력만 관측한다. 건설/GUI 완주 관측이 아니다.',
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8')
    print('inputs',record['inputs'],'normalReturns',record['normalReturns'])


if __name__=='__main__':
    main()
