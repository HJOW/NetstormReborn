#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""!color.dat 부모/독립 catch/실제 cleanup을 제품보다 먼저 관측한다. MSVCRT dispatch는 실행하지 않는다."""
import argparse
from collections import Counter
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

FRAME, READER, WRITER = STACK-4, STACK-4-0x58, STACK-4-0x94  # 실제 부모 EBP와 서로 다른24BYTE 지역 객체다.
PRIMARY, OWNERS = 0x555d88, 0x5319c8  # 경로 뒤 한 번 읽는 source와 매 행의 현재 DWORD 입력이다.
SOURCE_A, SOURCE_B = POOL+0x1000, POOL+0x6000  # 명시 raw ColorSort 두0x3800BYTE 할당이다.
OWNER_TABLES = 0x556538, 0x556e38  # 부모가 직접 생성하는 두8×256BYTE다.
TABLES = (0x556038,0x557738,0x556438,0x556238,0x556138,0x556338,0x555f38,0x555e38,0x557838)  # 실제 파일 순서다.
FIXTURE, REPORT = ROOT/'cpppj/tests/fixtures/colorsort-cache-parent-x86.tsv', ROOT/'cpppj/colorsort-cache-parent-native.json'  # 최초 결과는 덮어쓰지 않는다.


def source(profile, alternate):
    """생성자 결과를 가장하지 않는 명시56BYTE 기록256개를 구성한다."""
    raw=bytearray((0x37+byte*13+alternate*29)&255 for byte in range(0x3800))
    values=(0.8,0.800000011920929,0.80000002,float('nan'),float('inf'),float('-inf'),-1.0,1.0)
    for index in range(256):  # threshold의 같음/엄격비교/비순서 입력을 실제 double 비트로 넣는다.
        light=(index/255.0 if not profile else values[(index+alternate)%len(values)])
        struct.pack_into('<d',raw,index*56+8,light)
    return bytes(raw)


def cases():
    """파일/장치의 명시입력과 하나의 선택 변경만 교차한다."""
    rows=[(entry,profile,initial,2304,256,'none') for entry,profile,initial in itertools.product(('prefix','parent','catch','cleanupR','cleanupW'),(0,1),(0,1))]
    rows += [('parent',1,1,length,256,'none') for length in (0,1,255,256,257,1024,2303,2305)]
    rows += [('catch',1,1,2304,limit,'none') for limit in (0,1,17,255)]
    for change in ('path-sourceB','path-owner7','path-stop','path-unable','owners-sourceB','owners-pathChanged','owners-stop','owners-unable',
                   'openR-stop','openR-unable','openR-other','read1-stop','read4-stop','read9-stop','read3-unable','read5-other','read2-sourceB','read2-pathChanged',
                   'closeR-stop','closeR-unable','closeR-other'):
        rows.append(('parent',1,1,2304,256,change))
    for change in ('maps-stop','maps-unable','maps-pathChanged','openW-stop','openW-unable','openW-other','write1-stop','write4-stop','write9-stop','write3-unable','write5-other','closeW-stop'):
        rows.append(('catch',1,1,2304,256,change))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """실제 부모/funclet을 실행하고 Path/TYPE등록/9표계산/File 객체는 필수 하위 장치로 둔다."""
    def __init__(self):
        """FS 연결 DWORD와 누계만 추가하며 게임/OS 코드를 허용하지 않는다."""
        super().__init__();self.mu.mem_map(0,4096);self.total=0;self.cleanups=0;self.dispatches=0

    def event(self,kind,text):
        """현재 경계와1기준 번호를 실제 호출 순서로 남긴다."""
        self.calls[kind]+=1;self.trace.append(text);return kind+str(self.calls[kind])

    def mutate(self,boundary):
        """하위 장치의 변경/명시 예외만 공급하며 부모 출력은 계산하지 않는다."""
        change=self.case[-1]
        if '-' not in change:return
        trigger,action=change.split('-',1)
        if boundary!=trigger and boundary.rstrip('123456789')!=trigger:return
        if action in ('stop','unable','other'):
            self.pending=action;self.mu.emu_stop()
        elif action=='sourceB':self.put(PRIMARY,SOURCE_B)
        elif action=='owner7':self.put(OWNERS,0x12345607)
        elif action=='pathChanged':self.mu.mem_write(TEXT,b'changed-color.dat\0')
        else:raise RuntimeError('ColorCache 미해석 장치변경 '+action)

    def instruction(self,mu,address,size,data):
        """1163BYTE/별도256BYTE catch/실제 두 cleanup 밖의 효과는 명시 인자로 관측한다."""
        self.total+=1;self.instructions+=1
        if self.instructions>250000:raise RuntimeError('ColorCache 한 관측 명령 상한')
        if address==0x43c87c and self.entry=='prefix':self.completion='prefix';mu.emu_stop();return
        if 0x43c4e0<=address<=0x43ca6a or 0x4f9c20<=address<0x4f9c33:return
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,7)
        if address==0x459c60:
            if args!=(0xfffffffe,TEXT+0x100,0xfffffffe,TEXT+0x200,0xfffffffd,0x506718,0):raise RuntimeError('ColorCache PathSpec 원문 인수')
            boundary=self.event('path','P');self.mutate(boundary)
            if not self.pending:self.ret(TEXT)
        elif address==0x43be30:
            boundary=self.event('owners','T');self.mutate(boundary)
            if not self.pending:self.ret()
        elif address==0x43c010:
            for index,target in enumerate(TABLES):mu.mem_write(target,bytes((0x80+index*7+byte*3)&255 for byte in range(256)))  # 실제9표 계산을 대신하는 명시 하위 입력이며 그 계산 완료를 주장하지 않는다.
            boundary=self.event('maps','M');self.mutate(boundary)
            if not self.pending:self.ret()
        elif address in (0x41b4c0,0x41b8d0):
            role='R' if address==0x41b4c0 else 'W';expected=READER if role=='R' else WRITER
            if ecx!=expected or args[:3]!=(TEXT,0 if role=='R' else 1,1):raise RuntimeError('ColorCache 실제 File ctor 인수')
            boundary=self.event('open'+role,'O:'+role+':'+bytes(mu.mem_read(ecx,24)).hex()+':'+self.string(args[0]).hex())
            mu.mem_write(ecx,bytes((0x10+(role=='W')*0x30+byte)&255 for byte in range(24)))  # 생성자의 명시 물리 효과이며 반환값은 부모가 사용하지 않는다.
            self.mutate(boundary)
            if not self.pending:self.ret(0xdecafbad,12)
        elif address==0x41a7a0:
            target,width,limit=args[:3]
            if ecx!=READER or width!=256 or limit!=0xffffffff or target not in TABLES:raise RuntimeError('ColorCache 실제 read 인수')
            count=min(width,max(0,len(self.input)-self.position))
            if count:mu.mem_write(target,self.input[self.position:self.position+count])
            self.position+=count;boundary=self.event('read','R:'+str(TABLES.index(target))+':'+str(count));self.mutate(boundary)
            if not self.pending:self.ret(count,12)
        elif address==0x41a910:
            target,width,limit=args[:3]
            if ecx!=WRITER or width!=256 or limit!=0xffffffff or target not in TABLES:raise RuntimeError('ColorCache 실제 write 인수')
            count=min(width,self.case[4]);self.output.extend(mu.mem_read(target,count));boundary=self.event('write','W:'+str(TABLES.index(target))+':'+str(count));self.mutate(boundary)
            if not self.pending:self.ret(count,12)
        elif address==0x41b410:
            role='R' if ecx==READER else 'W' if ecx==WRITER else '?'
            if role=='?':raise RuntimeError('ColorCache 실제 close this')
            boundary=self.event('close'+role,'D:'+role+':'+bytes(mu.mem_read(ecx,24)).hex());mu.mem_write(ecx,bytes((0xd0+(role=='W')*7+byte)&255 for byte in range(24)))  # close의 하위 물리 효과도 버리지 않는다.
            self.mutate(boundary)
            if not self.pending:self.ret()
        else:raise RuntimeError('ColorCache 관측 밖 실행 '+hex(address))

    def separate(self,address):
        """MSVCRT dispatcher 대신 같은 실제 EBP의 독립 funclet을 호출한다."""
        self.mu.reg_write(UC_X86_REG_EBP,FRAME);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x400);self.put(STACK+0x400,STOP);self.mu.emu_start(address,STOP)
        if not self.pending and self.mu.reg_read(UC_X86_REG_ESP)!=STACK+0x404:raise RuntimeError('ColorCache 독립 funclet stack 반환')

    def unwind(self,state):
        """메타데이터가 선택한 실제 cleanup만 독립 실행하고 미완성 ctor는 건너뛴다."""
        if state not in (1,3):return
        self.cleanups+=1;self.sample_cleanups+=1;self.separate(0x4f9c20 if state==1 else 0x4f9c28)
        if self.pending:raise RuntimeError('ColorCache 이중예외 장치는 선택범위 밖')

    def sample(self,case,control):
        """선택 entry와 전체 초기물리를 새로 명시하고 부분효과를 고정한다."""
        self.case=case;self.entry,profile,initial,length,limit,change=case;self.trace=[];self.calls=Counter();self.pending='';self.completion='return';self.position=0;self.output=bytearray();self.instructions=0;self.sample_cleanups=self.sample_dispatches=0
        self.input=bytes((19+byte*7)&255 for byte in range(length));self.mu.mem_write(STACK-0x2000,bytes([0xa5 if initial else 0x3c])*0x2800);self.put(0,0)
        self.mu.mem_write(TEXT,b'!color.dat\0'+bytes(245));self.mu.mem_write(SOURCE_A,source(profile,0));self.mu.mem_write(SOURCE_B,source(profile,1));self.put(PRIMARY,SOURCE_A);self.put(0x540cec,TEXT+0x100);self.put(0x540cf8,TEXT+0x200)
        self.mu.mem_write(OWNERS,struct.pack('<8I',84,249,184,140,150,112,156,222))
        for index,target in enumerate(OWNER_TABLES):self.mu.mem_write(target,bytes((initial*31+index*7+byte*11)&255 for byte in range(2048)))  # 부모가 쓰지 않은 출력 잔여도 입력으로 보존한다.
        for index,target in enumerate(TABLES):self.mu.mem_write(target,bytes((initial*13+index*19+byte*5)&255 for byte in range(256)))  # shortread의 나머지는 이 물리다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)  # 다른 관측의 CPU 상태를 재사용하지 않는다.
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.put(STACK,STOP);self.put(FRAME-0x24,TEXT)
        if self.entry in ('parent','prefix'):self.mu.emu_start(0x43c4e0,STOP)
        else:
            self.put(FRAME-4,2 if self.entry=='catch' else 1 if self.entry=='cleanupR' else 3)
            if self.entry!='catch':self.cleanups+=1;self.sample_cleanups+=1  # 직접 cleanup entry도 실제 한 호출로 센다.
            self.separate(0x43c96b if self.entry=='catch' else 0x4f9c20 if self.entry=='cleanupR' else 0x4f9c28)
        if self.pending:
            pending=self.pending;self.pending='';state=self.words(FRAME-4,1)[0];self.unwind(state)
            if pending=='unable' and self.entry=='parent' and state in (0,1):
                self.dispatches+=1;self.sample_dispatches+=1;self.put(FRAME-4,2);self.separate(0x43c96b)
                if self.pending:raise RuntimeError('ColorCache 두 번째 예외입력 미지원')
            else:self.completion=pending
        if self.entry=='catch' and self.completion=='return' and self.mu.reg_read(UC_X86_REG_EAX)!=0x43c95a:raise RuntimeError('ColorCache catch의 Native 복귀주소')
        fields=(str(control),','.join(map(str,case)),self.completion,str(self.sample_cleanups),str(self.sample_dispatches),str(int(self.words(PRIMARY,1)[0]==SOURCE_B)),bytes(self.mu.mem_read(OWNERS,32)).hex(),b''.join(bytes(self.mu.mem_read(a,2048)) for a in OWNER_TABLES).hex(),b''.join(bytes(self.mu.mem_read(a,256)) for a in TABLES).hex(),(bytes(self.mu.mem_read(READER,24))+bytes(self.mu.mem_read(WRITER,24))).hex(),self.output.hex() or '-', ';'.join(self.trace) or '-',str(self.instructions))
        return '\t'.join(fields)


def main():
    """첫 관측은 제품 없이 작성하고 이후에는 최초 SHA 확인만 허용한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in report['sha256'].items():
            if digest(ROOT/name)!=expected:raise RuntimeError('ColorCache 최초 SHA 불일치 '+name)
        print('ColorCache 최초 자료 동일',report['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('ColorCache 최초 자료 덮어쓰기 금지')
    oracle=Oracle();rows=[];statuses=Counter();inputs=cases()
    for case in inputs:
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:-1]!=second.split('\t')[1:-1]:raise RuntimeError('ColorCache 정밀도차이 '+repr(case))
        rows.extend((first,second));statuses[first.split('\t')[2]]+=2
    FIXTURE.write_text('# control / entry,profile,initial,readLength,writeLimit,mutation / completion / independentCleanupCount / externalTypedDispatchCount / currentSourceB / ownerDwords32 / twoOwnerTables4096 / nineMaps2304 / twoFiles48 / written / trace / instructions\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_colorsort_cache_parent.py','cpppj/tests/fixtures/colorsort-cache-parent-x86.tsv')
    report={'status':'제품 전 최초 Native 고정','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entries':{'0043c4e0':1163,'0043c96b':256,'004f9c20':8,'004f9c28':11},'uniqueInputs':len(inputs),'observations':len(rows),'completions':dict(statuses),'instructions':oracle.total,'separateCleanupCalls':oracle.cleanups,'externalTypedDispatches':oracle.dispatches,'msvcrtTypedDispatchExecuted':False,'gameExecuted':False,'guiExecuted':False,'scope':'부모/prefix/독립catch/실제cleanup 명령만. EX_UNABLE_TO_OPEN_FILE 형은 PE EH 메타데이터로 분류하며 typed dispatch는 명시 외부 장치다. Path/TYPE표등록/43c010계산/File24BYTE 생성·읽기·쓰기·close는 명시 하위 입력이다. 미완성 ctor의 outer close 생략, 일반 오류 전파, 부분 read/write와 cleanup 순서를 보존한다. 이중예외는 미관측이다.','sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','observations','completions','instructions','separateCleanupCalls','externalTypedDispatches')}))


if __name__=='__main__':main()
