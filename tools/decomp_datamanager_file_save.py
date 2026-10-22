#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE0044d8d0의 이미지/현재 관문/두 이름/권한/쓰기/반납 순서를 격리 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1] #명시 프로젝트 루트다.
STACK=0x2100000;MEMORY=0x1100000;STOP=0x3100000 #분리 스택/manager/정상 복귀 표식이다.
MANAGER=MEMORY;VTABLE=MEMORY+0x100;IMAGE=MEMORY+0x1000;ALTERNATE=MEMORY+0x2000 #관측의 불투명 포인터 토큰이다.
READONLY=0x54db30;OWNER=0x540c70;HIDDEN=0x52f81e #현재 원본 저장/로컬 번호/숨김 전역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/datamanager-file-save-x86.tsv' #새 독립 관측만 저장한다.
REPORT=ROOT/'cpppj/datamanager-file-save-20261022.json' #범위와 입력 해시를 기록한다.


class Oracle:
    """게임/OS 파일 장치 없이 실제 저장 부모와 원본 가상 파일명 getter를 실행한다."""
    def __init__(self):
        """보호 PE와 분리 메모리/SEH 슬롯만 준비한다."""
        pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        size=(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095
        self.mu.mem_map(0x400000,size);self.mu.mem_write(0x400000,pe.get_memory_mapped_image())
        self.mu.mem_map(0,4096);self.mu.mem_map(MEMORY,0x10000);self.mu.mem_map(STACK-0x10000,0x20000);self.mu.mem_map(STOP,4096)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction)
        self.trace=[] #현재 입력의 외부 호출 순서다.
        self.names=0 #현재 저장 부모의 가상 이름 조회 횟수다.
        self.source=0 #실제 호출자 MarkStream20 주소다.
        self.hidden=0 #관측 입력의 초기 숨김 BYTE다.
        self.variant=0 #관측 장치가 수행할 명시 메모리 효과 종류다.

    def word(self,address,value=None):
        """같은 비정렬 DWORD를 읽거나 쓴다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def string(self,address):
        """현재 C문자열을 첫 NUL까지만 읽는다."""
        out=bytearray()
        for index in range(256):
            byte=self.mu.mem_read(address+index,1)[0]
            if not byte:return out.decode('ascii')
            out.append(byte)
        raise RuntimeError('관측 파일명256BYTE 범위')

    def returned(self,purge=0):
        """외부 cdecl/thiscall 반환의 정확한 스택만 복구한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);target=self.word(esp)
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def instruction(self,mu,address,size,data):
        """실제 부모/getter를 실행하고 이미지/CRT/파일/할당만 명시 경계로 둔다."""
        if address==STOP or 0x44d8d0<=address<0x44d980:return
        if address==0x4bcca0:
            self.names+=1;self.trace.append('N:'+self.string(MANAGER+12))
            if self.variant==6 and self.names==2:self.word(self.source,ALTERNATE);self.word(self.source+8,55)
            return #실제 getter의 LEA/RET를 그대로 실행한다.
        if 0x4bcca0<address<0x4bccb0:return
        esp=mu.reg_read(UC_X86_REG_ESP);args=[self.word(esp+4+index*4) for index in range(3)]
        if address==0x44d640:
            self.source=args[0];self.trace.append('S:'+bytes(mu.mem_read(self.source,20)).hex())
            self.word(self.source,0 if self.variant==1 else IMAGE);self.word(self.source+4,0);self.word(self.source+8,0xffffffff if self.variant==7 else 17);self.word(self.source+12,0x8000);self.word(self.source+16,0x12345678)
            if self.variant==2:self.word(READONLY,int(not self.word(READONLY)))
            if self.variant==3:
                owner=(self.word(OWNER)+1)%9;self.word(OWNER,owner);mu.mem_write(HIDDEN+owner,bytes([int(not self.hidden)]))
            self.returned(4);return
        if address==0x4f868b:
            self.trace.append('C:'+self.string(args[0])+':'+str(args[1]))
            if self.variant==4:
                mu.mem_write(MANAGER+12,b'changed.fort\0');self.word(READONLY,1);self.word(OWNER,4);mu.mem_write(HIDDEN,b'\xff'*9)
            if self.variant==5:self.word(self.source,ALTERNATE);self.word(self.source+8,33)
            self.returned();return
        if address==0x41bb90:self.trace.append('W:'+self.string(args[0])+':'+str(args[1])+':'+str(args[2]));self.returned();return
        if address==0x4e4354:self.trace.append('F:'+str(args[0]));self.returned();return
        raise RuntimeError(f'미명시 원본 저장 호출 {address:08x}')

    def run(self,case,precision):
        """직전 결과를 재사용하지 않고 현재 전역/manager/스택을 다시 만든다."""
        readonly,owner,self.hidden,self.variant=case;self.trace=[];self.names=0;self.source=0
        self.mu.mem_write(MEMORY,b'\xa7'*0x10000);self.mu.mem_write(STACK-0x10000,b'\0'*0x20000);self.word(0,0)
        self.word(MANAGER,VTABLE);self.word(VTABLE,0x4bcca0);self.mu.mem_write(MANAGER+12,b'unit.fort\0')
        self.word(READONLY,readonly);self.word(OWNER,owner);self.mu.mem_write(HIDDEN,b'\0'*9);self.mu.mem_write(HIDDEN+owner,bytes([self.hidden]))
        self.word(STACK,STOP);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,MANAGER);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(0x44d8d0,STOP,count=10000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('저장 부모 정상 반환 계약')
        return [bytes(self.mu.mem_read(MANAGER,64)).hex(),bytes(self.mu.mem_read(self.source,20)).hex(),str(self.word(READONLY)),str(self.word(OWNER)),bytes(self.mu.mem_read(HIDDEN,9)).hex(),';'.join(self.trace)]


def main():
    """서로 다른 x87 정밀도의 동일 결과만 새 fixture와 범위 보고서에 저장한다."""
    oracle=Oracle();rows=[];calls=0
    for case in itertools.product((0,1,0xffffffff),(0,1,8),(0,1),range(8)):
        a=oracle.run(case,0x7f);b=oracle.run(case,0x27f);calls+=2
        if a!=b:raise RuntimeError('두 정밀도 저장 부모 결과 불일치')
        rows.append([','.join(map(str,case)),*a])
    FIXTURE.write_text('\t'.join(('# input','manager','source','readonly','owner','hidden','trace'))+'\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest() #출력과 실제 보호 PE의 바이트 해시다.
    getters=2*sum(event.startswith('N:') for row in rows for event in row[-1].split(';')) #두 정밀도에서 실제 실행한 원본 이름 getter 횟수다.
    report={'date':'2026-10-22','host':socket.gethostname(),'edition':'10.78','native_entries':['0044d8d0','004bcca0'],'cases':len(rows),'native_parent_calls':calls,'native_file_name_calls':getters,'external_boundaries':['0044d640','004f868b','0041bb90','004e4354'],'fresh_game_run':False,'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (ROOT/'originals/Netstorm.exe',FIXTURE,Path(__file__))}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'cases':len(rows),'native_calls':calls}))


if __name__=='__main__':main()
