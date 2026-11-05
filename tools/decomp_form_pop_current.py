#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 Form Pop의 두 PE 부모만 격리 관측한다. 진단/전송/PostPop은 명시 장치다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
POOL, VT, POST = 0x13000000, 0x14000000, SCRATCH+0x1000  # 호스트/게임과 분리한 입력과 두 가상 PostPop 장치다.
FIXTURE=ROOT/'cpppj/tests/fixtures/form-pop-current-x86.tsv'  # 첫 유효 PE 관측만 새 파일로 동결한다.
SPEC={  # 보호 PE와 현재 정밀 소스의 실제 함수/전역이다.
    'originals':dict(entry=0x4ac560,pool=0x5c8464,capacity=0x5c847c,stride=50,server=0x540bc0,depth=0x5c8488,transmit=0x4ac260,assertion=0x4e0620),
    'originalCD':dict(entry=0x4abfb0,pool=0x5395dc,capacity=0x5395f4,stride=36,server=0x540a28,depth=0x539600,transmit=0x4abb40,assertion=0x490040)}
class FormPopOracle(Oracle):
    """현재 권한/상태/깊이와 실제 가상 선택 순서만 원문 명령으로 관측한다."""
    def __init__(self,edition):
        """부모 범위와 격리된 풀/VT를 준비한다."""
        self.s=SPEC[edition]  # 현재 판본의 실제 주소와 슬롯 폭이다.
        super().__init__(edition,[dict(name='pop',**{edition:f'{self.s["entry"]:08x}'})],helpers=[])
        self.mu.mem_map(POOL,0x200000)
        self.mu.mem_map(VT,0x1000)
        self.events=[]  # 원문 선택 호출과 그 직전 부분 상태다.
        self.profile=0  # 정상 복귀 장치가 입력하는 현재값 변경이다.
        self.object=POOL  # 같은 this 슬롯 주소다.
    def put(self,address,value,width=4):
        """명시 입력의 원문 폭 비트만 쓴다."""
        self.mu.mem_write(address,(value & ((1<<(8*width))-1)).to_bytes(width,'little'))
    def get(self,address,width=4):
        """현재 기계 메모리를 정수 비트로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')
    def resume(self,value=0,purge=0):
        """명시 자식의 스택 규약만 공급한다. 부모 ret4는 직접 실행한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)
        self.mu.reg_write(UC_X86_REG_EIP,self.get(sp))
    def on_instruction(self,mu,address,size,data):
        """진단 복귀/전송 장치/현재 VT+20 호출 직전 상태를 관측한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.s['assertion']:
            expression=self.read_string(self.get(sp+4)).decode('ascii')
            file=self.read_string(self.get(sp+8)).decode('ascii')
            self.events.append(f'D{expression}|{file}|{self.get(sp+12)},{self.get(self.object+11,1)},{self.get(self.s["server"])},{self.get(self.s["depth"])}')
            if self.profile==1:
                self.put(self.object+11,8,1)
                self.put(self.object,VT+0x100)
                self.put(self.s['server'],1-self.get(self.s['server']))
                self.put(self.s['depth'],0xffffffff)
            self.resume()
            return
        if address==self.s['transmit']:
            args=[self.get(sp+4+i*4) for i in range(4)]
            self.events.append('T'+','.join(map(str,args+[self.get(self.object+11,1)])))
            if self.profile==2:
                self.put(self.object+11,self.get(self.object+11,1)|0x40,1)
                self.put(self.object,VT+0x100)
                self.put(self.s['depth'],7)
            self.resume(purge=16)
            return
        if address in (POST,POST+0x10):
            table=VT+(0x100 if address==POST+0x10 else 0)
            self.events.append(f'P{table},{self.get(sp+4)},{self.get(self.object+11,1)},{self.get(self.s["depth"])}')
            self.put(self.s['depth'],self.get(self.s['depth'])+(1 if self.profile==3 else -1))
            self.resume(purge=4)
            return
        super().on_instruction(mu,address,size,data)
    def observe(self,case):
        """합성 입력을 놓고 trace/현재 권한/깊이/전체 this 슬롯을 읽는다."""
        sid,state,flags,server,depth,self.profile=case
        self.events=[]
        self.object=POOL+sid*self.s['stride']
        self.mu.mem_write(self.object,bytes(((sid*self.s['stride']+i)*37+19)&255 for i in range(self.s['stride'])))
        self.put(self.s['pool'],POOL)
        self.put(self.s['capacity'],32768)
        self.put(self.s['server'],server)
        self.put(self.s['depth'],depth)
        self.put(self.object,VT)
        self.put(self.object+11,state,1)
        self.put(VT+0x20,POST)
        self.put(VT+0x100+0x20,POST+0x10)
        self.call('pop',[flags],this=self.object,purge=4)
        return ';'.join(self.events),str(self.get(self.s['server'])),str(self.get(self.s['depth'])),bytes(self.mu.mem_read(self.object,self.s['stride'])).hex()
def main():
    """최초 기계어 결과를 동결하거나 같은 입력을 다시 관측해 확인한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    cases=list(itertools.product((0,5,6000,15000),(0,4,0x14,0x16),(0,5,8,0x10,0x40),(0,1),(0,0xffffffff),range(4)))
    rows=['# 공통 Form Pop PE 부모만 실행. 진단/전송/PostPop은 명시 장치. 실제 패킷/게임/통신 미실행.']
    binaries={}
    for edition in SPEC:  # 각 판본의 원문 부모만 독립 실행한다.
        oracle=FormPopOracle(edition)
        for case in cases:  # 기대값 모델 없이 첫 trace/현재 전역/전체 슬롯만 동결한다.
            rows.append('\t'.join((edition,','.join(map(str,case)),*oracle.observe(case))))
        binaries[edition]=oracle.sha256
    data=('\n'.join(rows)+'\n').encode('utf-8')
    if args.verify:
        assert FIXTURE.read_bytes()==data
    else:
        with FIXTURE.open('xb') as stream:
            stream.write(data)
    report=dict(host=socket.gethostname(),rows=len(rows)-1,rowsPerEdition=len(cases),binaries=binaries,fixtureSha256=hashlib.sha256(data).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),mode='verify' if args.verify else 'first',actualChildren=False,game=False,multiplayer=False)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(report['mode'],report['rows'],'공통 Form Pop 부모 관측',flush=True)
if __name__=='__main__':
    main()
