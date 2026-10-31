#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""별도 두 TAFF 목록의 실제 등록·생성·중복·실패 해제를 제품 변경 전에 관측한다."""
import csv
import json
import socket
import struct
import subprocess
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

# 원문 두 단일 연결 목록과 최초 관측 산출물의 고정 위치다.
HEADS=(0x5450b8,0x544f80)
FIXTURE=ROOT/'cpppj/tests/fixtures/taff-registration-x86.tsv'
REPORT=ROOT/'extracted/cpp-taff-registration-20261031-native.json'


def archive(kind):
    """원본 TAFF 헤더/인덱스/디렉터리에 명시 입력을 구성하며 성공 여부를 계산하지 않는다."""
    if kind=='missing':
        return None
    data=bytearray(0x56)
    data[:10]=b'TAFF v0.2\x1a'
    for offset,value in ((0x14,2),(0x1c,0x2c),(0x20,0x34),(0x24,0x20),(0x28,0x54),(0x2c,0x10),(0x30,0),(0x34,0),(0x38,1),(0x44,1),(0x48,1)):  # 기존 복원 검사의 비순차 이름 인덱스와 같은 입력 형식이다.
        struct.pack_into('<I',data,offset,value)
    data[0x3c:0x40]=b'\\d\\a';data[0x4c:0x50]=b'\\d\\b'
    data[0x54:0x56]=b'AB' if kind!='other' else b'XY'
    if kind=='bad-magic':
        data[0]=0
    elif kind=='bad-nul':
        data[10]=65
    elif kind=='short-header':
        data=data[:20]
    elif kind=='short-index':
        data=data[:0x30]
    elif kind=='short-directory':
        data=data[:0x50]
    return bytes(data)


class Oracle(MemoryOracle):
    """목록/생성자/정리 몸체는 실제 CPU로 실행하고 CRT 파일·힙만 격리 장치로 공급한다."""
    def __init__(self):
        """보호 PE와 실제 정밀 범위를 읽고 SEH 슬롯만 별도로 준비한다."""
        super().__init__();self.mu.mem_map(0,0x1000)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as file:
            facts={int(row['entry'],16):row for row in csv.DictReader(file,delimiter='\t')}
        self.allowed=[]
        for entry in (0x41bc20,0x41bc30,0x41b9d0,0x41b040,0x41b230,0x41a200):  # 원문 등록 wrapper와 실제 자식만 실행한다.
            for part in facts[entry]['ranges'].split(';'):  # 떨어진 함수 범위도 표 그대로 허용한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """문자열·FILE·할당만 명시 경계로 대체하며 원문 성공 판정과 연결 순서는 계산하지 않는다."""
        self.instructions+=1
        if self.instructions>10000:
            raise RuntimeError('TAFF 등록 원문 명령 상한')
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,5)
        if address==0x4e4391:
            pointer=self.heap;self.heap+=(max(args[0],1)+31)&~31
            if self.heap>=POOL+0xf0000:
                raise RuntimeError('TAFF 명시 힙 물리 상한')
            mu.mem_write(pointer,bytes(max(args[0],1)));self.events.append('N:'+str(args[0]));self.ret(pointer)
        elif address==0x4e4354:
            self.events.append('F');self.ret()
        elif address==0x4e65ed:
            left,right=self.string(args[0]).lower(),self.string(args[1]).lower();self.ret((left>right)-(left<right))
        elif address==0x4e51f0:
            source=self.string(args[1]);result=source[:args[2]];result+=bytes(args[2]-len(result));mu.mem_write(args[0],result);self.ret(args[0])
        elif address==0x4e545a:
            name=self.string(args[0]).decode('ascii');self.events.append('O:'+name)
            content=self.files.get(name.lower())
            if content is None:
                self.ret(0)
            else:
                self.opened=[content,0];self.ret(0x1a000000)
        elif address==0x4e54b9:
            self.events.append('C');self.ret(0)
        elif address==0x4e573c:
            assert args[0]==0x1a000000 and args[2]==0
            position=struct.unpack('<i',struct.pack('<I',args[1]))[0]
            if position<0:
                self.ret(0xffffffff)
            else:
                self.opened[1]=position;self.ret(0)
        elif address==0x4e586e:
            assert args[3]==0x1a000000
            content,position=self.opened;chunk=content[position:position+args[1]*args[2]]
            if chunk:
                mu.mem_write(args[0],chunk)
            self.opened[1]+=len(chunk);self.ret(len(chunk)//args[1] if args[1] else 0)
        elif address==0x4e58e9:
            assert self.string(args[1])==b'TAFF v%d.%d\x1a' and args[2:4]==(0,2)
            value=b'TAFF v0.2\x1a';mu.mem_write(args[0],value+b'\0');self.ret(len(value))
        elif not any(low<=address<high for low,high in self.allowed):
            raise RuntimeError('TAFF 등록 범위 밖 명령:'+hex(address))

    def names(self,head):
        """실제 연결 목록을 읽으며 관측에 없는 재정렬을 추가하지 않는다."""
        pointer=self.words(head,1)[0];result=[]
        while pointer:  # 원문 next DWORD와 원래 등록 이름을 그대로 따라간다.
            if len(result)>100:
                raise RuntimeError('TAFF 목록 순환')
            result.append(self.string(pointer+4).decode('ascii'));pointer=self.words(pointer,1)[0]
        return ';'.join(result)

    def sample(self,requests):
        """요청별 입력 파일 교체 후 원문 반환/목록/파일·힙 효과만 고정한다."""
        self.files={};self.heap=POOL+0x10000;self.events=[];self.instructions=0;returns=[]
        for head in HEADS:  # 두 원문 목록은 같은 파일 이름도 독립적으로 소유한다.
            self.put(head,0)
        self.put(0,0)
        for pool,name,kind in requests:  # 뒤의 같은 이름 입력 변경이 기존 등록을 바꾸는지도 관측한다.
            self.files[name.lower()]=archive(kind);mu=self.mu
            mu.mem_write(TEXT,name.encode('ascii')+b'\0');mu.mem_write(STACK-4096,bytes(4128))
            mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,0)
            mu.mem_write(STACK,struct.pack('<II',STOP,TEXT));mu.emu_start(0x41bc30 if pool else 0x41bc20,STOP)
            if mu.reg_read(UC_X86_REG_ESP)!=STACK+4:
                raise RuntimeError('TAFF 등록 원문 스택 복귀')
            from unicorn.x86_const import UC_X86_REG_EAX
            returns.append(str(mu.reg_read(UC_X86_REG_EAX)))
        return '\t'.join((';'.join(','.join(map(str,item)) for item in requests),','.join(returns),self.names(HEADS[0]),self.names(HEADS[1]),';'.join(self.events)))


def main():
    """기존 BaseFile 제품이 수정되기 전에만 최초 자료를 생성한다."""
    assert not FIXTURE.exists() and not REPORT.exists(),'최초 TAFF 관측 덮어쓰기 금지'
    subprocess.run(['git','diff','--exit-code','--','cpppj/src/o/BaseFile.h','cpppj/src/o/BaseFile.cpp','cpppj/src/client/TechnicalHelpClient.cpp'],cwd=ROOT,check=True,capture_output=True)
    cases=[]
    for kind in ('valid','other','missing','bad-magic','bad-nul','short-header','short-index','short-directory'):  # 각각의 실제 생성 실패 관문을 별도 입력으로 준다.
        cases.extend(([(0,'one.tarc',kind)],[(1,'one.tarc',kind)],[(0,'one.tarc',kind),(0,'ONE.TARC','other')]))
    cases.extend(([(0,'one.tarc','valid'),(1,'ONE.TARC','other')],[(0,'one.tarc','valid'),(0,'two.tarc','other'),(1,'two.tarc','other'),(1,'one.tarc','valid')],[(1,'one.tarc','valid'),(1,'ONE.TARC','missing')]))
    oracle=Oracle();lines=['# requests(pool,name,inputKind) / nativeReturns / primaryNames / secondaryNames / CRT effects']
    for case in cases:  # 입력별 첫 관측과 같은 새 목록 재관측의 바이트 일치를 확인한다.
        first,second=oracle.sample(case),oracle.sample(case);assert first==second;lines.append(first)
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_taff_registration.py','cpppj/tests/fixtures/taff-registration-x86.tsv')
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','observedBeforeBaseFileModification':True,'cases':len(cases),'observations':len(cases)*2,'gameOrOsExecuted':False,'sourceBeforeSha256':{name:digest(ROOT/name) for name in ('cpppj/src/o/BaseFile.h','cpppj/src/o/BaseFile.cpp','cpppj/src/client/TechnicalHelpClient.cpp')},'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
