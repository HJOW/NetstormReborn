#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 Regular 번호/프로세스 상한의 실제 finder와 두 생성자 전체를 격리 관찰한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_ESP

ROOT=Path(__file__).resolve().parents[1]  # 보호 PE/정밀 주소/독립 fixture의 기준이다.
POOL,STACK,STOP,OBJECTS=0x12000000,0x20000800,0x30000000,0x31000000  # 외부 OS/게임과 분리한 물리다.
FIND_FIXTURE=ROOT/'cpppj/tests/fixtures/regular-current-find-x86.tsv'  # 실제 탐색 반환/Kernel조회 순서다.
CTOR_FIXTURE=ROOT/'cpppj/tests/fixtures/regular-current-constructor-x86.tsv'  # 실제 생성자의 진단/부착번호/시각 저장이다.


class Oracle:
    """실제 종속 iterator/분류/Kernel getter까지 실행하고 생성자의 진단/부착만 경계로 둔다."""

    def __init__(self):
        """코드 패치 없이 PE/FS/스택/부모와 세 프로세스를 매핑한다."""
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
        image=pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000,(len(image)+4095)&~4095)
        self.mu.mem_write(0x400000,image)
        for address in (0,POOL,STACK&~4095,STOP,OBJECTS):  # FS는 격리한 정상 복귀 입력이다.
            self.mu.mem_map(address,4096)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[(0x425e80,0x425e88)]  # PE503ac4의 실제 predicate thunk는 4b1bf0→ret4다.
        for entry in (0x4afb40,0x4afbe0,0x4b22d0,0x4b2340,0x4b1b80,0x4b1bf0,0x471aa0,0x496e80,0x496f00):
            for part in rows[entry]['ranges'].split(';'):  # 끝 명령 바이트도 허용한다.
                low,high=(int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        self.put(0x5c8464,POOL)
        self.put(0x5e4794,0)
        self.put(0x594fc8,0)
        self.put(0x540bc4,1)
        self.context=self.mu.context_save()
        self.visits,self.calls,self.steps,self.mutate,self.attachTime=[],[],0,0,0
        self.missingPidMode,self.invalidReads=False,[]  # 정상 검색과 미등록 PID의 null 필드 읽기를 분리한다.
        self.mu.hook_add(UC_HOOK_CODE,self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def put(self,address,value,width=4):
        """원문 DWORD/WORD 폭으로 격리 입력을 기록한다."""
        self.mu.mem_write(address,int(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def get(self,address):
        """원문의 비정렬 DWORD를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,4),'little')

    def ret(self,clean,value=0):
        """부착/진단 명시 경계만 원문 호출 규약으로 복귀한다."""
        stack=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_ESP,stack+4+clean)
        self.mu.reg_write(UC_X86_REG_EIP,self.get(stack))

    def instruction(self,mu,address,size,data):
        """실제 Kernel getter의 인자와 생성자의 부착/진단 순서를 기록한다."""
        self.steps+=1
        stack=mu.reg_read(UC_X86_REG_ESP)
        if address==0x471aa0:
            self.visits.append(self.get(stack+4))  # getter 본체도 실제로 실행한다.
            if self.missingPidMode:
                self.put(0x56b808+4*self.get(stack+4),0)  # 실제 getter 진입 직전 현재 슬롯을 미등록으로 바꾼다.
        if address==0x41bd20:
            assert mu.reg_read(UC_X86_REG_ECX)==OBJECTS
            self.calls.append('attach:'+':'.join(str(self.get(stack+i)) for i in (4,8,12,16)))
            if self.attachTime:
                mu.mem_write(0x55b4d0,struct.pack('<d',33.25))
            self.ret(16,OBJECTS)
            return
        if address==0x4e0620:
            assert self.get(stack+12)==0x70
            self.calls.append('diagnostic:112')
            if self.mutate:
                self.put(0x541164,48)
            self.ret(0)
            return
        assert any(low<=address<high for low,high in self.allowed),hex(address)

    def write(self,mu,access,address,size,value,data):
        """함수 실행의 쓰기는 격리 스택/FS와 생성 객체의40BYTE로 제한한다."""
        assert (STACK-512<=address and address+size<=STACK+32) or (address==0 and size==4) or (OBJECTS<=address and address+size<=OBJECTS+40),(hex(address),size)

    def read(self,mu,access,address,size,value,data):
        """격리 FS 페이지 때문에 실제 OS의 null 접근이 정상 데이터처럼 보이지 않게 기록하고 중단한다."""
        if self.missingPidMode and 0<address<4096:
            self.invalidReads.append(dict(instruction=f'{mu.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size))
            mu.emu_stop()

    def missing_pid(self):
        """현재 예약의 미등록 PID를 원문이 빈 검색으로 건너뛰지 않음을 실제 읽기 위치로 증명한다."""
        self.missingPidMode,self.invalidReads=True,[]
        try:
            self.find((0,46,61,63,46,61,47))
        except AssertionError:
            pass  # 정상 반환 이전 중단이며 아래에서 정확한 원문 읽기를 요구한다.
        finally:
            self.missingPidMode=False
        assert self.invalidReads==[dict(instruction='004afbb1',address='00000018',size=4)]
        return self.invalidReads

    def prepare(self,args,this):
        """각 호출에서 스택/SEH/비휘발 레지스터를 새로 복원한다."""
        self.mu.context_restore(self.context)
        self.put(0,0x87654321)
        self.mu.mem_write(STACK-512,b'\xa5'*544)
        self.mu.mem_write(STACK,struct.pack('<I',STOP)+args)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,this)
        self.saved={register:0x11223344+i for i,register in enumerate((UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI))}
        for register,value in self.saved.items():  # thiscall의 비휘발 레지스터다.
            self.mu.reg_write(register,value)
        self.visits,self.calls=[],[]

    def finish(self,clean):
        """반환 위치/스택/SEH/레지스터와 호출 영역 밖 canary를 검증한다."""
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+clean
        assert self.get(0)==0x87654321 and all(self.mu.reg_read(r)==v for r,v in self.saved.items())
        assert bytes(self.mu.mem_read(STACK-512,16))==b'\xa5'*16
        assert bytes(self.mu.mem_read(STACK+4+clean,28-clean))==b'\xa5'*(28-clean)

    def find(self,case):
        """두 실제 finder가 같은 연결 목록을 첫 적합 항목까지만 방문한다."""
        mode,regular,shared,end,*types=case
        args=struct.pack('<2I',0xfff0,0x5100) if mode else struct.pack('<I',0x5102)
        self.prepare(args,POOL+2500)
        self.put(0x541128,regular)
        self.put(0x541164,shared)
        self.put(0x54116c,end)
        self.mu.mem_write(POOL,bytes(4096))
        self.put(POOL+2500+6,60,2)
        for index,kind in enumerate(types):  # parent50→60→61→62이며 자산/일반 Form도 같은 목록에 둔다.
            sid=60+index
            self.put(POOL+sid*50+4,sid+1 if index<2 else 0,2)
            self.put(POOL+sid*50+10,kind,1)
            self.put(POOL+sid*50+18,index+1)
            pointer=OBJECTS+index*40
            self.put(0x56b808+4*(index+1),pointer)
            self.put(pointer+0x18,(0x5101,0x5102,0x5203)[index])
        before=bytes(self.mu.mem_read(POOL,4096))
        self.mu.emu_start(0x4afbe0 if mode else 0x4afb40,STOP,count=2000)
        self.finish(len(args))
        assert bytes(self.mu.mem_read(POOL,4096))==before
        result=self.mu.reg_read(UC_X86_REG_EAX)
        return (0 if not result else (result-OBJECTS)//40+1),','.join(map(str,self.visits)) or '-'

    def constructor(self,case):
        """Shared 진단 뒤의 현재 TYPE와 부착 뒤의 현재 double 시각을 관찰한다."""
        shared,kind,depth,self.mutate,self.attachTime=case
        self.prepare(struct.pack('<3I',0x5102,50,0x3d4ccccd),OBJECTS)
        self.put(0x541128,kind)
        self.put(0x541164,kind)
        self.put(0x5c8488,depth)
        self.mu.mem_write(0x55b4d0,struct.pack('<d',17.5))
        self.mu.mem_write(OBJECTS,b'\x5a'*40)
        self.mu.emu_start(0x496f00 if shared else 0x496e80,STOP,count=500)
        self.finish(12)
        assert self.mu.reg_read(UC_X86_REG_EAX)==OBJECTS
        return '|'.join(self.calls),self.get(OBJECTS+0x18),self.get(OBJECTS+0x1c),self.get(OBJECTS+0x20),struct.unpack('<d',self.mu.mem_read(OBJECTS+0x10,8))[0]


def main():
    """새 원문 생성과 동일 원문 재관측을 분리한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    oracle=Oracle()
    pairs=((46,61),(47,62),(46,46),(302,61),(0xffffffff,61),(74,61),(5,69),(10,255))  # BYTE 절삭 금지/중복/일반 Form/자산 분류를 교차한다.
    ends=(0,10,46,47,61,62,63,70,100,0x80000000,0xffffffff)  # 원문의 signed 경계다.
    types=((46,61,47),(47,46,61),(74,69,255),(5,10,62),(0,9,69),(70,63,61),(255,255,255))  # 같은 연결 순서의 세 TYPE BYTE다.
    find_cases=[(mode,*pair,end,*kinds) for mode,pair,end,kinds in itertools.product((0,1),pairs,ends,types)]
    ctor_cases=list(itertools.product((0,1),(46,47,61,62),(0,1),(0,1),(0,1)))
    outputs=[]  # 입력의 기대값은 Python 복원식으로 계산하지 않는다.
    for path,cases,observe,header in ((FIND_FIXTURE,find_cases,oracle.find,'mode regular shared end type1 type2 type3 result visits'),(CTOR_FIXTURE,ctor_cases,oracle.constructor,'shared kind depth mutation attachTime calls event payload count time')):
        lines=['\t'.join(map(str,(*case,*observe(case)))) for case in cases]
        content='# '+header+'\n'+'\n'.join(lines)+'\n'
        if args.verify:
            assert path.read_text(encoding='utf-8')==content
        else:
            assert not path.exists()
            path.write_text(content,encoding='utf-8',newline='\n')
        outputs.append(dict(path=path.relative_to(ROOT).as_posix(),cases=len(cases),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    missing=oracle.missing_pid()  # 정상 입력과 별도로 null+18 읽기 직전만 관찰한다.
    report=dict(host=socket.gethostname(),date='2026-11-05',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-11-03'),fixtures=outputs,nativeMissingPidRead=missing,instructions=oracle.steps,verify=args.verify,boundaries='4afb40/4afbe0/iterator3/4b1bf0/predicateThunk/471aa0 및496e80/496f00 전체. 생성자41bd20/4e0620만 인자/정상복귀/현재변경 명시 경계다. 미등록PID는실제4afbb1/null+18읽기직전중단한다. 게임/MP/시전/포획/공중3종0.',sourceSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),originalSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest())
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=1)+'\n',encoding='utf-8')
    print(json.dumps(dict(fixtures=outputs,instructions=oracle.steps,verify=args.verify)))


if __name__=='__main__':
    main()
