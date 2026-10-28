#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다리 수명5의 실제 한 칸 탐색/현재 이동 Form/사제·rainWalker 대피를 원문에서 기록한다."""
import csv
import itertools
import json
import socket
import struct
from decomp_rawfinder_oracle import FinderOracle, ROOT, POOL, MAP, FPU_CONTROLS
from decomp_bridgedecay_oracle import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ECX
from decomp_bridgeeffects_oracle import bits

# 실제 호출 몸체와 이번에 대체할 큰 이동 장치의 경계를 분리한다.
ENTRIES=(0x421c30,0x4214a0,0x4202f0,0x427de0,0x426120,0x427e00,0x427da0,
         0x422230,0x41d7a0,0x41d770,0x41cfc0,0x422510,0x40e800,
         0x462c80,0x462bc0,0x462b80,0x4ad390,0x40eaf0,0x4ac1e0,0x4ac200,0x4ac550)
WORK=ROOT/'extracted/cpp-bridge-runtime-552-20261029'
FIXTURE=ROOT/'cpppj/tests/fixtures/bridge-carrier-retreat-x86.tsv'
REPORT=ROOT/'cpppj/bridge-carrier-retreat.json'


class RetreatOracle(FinderOracle):
    """일반 finder·Graph·수명·거리·선택은 원문이며 이동 프로세스/실제 이동 시작만 대체한다."""
    def __init__(self):
        super().__init__('originals')
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in ENTRIES:
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'))
                self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high-low+1)
        self.spec=dict(self.spec,entries=dict(self.spec['entries'],Retreat=0x427e00,Life=0x421c30))
        self.stubs.pop(0x4202f0,None)  # 이번에는 칸 위 이동체 생성자도 실제 실행한다.
        self.calls=0  # 이번 호출의 실제 명령 수이며 파생 hook에서 누적한다.

    def put(self,address,value,width=4):
        """격리 입력 또는 외부 효과 결과만 명시 DWORD/WORD/BYTE 폭으로 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def returned(self,value=0,purge=0):
        """큰 효과 경계의 원문 cdecl 반환을 수행하며 순수 선택 계산을 대신하지 않는다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);target=int.from_bytes(self.mu.mem_read(sp,4),'little')
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def on_instruction(self,mu,address,size,data):
        """finder/Graph/좌표/거리의 실행은 유지하고 외부 이동 계약의 실제 인자만 기록한다."""
        self.calls+=1
        sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<8I',mu.mem_read(sp+4,32))
        if address==0x488fe0:
            self.events.append('T:'+':'.join(map(str,args[:3])));self.returned(46);return
        if address==0x4b1a90:
            self.events.append('P:'+':'.join(map(str,args[:4])));self.returned(900 if self.case[3] else 0);return
        if address==0x489080:
            self.events.append('M:'+':'.join(map(str,args)));self.returned(self.case[8]);return
        if address==0x4e0620:
            self.events.append('A:'+str(args[2]));self.returned();return
        if address==0x4b16d0:
            self.events.append('F:'+':'.join(map(str,args[:4])))
        super().on_instruction(mu,address,size,data)

    def sample(self,case,control):
        """실제 해시 입력을 설치하고 원문이 고른 이동/거부와 전체 슬롯을 읽는다."""
        self.case=case;op,boss,kind,path,scenario,life,flags,graph,start,check,reverse,actorX,actorY=case
        # 동일 부모의 지면과 사제/rainWalker, 두 후보는 서로 다른 실제 TYPE를 쓴다.
        points=((22.0,21.0,20.0,23.0),(21.0,21.0,20.0,22.0),(21.0,22.0,19.0,20.0),
                (22.0,21.0,18.0,21.0),(20.125,21.0,20.0,22.125),(20.0,21.0,22.0,23.0))[scenario]
        objects=[(50,82,0,0,bits(20),bits(21),1,1,0,0),
                 (60,kind,0,0,actorX,actorY,1,1,0,1),
                 (61,70,0,0,bits(points[0]),bits(points[1]),1,1,0,0),
                 (62,71,0,0,bits(points[2]),bits(points[3]),1,1,0,0)]
        if reverse:objects=objects[:2]+list(reversed(objects[2:]))
        self.setup(objects)
        for typ,first,second in ((82,0x800,4),(kind,0,0x10000),(70,flags,4),(71,0x800,2)):
            self.put(0x11100000+typ*500+0xe8,first);self.put(0x11100000+typ*500+0xec,second)
        for sid,g,word in ((50,1,5<<3),(60,254,0),(61,graph,life<<3),(62,1,0)):
            self.put(self.slot(sid)+30,g,1);self.put(self.slot(sid)+12,word,2)
        self.put(0x540bc4,boss);self.put(0x540bc0,0);self.put(0x5e4794,check);self.put(0x5411a0,82)
        self.put(0x5412d0,158);self.put(0x5412c8,156);self.put(0x540414,1);self.put(0x531928,256)
        self.events=[]
        if op:self.call('Life',[0],self.slot(50),4,control)
        else:self.call('Retreat',[],self.slot(60),0,control)
        return ';'.join(self.events) or '-',','.join(bytes(self.mu.mem_read(self.slot(sid),50)).hex() for sid in (50,60,61,62))


def main():
    """초기관문/거리동률/수명0·6·7/Graph/대상 TYPE를 실제 두 정밀도에서 독립 실행한다."""
    WORK.mkdir(parents=True,exist_ok=True);oracle=RetreatOracle();cases=[]
    for op,boss,kind,path in itertools.product((0,1),(0,1),(133,156,158),(0,1)):
        cases.append((op,boss,kind,path,0,0,0x800,1,1,0,0))
    for scenario,life,flags,graph,reverse in itertools.product(range(6),(0,1,6,7),(0,0x800),(1,2),(0,1)):
        cases.append((0,1,158,0,scenario,life,flags,graph,1,0,reverse))
    for start,check in itertools.product((0,1),(0,1)):
        cases.append((0,1,158,0,0,0,0x800,1,start,check,0))
    cases=[(*case,bits(20),bits(21)) for case in dict.fromkeys(cases)]
    for point,op,scenario,path in itertools.product(((20.25,21),(20.99999,21),(20.5,21.5),(20.9999,21.9999),(20.99999,21.99999)),(0,1),(1,2),(0,1)):
        cases.append((op,1,158,path,scenario,0,0x800,1,1,0,0,bits(point[0]),bits(point[1])))
    cases=list(dict.fromkeys(cases));rows=['# operation,boss,type,path,scenario,life,flags1,graph,start,checking,reverse,actorXbits,actorYbits => events,raw50/60/61/62'];different=[]
    for index,case in enumerate(cases):
        a=oracle.sample(case,FPU_CONTROLS[0]);b=oracle.sample(case,FPU_CONTROLS[1])
        if a!=b:different.append(index)
        rows.append('\t'.join([','.join(map(str,case)),*a]))
    FIXTURE.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),last_full_decompilation='HJOW-Athlon / 2026-10-23',unique_inputs=len(cases),native_observations=len(cases)*2,precision_different=different,
                boundary='실제 finder/Graph/원천 수명/대상 predicate/거리/선택; 이동 종류 선택/현재 Form/실제 이동 시작/진단 보고만 대체',binary_sha256=oracle.sha256)
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':main()
