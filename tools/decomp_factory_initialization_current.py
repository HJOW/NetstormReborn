#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 공통 PostCreate/PostTake와 실제 HP 하위를 격리 실행한다. 게임/통신은 시작하지 않는다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

TYPES=0x12000000  # 실제 GetType과 getter가 읽는 분리된 TYPE 배열이다.
OBJECT=SCRATCH+0x2000  # 부모와 실제 HasHP/setter가 공유하는 물리 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/factory-initialization-current-x86.tsv'  # 최초 실제 바이트/진단 결과를 동결한다.
SPEC={  # 현재 HJOW-Athlon 정밀 함수/전역과 판본별 필드 폭이다.
    'originals':dict(create=0x4b0c60,take=0x4ad4a0,helpers=[0x49a840,0x49fce0],base=0x59ab20,stride=500,server=0x540bc0,quarter=0x5ca8f0,priest=0x5412d0,dais=0x5412ec,checking=0x5e4794,assertion=0x4e0620,width=50,extra=40),
    'originalCD':dict(create=0x4ae040,take=0x4ae120,helpers=[0x4aecf0,0x4abae0,0x4abac0,0x444720,0x4448e0,0x4aed20],base=0x51c960,stride=468,server=0x540a28,quarter=0x511c90,priest=0x51cbbc,dais=0x51cbd8,checking=0x540a24,assertion=0x490040,width=36,extra=35)}


def sha(path):
    """원본/정밀색인/도구/고정관측의 실제 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class InitializationOracle(Oracle):
    """외부 진단 UI만 명시 장치로 두고 초기화/HP 전체 순수 하위를 실행한다."""
    def __init__(self,edition):
        """현재 refined의 실제 두 부모/하위 범위만 허용한다."""
        self.s=SPEC[edition]  # 같은 판본의 원문 메타데이터다.
        super().__init__(edition,[dict(name=name,**{edition:f'{self.s[name]:08x}'}) for name in ('create','take')],helpers=self.s['helpers'])
        self.mu.mem_map(TYPES,0x20000);self.events=[];self.profile=0;self.diagnostics=0  # 독립 입력별 관측/복귀 프로필이다.

    def put(self,address,value,width=4):
        """명시 입력은 지정한 원문 정수 폭으로만 저장한다."""
        self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def get(self,address,width=4):
        """현재 실제 메모리의 비트를 읽으며 판단값을 계산하지 않는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """진단 인자/그때 슬롯을 관측하고 명시 외부 변경 뒤 정상 반환한다."""
        if address==self.s['assertion']:
            sp=mu.reg_read(UC_X86_REG_ESP);expression=self.read_string(self.get(sp+4)).decode('ascii');file=self.read_string(self.get(sp+8)).decode('ascii');line=self.get(sp+12)
            self.events.append(f'{expression}|{file}|{line}|'+bytes(mu.mem_read(OBJECT,self.s['width'])).hex());self.diagnostics+=1
            if self.profile==1:
                self.put(OBJECT+10,75,1);self.put(TYPES+75*self.s['stride'],(-37)&0xffffffff);self.put(TYPES+75*self.s['stride']+0x104,0x345);self.put(self.s['quarter'],1);self.put(self.s['priest'],75)
            elif self.profile==2:
                self.put(OBJECT+10,76,1);self.put(self.s['server'],0);self.put(OBJECT+11,self.get(OBJECT+11,1)&~8,1)
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,self.get(sp));return
        super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """함수 반환 뒤 동일 슬롯 전체와 현재 외부 전역/진단만 기록한다."""
        operation,number,state,server,mode,self.profile=case;self.events=[];self.diagnostics=0
        self.mu.mem_write(OBJECT,bytes((i*17+23)&255 for i in range(self.s['width'])))
        self.put(OBJECT+10,number,1);self.put(OBJECT+11,state,1);self.put(OBJECT+self.s['extra'],1 if mode==2 else 0,1)
        self.put(self.s['base'],TYPES);self.put(self.s['server'],server);self.put(self.s['quarter'],0);self.put(self.s['checking'],0);self.put(self.s['priest'],158);self.put(self.s['dais'],165)
        for selected in (3,74,75,76):  # TYPE 자체의 입력은 모든 반복에서 처음 값으로 복원한다.
            p=TYPES+selected*self.s['stride'];self.mu.mem_write(p,bytes(self.s['stride']));self.put(p,0x12345678 if selected==74 else -37);self.put(p+0xe8,16 if mode==0 else 0);self.put(p+0xec,0x400000 if mode else 0);self.put(p+0x104,0x123+selected)
        self.call(operation,[],this=OBJECT)
        return bytes(self.mu.mem_read(OBJECT,self.s['width'])).hex(),';'.join(self.events) or '-',self.get(self.s['server']),self.get(self.s['quarter']),self.get(self.s['priest'])


def inputs():
    """원문 출력과 독립인 객체 상태/권한/HP 유무와 진단 복귀 입력을 교차한다."""
    return list(itertools.product(('create','take'),(3,74),(0,7,8,13),(0,1),(0,1,2),(0,1,2)))


def main():
    """최초 쓰기는 배타적으로 동결하고 재관측은 기존 fixture를 읽기만 한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();rows=['# 공통 초기화/실제 HasHP·TYPE HP/setter. 진단 UI 정상 복귀만 명시 장치이며 게임/MP는 실행하지 않는다.'];editions={}
    for edition in SPEC:  # 두 보호 PE의 실제 부모를 별도로 관측한다.
        oracle=InitializationOracle(edition)
        for case in inputs():  # 원문 전체 슬롯/진단 당시 슬롯/현재 전역을 그대로 고정한다.
            rows.append('\t'.join(map(str,(edition,','.join(map(str,case)),*oracle.observe(case)))))
        editions[edition]=dict(samples=len(inputs()),binarySha256=oracle.sha256,refinedIndexSha256=sha(ROOT/f'extracted/refined/{edition}/functions.tsv'))
    content='\n'.join(rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(content)
    report=dict(host=socket.gethostname(),mode='verify' if args.verify else 'first',samples=len(rows)-1,editions=editions,fixtureSha256=sha(FIXTURE),toolSha256=sha(Path(__file__)),boundaries=['진단 UI 정상복귀/명시 TYPE·HP·서버·약화 변경'],actualHasHitPoints=True,actualTypeHitPoints=True,actualSetter=True,game=False,multiplayer=False)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(report['mode'],report['samples'],'초기화 원문 행 일치',flush=True)


if __name__=='__main__':
    main()
