"""명시 진단 플래그의 실제 네 후단과 VFX를 게임 없이 메모리 장치로 관측한다."""
from pathlib import Path
import argparse
import collections
import csv
import hashlib
import importlib.util
import itertools
import json
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  # 같은 호스트의 보호 기계어만 읽는다.
OUT=ROOT/'extracted/cpp-renderer-diagnostics'  # 독립 fixture는 준비본 경로에만 새로 쓴다.
sys.path.insert(0,str(ROOT/'tools'))
import decomp_draw_water as water  # 같은 실제 finder/투영/VFX 관측기를 재사용한다.
from unicorn.x86_const import UC_X86_REG_ESP

ENTRIES=(0x434410,0x434380,0x434880,0x434690,0x434790,0x434a00,0x434bf0,0x4ad390,0x488a50,0x4a23b0,0x4ac1e0)  # 새 진단 계산과 실제 선 wrapper만 허용한다.
FLAGS=(0x54d91c,0x54d920,0x54d924,0x54d92c)  # 각 진단의 최초 관문이다.
DISTANCE,SPOTS=0x12800000,0x12820000  # 원문 표를 복제한 독립 명시 입력이다. BFS 계산을 합성하지 않는다.


class Oracle(water.Oracle):
    """기존768의 실제 finder/투영/외곽선/VFX와 화면 메모리 경계를 재사용한다."""
    def __init__(self):
        """새 몸체와 독립 spot/거리표의 읽기 공간만 추가한다."""
        super().__init__();self.mu.mem_map(DISTANCE,0x30000)
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in ENTRIES:  # 정밀 함수표의 실제 불연속 범위만 허용한다.
            for extent in facts[entry]['ranges'].split(';'):
                first,last=(int(value,16) for value in extent.split('-'));self.allowed[first-self.base:last+1-self.base]=b'\1'*(last-first+1)
        self.diagnostic=0;self.mode=0;self.spot=0;self.distance=0;self.active=0

    def on_write(self,mu,access,address,size,value,data):
        """Hash의 실제Renderer+60 외 게임 쓰기는 기존768 관문대로 거부한다."""
        if water.VIEW+0x60<=address and address+size<=water.VIEW+0x64: return
        super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """새 본문은 실행하며 선/진단 호출을 추가 기록한다."""
        if address==0x4a23b0:
            esp=mu.reg_read(UC_X86_REG_ESP);args=tuple(self.number(esp+4+i*4) for i in range(5));self.events.append('Line:'+':'.join(str(water.signed(value)) for value in args))
        if address==0x4e0620:
            self.events.append('Assert:'+str(self.number(mu.reg_read(UC_X86_REG_ESP)+12)));self.done();return
        super().on_instruction(mu,address,size,data)

    def call(self,name,*args,**kwargs):
        """부모의 장치 초기화 뒤 실제 대상 네 몸체로 진입점을 바꾼다."""
        if name=='DrawWater':
            for index,address in enumerate(FLAGS): self.number(address,self.active if index==self.diagnostic else 0)  # 최초 플래그는 case에서만 공급한다.
            self.number(water.VIEW+0x60,73);self.number(0x531928,256);self.number(0x595144,self.mode);self.number(0x59514c,DISTANCE if self.distance else 0)
            self.number(0x5c7c44,SPOTS);self.mu.mem_write(SPOTS,bytes([self.spot])*65536)
            self.mu.mem_write(DISTANCE,struct.pack('<h',self.distance)*65536)
            self.number(0x54d928,water.VIEW+0x100);self.number(0x531858,9);self.mu.mem_write(water.VIEW+0x100,struct.pack('<9I',10,11,12,13,14,15,16,17,18));self.number(0x540410,254)
            for address,color in ((0x5a4058,10),(0x5b5e58,11),(0x59afd8,15),(0x5bec2c,17),(0x5bec3c,18)): self.number(address,color)  # 실제 named색을 독립 팔레트 번호로 공급한다.
            for index,sid in enumerate((50,60,61,62,63,64)):  # 원래768의 일반 합성TYPE/SID만 읽고 종류별 생성/행동은 실행하지 않는다.
                self.number(0x11100000+(100+index)*self.type_stride+0xe8,0x800 if index%2==0 else 0)
                self.mu.mem_write(self.slot(sid)+30,bytes([(2,0,254,1,0,3)[index]]));self.mu.mem_write(self.slot(sid)+12,bytes([index&1]))
            self.spec['entries']['DrawWater']=(0x434410,0x434880,0x434a00,0x434bf0)[self.diagnostic]
        return super().call(name,*args,**kwargs)

    def sample(self,case,precision):
        """네 진단/현재 모드/spot/거리값과 Renderer/snapshot 차이를 명시한다."""
        self.diagnostic,self.active,self.mode,self.spot,self.distance,zoom=case
        result=super().run((self.active,16 if not zoom else 32,11 if not zoom else 22,19.25,19.5,304,209,0,0),precision)
        return result+'\t'+str(self.number(water.VIEW+0x60))


def main():
    """첫 독립 관측만 고정하며 기존 제품/fixture와 OS 화면은 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();destination=args.output.resolve();destination.mkdir(parents=True,exist_ok=False)
    fixture=destination/'modified/cpppj/tests/fixtures/renderer-diagnostics-x86.tsv';report=destination/'native.json'
    if report.exists() or fixture.exists(): raise FileExistsError('최초 진단 fixture 덮어쓰기 거부')
    cases=list(itertools.product(range(4),(0,1,0xffffffff),(0,1),(0,6,8,64,128,192),(0,7),(0,1)))
    # Fire 이외 몸체는 mode/거리표를 읽지 않으므로 불필요한 중복은 제거한다.
    cases=[row for row in cases if row[0]==1 or (row[2]==0 and row[4]==0)]
    oracle=Oracle();lines=['# inputJson\tchangedPixels\tfinalClip\twaterFlag\twaterColor\tevents\trendererBackground']
    for case in cases:  # 기대 픽셀과 호출 순서는 실제 원문에서만 읽는다.
        first=oracle.sample(case,0x27f);assert first==oracle.sample(case,0x37f),case;lines.append(json.dumps(case,separators=(',',':'))+'\t'+first)
    payload='\n'.join(lines)+'\n'
    fixture.parent.mkdir(parents=True,exist_ok=True);fixture.write_text(payload,'utf-8',newline='\n')
    record=dict(unit=776,host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',inputs=len(cases),observations=len(cases)*2,actualBodies=[f'{entry:08x}' for entry in ENTRIES],calls=dict(oracle.calls),fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),gameExecutions=0,multiplayerTests=0,builds=0,cppTests=0,boundary='실제네후단/finder/투영/rect/line/VFX; Screen Lock/Unlock/Present와assert는메모리/기록경계. 거리표는명시입력이며BFS를생성하지않았다.')
    report.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8');print(json.dumps(dict(inputs=len(cases),observations=len(cases)*2)))


if __name__=='__main__':
    main()
