#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CD 프로세스 요약의 원문 슬롯/이름/부모 캐시와 NULL 문자열 CRT를 창 없이 대조한다."""
import csv
import hashlib
import json
import socket
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent  # 현재 완료된 정밀 산출물/보호 실행 파일만 읽는 저장소 루트다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW)

POOL, TYPES, HEAP = 0x12000000, 0x11100000, 0x11010000  # 호스트 주소와 무관한 CD36바이트 슬롯/468바이트 TYPE/입력 객체 영역이다.
STACK, STOP = 0x30030000, 0x20000000  # 원문 cdecl 복귀/스택 균형을 검사할 가상 주소다.
TABLE = 0x5b7bb0  # CD의 실제4000개 Kernel 포인터 표이며0은 순회하지 않는다.
WORK = ROOT / 'extracted/cpp-resume-974'  # 이번 정밀 대조의 PC 전용 원시와 준비 이력을 보관한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/cd-process-summary-x86.tsv'  # 기존 PATCH fixture를 덮어쓰지 않는 CD 독립 결과다.


class CdSummaryOracle:
    """현재 canonical 정밀 몸체만 허용하고 파일 CRT 경계 세 곳만 사건으로 대체한다."""
    def __init__(self):
        """보호PE/가상 메모리와 현재 호스트의 정밀 함수 범위를 설치한다."""
        binary = ROOT / 'originalCD/Netstorm.exe'
        self.sha256 = hashlib.sha256(binary.read_bytes()).hexdigest()  # 원문 실행 바이트의 불변 증거다.
        pe = pefile.PE(str(binary)); self.base = pe.OPTIONAL_HEADER.ImageBase  # 보호PE의 실제 로드 기준이다.
        image = pe.get_memory_mapped_image()
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)  # 게임/OS를 실행하지 않는 단일 CPU다.
        self.mu.mem_map(self.base, (len(image)+4095)&~4095); self.mu.mem_write(self.base, image)
        for address, size in ((0,0x1000),(POOL,0x100000),(TYPES,0x40000),(HEAP,0x10000),(0x30000000,0x40000),(STOP,0x1000)):  # 원문의 명시 입력/스택 영역만 분리해 확보한다.
            self.mu.mem_map(address,size)
        self.allowed = bytearray(len(image))  # 실제 디컴파일 몸체 밖의 명령은 실행하지 않는다.
        self.summary_allowed = bytearray(len(image))  # 요약은 세 함수만 허용하며 CRT 전체 허용과 구별한다.
        with (ROOT/'extracted/refined/originalCD/functions.tsv').open(encoding='utf-8') as source:
            for row in csv.DictReader(source,delimiter='\t'):  # 현재 완료된 정밀 범위만 읽고 옛 bridgedecay 산출물을 요구하지 않는다.
                for part in row['ranges'].split(';'):  # 불연속 함수 몸체의 실제 바이트 범위를 보존한다.
                    low,high=(int(value,16) for value in part.split('-'));a,b=low-self.base,high+1-self.base
                    self.allowed[a:b]=b'\1'*(b-a)
                    if int(row['entry'],16) in (0x4ed400,0x48f8b0,0x443400):self.summary_allowed[a:b]=b'\1'*(b-a)
        self.events = []  # 실제 CRT 경계에 도달한 출력 바이트/인자만 기록한다.
        self.summary = False  # CRT 실제 실행과 요약의 명시 파일 경계를 구별한다.
        self.instructions = 0  # 각 원문 호출이 유한하게 복귀했는지 검사한다.
        self.null_text = b''  # 실제 CD CRT 관측 뒤에만 파일 경계의 NULL %s 표현에 사용한다.
        self.mu.hook_add(UC_HOOK_CODE,self.on_instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)
        self.number(0x5395dc,POOL);self.number(0x51c960,TYPES)

    def number(self,address,value=None,width=4):
        """little endian 명시 입력/현재 필드를 호스트 정수로만 읽고 쓴다."""
        if value is None:return int.from_bytes(self.mu.mem_read(address,width),'little')
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def text(self,address):
        """실제 문자열의 첫NUL까지만 읽으며 NULL을 빈 문자열로 대신하지 않는다."""
        if not address:raise RuntimeError('명시 문자열 주소 NULL')
        result=bytearray()
        while (value:=self.number(address+len(result),width=1))!=0:result.append(value)  # 게임 이름/CRT 출력의 실제 끝만 읽는다.
        return bytes(result)

    def pointer(self,handle):
        """불투명 입력 번호를 실제 원문 +4/+8 필드가 있는 가상 객체로 바꾼다."""
        return HEAP+0x100+handle*0x20 if handle else 0

    def returned(self,value=0):
        """파일 CRT의 cdecl 반환만 대체하고 인자 정리는 호출 원문에 맡긴다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);target=self.number(sp)
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4);self.mu.reg_write(UC_X86_REG_EIP,target)

    def on_write(self,_mu,_access,address,size,_value,_data):
        """원문의 CPU 쓰기를 실제 스택/출력 버퍼/FS 목록으로 제한한다."""
        if 0x30000000<=address and address+size<=0x30040000:return
        if HEAP<=address and address+size<=HEAP+0x10000:return
        if address==0 and size==4:return
        raise RuntimeError(f'CD 요약 허용 밖 쓰기 {address:08x}+{size}')

    def mutate(self):
        """첫 파일 출력 뒤의 명시 외부 효과만 다음 현재 조회에 반영한다."""
        mode=self.mutation
        if mode==1:self.number(TABLE+8,0)
        elif mode==2:self.number(TABLE+400,self.pointer(4))
        elif mode==3:self.number(self.pointer(2)+8,0)
        elif mode==4:self.number(POOL+51*36+10,54,1)
        elif mode==5:self.number(POOL+61*36+10,157,1)
        elif mode==6:self.number(TABLE+8,self.pointer(3))
        elif mode==7:self.number(TABLE+4,self.pointer(4))
        elif mode==8:self.number(POOL+50*36+10,61,1)
        elif mode==13:self.number(TABLE+3999*4,0)
        elif mode==14:self.number(TABLE+12,self.pointer(4))

    def on_instruction(self,mu,address,_size,_data):
        """요약의 세 함수와 CRT 실제 몸체를 구별하고 원문 소비 시점에만 외부 변형을 적용한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP)
        if self.summary:
            if address==0x4f2000:
                self.events.append(f'O:{self.text(self.number(sp+4)).hex()}:{self.text(self.number(sp+8)).hex()}:{self.file}');self.returned(self.file);return
            if address==0x4f28a0:
                stream,fmt,name_pointer=self.number(sp+4),self.text(self.number(sp+8)),self.number(sp+12)
                name=self.text(name_pointer) if name_pointer else self.null_text
                if fmt==b'pType=%s sid=0, attachToType=NULL\n':line=fmt%name
                else:
                    assert fmt==b'pType=%s sid=%d, attachToType=%s\n'
                    parent=self.number(sp+16);parent=parent if parent<0x80000000 else parent-0x100000000
                    line=fmt%(name,parent,self.text(self.number(sp+20)))
                self.events.append(f'W:{stream}:{line.hex()}');self.lines+=1
                if self.lines==1:self.mutate()
                self.returned(len(line));return
            if address==0x4f1f20:self.events.append(f'C:{self.number(sp+4)}');self.returned();return
            if address==0x443400:
                self.name_calls+=1
                if self.name_calls==1 and self.mutation==9:self.number(TABLE+4,self.pointer(2))
            if address==0x4ed43c and not self.parent_mutated and self.mutation in (10,11):
                self.parent_mutated=True
                if self.mutation==10:self.number(mu.reg_read(UC_X86_REG_ECX)+8,61)
                else:self.number(TABLE+4,self.pointer(2))
        allowed=self.summary_allowed if self.summary else self.allowed
        if not self.base<=address<self.base+len(allowed) or not allowed[address-self.base]:raise RuntimeError(f'CD 허용 밖 명령 {address:08x}')

    def call(self,entry,args=(),control=0x27f):
        """명시 cdecl 스택에서 원문 정상 복귀와 정리 균형을 검사한다."""
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 이전 호출의 흔적을 게임 입력으로 사용하지 않는다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.events=[];self.instructions=0
        self.mu.emu_start(entry,STOP,count=4000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        return self.mu.reg_read(UC_X86_REG_EAX)

    def probe_null(self):
        """fprintf와 같은004f6c60을 호출하는 실제 CD vsprintf004f4920에서 NULL %s를 관측한다."""
        self.summary=False;self.mu.mem_write(HEAP+0x1000,b'%s\0');self.mu.mem_write(HEAP+0x2000,b'x'*32+b'\0')
        self.number(HEAP+0x3000,0);self.call(0x4f4920,(HEAP+0x2000,HEAP+0x1000,HEAP+0x3000));self.null_text=self.text(HEAP+0x2000)
        return self.null_text

    def sample(self,scenario,mutation,file,control):
        """CD의 실제36/468바이트 입력만 만들고 출력/조회는 원문에 맡긴다."""
        self.summary=True;self.file=file;self.mutation=mutation;self.lines=0;self.name_calls=0;self.parent_mutated=False  # 현재 파일 토큰/선택 변형/원문 호출 횟수다.
        self.mu.mem_write(TABLE,bytes(4001*4));self.mu.mem_write(POOL,bytes(24000*36));self.mu.mem_write(TYPES,bytes(171*468))
        for kind,name in ((82,b'bridge'),(94,b'island'),(157,b'aLongParentNameWith%and\tText'),(0,b'none\0ignored')):self.mu.mem_write(TYPES+kind*468+4,name+b'\0')  #20글자 축약이나 서식 재해석을 적용하지 않는다.
        for sid,kind in ((60,82),(61,94),(62,157),(63,0)):self.number(POOL+sid*36+10,kind,1)  # 부모 free/dead를 별도 필터로 만들지 않는다.
        for handle,kind,parent in ((1,46,60),(2,10,61),(3,55,0),(4,54,62),(5,0,63)):  # 원문 프로세스의 두 필드와 Form TYPE만 명시한다.
            self.number(self.pointer(handle)+4,49+handle);self.number(self.pointer(handle)+8,parent);self.number(POOL+(49+handle)*36+10,kind,1)
        slots={0:5,4000:5}
        if scenario==1:slots[1]=1;self.number(self.pointer(1)+8,0);self.number(POOL+50*36+11,1,1)
        elif scenario==2:slots.update({1:1,2:2,3:3,3999:5})
        elif scenario==3:
            for kind in range(62):  # CD의 NULL9까지 포함한 전체62개 포인터 이름을 실제 조회한다.
                handle=10+kind;sid=100+kind;slots[kind+1]=handle;self.number(self.pointer(handle)+4,sid);self.number(self.pointer(handle)+8,(0,60,61,62,63)[kind%5]);self.number(POOL+sid*36+10,kind,1)
        elif scenario==4:slots.update({1:3,3998:1,3999:2})
        for index,handle in slots.items():self.number(TABLE+index*4,self.pointer(handle))  #0/4000 sentinel을 읽는 구현은 원문 결과와 달라진다.
        self.call(0x4ed400,control=control)
        return ';'.join(self.events)


def main():
    """같은 입력을 두 x87 제어값에서 동결하고 기존 fixture와 다른 재생성은 거부한다."""
    WORK.mkdir(parents=True,exist_ok=True);oracle=CdSummaryOracle();null=oracle.probe_null()
    cases=[(scenario,0,700) for scenario in range(5)]+[(0,0,0),(2,0,0)]+[(2,mutation,700) for mutation in (*range(1,12),13,14)]
    rows=['# scenario\tmutation\tfile => CD 실제 CRT 호출/바이트 기록']
    for case in cases:  # 계산한 제품 기대값 없이 실제 두 원문 관측을 비교한다.
        a,b=(oracle.sample(*case,control) for control in (0x27f,0x37f));assert a==b;rows.append('\t'.join(map(str,(*case,a))))
    content=('\n'.join(rows)+'\n').encode('utf-8')
    if FIXTURE.exists():assert FIXTURE.read_bytes()==content,'기존 CD fixture와 원문 재관측 불일치'
    else:FIXTURE.write_bytes(content)
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-11-03',binarySha256=oracle.sha256,uniqueInputs=len(cases),nativeObservations=2*len(cases),nullCrtHex=null.hex(),nullCrtText=null.decode('ascii'),fixtureSha256=hashlib.sha256(content).hexdigest(),boundary='CD004ed400/0048f8b0/00443400 원문; 파일 CRT만 경계 대체. NULL 문자열은004f4920/004f6c60 실제 CRT 관측.')
    (WORK/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':main()
