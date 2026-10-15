#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH 로컬 간헐천 후보/100회 생성/Player 부모를 실제 바이트로 독립 실행한다."""
import argparse
import collections
import csv
import json
import struct
from pathlib import Path
import decomp_missionframe_oracle as common
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

# 실제 PE는 읽기 전용이고 raw입력/반환/스택은 분리된 가상 주소다.
ROOT=common.ROOT
POOL,TYPES,PLAYERS,SPOTS,RANDOM,FINDER,POINT=0x12000000,0x13000000,0x14000000,0x15000000,0x16000000,0x17000000,0x17000100
WORDS,THREAD,LOCALE,CODES=0x18000000,0x19000000,0x19000100,0x19000200
STOP,STACK=common.STOP,common.STACK
DESTROY,POP,VTABLE=STOP+0x100,STOP+0x120,STOP+0x200
FIXTURE=ROOT/'cpppj/tests/fixtures/local-geyser-x86.tsv'
REPORT=ROOT/'cpppj/local-geyser.json'
# 원본 최근5목록은 각 생성요청에서count만 지운다. 나머지 stale칸도 비교한다.
RECENT=0x595098
KEYS=('op','seed','rectangle','spots','quantity','boss','fort','pool','empty','players','predictable','time','first','last','mutation','surfaces')


def rectangles(profile):
    """유효/소수/정수범위0/음수extent/포함경계의 명시 입력 사각형이다."""
    return [(16.0,24.0,48.0,56.0),(16.25,24.5,48.75,56.25),(16.0,24.0,16.5,24.5),
        (16.0,24.0,15.0,23.0),(0.0,0.0,2.0,2.0)][profile]


def case(**changes):
    """기대 좌표/성공을 계산하지 않고 실제 몸체 입력만 만든다."""
    value=dict(zip(KEYS,('T',0xfed,0,0,1,1,0,8000,0,0,1,common.bits(12.25),3,4,0,0)));value.update(changes);return value


def cases():
    """후보·재시도·모드·자원·최근상한·Player 순회/시드를 독립 입력한다."""
    result=[]
    # candidate의local시드/소수사각형/spot6비트와0extent소비를실제함수로검사한다.
    for seed in (0,0xfed,1,0xffffffff,0x12345678):
        for rectangle in range(5):
            for spots in (0,1,2,0x40,0xc0):result.append(case(seed=seed,rectangle=rectangle,spots=spots))
    # 각권한/요새조합과수량경계·100실패/실제생성·상한5를확인한다.
    for quantity in (-1,0,1,3,6):
        for spots in (0,1,0xc0):
            for boss,fort in ((0,0),(1,0),(1,1)):result.append(case(op='P',quantity=quantity,spots=spots,boss=boss,fort=fort))
    # raw cost를빼기전현재pool을float로저장하는경계와emptyGeyser삭제다.
    for pool in (-1,0,1999,2000,16777217,0x7fffffff):
        for empty in (0,1):result.append(case(op='P',quantity=3,pool=pool,empty=empty))
    # 마지막eligiblePlayer의최근목록만남고localPRNG는계속소비한다.
    for players in range(5):
        for predictable in (0,1):
            for quantity in (0,1,3):result.append(case(op='G',players=players,predictable=predictable,first=quantity,last=quantity+1))
    # Pop/현재Player사각형효과뒤의pool/cost/수량/후속Player재조회를확인한다.
    for mutation in (1,2):result.append(case(op='P' if mutation==1 else 'G',mutation=mutation))
    # 현재3×3isle프레임문자의대소문자일치거부와다른무늬의허용을확인한다.
    for surfaces in (1,2,3):
        for op in ('T','P'):result.append(case(op=op,surfaces=surfaces))
    unique={}
    # 중복된명시입력을새관찰수에다시세지않는다.
    for value in result:unique.setdefault(json.dumps(value,sort_keys=True),value)
    return list(unique.values())


def player_input(profile):
    """9개의현재flags/region을그대로준비한다."""
    flags=[0]*9;regions=[127]*9
    if profile==1:flags[1]=2;regions[1]=1
    elif profile==2:flags[1]=2;regions[1]=1;flags[2]=32;regions[2]=2
    elif profile==3:flags[4]=34;regions[4]=4;flags[5]=2;regions[5]=5
    elif profile==4:
        flags=[0,1,2,32,34,0,0,2,32];regions=[127,1,127,3,4,5,127,7,8]
    return flags,regions


class Oracle:
    """현재로컬경로와기존난수/절삭/TimeSeed만실행하며다른몸체/OS는거부한다."""
    def __init__(self):
        """보호PE사본과선택export의정확범위/외부가상경계를등록한다."""
        if common.digest(common.BINARY)!=common.SHA:raise RuntimeError('로컬간헐천 보호PE SHA불일치')
        pe=common.pefile.PE(str(common.BINARY));self.data=pe.get_memory_mapped_image();self.base=pe.OPTIONAL_HEADER.ImageBase
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(self.data)+4095)&~4095);self.mu.mem_write(self.base,self.data)
        for address,size in ((POOL,0x10000),(TYPES,0x20000),(PLAYERS,0x1000),(SPOTS,0x10000),(RANDOM,0x1000),(FINDER,0x1000),(WORDS,0x20000),(THREAD,0x1000),(STOP,0x1000),(common.STACK_BASE,0x10000)):self.mu.mem_map(address,size)
        self.paths=[ROOT/'extracted/local-geyser/originals/functions.tsv',ROOT/'extracted/fortfinishowner/originals/functions.tsv']
        self.allowed=[];self.entries={}
        # 기존TimeSeed/TYPEgetter만보충하며관계없는Fort상위는실행하지않는다.
        for index,path in enumerate(self.paths):
            with path.open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    if index and row['entry'] not in ('004bcab0','0049a840'):continue
                    if not index and row['entry'] in ('0048f5c0','0048f550'):continue
                    self.entries[int(row['entry'],16)]=row['entry']
                    for part in row['ranges'].split(';'):
                        low,high=(int(item,16) for item in part.split('-'));self.allowed.append((low,high+1))
        # 실제PE바이트에서확인한프레임문자getter와CRT ASCII비교두몸체다.
        self.allowed.extend([(0x4ad680,0x4ad6be),(0x4e65ed,0x4e6656),(0x4ef510,0x4ef55e)])
        self.entries.update({0x4ad680:'004ad680',0x4e65ed:'004e65ed',0x4ef510:'004ef510'})
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.native=collections.Counter();self.boundaries=collections.Counter();self.returns=0

    def get(self,address,width=4):
        """현재실제폭의가상raw비트를읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시입력/외부효과만원본폭으로쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def ret(self,result=0,cleanup=0):
        """원본외부하위의caller/callee정리규약대로복귀한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);target=self.get(esp);self.mu.reg_write(UC_X86_REG_EAX,result);self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);self.mu.reg_write(UC_X86_REG_EIP,target)

    def instruction(self,mu,address,size,data):
        """생성/공간/Tell가없는명시하위만관찰하고candidate/반복/난수는실행한다."""
        if address==STOP:return
        if address in self.entries:self.native[self.entries[address]]+=1
        esp=mu.reg_read(UC_X86_REG_ESP);kind={0x441470:'Config',0x460d70:'Time',0x48f5c0:'Bounds',0x4b1fa0:'Find',
            0x444590:'Confirm',0x4af530:'Create',DESTROY:'Destroy',POP:'Pop',0x4e0620:'Assert',0x4ea61d:'Thread'}.get(address)
        if kind:
            self.boundaries[kind]+=1
            if kind=='Thread':self.ret(THREAD);return
            if kind=='Config':
                if self.get(esp+8)!=1:raise RuntimeError('predictableGeysers 기본인자불일치')
                self.trace.append('Config');self.ret(self.c['predictable']);return
            if kind=='Time':self.trace.append('Time');output=self.get(esp+4);self.put(output,self.c['time'],8);self.ret(output);return
            if kind=='Bounds':
                owner=(mu.reg_read(UC_X86_REG_ECX)-PLAYERS)//0xb4;self.trace.append('Bounds:'+str(owner));output=self.get(esp+4)
                mu.mem_write(output,struct.pack('<4f',16.0*owner,16.0*owner,16.0*owner+32.0,16.0*owner+32.0))
                if self.c['mutation']==2 and owner==1:self.put(0x540bd4,1);self.put(PLAYERS+2*0xb4+0x50,32,1);self.put(PLAYERS+2*0xb4+0x78,2)
                self.ret(output,4);return
            if kind=='Assert':self.trace.append('Assert');self.ret();return
            if kind=='Find':
                x,y,typ=(self.get(esp+i) for i in (4,8,12))
                if typ!=84:raise RuntimeError('로컬간헐천현재empty조회타입불일치')
                self.ret(9 if self.c['empty'] else 0);return
            if kind=='Destroy':
                if mu.reg_read(UC_X86_REG_ECX)!=POOL+9*50 or self.get(esp+4)!=0x400000:raise RuntimeError('실제emptyGeyser삭제인자불일치')
                self.trace.append('Destroy:9:4194304');self.ret(cleanup=4);return
            if kind=='Confirm':
                values=[self.get(esp+4+i*4) for i in range(10)];self.trace.append('Confirm:'+','.join(map(str,values)));self.ret();return
            if kind=='Create':
                typ,flags=self.get(esp+4),self.get(esp+8);sid=50+self.created;self.created+=1;self.trace.append(f'Create:{typ}:{flags}:{sid}')
                self.put(POOL+sid*50,VTABLE);self.put(POOL+sid*50+10,typ,1);self.ret(POOL+sid*50);return
            if kind=='Pop':
                sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;x,y,flags=(self.get(esp+i) for i in (4,8,12));self.trace.append(f'Pop:{sid}:{x}:{y}:{flags}')
                if self.c['mutation']==1:self.put(0x595088,16777217);self.put(TYPES+122*500+0xc4,common.bits(4000.0,4));self.put(0x595094,4)
                self.ret(cleanup=12);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미허용로컬간헐천native실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제body는stack/localPRNG/후보/최근5/pool밖을바꾸지못한다."""
        allowed=[(common.STACK_BASE,common.STACK_BASE+0x10000),(RANDOM,RANDOM+4),(FINDER,FINDER+0x200),(0x595088,0x5950b0)]
        if not any(low<=address and address+size<=high for low,high in allowed):raise RuntimeError(f'미허용로컬간헐천쓰기 {address:08x}+{size}')

    def run(self,value):
        """원본입력만준비하고실제최상위정상반환에서좌표/난수/목록/상태/순서를얻는다."""
        self.c=dict(value);self.trace=[];self.created=0;mu=self.mu
        mu.mem_write(POOL,bytes(0x10000));mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(PLAYERS,bytes(0x1000));mu.mem_write(SPOTS,bytes([value['spots']])*65536)
        mu.mem_write(WORDS,bytes(0x20000));self.put(0x5c84bc,WORDS);self.put(0x5411dc,97)
        self.put(THREAD+0x64,LOCALE);self.put(0x543efc,LOCALE);self.put(LOCALE+0x14,0)
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x59534c,PLAYERS);self.put(0x5c7c44,SPOTS);self.put(0x541348,188)
        self.put(0x540bc4,value['boss']);self.put(0x594fb8,value['fort']);self.put(0x540bd4,value['first']);self.put(0x540bd8,value['last'])
        self.put(0x5411a8,84);self.put(0x541240,122);self.put(0x5412cc,157);self.put(TYPES+122*500+0xc4,common.bits(2000.0,4));self.put(0x595088,value['pool'])
        self.put(0x595094,3)
        for index in range(7):self.put(RECENT+index*4,31+index)
        self.put(POOL+9*50,VTABLE);self.put(VTABLE+0x10,DESTROY);self.put(VTABLE+0x90,POP)
        self.put(POOL+9*50+14,common.bits(255.0,4));self.put(POOL+9*50+18,common.bits(255.0,4))
        if value['surfaces']:
            pattern='FFFFFFFFF' if value['surfaces']==1 else ('FBICANGDH' if value['surfaces']==2 else 'fbicangdh')
            mu.mem_write(CODES,b''.join(bytes((ord(letter),65,index,0)) for index,letter in enumerate(pattern)));self.put(TYPES+97*500+0x124,CODES)
            for index in range(9):
                x=31+index//3;y=35+index%3;sid=20+index
                self.put(WORDS+(y*256+x)*2,sid,2);self.put(POOL+sid*50+10,97,1);self.put(POOL+sid*50+36,index)
        flags,regions=player_input(value['players'])
        if value['mutation']==2:flags[1]=2;regions[1]=1
        for owner in range(9):self.put(PLAYERS+owner*0xb4+0x50,flags[owner],1);self.put(PLAYERS+owner*0xb4+0x78,regions[owner])
        self.put(RANDOM,value['seed']);mu.mem_write(FINDER,bytes(0x200));self.put(POINT,0xdeadbeef);self.put(POINT+4,0xcafebabe)
        # 정상finite floor의실제CRTfastpath선택입력이며명령/반환을대체하지않는다.
        self.put(0x5e5340,1);mu.reg_write(UC_X86_REG_MXCSR,0x1f80)
        rect=rectangles(value['rectangle']);args=[];entry=0x486360;cleanup=0;ecx=0
        if value['op']=='P':args=[value['quantity']&0xffffffff,RANDOM,*[common.bits(item,4) for item in rect],0,0,1];entry=0x486130
        elif value['op']=='T':
            mu.mem_write(FINDER,struct.pack('<4f',*rect));self.put(FINDER+0xa8,1);self.put(FINDER+0xac,84);self.put(FINDER+0xb0,1);args=[RANDOM,POINT];entry=0x485890;cleanup=8;ecx=FINDER
        for reg,sentinel in common.SAVED.items():mu.reg_write(reg,sentinel)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_ECX,ecx);mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.emu_start(entry,STOP,count=4000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+cleanup:raise RuntimeError('로컬간헐천정상반환/스택오류')
        if any(mu.reg_read(reg)!=sentinel for reg,sentinel in common.SAVED.items()):raise RuntimeError('로컬간헐천보존레지스터오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=0x027f or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('로컬간헐천x87정상반환오류')
        self.returns+=1
        return [mu.reg_read(UC_X86_REG_EAX) if value['op']=='T' else 0,self.get(POINT),self.get(POINT+4),self.get(RANDOM),self.get(0x595094),
            ','.join(str(self.get(RECENT+i*4)) for i in range(7)),self.get(0x595088),self.get(0x540bd4),self.created,'|'.join(self.trace) or '-'] # 빈관찰열은끝TAB대신sentinel로저장한다.


def generate():
    """원본독립출력과정상반환/파일해시를저장한다. C++는읽거나호출하지않는다."""
    oracle=Oracle();rows=[]
    for value in cases():rows.append([*[value[key] for key in KEYS],*oracle.run(value)])
    FIXTURE.write_text('# PATCH local args0/0/1 실제PE 독립관찰. GUI/OS실행0.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),Path(common.__file__),common.BINARY,FIXTURE,*oracle.paths,ROOT/'extracted/local-geyser/originals/creation.c']
    REPORT.write_text(json.dumps(dict(host=common.socket.gethostname(),date='2026-10-15',total=len(rows),returns=oracle.returns,native=dict(oracle.native),boundaries=dict(oracle.boundaries),os_calls=0,
        files={path.relative_to(ROOT).as_posix():common.digest(path) for path in paths},
        limits=['PATCH의args0/0/1및부모의empty타입재사용/현재isle프레임무늬검사를실행한다. genericexclusion모드는범위밖이다.',
            '실제부모/후보/100회/최근5/pool/난수/CRT정상finitefloor/절삭은기계어다. 생성/Pop/typedFind/건설확정/Player범위/설정/벽시각은명시관찰경계다.',
            '실제GUI/원본게임/캠페인완주및동일풀Session연결은이번검사로증명하지않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('로컬간헐천실제PE',len(rows),'정상반환',dict(oracle.native),flush=True)


def verify():
    """저장근거의해시/명시입력계획/정상반환수를재계산없이검사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];plan=cases()
    for path,sha in report['files'].items():
        if common.digest(ROOT/path)!=sha:raise RuntimeError('로컬간헐천근거SHA불일치 '+path)
    if len(rows)!=len(plan) or report['returns']!=len(rows) or report['os_calls']:raise RuntimeError('로컬간헐천실행수불일치')
    for row,value in zip(rows,plan):
        if len(row)!=26 or row[:16]!=[str(value[key]) for key in KEYS]:raise RuntimeError('로컬간헐천입력계획불일치')
    print('로컬간헐천독립근거감사통과',len(rows),flush=True)


def main():
    """요청된독립관찰생성또는저장자료감사를수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate()


if __name__=='__main__':main()
