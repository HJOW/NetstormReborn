#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Form 전체 연결 검사와 건설 checking 진단 앞단을 독립 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,POOL,TEXT,STACK,STOP,digest
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

OBJECT,VTABLE,PARENT=TEXT+0x1000,TEXT+0x2000,STOP+0x100  #실제 호스트와 무관한 객체/가상 표/명시 부모 반환 장치다.
CHECKING=0x5e4794  #현재 전체 진단 DWORD다.
CHAINS=((),(20,),(50,),(20,50),(50,20),(20,30,50,63),(20,64),(50,64),(64,),(65535,),(20,50,65535),(63,30,20))  #순환 없이 자기 뒤 범위 실패도 포함하는 명시 입력이다.

class ConstructionFormOracle(MemoryOracle):
    """Form 전체 몸체와 건설 진단 앞단만 실행하며 getter/진단을 명시 장치로 받는다."""
    def __init__(self):
        """검토한 두 함수 범위와 명시 가상 반환 주소를 연결한다."""
        super().__init__();add_ranges(self,(0x4ae080,0x4428c0));self.put(VTABLE,PARENT)

    def instruction(self,mu,address,size,data):
        """현재 부모 반환과 건설의 조회/진단 복귀 효과를 구별한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);args=self.words(sp+4,3)
        if self.kind=='F' and address==PARENT:
            assert mu.reg_read(UC_X86_REG_ECX)==POOL+self.form*50;self.events.append('P:'+str(self.form)+':'+str(self.parent));self.ret(self.parent);return
        if self.kind=='C':
            if address==0x41bfb0:self.events.append('B');raise BoundaryFailure('body')
            if address==0x41bfa0:
                assert mu.reg_read(UC_X86_REG_ECX)==OBJECT;form=self.words(OBJECT+4,1)[0];self.events.append('F:'+str(form))
                if self.mutate:self.put(CHECKING,0);self.put(OBJECT+4,1)
                self.ret(POOL+form*50);return
            if address==0x4ae080:
                pointer=mu.reg_read(UC_X86_REG_ECX);assert (pointer-POOL)%50==0;self.events.append('R:'+str((pointer-POOL)//50)+':'+str(self.robust));self.ret(self.robust);return
            if address==0x4e0620:
                self.events.append('D:'+self.string(args[0]).hex()+':'+self.string(args[1]).hex()+':'+str(args[2]))
                if self.mutate:self.put(CHECKING,0)
                if not self.resume:raise BoundaryFailure('assertion')
                self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미확인 건설/Form 명령 '+hex(address))

    def call(self,entry,this,precision):
        """호출별 스택/this/x87 입력을 다시 공급하고 도달 지점을 읽는다."""
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,this);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='return'
        try:self.mu.emu_start(entry,STOP,count=10000)
        except BoundaryFailure as boundary:outcome=str(boundary)
        if outcome=='return':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision;return outcome

    def linked(self,case,precision):
        """실제 전체 체인의 반환과 풀 불변을 관측한다. 순환 입력은 넣지 않는다."""
        self.kind='F';self.form,self.parent,pattern=case;self.events=[];chain=CHAINS[pattern];self.mu.mem_write(POOL,bytes(64*50));self.put(0x5c8464,POOL);self.put(0x5c847c,64);self.put(POOL+self.form*50,VTABLE);self.put(POOL+self.parent*50+6,chain[0] if chain else 0,2)
        for index,number in enumerate(chain):  #논리 범위 밖 번호는 head/next로만 쓰고 물리에 접근하지 않는다.
            if number<64:self.put(POOL+number*50+4,chain[index+1] if index+1<len(chain) else 0,2)
        before=bytes(self.mu.mem_read(POOL,64*50));assert self.call(0x4ae080,POOL+self.form*50,precision)=='return';assert before==bytes(self.mu.mem_read(POOL,64*50))
        return '\t'.join([','.join(map(str,case)),str(self.mu.reg_read(UC_X86_REG_EAX)),';'.join(self.events),'1'])

    def construction(self,case,precision):
        """선택된 현재 Form과 진단 복귀 뒤 본문 첫 부모 조회 도달을 구별한다."""
        self.kind='C';checking,form,self.robust,self.resume,self.mutate=case;self.events=[];self.mu.mem_write(OBJECT,bytes(64));self.put(OBJECT+4,form);self.put(CHECKING,checking);outcome=self.call(0x4428c0,OBJECT,precision)
        assert outcome in ('body','assertion');return '\t'.join([','.join(map(str,case)),outcome,str(self.words(CHECKING,1)[0]),str(self.words(OBJECT+4,1)[0]),';'.join(self.events)])

def main():
    """두 원문 관측을 새 fixture로 동결하거나 기존 바이트와 재관측을 비교한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--form-fixture',required=True,type=Path);parser.add_argument('--guard-fixture',required=True,type=Path);parser.add_argument('--output',required=True,type=Path);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists();oracle=ConstructionFormOracle();outputs=[]
    for fixture,cases,method in ((args.form_fixture,list(itertools.product((0,1,20,50,63),(0,1),range(len(CHAINS)))),oracle.linked),(args.guard_fixture,list(itertools.product((0,1,0x80000000,0xffffffff),(0,50),range(2),range(2),range(2))),oracle.construction)):  #Form120・건설앞단64의 고유 입력만 사용한다.
        assert fixture.exists()==args.verify;rows=['# case result current effects']
        for case in cases:  #두 x87 제어값의 효과/반환을 독립 관측한다.
            first=method(case,0x027f);assert first==method(case,0x037f);rows.append(first)
        content='\n'.join(rows)+'\n'
        if args.verify:assert fixture.read_text(encoding='utf-8')==content
        else:fixture.write_text(content,encoding='utf-8',newline='\n')
        outputs.append(dict(fixture=str(fixture),rows=len(cases),observations=2*len(cases)))
    record=dict(unit=800,host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',outputs=outputs,rows=sum(row['rows'] for row in outputs),observations=sum(row['observations'] for row in outputs),gameExecutions=0,multiplayerTests=0,boundary='실제ae080전체·부모가상getter만명시. 건설4428c0은checking/Form/진단앞단만·41bfb0본문첫조회전중단. getter/연결반환/진단은명시장치·전체SEH/UI아님. 순환은원문정상반환표본에서제외.',sha256={str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.form_fixture,args.guard_fixture)})
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=record['rows'],observations=record['observations'])))

if __name__=='__main__':main()
