#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""회복 HP 분기 내부의 실제 명령과 현재 번호/HP 읽기 순서를 관측한다. 게임/Setter는 실행하지 않는다."""
import argparse
import hashlib
import itertools
import json
import struct
import socket
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS

OBJECT=SCRATCH+0x2000  # 부모가 읽는 같은 물리 Actor HP 입력이다.
TYPES=0x12000000  # 실제 GetType 산술이 선택할 TYPE 토큰 영역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/priest-regen-hit-points-current-x86.tsv'  # 최초 부모 trace/Setter 인자와 후상태를 고정한다.
SPEC={  # 현재 정밀 산출물의 정렬된 실제 명령 경계다.
    'originals':dict(root=0x494580,start=0x4945ee,skip=0x494683,maximum=0x4adcb0,typeMaximum=0x49fce0,setter=0x4942b0,base=0x59ab20,priest=0x5412d0,width=4,stride=500,helpers=[0x49a840]),
    'originalCD':dict(root=0x40ccd0,start=0x40cd15,skip=0x40cdaa,maximum=0x4aed30,typeMaximum=0x444720,setter=0x40ca40,base=0x51c960,priest=0x51cbbc,width=2,stride=468,helpers=[])}


def sha(path):
    """원본/도구/입력의 실제 바이트 정체성을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class RegenOracle(Oracle):
    """HP 조건/증분/분기/ADD/현재 번호 읽기는 실제 명령으로 실행한다."""
    def __init__(self,edition):
        """허용 부모/순수 GetType과 명시 자식 장치만 준비한다."""
        self.s=SPEC[edition]  # 현재 판본의 폭과 실제 함수/전역이다.
        super().__init__(edition,[dict(name='regen',**{edition:f'{self.s["root"]:08x}'})],helpers=self.s['helpers'])
        self.mu.mem_map(TYPES,0x20000);self.mu.hook_add(UC_HOOK_MEM_READ,self.on_read)
        self.actor_maxima=[];self.type_maxima=[];self.profile=0;self.actor_calls=0;self.type_calls=0;self.events=[];self.output=None;self.done=False  # 각 입력의 장치 반환/변경과 관측이다.

    def put(self,address,value,width=4):
        """외부 장치 변경을 원문 저장 폭으로 공급한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def get(self,address,width=4,signed=False):
        """실제 입력/출력 메모리만 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little',signed=signed)

    def on_read(self,mu,access,address,size,value,data):
        """현재HP와현재Priest번호의실제메모리읽기를같은순서로추적한다."""
        if address==OBJECT+26 and size==self.s['width']:
            self.events.append('H'+str(self.get(address,size,True)))
        if address==self.s['priest']:
            self.events.append('N'+str(self.get(address)))

    def mutate(self,actor,count):
        """원문 자식의 명시 정상 복귀 중 HP/현재 번호 변경을 공급한다."""
        if not self.profile:
            return
        if self.profile==1:
            hp=90-count*17 if actor else -13+count*40;number=80+count if actor else 90+count
        else:
            hp=(0x7ffffff0+count if self.s['width']==4 else 32767-count*100) if actor else (-2147483648+count if self.s['width']==4 else -32768+count)
            number=110+count if actor else 120+count
        self.put(OBJECT+26,hp,self.s['width']);self.put(self.s['priest'],number)

    def resume(self,value):
        """원문 무인자 thiscall getter가 실제 반환 주소로 복귀하게 한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);ret=self.get(sp)
        self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_ESP,sp+4);self.mu.reg_write(UC_X86_REG_EIP,ret)

    def on_instruction(self,mu,address,size,data):
        """Actor/TYPE getter만명시반환경계로두고Setter진입에서인자를관측한다."""
        if address==self.s['maximum']:
            assert mu.reg_read(UC_X86_REG_ECX)==OBJECT;value=self.actor_maxima[self.actor_calls];self.actor_calls+=1;self.events.append('A'+str(value));self.mutate(True,self.actor_calls);self.resume(value);return
        if address==self.s['typeMaximum']:
            pointer=mu.reg_read(UC_X86_REG_ECX);assert pointer>=TYPES and (pointer-TYPES)%self.s['stride']==0;number=(pointer-TYPES)//self.s['stride']
            value=self.type_maxima[self.type_calls];self.type_calls+=1;self.events.append('T'+str(number)+':'+str(value));self.mutate(False,self.type_calls);self.resume(value);return
        if address in (self.s['setter'],self.s['skip']):
            self.done=True
            if address==self.s['setter']:
                assert mu.reg_read(UC_X86_REG_ECX)==OBJECT;self.output=self.get(mu.reg_read(UC_X86_REG_ESP)+4,signed=True)
            mu.emu_stop();return
        super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """내부 시작 문맥에서 Setter/회복없음까지의실제결과와trace를읽는다."""
        hp,a,b,c,t,u,self.profile=case;self.actor_maxima=[a,b,c];self.type_maxima=[t,u];self.actor_calls=0;self.type_calls=0;self.events=[];self.output=None;self.done=False
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 이전 입력의 계산값을 새 입력에 숨기지 않는다.
            self.mu.reg_write(register,0)
        self.mu.mem_write(STACK,bytes(512));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ESI,OBJECT);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.put(self.s['base'],TYPES);self.put(self.s['priest'],74);self.put(OBJECT+26,hp,self.s['width']);self.mu.emu_start(self.s['start'],0,count=1000);assert self.done
        return '-' if self.output is None else self.output,';'.join(self.events),self.get(OBJECT+26,self.s['width'],True),self.get(self.s['priest'])


def inputs():
    """출력과독립인두HP폭/각getter반환/정상복귀변경을교차한다."""
    actors=[(200,200,200),(100,50,25),(50,1,-1),(2147483647,-2147483648,2147483647)]
    types=[(60,60),(6,1200),(-7,-37),(-2147483648,2147483647),(2147483647,-2147483648),(0,0)]
    return [(hp,*actor,*kind,profile) for hp,actor,kind,profile in itertools.product((-2147483648,-1,0,40,100,2147483647),actors,types,(0,1,2))]


def main():
    """최초원문결과를한번동결하거나동일명령의재관측으로읽기전용검증한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    rows=['# 실제 회복 HP 내부 부모. Actor/TYPE getter 반환·HP/번호변경과Setter경계만명시장치. 게임/공간/표시/중립동작미실행.'];editions={}
    for edition in SPEC:  # 두 보호 PE의 실제 분기/읽기/ADD를 독립 실행한다.
        oracle=RegenOracle(edition)
        for case in inputs():  # 원문 trace/Setter 인자만 고정한다.
            rows.append('\t'.join(map(str,(edition,','.join(map(str,case)),*oracle.observe(case)))))
        editions[edition]=dict(samples=len(inputs()),binarySha256=oracle.sha256,refinedIndexSha256=sha(ROOT/f'extracted/refined/{edition}/functions.tsv'),**SPEC[edition])
    content='\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report=dict(host=socket.gethostname(),mode='verify' if args.verify else 'first',samples=len(rows)-1,editions=editions,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),boundaries=['HP 분기 내부 시작','Actor/TYPE getter 반환·HP/현재번호변경','Setter인자/회복없는후단관측'],fullHandler=False,actualGetter=False,actualSetter=False)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(report['mode'],report['samples'],'회복 HP 원문 행 일치',flush=True)


if __name__=='__main__':
    main()
