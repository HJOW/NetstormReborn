#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제004ac040/004ac140의SID범위·현재읽기·진단bit·assert복귀를격리관측한다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_priestmenu import Oracle as Base, ROOT, STACK, STOP, digest
from decomp_file_name_leaf import add_ranges
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_FPCW

POOL=0x14010000  # 기존게임/객체를호출하지않는합성PoolDWORD원점이다.
MEMORY=0x14000000  # 논리Pool밖type선행읽기도독립물리입력으로허용할넓은격리배열이다.
ROBUST,TYPE,BASE,CAPACITY,CHECKING=0x5c8450,0x5c7c48,0x5c8464,0x5c847c,0x5e4794  # 실제원문전역주소다.
OLD_ROBUST,OLD_TYPE=0xaaaa5555,0x55bbdd66  # 미선택부모가진단을보존하는지구별할입력seed다.


class AssertionFailure(Exception):
    """명시진단callee의비복귀입력이며원문UI/SEH자체를실행하지않는다."""


class SidOracle(Base):
    """두부모만실행하고임의vtable토큰·free/dead/typeBYTE는단순입력으로만읽는다."""
    def __init__(self):
        """보호PE를읽고실제함수범위와별도명시물리만설치한다."""
        super().__init__();self.allowed=[];add_ranges(self,(0x4ac040,0x4ac140));self.mu.mem_map(MEMORY,0x400000);self.mu.mem_map(0,0x10000);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def state(self):
        """같은현재진단두DWORD를읽고순서검증용trace에그시점값을붙인다."""
        return ':'.join(str(self.words(address,1)[0]) for address in (ROBUST,TYPE))

    def read(self,mu,access,address,size,value,data):
        """실제선택된전역/필드읽기만기록하고스택읽기는섞지않는다."""
        if address in (BASE,CAPACITY,CHECKING):self.trace.append({BASE:'P',CAPACITY:'C',CHECKING:'K'}[address]+':'+str(self.words(address,1)[0])+':'+self.state())
        elif self.object is not None and (address,size) in ((self.object,4),((self.object+10)&0xffffffff,1),((self.object+11)&0xffffffff,1)):self.trace.append(f'M:{address}:{size}:'+self.state())

    def instruction(self,mu,address,size,data):
        """진단의인수/현재값을관측하고정상복귀·예외·재진입쓰기를명시구별한다."""
        if address==0x4ac040:self.object=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4e0620:
            esp=mu.reg_read(UC_X86_REG_ESP);expression,file,line=self.words(esp+4,3);assert self.string(expression)==b'resolve( sid ).isRobust()' and line==0x218;self.trace.append('A:'+str(line)+':'+self.state())
            if self.assertMode==1:raise AssertionFailure()
            if self.assertMode==2:self.put(ROBUST,0x11223344);self.put(TYPE,0x55667788);self.put(CHECKING,0);self.put(BASE,POOL+50);self.put(CAPACITY,7)
            self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError(f'SID진단실행범위밖 {address:08x}')

    def sample(self,case,precision):
        """원문32비트주소감김과미선택장치·assert뒤반환을실제명령결과로보관한다."""
        operation,value,live,checking,base,capacity,table,flags,kind,self.assertMode=case;self.trace=[];self.object=None
        self.put(ROBUST,OLD_ROBUST);self.put(TYPE,OLD_TYPE);self.put(BASE,base);self.put(CAPACITY,capacity);self.put(CHECKING,checking)
        pointer=value&0xffffffff if operation=='object' else (base+(value&0xffffffff)*50)&0xffffffff
        if operation=='object' or (1<=value<=65534 and live and checking):
            assert MEMORY<=pointer<MEMORY+0x400000-50 or 0<=pointer<0x10000-50
            self.mu.mem_write(pointer,struct.pack('<I',table)+bytes(6)+bytes((kind,flags))+bytes(38))
        if operation=='object':self.object=pointer;words=(STOP,live&0xffffffff);entry=0x4ac040;self.mu.reg_write(UC_X86_REG_ECX,pointer)
        else:words=(STOP,value&0xffffffff,live&0xffffffff);entry=0x4ac140
        self.mu.mem_write(STACK,struct.pack('<'+'I'*len(words),*words));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(entry,STOP,count=10000)
        except AssertionFailure:outcome='assertion'
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        result=self.mu.reg_read(UC_X86_REG_EAX) if outcome=='ok' else 0
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+(8 if operation=='object' else 4)
        final=[self.words(address,1)[0] for address in (ROBUST,TYPE,CHECKING,BASE,CAPACITY)]  # 선택경계뒤실제현재DWORD이며진입값으로되돌리지않는다.
        return '\t'.join([str(x) for x in case]+[outcome,str(result)]+[str(x) for x in final]+[';'.join(self.trace) or '-'])


def cases():
    """일반raw슬롯만입력하며세대/전술/공중유닛생산과실제게임을만들지않는다."""
    # 두 객체 부모의 vtable·상태·타입·live 조합을 만든다.
    result=[('object',POOL+50,live,1,POOL,64,table,flags,kind,0) for table,flags,kind,live in itertools.product((0,0x12345678,0xdddddddd,0xcdcdcdcd),(0,1,2,3,4,5,6,255),(0,1,187,255),(0,1))]
    # SID 부모의 checking과 실제 필드 조합을 독립 입력으로 만든다.
    result += [('sid',1,live,checking,POOL,64,table,flags,kind,0) for table,flags,kind,live,checking in itertools.product((0,0x12345678,0xdddddddd,0xcdcdcdcd),(0,1,2,3,4,5,6,255),(0,1,187,255),(0,1),(0,1))]
    # signed SID 번호 경계와 live/checking 관문을 조합한다.
    result += [('sid',sid,live,checking,POOL,64,0x12345678,0,7,0) for sid,live,checking in itertools.product((-2147483648,-1,0,1,4,63,64,65,65534,65535,65536,2147483647),(0,1,-1),(0,2,0xffffffff))]
    # Pool 아래/안/상한 포인터와 capacity 조합을 만든다.
    result += [('object',POOL+offset,live,1,POOL,capacity,0x12345678,0,7,0) for offset,capacity,live in itertools.product((-50,0,49,50,51,99,100,3199,3200,3250),(0,1,2,64),(0,1))]
    # 진단의 예외와 복귀 뒤 전역 변경 입력을 조합한다.
    result += [('sid',1,1,checking,POOL,64,table,flags,kind,mode) for checking,table,flags,kind,mode in itertools.product((1,0xffffffff),(0,0x12345678),(0,3),(0,7),(1,2))]
    # 32비트 주소 감김 뒤의 객체 필드 읽기 입력을 만든다.
    result += [('object',34,live,1,0xfffffff0,64,table,flags,kind,0) for live,table,flags,kind in itertools.product((0,1),(0,0x12345678),(0,1),(0,7))]
    # 같은 주소 감김을 SID 부모에서도 관측한다.
    result += [('sid',1,1,1,0xfffffff0,64,table,flags,kind,0) for table,flags,kind in itertools.product((0,0x12345678),(0,1),(0,7))]
    return list(dict.fromkeys(result))


def main():
    """새fixture최초관측또는읽기전용재관측만허용한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert not a.output.exists() and (a.fixture.exists() if a.verify else not a.fixture.exists())
    oracle=SidOracle();rows=['# operation input live checking base capacity vtable flags type assertMode outcome result lastRobust lastType finalChecking finalBase finalCapacity trace']
    for case in cases():  # 기대값은원문실제명령의두FPCW결과에서만얻는다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    text='\n'.join(rows)+'\n'
    if a.verify:assert a.fixture.read_text('utf-8')==text
    else:a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(text,encoding='utf-8',newline='\n')
    report=dict(unit=779,part='SID diagnostic parent',rows=len(rows)-1,observations=(len(rows)-1)*2,host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제ac040/ac140와선택전역/물리읽기순서.진단4e0620은명시반환/예외/현재전역변경장치.32비트합성Pool물리만읽으며OS/게임/가상vtable호출/SEH UI/실제64비트호스트포인터는CPU관측범위밖',sha256={name:digest(ROOT/name) for name in ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv')});a.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=report['rows'],observations=report['observations'])))


if __name__=='__main__':main()
