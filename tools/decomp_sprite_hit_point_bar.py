#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""선택 HP 막대의 명시 내부 명령 구간만 실행해 부모의 조회 순서/색/값을 관측한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS

OBJECT=SCRATCH+0x2000  # getter 진단 중 HP를 바꿀 수 있는 격리된 실제 raw 입력이다.
TYPE=SCRATCH+0x3000  # 부모가 저장한 동일 TYPE this를 확인하는 불투명 입력 토큰이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/sprite-hit-point-bar-x86.tsv'  # 최초 부모 관측만 동결한다.
SPEC={  # 현재 HJOW-Athlon 정밀 함수의 실제 명령 경계다.
    'originals':dict(root=0x498220,start=0x49898c,stop=0x4972f0,getter=0x49fce0,width=4,colors=[0x5b5dc4,0x59afd8,0x5b5e58]),
    'originalCD':dict(root=0x455f60,start=0x456789,stop=0x45680d,getter=0x444720,width=2,colors=[0x564bcc,0x55bf50,0x55bac8])}


def sha(path):
    """실제 입력/도구/보호 PE의 바이트 정체성을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class BarOracle(Oracle):
    """HP 존재 분기 안의 실제 정수 부모를 실행하고 자식 최대HP 반환만 공급한다."""
    def __init__(self,edition):
        """전체 원문 함수 범위를 엄격 허용하되 지정 내부 시작/끝만 실행한다."""
        self.s=SPEC[edition]  # 판본별 명령 구간/색 주소/HP 폭이다.
        super().__init__(edition,[dict(name='draw',**{edition:f'{self.s["root"]:08x}'})],helpers=[])
        self.mu.hook_add(UC_HOOK_MEM_READ,self.on_read)
        self.events=[];self.calls=0;self.maxima=[];self.mutate=0;self.recolor=0;self.output=None  # 독립 장치 입력과 실제 부모 관측이다.

    def put(self,address,value,width=4):
        """외부 장치의 명시 변경을 원문 저장 폭으로 공급한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def get(self,address,width=4,signed=False):
        """부모 후상태를 계산 없이 실제 메모리에서 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little',signed=signed)

    def on_read(self,mu,access,address,size,value,data):
        """자식 호출 전후에 일어나는 실제 signed raw HP 읽기만 추적한다."""
        if address==OBJECT+26 and size==self.s['width']:
            self.events.append('H'+str(self.get(address,size,True)))

    def on_instruction(self,mu,address,size,data):
        """최대HP 자식은 명시 반환/변경 경계이며 막대 출력 전 원문 결과를 관측한다."""
        if address==self.s['getter']:
            assert mu.reg_read(UC_X86_REG_ECX)==TYPE
            self.calls+=1;value=self.maxima[self.calls-1];self.events.append('M'+str(value))
            if self.mutate:
                self.put(OBJECT+26,[-5,17,200][self.calls-1],self.s['width'])
            if self.recolor:
                for index,target in enumerate(self.s['colors']):  # 각 자식 복귀 때 현재 세 색 저장소를 명시 갱신한다.
                    self.put(target,(index+1)*100+self.calls)
            sp=mu.reg_read(UC_X86_REG_ESP);ret=self.get(sp)
            mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if address==self.s['stop']:
            if self.edition=='originals':
                sp=mu.reg_read(UC_X86_REG_ESP);assert self.get(sp+4)==0
                self.output=(self.get(sp+8),self.get(sp+12,signed=True),self.get(sp+16,signed=True))
            else:
                self.output=(self.get(mu.reg_read(UC_X86_REG_ESP)+0x28),struct.unpack('<i',struct.pack('<I',mu.reg_read(UC_X86_REG_EBX)))[0],struct.unpack('<i',struct.pack('<I',mu.reg_read(UC_X86_REG_EDI)))[0])
            mu.emu_stop();return
        super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """각 명시 시작 문맥에서 반환 경계까지 실행해 trace/색/값/최종 HP를 읽는다."""
        current,a,b,c,self.mutate,self.recolor=case;self.maxima=[a,b,c];self.calls=0;self.events=[];self.output=None
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 이전 구간의 레지스터를 다음 입력에 숨기지 않는다.
            self.mu.reg_write(register,0)
        self.mu.mem_write(STACK,bytes(512));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.put(OBJECT+26,current,self.s['width'])
        for address,value in zip(self.s['colors'],[11,22,33]):  # 원문 색 전역의 초기 입력이며 getter에서 변경할 수 있다.
            self.put(address,value)
        if self.edition=='originals':
            self.mu.reg_write(UC_X86_REG_EBP,OBJECT);self.put(STACK+0x48,TYPE)
        else:
            self.mu.reg_write(UC_X86_REG_EBP,TYPE);self.put(STACK+0x14,OBJECT)
        self.mu.emu_start(self.s['start'],0,count=1000);assert self.output is not None
        return ','.join(map(str,self.output)),';'.join(self.events),self.get(OBJECT+26,self.s['width'],True),self.calls


def inputs():
    """반환과 독립인 HP/세 최대값/진단 중 HP·색 변경 입력을 교차한다."""
    maxima=[(100,100,100),(100,40,80),(0,20,30),(-7,-13,-37),(-2147483648,2147483647,0),(1,0,7),(2147483647,-2147483648,-7)]
    return [(hp,*values,mutate,recolor) for hp,values,mutate,recolor in itertools.product((-2147483648,-1,0,10,25,50,100,32767,2147483647),maxima,(0,1),(0,1))]


def main():
    """최초 fixture를 한 번 생성하거나 같은 원문 구간을 재관측해 읽기 전용 검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    rows=['# 실제 양판본 HP 막대 내부 부모 구간. HP 존재/기하/픽셀은 제외, 최대HP 자식/HP·색 변경은 명시 장치 입력.'];editions={}
    for edition in SPEC:  # 각 보호 PE의 독립 명령 구간을 실행한다.
        oracle=BarOracle(edition)
        for case in inputs():  # 원문에서 관측한 추적/색/값만 고정한다.
            rows.append('\t'.join(map(str,(edition,','.join(map(str,case)),*oracle.observe(case)))))
        editions[edition]=dict(samples=len(inputs()),binarySha256=oracle.sha256,refinedIndexSha256=sha(ROOT/f'extracted/refined/{edition}/functions.tsv'),**SPEC[edition])
    content='\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report=dict(host=socket.gethostname(),mode='verify' if args.verify else 'first',samples=len(rows)-1,editions=editions,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),boundaries=['HP 존재 분기 내부 시작 문맥','최대HP getter 반환/정상 복귀와 raw HP·색 변경','PATCH bar 호출 인자/CD 기하 전 결과 관측'],fullDraw=False,childGetterBody=False,pixelOutput=False)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(report['mode'],report['samples'],'HP 막대 원문 행 일치',flush=True)


if __name__=='__main__':
    main()
