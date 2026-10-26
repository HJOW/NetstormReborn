#!/usr/bin/env python3
"""실제0044d870/004bccb0/두SNART함수를 실행하여 빈manager 초기화의 공유 이름표를 관측한다."""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

# 보호 원본 분석 도구를 가져오고 새 준비본 안에만 결과를 쓴다.
STAGE=Path(__file__).resolve().parents[1]
ROOT=STAGE.parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle,SCRATCH,STOP,STACK,MAX_INSTRUCTIONS
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS

# 아래 주소는 에뮬레이터 입력 저장소이며 실제 파일/OS 장치가 아니다.
MANAGER,TABLE,ALTERNATE=SCRATCH+0x1000,SCRATCH+0x2000,SCRATCH+0x3000


class InitializeOracle(Oracle):
    """이름표 생성과 변환·부모 반복만 실제 실행하고 큰stream 생성은 명시장치로 분리한다."""
    def __init__(self):
        """정밀 함수표의 보호 범위와 실제 이름표 getter의 가상 포인터만 사용한다."""
        super().__init__('originals',[dict(name='initialize',originals='0044d870'),dict(name='names',originals='004bccb0')],[0x4a8850,0x4a8800,0x4437f0])
        self.code={hex(lo):bytes(self.mu.mem_read(lo,hi-lo)).hex() for lo,hi in self.allowed}

    def call(self,name,args,this=0,purge=0):
        """공통Oracle와같은고정레지스터/스택을사용하되35회문자열순회의가상CPU시간만5초로늘린다."""
        # 모든일반레지스터는별도호출마다0으로만들고실제cdecl/thiscall스택을그대로검사한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,this)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.mu.emu_start(self.entries[name],STOP,timeout=5000000,count=MAX_INSTRUCTIONS)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP:
            raise RuntimeError(name+': 5초/명령수 내 미반환')
        if self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4+purge:
            raise RuntimeError(name+': 스택복구 불일치')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def put(self,address,value):
        """명시 가상 입력의 DWORD를 little endian으로 쓴다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """현재 같은 가상 DWORD를 다시 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def ret(self,value=0,purge=0):
        """큰외부하위의cdecl반환만 수행하고 상위 부모의 기계어는 계속 실행한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        target=self.get(sp)
        self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)
        self.mu.reg_write(UC_X86_REG_EIP,target)

    def on_instruction(self,mu,address,size,data):
        """할당/서식/stream생성을명시입력으로제공하고 실제부모의순서와BYTE변화를보존한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4e0620:
            line=self.get(sp+12)
            self.trace.append('assert:'+str(line))
            if self.mutation==1 and line==0xc9:
                self.put(0x5c8aa4,ALTERNATE)
            self.ret()
            return
        if address==0x4e4391:
            self.trace.append('allocate:'+str(self.get(sp+4)))
            self.ret(TABLE if self.allocation else 0)
            return
        if address==0x4a89c0:
            destination,format_pointer,index=struct.unpack('<III',mu.mem_read(sp+4,12))
            if self.read_string(format_pointer)!=b'Terr%02d':
                raise RuntimeError('미검토 문자열서식')
            self.trace.append('format:'+str(index))
            mu.mem_write(destination,('Terr%02d'%index).encode('ascii')+b'\0')
            self.ret(destination)
            return
        if address==0x4bccb0:
            self.trace.append('names:'+str(self.get(0x5c8aa4)))
        elif address==0x4a8850:
            self.trace.append('at:'+str(self.get(sp+4))+':'+str(self.get(sp+8)))
        elif address==0x4437f0:
            self.trace.append('baseNoop')
        elif address==0x44d1b0:
            self.creates+=1
            pointer=self.get(sp+4)
            self.trace.append('create:'+str(mu.reg_read(UC_X86_REG_ECX))+':'+str(pointer)+':'+self.read_string(pointer).hex())
            self.put(MANAGER+4,0x10004000)
            self.put(MANAGER+8,0x10004000+self.creates*60)
            if self.creates==1 and self.mutation==2:
                self.put(0x5c8aa4,ALTERNATE)
            if self.creates==1 and self.mutation==3:
                table=bytes(mu.mem_read(TABLE,512))
                for before,after in ((b'State\0',b'Other\0'),(b'Two\0',b'Six\0')):
                    at=table.find(before)
                    if at>=0:
                        mu.mem_write(TABLE+at,after)
            self.ret(0x10004000+self.creates*60,4)
            return
        return super().on_instruction(mu,address,size,data)

    def reset(self,kind,root,mutation,allocation=1):
        """동일manager12와 두512BYTE입력표를 준비하고 Client의 현재 이름표 포인터를 설정한다."""
        self.trace=[]
        self.creates=0
        self.mutation=mutation
        self.allocation=allocation
        self.mu.mem_write(MANAGER,struct.pack('<III',0x514690,root,0xa5a5a5a5))
        self.mu.mem_write(TABLE,bytes([0xa5])*512)
        self.mu.mem_write(ALTERNATE,b'/Alt/Last//\0'+bytes([0xa5])*(512-11))
        inputs={1:b'/One/Two//\0',2:b'\0One\0Two\0\0\xff',3:b'//\0'}
        if kind:
            self.mu.mem_write(TABLE,inputs[kind])
        self.put(0x5c8aa4,TABLE if kind else 0)

    def run(self,kind,root,repeats,mutation,allocation=1,action='initialize'):
        """새Client1회및같은Client반복에서모든이름BYTE/manager/trace와NULL접근을읽는다."""
        self.reset(kind,root,mutation,allocation)
        completed=1
        fault=0
        returns=[]
        try:
            for unused in range(repeats):
                returns.append(self.call(action,[],MANAGER if action=='initialize' else 0))
        except UcError:
            completed=0
            fault=self.mu.reg_read(UC_X86_REG_EIP)
        return (completed,hex(fault),self.get(0x5c8aa4),bytes(self.mu.mem_read(MANAGER,12)).hex(),bytes(self.mu.mem_read(TABLE,512)).hex(),bytes(self.mu.mem_read(ALTERNATE,512)).hex(),';'.join(self.trace))


def main():
    """추가정적관측만생성하거나재실행하고기존NewFort1400행자료에는접근하지않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    oracle=InitializeOracle()
    output={}
    rows=[]
    # 실제신규표·사용전표·사용후표·빈표와진단/반복/외부재진입을교차한다.
    for kind,root,repeats,mutation in itertools.product(range(4),(0,0x12345678),(1,2),range(4)):
        rows.append('\t'.join(map(str,(kind,root,repeats,mutation,*oracle.run(kind,root,repeats,mutation)))))
    output['data-manager-initialize-x86.tsv']=('# kind/root/repeats/mutation/completed/faultPC/currentTable/manager12/table512/alternate512/trace\n'+'\n'.join(rows)+'\n').encode('utf-8')
    rows=[]
    # getter만반복하여512할당의첫한번수명과NULL실패를별도고정한다.
    for kind,allocation,repeats in itertools.product((0,1,2),(0,1),(1,2)):
        rows.append('\t'.join(map(str,(kind,allocation,repeats,*oracle.run(kind,0,repeats,0,allocation,'names')))))
    output['template-section-names-x86.tsv']=('# kind/allocation/repeats/completed/faultPC/currentTable/manager12/table512/alternate512/trace\n'+'\n'.join(rows)+'\n').encode('utf-8')
    destination=STAGE/'fixtures'
    destination.mkdir(exist_ok=True)
    report=dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon / 2026-10-23',originalGameRuns=0,cloneGameRuns=0,osCalls=0,binarySha256=oracle.sha256,toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),nativeRanges=oracle.code,fixtures={},limits=['실제0044d870/004bccb0/004a8850/004a8800/004437f0실행','operatornew/문자열서식/실제stream생성하위는명시장치;C++실장없음','root진단뒤전역교체/첫Create뒤전역교체/캡처된표내용변경관측'])
    # 각새fixture의전체BYTE를검증하며기존파일을덮어쓰지않는다.
    for name,payload in output.items():
        path=destination/name
        if args.verify:
            if path.read_bytes()!=payload:
                raise RuntimeError('초기화추가fixture불일치:'+name)
        else:
            if path.exists():
                raise RuntimeError('초기화추가fixture덮어쓰기금지:'+name)
            path.write_bytes(payload)
        report['fixtures'][name]=dict(rows=len(payload.splitlines())-1,sha256=hashlib.sha256(payload).hexdigest())
    (STAGE/'initialize-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report['fixtures'],ensure_ascii=False))


if __name__=='__main__':
    main()
