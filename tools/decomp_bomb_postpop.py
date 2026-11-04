#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 효과 postPop 전체와 실제455930 난수를 실행하고 예약/공통 장부 호출만 경계로 관찰한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 독립 fixture의 기준 경로다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/bomb-postpop-x86.tsv'  # 세 함수의 호출열/현재 난수 상태다.
POOL, STACK, STOP, RESERVED = 0x12000000, 0x20000800, 0x30000000, 0x31000000  # 게임/OS와 분리한 입력 및 반환 주소다.
ENTRIES = (0x41f870, 0x41f920, 0x41fab0)  # 실제 전체 함수만 허용한다.
TABLES = (0x502c38, 0x502cf0, 0x502db0)  # 각각의 보호 PE 가상 표다.


class Oracle:
    """SEH 정상 복귀/스택·비휘발 레지스터·raw 불변과 실제 난수 DWORD를 검사한다."""

    def __init__(self):
        """원본 코드를 수정하지 않고 읽어서 격리 주소 공간에 매핑한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))
        image = pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image)+4095)&~4095)
        self.mu.mem_write(0x400000, image)
        for address in (0, POOL, STACK&~4095, STOP, RESERVED):  # FS:0은 원본 OS의 예외 체인이 아닌 격리 페이지다.
            self.mu.mem_map(address, 4096)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows = {int(row['entry'],16):row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []  # 큰 공통 장부/프로세스/게임 루프는 허용하지 않는다.
        for entry in (*ENTRIES, 0x455930):  # 난수는 경계 반환을 합성하지 않고 실제 몸체를 실행한다.
            for part in rows[entry]['ranges'].split(';'):
                low,high = (int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        self.put(0x5c8464,POOL)
        self.context = self.mu.context_save()
        self.events, self.found, self.allocation, self.steps = [],0,0,0
        self.mu.hook_add(UC_HOOK_CODE,self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        for table,entry in zip(TABLES,ENTRIES):  # 재사용되는 가상 표 주소도 현재 PE에서 확인한다.
            assert struct.unpack('<I',pe.get_data(table-0x400000+0x20,4))[0] == entry

    def put(self,address,value):
        """격리 DWORD를 낮은 바이트부터 기록한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """현재 격리 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def ret(self,clean,value=0):
        """외부 명시 경계의 ABI만 정상 복귀시키며 원문 몸체는 덮어쓰지 않는다."""
        stack=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_ESP,stack+4+clean)
        self.mu.reg_write(UC_X86_REG_EIP,self.get(stack))

    def instruction(self,mu,address,size,data):
        """중복 조회/new/생성자/공통 장부 네 하위의 인자를 순서대로 보존한다."""
        self.steps+=1
        stack=mu.reg_read(UC_X86_REG_ESP)
        parent=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50
        if address==0x4afb40:
            self.events.append(f'find:{parent}:{self.get(stack+4)}')
            self.ret(4,self.found)
        elif address==0x4e4391:
            self.events.append(f'alloc:{self.get(stack+4)}')
            self.ret(0,RESERVED if self.allocation else 0)
        elif address==0x496e80:
            assert mu.reg_read(UC_X86_REG_ECX)==RESERVED
            self.events.append('regular:'+':'.join(str(self.get(stack+offset)) for offset in (4,8,12)))
            self.ret(12,RESERVED)
        elif address==0x4b0d30:
            self.events.append(f'base:{parent}:{self.get(stack+4)}')
            self.ret(4)
        else:
            assert any(low<=address<high for low,high in self.allowed),hex(address)

    def write(self,mu,access,address,size,value,data):
        """입구 SEH/스택 및 실제 난수 상태 외의 쓰기는 허용하지 않는다."""
        assert (STACK-512<=address and address+size<=STACK+32) or (address==0 and size==4) or (address==0x532710 and size==4),(hex(address),size)

    def observe(self,case,control):
        """두 x87 정밀도에서 같은 입력·난수/레지스터를 독립 복원한다."""
        kind,flags,found,allocation,seed=case
        self.mu.context_restore(self.context)
        self.events,self.found,self.allocation=[],found,allocation
        self.put(0,0x87654321)
        self.put(0x532710,seed)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK-512,b'\xa5'*544)
        self.mu.mem_write(STACK,struct.pack('<2I',STOP,flags))
        raw=bytes([0x5a])*50
        self.mu.mem_write(POOL+2500,raw)
        self.mu.reg_write(UC_X86_REG_ECX,POOL+2500)
        saved={register:0x11223344+index for index,register in enumerate((UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI))}
        for register,value in saved.items():  # 실제 thiscall의 비휘발 레지스터를 검증한다.
            self.mu.reg_write(register,value)
        self.mu.emu_start(ENTRIES[kind],STOP,count=500)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8
        assert self.get(0)==0x87654321 and bytes(self.mu.mem_read(POOL+2500,50))==raw
        assert bytes(self.mu.mem_read(STACK-512,16))==b'\xa5'*16 and bytes(self.mu.mem_read(STACK+8,24))==b'\xa5'*24
        assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        return '|'.join(self.events),self.get(0x532710)


def main():
    """새 fixture 생성과 보호된 기대값에 대한 재관측을 구분한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    cases=list(itertools.product(range(3),(0,1,2,3,0x2000040,0xffffffff),(0,1),(0,1),(0,1,55,0x12345678,0x7fffffff,0xffffffff)))
    oracle=Oracle()
    observed=[]  # 기대값은 Python의 복원식이 아니라 원본 함수 반환에서만 얻는다.
    for case in cases:
        first=oracle.observe(case,0x027f)
        assert oracle.observe(case,0x037f)==first,(case,first)
        observed.append('\t'.join(map(str,(*case,*first))))
    content='# kind\tflags\tfound\tallocation\tseed\tevents\tfinalSeed\n'+'\n'.join(observed)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==content
    else:
        assert not FIXTURE.exists()
        FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),date='2026-11-05',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-11-03'),cases=len(cases),observations=len(cases)*2,instructions=oracle.steps,verify=args.verify,boundaries='41f870/41f920/41fab0 및455930 전체. 4afb40/new28/496e80/4b0d30은 명시 인자/반환 경계. 실제 시전/공격/게임/MP0.',sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe','cpppj/tests/fixtures/bomb-postpop-x86.tsv','tools/decomp_bomb_postpop.py')})
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=1)+'\n',encoding='utf-8')
    print(json.dumps({key:report[key] for key in ('cases','observations','instructions','verify')},ensure_ascii=False))


if __name__=='__main__':
    main()
