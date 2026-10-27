#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 ProcessFinder predicate와 제거 부모를 혼합 Form 목록에서 실제 명령으로 관측한다."""
import bisect
import collections
import csv
import json
import platform
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,digest
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

ENTRIES=(0x4b27a0,0x4b22d0,0x4b1b80,0x4b1bf0,0x4ae2d0,0x4ae340,0x41bff0,0x41bf10,0x471aa0) # Finder/실제 Kernel 조회/타입/권한까지 실행한다.
POOL,HEAP,VTABLE,STOP,DESTROY,STACK=0x12000000,0x11330000,0x11320000,0x20000000,0x20001000,0x3003f000 # 실제 프로세스와 연결하지 않는 입력/종료 경계다.
PATTERNS=((),(0,1,2,3,6,9),(63,64,69,70,100,158,255),(10,11,39,46,61,62),(0,1,2,3,6,9,10,39,46,61,62,63,69,70,100,158,255),(255,158,100,70,69,63,62,61,46,39,10,9,6,3,2,1,0),(3,39,6,62,9,10,70,61),(9,10,62,63)) # 낮은 Form/실제 상한/자산과 두 순서를 구별한다.
REQUESTS=(0,10,39,62,63,69,70,0xffffffff) #0은 전체이며 나머지는 원문 정확 타입 비교다.
FIXTURE=ROOT/'cpppj/tests/fixtures/process-removal-filter-x86.tsv' # 기존 입력/검사와 분리된 최초 원문 결과다.
REPORT=ROOT/'cpppj/process-removal-filter-native-20261027.json' # 실제 실행/명시 경계/보호 파일 식별자다.


class RemovalOracle:
    """가상 Form 삭제 효과만 경계이며 탐색/필터/조회/권한은 보호 PE로 실행한다."""
    def __init__(self):
        """현재 정밀 표의 지정 함수와 실제425e80 thunk8바이트만 허용한다."""
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));self.base=pe.OPTIONAL_HEADER.ImageBase
        image=pe.get_memory_mapped_image();self.mu.mem_map(self.base,(len(image)+4095)&~4095);self.mu.mem_write(self.base,image)
        for address,size in ((POOL,0x200000),(HEAP,0x10000),(VTABLE,0x1000),(STOP,0x2000),(0x30000000,0x40000),(0,0x1000)): # 격리된 스택/입력/콜백 주소만 확보한다.
            self.mu.mem_map(address,size)
        predicate=struct.unpack('<I',pe.get_data(0x503ac4-self.base,4))[0];end=struct.unpack('<I',pe.get_data(0x54116c-self.base,4))[0]
        thunk=pe.get_data(predicate-self.base,8)
        assert predicate==0x425e80 and end==63 and thunk[0]==0xe8 and thunk[-3:]==b'\xc2\x04\x00' and predicate+5+struct.unpack('<i',thunk[1:5])[0]==0x4b1bf0
        self.ranges=[];found=set()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'): # 현재 보호 함수 몸체 범위만 실행한다.
                entry=int(row['entry'],16)
                if entry in ENTRIES:
                    found.add(entry)
                    for part in row['ranges'].split(';'): # 분리된 함수 조각도 현재 정밀 표를 따른다.
                        low,high=(int(v,16) for v in part.split('-'));self.ranges.append((low,high+1))
        assert found==set(ENTRIES);self.ranges.append((predicate,predicate+8));self.ranges.sort();self.starts=[r[0] for r in self.ranges]
        self.calls=collections.Counter();self.instructions=0;self.events=[];self.nodes=();self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def put(self,address,value,width=4):
        """명시 입력을 little endian으로 공급한다."""
        self.mu.mem_write(address,int(value).to_bytes(width,'little',signed=False))

    def read(self,address,width=4):
        """현재 원문 인자/필드만 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def write(self,mu,access,address,size,value,data):
        """원문은 로컬 Finder/스택 외에 상태를 쓰지 않아야 한다."""
        if 0x30000000<=address and address+size<=0x30040000:
            return
        if address==0 and size==4:
            return
        raise RuntimeError(f'제거 부모의 미허용 쓰기 {address:08x}+{size}')

    def instruction(self,mu,address,size,data):
        """실제 가상 Form 삭제 진입의 선택 순번/flags만 기록한다."""
        self.instructions+=1
        if address==DESTROY:
            raw=mu.reg_read(UC_X86_REG_ECX);sid=(raw-POOL)//50;index=sid-60;sp=mu.reg_read(UC_X86_REG_ESP);flags=self.read(sp+4)
            assert raw==POOL+sid*50 and 0<=index<len(self.nodes) and flags==0x40
            self.events.append(f'{index}:{flags}');mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EIP,self.read(sp));mu.reg_write(UC_X86_REG_ESP,sp+8);return
        if address==0x4e0620:
            raise RuntimeError('정상 혼합 목록의 원문 assert 도달')
        index=bisect.bisect_right(self.starts,address)-1
        if index<0 or address+size>self.ranges[index][1]:
            raise RuntimeError(f'제거 부모의 미허용 실행 {address:08x}')
        if address in ENTRIES or address==0x425e80:
            self.calls[hex(address)]+=1

    def run(self,nodes,request,precision):
        """현재 목록/실제63상한/PID표/권한과 두 정밀도만 입력으로 제공한다."""
        self.nodes=nodes;self.events=[];mu=self.mu;mu.mem_write(POOL,bytes(128*50));mu.mem_write(HEAP,bytes(0x10000));mu.mem_write(VTABLE,bytes(0x1000))
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x54116c,63);self.put(0x5e4794,0);self.put(0x540bc4,0);self.put(0x594fc8,0);self.put(POOL+50*50+6,60 if nodes else 0,2);self.put(VTABLE+0x10,DESTROY)
        for index,kind in enumerate(nodes): # 각 후보의 다음/타입/PID와 같은 Kernel의 현재 포인터 표다.
            sid=60+index;raw=POOL+sid*50;pointer=HEAP+index*32;pid=0x100+index
            self.put(raw,VTABLE);self.put(raw+4,sid+1 if index+1<len(nodes) else 0,2);self.put(raw+10,kind,1);self.put(raw+18,pid);self.put(pointer+4,sid);self.put(0x56b808+pid*4,pointer)
        preserved=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)
        for index,reg in enumerate(preserved): # 정상 callee의 보존 GP 계약을 확인한다.
            mu.reg_write(reg,0x12340000+index)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_ECX,0);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,precision);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<3I',STOP,50,request));mu.emu_start(0x4b27a0,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4 and not(mu.reg_read(UC_X86_REG_FPSW)&0x3800)
        assert all(mu.reg_read(reg)==0x12340000+index for index,reg in enumerate(preserved))
        return ';'.join(self.events) or '-'


def main():
    """동일 입력의 실제 두 정밀도가 같을 때만 새64행을 보존한다."""
    if REPORT.exists():
        raise FileExistsError('최초 제거 predicate 자료를 덮어쓰지 않는다')
    recovering=FIXTURE.exists();oracle=RemovalOracle();rows=[] # 최초 fixture 뒤 메타데이터 실패면 같은 원문을 재관측해 바이트 동일성만 확인한다.
    for nodes in PATTERNS: # 혼합 목록의 후보 선택을 C++에서 계산하지 않는다.
        for request in REQUESTS: # 각 원문 타입 선택 조건을 교차한다.
            first=oracle.run(nodes,request,0x027f);second=oracle.run(nodes,request,0x037f);assert first==second;rows.append([','.join(map(str,nodes)) or '-',request,first])
    text='# 원문4b27a0/ProcessFinder predicate. types requested index:flags trace. 가상Form삭제만명시경계.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n'
    if recovering:
        assert FIXTURE.read_bytes()==text.encode('utf-8')
    else:
        with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(text)
    files=[Path(__file__).resolve(),ROOT/'tools/decomp_owner_oracle.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',FIXTURE]
    report=dict(host=platform.node(),cases=len(rows),runs=len(rows)*2,normalReturns=len(rows)*2,precisionDifferences=0,processTypeEnd=63,predicate='503ac4→425e80→4b1bf0',executedBodies=[hex(x) for x in ENTRIES]+['0x425e80'],boundaries=['현재 가상 Form 삭제의 순번/flags 관측. 실제 파괴·Kernel 반납/목록 변이는 이 관측에서 실행하지 않는다.'],originalGameExecutions=0,recoveredExistingFixture=recovering,calls=dict(oracle.calls),instructions=oracle.instructions,sha256={p.relative_to(ROOT).as_posix():digest(p) for p in files})
    with REPORT.open('x',encoding='utf-8',newline='\n') as stream:
        json.dump(report,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps({k:v for k,v in report.items() if k!='sha256'},ensure_ascii=False))


if __name__=='__main__':
    main()
