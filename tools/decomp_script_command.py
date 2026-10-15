#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 토큰·Config·HTML 명령 큐·80칸 문자열 수명을 격리 실행한다. 게임/OS 실행은 없다."""
import csv
import json
import random
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, TYPES, POOL, STACK, STOP, digest
from unicorn.x86_const import *

# 토큰 출력/현재 입력 포인터/TLS 포인터만 새 가상 저장소를 사용한다.
OUTPUT,CURSOR,THREAD,CHANGED=TEXT+0x2000,TEXT+0x3000,TEXT+0x4000,STOP+0x100
FIXTURE=ROOT/'cpppj/tests/fixtures/script-command-x86.tsv'
REPORT=ROOT/'cpppj/script-command.json'


class Oracle(MemoryOracle):
    """선택 export의 실제 몸체와 CRT만 실행하며 설정/메모리/로그 효과는 관찰한다."""
    def __init__(self):
        """보호PE 사본과 정확 범위·원본 C로케일을 준비한다."""
        super().__init__();self.allowed=[];self.exports=ROOT/'extracted/script-command/originals/functions.tsv'
        facts={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        selected=list(csv.DictReader(self.exports.open(encoding='utf-8'),delimiter='\t'))
        selected += [facts[a] for a in (0x4e63e0,0x4e6fa1,0x4e6656,0x4e65ed,0x4ef510)]
        # 실제 불연속 함수 범위를 그대로 허용하고 다른 게임 함수를 묵시 실행하지 않는다.
        for row in selected:
            for part in row['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.put(THREAD+0x64,self.words(0x543efc,1)[0]);self.reset_ring();self.allocations=0

    def reset_ring(self):
        """최초 BSS0의 실제80칸/인덱스만 초기화한다."""
        self.mu.mem_write(0x5ca9e8,bytes(320));self.put(0x5cab28,0)

    def instruction(self,mu,address,size,data):
        """원본 계산은 실제 명령으로 실행하고 명시 외부효과만 스택 규약대로 관찰한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4)
        if address==0x4ea61d:self.ret(THREAD)
        elif address==0x4c2630:
            self.events.append('L,'+self.string(args[1]).hex());self.ret()
        elif address==0x4404e0:
            self.events.append('S,'+self.string(args[0]).hex()+','+self.string(args[1]).hex());self.ret(self.existed,8)
        elif address==CHANGED:self.events.append('Changed');self.ret()
        elif address==0x4e4354:self.events.append('Free');self.ret()
        elif address==0x4e4391:
            pointer=POOL+self.allocations*128;self.allocations+=1
            self.events.append('New,'+str(args[0]));self.ret(pointer)
        else:
            if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'미등록 스크립트 명령 {address:08x}')
            self.instructions+=1

    def call(self,address,args,ecx=0,purge=0):
        """정상 반환·스택·비휘발 레지스터·x87 제어어를 독립 확인한다."""
        saved={UC_X86_REG_EBX:0x123456,UC_X86_REG_ESI:0x234567,UC_X86_REG_EDI:0x345678,UC_X86_REG_EBP:0x456789}
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,ecx)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.emu_start(address,STOP,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        assert all(self.mu.reg_read(r)==v for r,v in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW)==0x027f
        return self.mu.reg_read(UC_X86_REG_EAX)

    def run(self,operation,source,delimiter=b'',existed=0):
        """같은 입력 바이트를 전달하고 출력/포인터/효과/큐 내용을 관찰한다."""
        self.events=[];self.existed=existed;self.mu.mem_write(TEXT,source+b'\0');self.mu.mem_write(OUTPUT,b'\xcc'*100)
        self.mu.mem_write(TYPES,delimiter+b'\0');self.put(CURSOR,TEXT);self.put(0x558de8,POOL+0xf0000);self.put(0x558dd0,CHANGED)
        value='-';next_offset='-';queue='-';ring='-'
        if operation=='Token':
            result=self.call(0x487710,[CURSOR,TYPES if delimiter!=b'DEFAULT' else 0],OUTPUT,8);assert result==OUTPUT
            value=self.string(OUTPUT).hex() or '-';next_offset=str(self.words(CURSOR,1)[0]-TEXT)
        elif operation=='Config':value=str(self.call(0x441580,[TEXT]))
        elif operation=='Command':
            self.reset_ring();self.put(0x55a508,0);self.put(0x55a50c,0)
            self.put(0x5428ac,0 if existed else 0x0200006d);self.call(0x4c9090,[TEXT]);self.put(0x5428ac,0x0200006d)
            if self.words(0x55a50c,1)[0]:
                code,x,y=self.words(0x55a510,3);assert x==y
                queue=str(code)+','+('s'+self.string(x).hex() if POOL<=x<POOL+0x100000 else 'n'+str(x))
            ring=str(self.words(0x5cab28,1)[0])
        elif operation=='Ring':
            pointer=self.call(0x4c7910,[TEXT]);value=self.string(pointer).hex() or '-';ring=str(self.words(0x5cab28,1)[0])
            assert self.words(0x5ca9e8+int(ring)*4,1)[0]==pointer
        else:raise ValueError(operation)
        return [operation,source.hex() or '-',delimiter.hex() or '-',str(existed),value,next_offset,queue,ring,';'.join(self.events) or '-']


def inputs():
    """정답을 계산하지 않고 경계·인용·빈 값·큐 명령·고리 덮어쓰기 입력을 준비한다."""
    values=[]
    # 공통 토큰의 입력78바이트·인용 포함 소비·연속 구분자와 모든 공백을 확인한다.
    for text in (b'',b' \t',b'abc',b'  a=b==c',b'"a=b"=c',b'"  "=x',b'"\t\t"=x',b'a \t =b',b'\na=b',b'a\0b=c',b'"a=b',b'='+b'A'*80):
        for delimiter in (b'=',b';>',b'DEFAULT'):values.append(('Token',text,delimiter,0))
    for length in (76,77,78,79,80,155):
        for quoted in (False,True):values.append(('Token',(b'"' if quoted else b'')+b'a'*length+b'==b',b'=',0))
    rng=random.Random(0x487620)
    # 고정시드 입력은 구현의 결과를 사용하지 않으며 구분자/인용 순서만 넓힌다.
    for index in range(80):values.append(('Token',bytes(rng.choice(b'ab=;> \t"\n') for _ in range(index*2)),b'=;>',0))
    for text in (b'',b'=x',b'key=1',b'key==2',b'key = "a=b;c"\nignored=3',b' key=""',b'" "=x',b'k='+b'a'*80,b'k=\0tail'):
        for existed in (0,1):values.append(('Config',text,b'',existed))
    for command in (b'Config',b'Warning',b'MISSING',b'cOnFiG'):
        for argument in (b'',b'123tail',b'-1',b'+2',b'4294967296',b'"A, B > C"',b'DoneTutorial1=1'):
            values.append(('Command',command+b','+argument+b'>',b'',0))
    # 실제Config항목의현재코드0은진단뒤에도예약하며미등록-1과다르다.
    for argument in (b'',b'123tail',b'-1'):values.append(('Command',b'Config,'+argument+b'>',b'',1))
    # 이85호출은고리를초기화하지않는하나의연속수명검사다.
    for index in range(85):values.append(('Ring',b'same' if index%2 else str(index).encode(),b'',0))
    return values


def main():
    """fixture와 해시 보고서를 만들거나 현재 근거의 무결성만 감사한다."""
    paths=['originals/Netstorm.exe','tools/decomp_script_command.py','tools/ghidra/script-command-functions.json',
        'extracted/script-command/originals/creation.c','extracted/script-command/originals/functions.tsv',
        'cpppj/src/o/RawTextToken.h','cpppj/src/o/RawTextToken.cpp','cpppj/src/client/ScriptCommand.h','cpppj/src/client/ScriptCommand.cpp',
        'cpppj/tests/ScriptCommandTests.cpp','cpppj/tests/fixtures/script-command-x86.tsv']
    if '--verify' in sys.argv:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        assert all(digest(ROOT/path)==expected for path,expected in report['sha256'].items())
        assert len(FIXTURE.read_text(encoding='utf-8').splitlines())-1==report['observations'];print('verified',report['observations']);return
    oracle=Oracle();rows=[]
    ring_started=False
    for operation,source,delimiter,existed in inputs():
        if operation=='Ring' and not ring_started:oracle.reset_ring();ring_started=True
        rows.append(oracle.run(operation,source,delimiter,existed))
    FIXTURE.parent.mkdir(parents=True,exist_ok=True)
    with FIXTURE.open('w',encoding='utf-8',newline='') as fp:
        writer=csv.writer(fp,delimiter='\t',lineterminator='\n');writer.writerow(['operation','source','delimiter','existed','value','next','queue','ring','events']);writer.writerows(rows)
    report=dict(host='VM-W11-Codex',date='2026-10-15',edition='10.78',observations=len(rows),nativeInstructions=oracle.instructions,
        limitations=['설정 Set/변경통지/로그와 메모리 할당·반납/TLS 포인터는 명시 경계다.','원본 명령 표·문자열 복사/고리·입력큐·토큰·C로케일CRT 몸체는 실제 명령이다.','Ring85행은 연속 수명이며 같은 문자열의 재등록은 서로 다른 상태 입력이다.'],sha256={p:digest(ROOT/p) for p in paths if (ROOT/p).exists()})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('observed',len(rows))


if __name__=='__main__':main()
