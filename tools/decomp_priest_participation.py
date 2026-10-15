#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 참여 조회/갱신과 전체 하위 탐색을10.78 실제 명령으로 격리 관찰한다.

게임/GUI/OS/대체 함수 없이 실제 live·풀 iterator·Carrier·Contained·Altar·typed finder·CRT를 실행한다.
python -X utf8 tools/decomp_priest_participation.py [--smoke | --verify]
"""
import argparse
import collections
import csv
import itertools
import json
import socket
import struct
import zlib
from pathlib import Path

import decomp_missionframe_oracle as common
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EIP,
    UC_X86_REG_ESP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 보호PE/현재 full refined 근거와 별도 산출물 경로다.
ROOT=common.ROOT
BINARY=common.BINARY
BODY=ROOT/'extracted/refined/originals/functions.tsv'
FIXTURE=ROOT/'cpppj/tests/fixtures/priest-participation-x86.tsv'
REPORT=ROOT/'cpppj/priest-participation.json'
# 에뮬레이터에만 존재하는 풀·타입·목록·해시·정상 반환·스택 메모리다.
POOL,TYPES,LIST,MAP,STOP,STACK_BASE,STACK=0x12000000,0x11100000,0x11200000,0x11300000,0x20000000,0x30000000,0x3003f000
CAPACITY,STRIDE,TYPE_STRIDE=24000,50,500
CONTROLS=(0x027f,0x037f)
# 직접 검토한 전체 함수와 두 가상표의 실제 기본true/Next 몸체만 허용한다.
ENTRIES=(0x494f10,0x495060,0x4921f0,0x40eb40,0x4ac200,0x4ac9f0,0x448bb0,0x4b2270,
    0x4b1b20,0x4b1fa0,0x4b16d0,0x4b1810,0x41d770,0x49a840,0x49a900,0x434380,
    0x4b2b00,0x4e49c0,0x44daa0)
# 모두 실제 현재 전역이며 결과 상태는 기존 ownerState9DWORD에 직접 기록된다.
GLOBALS=dict(pool=0x5c8464,capacity=0x5c847c,types=0x59ab20,live=0x5954c4,count=0x5954cc,
    priest=0x5412d0,altar=0x5412e8,dais=0x5412ec,contained=0x541088,boss=0x540bc4,checking=0x5e4794,
    ownerState=0x595428)


def bits(value):
    """명시float 입력을 원본DWORD 비트로 고정한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def node(sid,typ=82,state=0,owner=3,original=1,extra=0,head=0,nxt=0,kind=0,mask=0,
         x=40.75,y=41.25,level=-1,width=1,height=1,genus=0):
    """결과를 계산하지 않고 슬롯·타입·공간등록 입력만 만든다."""
    return [sid,typ,state,owner,original,extra,head,nxt,kind,mask,bits(x),bits(y),level,width,height,genus]


def base():
    """모든 미사용슬롯은free이며 사제목록과 실제현재타입을 명시한다."""
    return dict(player=1,count=0,live=[50,51,52,50],priest=158,altar=164,dais=165,contained=6,boss=1,
        nodes=[node(5,82,head=10,genus=0x10000),node(10,6,state=8,kind=158,mask=0x20)])


def cases():
    """live우선·원소유자폭·첫Carrier/마지막Altar·상태/버킷/현재타입의 독립 입력을 만든다."""
    result=[]
    # live는타입/free/contained/extra를추가로제외하지않고dead만거른다.
    for player,state,typ in itertools.product((0,1,8,255,257,0xffffffff),(0,1,2,4,8,255),(158,82)):
        case=base();case.update(player=player,count=1);case['nodes'].append(node(50,typ,state,owner=player&255,original=8,extra=255));result.append(case)
    # 실제종속체인의타입/WORDmask/kind/첫일치와포획BYTE마스킹을교차한다.
    for index in range(90):
        case=base();case['player']=(0,1,127,128,129,257)[index%6]
        case['nodes']=[node(5,82,(0,2,4,8,1)[index%5],head=10,genus=(0,0x10000,0x20000,0x30000,0x210000,0x80000)[index%6]),
            node(10,(6,7)[index%2],state=(0,1,2,4,8,255)[index%6],original=(0,1,127,128,129,257,65535)[index%7],
                 nxt=11,kind=(158,157,0,65535)[index%4],mask=(0,1,0x20,0x21,65535)[index%5]),
            node(11,6,state=255,original=case['player']&0x7f,kind=158,mask=0x20)]
        result.append(case)
    # 복수Altar와같은칸첫Dais가dead이면뒤의liveDais를대신고르지않는다.
    for index in range(72):
        case=base();case['player']=(0,1,8,129,257,65535)[index%6]
        case['nodes']=[node(5,164,(0,2,4,8,1)[index%5],original=(1,129,257,65535)[index%4]),
            node(6,165,(0,1,2,3,4,8)[index%6],extra=8 if index%11==0 else 0,level=index%4,width=3,height=3),
            node(7,164,original=case['player'],x=60.25,y=61.75),
            node(8,165,state=2 if index%7==0 else 0,x=60.25,y=61.75,level=1,width=3,height=3)]
        if index%3==0:case['nodes'].append(node(9,165,state=2,x=60.25,y=61.75,level=1,width=3,height=3))
        result.append(case)
    # 앞Altar의지연반환보다뒤Carrier의즉시반환이우선하며마지막번호까지검사한다.
    for index in range(16):
        case=base();case['nodes']=[node(2,164,original=1),node(3,165,level=1,width=3,height=3),
            node(5 if index%2 else 23999,82,head=10,genus=0x20000),node(10,6,original=129,kind=158,mask=0x20)]
        if index%4==0:case['nodes'][2][2]=1
        case['player']=(1,9,0xffffffff,129)[index%4];result.append(case)
    # 현재타입DWORD와signed목록개수를고정값으로바꾸면실패하는입력이다.
    for priest,altar,dais,contained,count in itertools.product((0,158,159),(164,163),(165,166),(6,7),(0,0x80000000,0xffffffff)):
        case=base();case.update(priest=priest,altar=altar,dais=dais,contained=contained,count=count)
        case['nodes']=[node(5,82,head=10,genus=0x10000),node(10,contained,original=129,kind=priest,mask=0x20),
            node(7,altar,original=1),node(8,dais,level=1,width=3,height=3)];result.append(case)
    unique={} # 같은입력을중복된독립관찰로합산하지않는다.
    # 정규JSON입력키로중복만제거하며예상반환값은계산하지않는다.
    for case in result:unique.setdefault(json.dumps(case,sort_keys=True),case)
    return list(unique.values())


def columns(case):
    """입력열은출력계산전고정한다.일반정수만사용해C++에서도별도계산없이재생한다."""
    return [case['player'],case['count'],','.join(map(str,case['live'])),
        ','.join(str(case[name]) for name in ('priest','altar','dais','contained','boss')),
        ';'.join(','.join(map(str,item)) for item in case['nodes'])]


class Oracle:
    """허용된19개전체몸체외실행과ownerState외제품쓰기를즉시거부한다."""
    def __init__(self):
        """보호바이트를가상메모리에복사하고실제full/refined함수범위를고정한다."""
        if common.digest(BINARY)!=common.SHA:raise RuntimeError('보호PE SHA 불일치')
        pe=common.pefile.PE(str(BINARY));self.base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image()
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(image)+4095)&~4095);self.mu.mem_write(self.base,image)
        self.allowed=bytearray(len(image));self.ranges={}
        # 선택함수의불연속범위만허용한다.전체PE나진단/운영체제함수는허용하지않는다.
        for row in csv.DictReader(BODY.open(encoding='utf-8'),delimiter='\t'):
            entry=int(row['entry'],16)
            if entry not in ENTRIES:continue
            self.ranges[f'{entry:08x}']=row['ranges']
            # body종료주소는Ghidra의포함끝주소다.
            for part in row['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        if len(self.ranges)!=len(ENTRIES):raise RuntimeError('전체함수범위누락')
        self.map_end=MAP+sum((256//scale)**2*2 for scale in (1,2,4,16))
        # 풀은실제C++풀과동일24000슬롯이며tailfree스캔도원본명령이다.
        for address,size in ((POOL,CAPACITY*STRIDE),(TYPES,256*TYPE_STRIDE),(LIST,0x1000),(MAP,self.map_end-MAP),(STOP,0x1000),(STACK_BASE,0x40000)):
            self.mu.mem_map(address,(size+4095)&~4095)
        self.bases=[];address=MAP
        # 실제네해시배열포인터와크기만입력한다.탐색반환은기계어가계산한다.
        for scale in (1,2,4,16):self.bases.append(address);address+=(256//scale)**2*2
        self.native=collections.Counter();self.reads=collections.Counter();self.writes=collections.Counter();self.returns=0
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def put(self,address,value,width=4):
        """외부의명시입력을원래필드폭으로쓴다.원본효과를대신계산하지않는다."""
        self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))

    def get(self,address,width=4):
        """현재원본메모리의정수비트를읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def instruction(self,mu,address,size,data):
        """선택전체몸체만허용하며호출진입횟수를기록한다.함수대체는없다."""
        offset=address-self.base
        if not 0<=offset<len(self.allowed) or not self.allowed[offset]:raise RuntimeError(f'허용밖명령 {address:08x}')
        if address in ENTRIES:self.native[f'{address:08x}']+=1

    def read(self,mu,access,address,size,value,data):
        """실제명령의현재raw/타입/해시/목록읽기수를기록하며반환을공급하지않는다."""
        for name,low,high in (('pool',POOL,POOL+CAPACITY*STRIDE),('types',TYPES,TYPES+256*TYPE_STRIDE),('hash',MAP,self.map_end),('list',LIST,LIST+16)):
            if low<=address and address+size<=high:self.reads[name]+=1;return

    def write(self,mu,access,address,size,value,data):
        """풀·타입·해시·목록은읽기전용이고Refresh의ownerStateDWORD만제품효과로허용한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if self.operation=='Refresh' and GLOBALS['ownerState']+4<=address<GLOBALS['ownerState']+36 and size==4 and address%4==0:
            self.writes[f'{(address-GLOBALS["ownerState"])//4}:{value&0xffffffff}']+=1;return
        raise RuntimeError(f'사제참여허용밖쓰기 {address:08x}+{size}')

    def prepare(self,case):
        """현재슬롯·전역·목록·실제해시체인만공급하고출력은계산하지않는다."""
        raw=bytearray(CAPACITY*STRIDE)
        # SID0/1도명시free로두어예약슬롯을순회에섞는오류를검출한다.
        for sid in range(CAPACITY):raw[sid*STRIDE+11]=1
        types=bytearray(256*TYPE_STRIDE);self.mu.mem_write(MAP,bytes(self.map_end-MAP))
        # 입력순서의해시머리삽입과종속체인은명시월드입력이며판정결과가아니다.
        for sid,typ,state,owner,original,extra,head,nxt,kind,mask,xb,yb,level,width,height,genus in case['nodes']:
            start=sid*STRIDE;raw[start:start+STRIDE]=bytes(STRIDE);raw[start+10]=typ;raw[start+11]=state;raw[start+34]=owner
            struct.pack_into('<HH',raw,start+4,nxt,head);struct.pack_into('<H',raw,start+12,original&65535)
            struct.pack_into('<II',raw,start+14,xb,yb)
            if kind or mask or typ==case['contained']:struct.pack_into('<HH',raw,start+18,kind,mask)
            raw[start+40]=extra;struct.pack_into('<I',types,typ*TYPE_STRIDE+0xec,genus)
            struct.pack_into('<ii',types,typ*TYPE_STRIDE+0x1d4,width,height)
            if level>=0:
                x,y=(int(struct.unpack('<f',struct.pack('<I',value))[0]) for value in (xb,yb));scale=(1,2,4,16)[level]
                address=self.bases[level]+((y//scale)*(256//scale)+x//scale)*2
                struct.pack_into('<H',raw,start+4,self.get(address,2));self.put(address,sid,2);raw[start+33]=level
        self.raw=bytes(raw);self.type_input=bytes(types);self.mu.mem_write(POOL,self.raw);self.mu.mem_write(TYPES,self.type_input)
        self.list_input=struct.pack('<4I',*case['live']);self.mu.mem_write(LIST,self.list_input)
        values=dict(pool=POOL,capacity=CAPACITY,types=TYPES,live=LIST,count=case['count'],checking=0,
            **{name:case[name] for name in ('priest','altar','dais','contained','boss')})
        # 모두실제포인터/정수의명시입력이며lookup반환값은포함하지않는다.
        for name,value in values.items():self.put(GLOBALS[name],value)
        self.mu.mem_write(0x5424f8,struct.pack('<4I',1,2,4,16));self.mu.mem_write(0x542508,struct.pack('<7I',0,1,256,*self.bases));self.put(0x531928,256)
        self.initial=tuple(0x9000+index for index in range(9));self.mu.mem_write(GLOBALS['ownerState'],struct.pack('<9I',*self.initial))
        self.hash_input=bytes(self.mu.mem_read(MAP,self.map_end-MAP))

    def call(self,operation,player,control):
        """정상반환·cdecl스택·보존레지스터·x87균형과읽기전용입력을검사한다."""
        self.operation=operation
        # 각호출은서로다른보존레지스터입력으로ABI오염을검출한다.
        for register,value in common.SAVED.items():self.mu.reg_write(register,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.mem_write(STACK,struct.pack('<II',STOP,player));self.mu.emu_start(0x494f10 if operation=='Find' else 0x495060,STOP,count=1000000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('정상반환/스택오류')
        if any(self.mu.reg_read(register)!=value for register,value in common.SAVED.items()):raise RuntimeError('보존레지스터오류')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87균형오류')
        if bytes(self.mu.mem_read(POOL,len(self.raw)))!=self.raw or bytes(self.mu.mem_read(TYPES,len(self.type_input)))!=self.type_input or bytes(self.mu.mem_read(LIST,16))!=self.list_input or bytes(self.mu.mem_read(MAP,len(self.hash_input)))!=self.hash_input:raise RuntimeError('조회입력변경')
        self.returns+=1;return self.mu.reg_read(UC_X86_REG_EAX)

    def run(self,case,control):
        """실제두진입점의반환/상태/입력checksum을독립적으로얻는다."""
        self.prepare(case);result=self.call('Find',case['player'],control)
        if tuple(struct.unpack('<9I',self.mu.mem_read(GLOBALS['ownerState'],36)))!=self.initial:raise RuntimeError('Find상태변경')
        self.call('Refresh',case['player'],control)
        state=','.join(str(value) for value in struct.unpack('<9I',self.mu.mem_read(GLOBALS['ownerState'],36)))
        return [result,state,zlib.adler32(self.raw)&0xffffffff,zlib.adler32(self.list_input)&0xffffffff]


def generate(smoke=False):
    """두정밀도의실제반환이일치할때만새fixture와출처/경계보고서를저장한다."""
    oracle=Oracle();plan=cases()[::41] if smoke else cases();rows=[]
    # 기대출력은모두실제19함수의원본명령실행에서만읽는다.
    for index,case in enumerate(plan):
        observed=[oracle.run(case,control) for control in CONTROLS]
        if observed[0]!=observed[1]:raise RuntimeError('두정밀도차이')
        rows.append([*columns(case),*observed[0]])
        if index%100==0:print(f'사제참여관찰 {index+1}/{len(plan)}',flush=True)
    if smoke:print('smoke',len(rows),oracle.returns,dict(oracle.native));return
    FIXTURE.write_text('#10.78실제전체사제참여조회/갱신.대체함수/게임/GUI/OS실행없음.\n#player count live currentTypes nodes result ownerState rawAdler listAdler\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_missionframe_oracle.py',BODY,ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE,BINARY]
    report=dict(total=len(rows),host=socket.gethostname(),date='2026-10-15',last_full_refined_host='HJOW-Athlon',controls=list(CONTROLS),capacity=CAPACITY,
        binary_sha=common.digest(BINARY),ranges=oracle.ranges,returns=oracle.returns,native_calls=dict(oracle.native),data_reads=dict(oracle.reads),owner_writes=dict(oracle.writes),
        substitutions={},os_calls=0,gui_runs=0,files={path.relative_to(ROOT).as_posix():common.digest(path) for path in paths},
        limits=['10.78보호PE의19개전체함수만실행.두기본가상필터/Next도원본주소다.',
            '동일24000슬롯합성월드/직접해시등록.실제Pop/게임/로그인전체는실행하지않음.',
            '허용제품쓰기는Refresh의ownerState1..8DWORD뿐.풀/타입/목록/해시불변과cdecl/x87정상반환검사.',
            '기계어의풀/타입/해시/목록읽기수는관찰통계이며C++메모리읽기명령수동일성을주장하지않음.',
            'CD0040ef50/0040f0e0은정적대응만확인하며이fixture는10.78전용.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('완료',len(rows),oracle.returns,flush=True)


def verify():
    """원본을재실행하지않고출처SHA·입력계획·함수범위·반환횟수·부작용경계를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 생성당시의모든근거파일을정확한바이트SHA로확인한다.
    for name,digest in report['files'].items():
        if common.digest(ROOT/name)!=digest:raise RuntimeError('SHA불일치:'+name)
    if report['total']!=len(rows) or report['returns']!=4*len(rows) or report['controls']!=list(CONTROLS) or report['capacity']!=CAPACITY or report['substitutions'] or report['os_calls'] or report['gui_runs']:raise RuntimeError('행수/반환/경계불일치')
    if set(report['ranges'])!={f'{entry:08x}' for entry in ENTRIES}:raise RuntimeError('전체몸체범위불일치')
    # 출력기대값을재계산하지않고각입력의정확한열과결과폭만검사한다.
    for row,case in zip(rows,cases(),strict=True):
        if len(row)!=9 or row[:5]!=list(map(str,columns(case))) or len(row[6].split(','))!=9:raise RuntimeError('입력열/상태폭불일치')
    print('priest-participation SHA/전체함수/입력검증통과:',len(rows))


def main():
    """명시옵션에따라전체생성·무저장표본·저장근거감사를선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
