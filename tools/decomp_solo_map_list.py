#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""단독 맵 캐시004cafd0/행004cb210의 실제 PE 분기와 표를 격리 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1]  # 명시 프로젝트 루트다.
STACK=0x2100000; HEAP=0x1000000; STOP=0x3100000  # 격리 스택/문자열과 정상 반환 표식이다.
NAMES=0x5cab90; FLAGS=0x5cac58; COUNT=0x5cad20; DIRTY=0x542678  # 보호 PE의 실제 목록 전역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/solo-map-list-x86.tsv'  # 기존 관측을 바꾸지 않는 새 자료다.
REPORT=ROOT/'cpppj/solo-map-list-20261022.json'  # 실제 범위/경계를 분리한다.


def digest(path):
    """전체 입력 파일의 SHA256을 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def hx(value):
    """빈 값도 보존하는 ASCII 기반 원문 열이다."""
    return value.encode('cp1252').hex() or '_'


def lower(value):
    """기본 CRT 로케일의 ASCII 대문자만 비교용으로 접는다."""
    return bytes(x+32 if 65<=x<=90 else x for x in value)


class Oracle:
    """게임/OS를 실행하지 않고 목록 본체와 실제 두 getter를 실행한다."""
    def __init__(self):
        """보호 PE·분리 힙/스택·SEH 저장 슬롯만 준비한다."""
        self.pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        size=(self.pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095
        self.mu.mem_map(0x400000,size);self.mu.mem_write(0x400000,self.pe.get_memory_mapped_image())
        self.mu.mem_map(0,4096);self.mu.mem_map(HEAP,0x100000);self.mu.mem_map(STACK-0x10000,0x20000);self.mu.mem_map(STOP,4096)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.calls={};self.actual={}

    def word(self,address,value=None):
        """실제 비정렬 DWORD만 읽거나 쓴다."""
        if value is not None:self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def string(self,address):
        """현재 C 문자열을 첫 NUL까지 읽는다."""
        if not address:raise RuntimeError('관측의 null 문자열')
        result=bytearray()
        for index in range(1024):
            byte=self.mu.mem_read(address+index,1)[0]
            if not byte:return bytes(result)
            result.append(byte)
        raise RuntimeError('관측 문자열 범위')

    def allocate(self,value):
        """각 strdup/문자열 입력에 독립한 비영 포인터를 부여한다."""
        raw=value.encode('cp1252') if isinstance(value,str) else value
        address=self.next;self.next+=(len(raw)+16)&~7;self.mu.mem_write(address,raw+b'\0');return address

    def returned(self,value=0,purge=0):
        """외부 경계의 원래 cdecl/thiscall 반환만 수행한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);target=self.word(esp)
        self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def instruction(self,mu,address,size,data):
        """파일/Template/CRT/메뉴 경계만 명시하고 본체의 표/분기를 그대로 실행한다."""
        self.recent=(self.recent+[address])[-16:]
        if address in (0x4cafd0,0x4cb210,0x41aad0,0x4bd340):self.actual[f'{address:08x}']=self.actual.get(f'{address:08x}',0)+1
        if address==STOP:return
        if address in (0x41aad0,0x4bd340):
            self.trace.append('P' if address==0x41aad0 else 'R');return
        if 0x4cafd0<=address<0x4cb320 or 0x41aad0<=address<0x41aae0 or 0x4bd340<=address<0x4bd360:return
        esp=mu.reg_read(UC_X86_REG_ESP);args=[self.word(esp+4+i*4) for i in range(6)];this=mu.reg_read(UC_X86_REG_ECX)
        self.calls[f'{address:08x}']=self.calls.get(f'{address:08x}',0)+1
        if address==0x4e4354:self.returned();return  # 원본 소유 문자열 free 경계다.
        if address==0x441270:self.trace.append('O');self.word(args[1],self.case['option']);self.returned(1);return
        if address==0x460650:self.returned(args[0]);return  # 실제 PathSpec의 파일 이름 입력을 보존한다.
        if address==0x459c60:self.returned(self.allocate('*'));return
        if address==0x45a220:
            self.trace.append('E');self.iterator=this;self.fileIndex=0;self.word(this+260,int(bool(self.case['files'])));self.returned(0,4);return
        if address==0x459c40:self.returned(0,4);return
        if address==0x459b20:self.returned(self.filePointers[self.fileIndex],4);return
        if address==0x460c40:
            name=self.string(args[0]).decode('cp1252');self.trace.append('K:'+name);self.returned(self.profiles[name]['kind']);return
        if address==0x4e78a5:self.returned(self.allocate(self.string(args[0])));return
        if address==0x4bd2e0:self.trace.append('C');self.mu.mem_write(this,bytes(20));self.returned(this);return
        if address==0x4bf550:
            name=self.string(args[0]).decode('cp1252');self.trace.append('L:'+name);self.active=self.profiles[name];self.word(this,int(self.active['present']));self.returned(0,4);return
        if address==0x4bd4d0:self.trace.append('S');self.mu.mem_write(args[0],self.active['subscriber'].encode('cp1252')+b'\0');self.returned(0,4);return
        if address==0x4bd550:
            self.trace.append('B');self.mu.mem_write(self.state,bytes([self.active['state']])+bytes(27));self.returned(self.state);return
        if address==0x4e65ed:
            a,b=(lower(self.string(arg)) for arg in args[:2]);self.returned((a>b)-(a<b))
            if self.phase=='list':self.trace.append('U')
            return
        if address==0x4597c0:
            self.fileIndex+=1;self.word(this+260,int(self.fileIndex<len(self.case['files'])));self.returned();return
        if address==0x4595f0:self.returned();return
        if address==0x4de9a0:self.trace.append('T');self.returned(self.allocate('%s (~rCORRUPT~.)'),4);return
        if address==0x4a89e0:
            pattern=self.string(args[1]).decode('cp1252');text=pattern%self.string(args[2]).decode('cp1252');self.mu.mem_write(args[0],text.encode('cp1252')+b'\0');self.returned(len(text));return
        if address==0x477dd0:
            self.row=f'{args[0]},{args[5]},{hx(self.string(args[1]).decode("cp1252"))},{hx(self.string(args[3]).decode("cp1252"))}'
            if args[2]!=0x020000ca or args[4]!=1:raise RuntimeError('원본 목록 행 명령/인자 불일치')
            self.returned();return
        raise RuntimeError(f'명시하지 않은 관측 코드 {address:08x}')

    def invoke(self,entry,arg,control):
        """정상 반환·스택/보존 레지스터와 x87 상태를 확인한다."""
        registers={UC_X86_REG_EBX:0x11112222,UC_X86_REG_ESI:0x22223333,UC_X86_REG_EDI:0x33334444,UC_X86_REG_EBP:0x44445555}
        for reg,value in registers.items():self.mu.reg_write(reg,value)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(STACK,struct.pack('<II',STOP,arg&0xffffffff));self.mu.emu_start(entry,STOP,count=300000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('원본 목록 반환/스택 불일치')
        if any(self.mu.reg_read(reg)!=value for reg,value in registers.items()):raise RuntimeError('원본 목록 보존 레지스터 불일치')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('원본 목록 x87 불일치')
        return self.mu.reg_read(UC_X86_REG_EAX)

    def run(self,case,control):
        """두 본체의 결과·50개 표·행·외부 요청 순서를 관측한다."""
        self.case=case;self.next=HEAP;self.trace=[];self.phase='list';self.row='n';self.recent=[]
        self.profiles={p['name']:p for p in case['files']};self.filePointers=[self.allocate(p['name']) for p in case['files']];self.state=self.allocate(bytes(28))
        for i,name in enumerate(case['names']):self.word(NAMES+i*4,0 if name is None else self.allocate(name));self.word(FLAGS+i*4,case['flags'][i])
        self.word(COUNT,case['count']);self.word(DIRTY,case['dirty']);self.word(0x540ca8,self.allocate(case['user']))
        self.word(0x55b490,self.allocate(case['current']));self.word(0x55b494,self.allocate(case['fallback']));self.word(0x542a1c,0x020000ca)
        result=self.invoke(0x4cafd0,case['mode'],control)
        names=[None if not self.word(NAMES+i*4) else self.string(self.word(NAMES+i*4)).decode('cp1252') for i in range(50)]
        flags=[self.word(FLAGS+i*4) for i in range(50)];indices=[i for i in range(min(result,50)) if names[i] is not None][:5]+[result,result+1]
        self.phase='row';rows=[]
        for index in indices:
            self.word(self.state,index);self.row='n';returned=self.invoke(0x4cb210,self.state,control)
            if bool(returned)!=(self.row!='n'):raise RuntimeError('원본 행 반환 불일치')
            rows.append(self.row)
        return result,self.word(DIRTY),names,flags,indices,rows,','.join(self.trace)


def cases():
    """종류/옵션/빈·기존·더러운 캐시와50개 상한을 교차한다."""
    result=[]
    for mode,option,seed,length in itertools.product((-1,0,1,2,3,256),(0,1,-1),range(4),(0,7,60)):
        names=[None]*50;flags=[0]*50;count=0;dirty=seed%2
        if seed==1:flags=[(i*7+3)%16 for i in range(50)];names[4]='Old4'
        if seed==2:names[:5]=['Cached','MAP00','map01','Map02',''];flags[:5]=[0,1,2,8,3];count=5
        if seed==3:names[:5]=['Cache0','Cache1','Cache2','Cache3','Cache4'];count=5
        files=[dict(name=f'Map{i:02}',kind=i%4,present=int(i%5!=0),state=i%2,subscriber=('User' if i%3 else 'Other')) for i in range(length)]
        result.append(dict(mode=mode,option=option,dirty=dirty,count=count,names=names,flags=flags,files=files,user='USER',current='' if length%2 else 'map00',fallback='Map02'))
    return result


def main():
    """독립 원본 관측을 새 TSV/출처에 동결한다."""
    oracle=Oracle();rows=[]
    for index,case in enumerate(cases()):
        observations=[oracle.run(case,cw) for cw in (0x27f,0x37f)]
        if observations[0]!=observations[1]:raise RuntimeError('정밀도별 목록 결과 불일치')
        count,dirty,names,flags,indices,display,trace=observations[0]
        fields=[index,case['mode'],case['option'],case['dirty'],case['count'],','.join('-' if n is None else hx(n) for n in case['names']),','.join(map(str,case['flags'])),
            ';'.join('/'.join(map(str,(hx(p['name']),p['kind'],p['present'],p['state'],hx(p['subscriber'])))) for p in case['files']) or '-',hx(case['user']),hx(case['current']),hx(case['fallback']),count,dirty,
            ','.join('-' if n is None else hx(n) for n in names),','.join(map(str,flags)),','.join(map(str,indices)),';'.join(display),trace]
        rows.append('\t'.join(map(str,fields)))
    FIXTURE.write_text('# 실제 단독 맵 캐시/행 본체. 파일·Template·CRT·장치 경계는 명시 입력이다.\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=[ROOT/'originals/Netstorm.exe',Path(__file__),FIXTURE]
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),date='2026-10-22',inputs=len(rows),precision_runs=len(rows)*2,entries=['004cafd0','004cb210','0041aad0','004bd340'],actual_calls=oracle.actual,boundaries=oracle.calls,new_game_runs=0,sha256={str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in paths}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('native cases',len(rows),'precision runs',len(rows)*2,flush=True)


if __name__=='__main__':main()
