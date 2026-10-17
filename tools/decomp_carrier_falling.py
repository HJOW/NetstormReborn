#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 실제 캐리어 낙하/다리 판정/사제 해방 몸체를 격리 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,digest
from decomp_cursor_position import ranges
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

POOL,HASH,CODES,PLAYERS=0x14000000,0x14100000,0x14200000,0x14300000  # 독립 raw/hash0/코드/Player 입력이다.
DESTROY,ALT_DESTROY,UNPOP,POP,ALT_POP,OWNER,ALT_OWNER,FALL,ALT_FALL=(STOP+0x200+32*i for i in range(9))  # 재진입 뒤 현재 가상 효과를 구별한다.
CALLER,RESULT,ALT_VT=STOP+0x500,STOP+0x580,STOP+0x800  # 실제 float32 호출자/결과/교체 가상 표다.
INPUT,THUNK=STOP+0x5a0,STOP+0xb00  # cdecl 인자 메모리와 변경하지 않는 실제 호출 코드다.
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-falling-x86.tsv'  # 새 원문이며 기존 기대값은 변경하지 않는다.
REPORT=ROOT/'cpppj/carrier-falling.json'  # 실제 실행 몸체/정밀도/보호 해시 기록이다.
VTABLES=ROOT/'cpppj/tests/fixtures/carrier-falling-vtables.tsv'  # 실제 사제/여섯 캐리어/보호막의 수명과 낙하 슬롯이다.
ENTRIES=(0x4225b0,0x426ef0,0x426c80,0x40e800,0x4ac200,0x4ac550,0x49a840,0x4e49c0)  # 허용할 실제 몸체 전체다.


def points():
    """정상/경계/무효 변환/하위 DWORD 별칭을 독립 입력으로 제공한다."""
    return tuple((bits(x),bits(y)) for x,y in ((20.5,21.25),(0,0),(-1,-1),(-2,1),(255,255),(256,256),(2**32,1),(1,2**32),(254.999,255.001),(1,1)))+((0x7fc01234,bits(20)),(bits(20),0x7fc01234),(0x7f800000,bits(1)),(bits(1),0xff800000))


def cases():
    """좌표/현재 코드/권한/포획물/지연된 진단/가상 표 변경을 교차한다."""
    base=[0,1,*points()[0],1,0,51,51,0x1281,bits(30.5),bits(31.5),256,4,0,32,1,0,0,158];rows=[]
    # op/count/x/y/boss/checking/masked/kind/word/homeX/homeY/limit/genus/frame/code/surface/pattern/mutation/priest다.
    for point,genus,frame,code,surface,pattern in itertools.product(points(),(0,4,0xffffffff),(0,1,7),(0,32,255),(0,1),(0,1)):
        row=base.copy();row[2:4]=point;row[12:17]=genus,frame,code,surface,pattern;rows.append(tuple(row))
    # signed count와 실제 낙하 float 저장/상한/권한/내용물 검색을 모두 관측한다.
    for count,point,boss,masked,mutation in itertools.product((0,1,9,10,11,100,101,0x7fffffff,0x80000000,0xffffffff),points(),(0,1),(0,51),(0,1,2,10)):
        row=base.copy();row[0:2]=1,count;row[2:4]=point;row[4]=boss;row[6]=masked;row[17]=mutation;rows.append(tuple(row))
    # 진단이 복귀하는 경우에도 캡처한 좌표와 현재 소유자/TYPE/가상 표를 구별한다.
    for boss,checking,word,home,limit,mutation in itertools.product((0,1),(0,1),(0,0x1281,0xffff),points(),(0,256,0x7fffffff,0xffffffff),(0,3,4,5,6,7,8,9,10)):
        row=base.copy();row[0]=2;row[4:6]=boss,checking;row[8:12]=word,*home,limit;row[17]=mutation;rows.append(tuple(row))
    # 좌표 검증의 signed 제한과 float32 반올림을 실제 보조 함수로 관측한다.
    for point,limit in itertools.product(points(),(0,1,256,0xffffffff,0x7fffffff,16777217)):
        row=base.copy();row[0]=3;row[2:4]=point;row[11]=limit;rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """8개 원본 몸체를 실행하고 기존 검색/생성/가상 효과/진단만 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE와 독립 상태 및 실제 float 반환 호출자를 준비한다."""
        super().__init__()
        # 실제 함수가 접근하는 독립 메모리만 추가한다.
        for address,size in ((POOL,0x10000),(HASH,0x20000),(CODES,0x1000),(PLAYERS,0x6000)):self.mu.mem_map(address,size)
        self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter()
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))
        self.mu.mem_write(THUNK,b'\xff\x35'+struct.pack('<I',INPUT+4)+b'\xff\x35'+struct.pack('<I',INPUT)+b'\xe8'+struct.pack('<i',0x4225b0-(THUNK+17))+b'\x83\xc4\x08\xc3');self.allowed.append((THUNK,THUNK+21))
        # 원래 가상 표와 교체 표에서 서로 다른 실제 효과 경계를 사용한다.
        for table,destroy,pop,owner,fall in ((0x50f210,DESTROY,POP,OWNER,FALL),(ALT_VT,ALT_DESTROY,ALT_POP,ALT_OWNER,ALT_FALL)):
            self.put(table+0x10,destroy);self.put(table+0x48,UNPOP);self.put(table+0x90,pop);self.put(table+0x74,owner);self.put(table+0xc8,fall)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
        self.boards={0:bytes(0x20000),1:struct.pack('<H',60)*65536,2:b''.join(struct.pack('<H',0 if i%7==0 else 60+i%2) for i in range(65536))}  # 고정 독립 지도 입력을 재사용한다.

    def instruction(self,mu,address,size,data):
        """검색/생성 효과에서 현재 입력이 바뀌는 경우까지 순서대로 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<4I',mu.mem_read(sp+4,16));this=mu.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50;mutation=self.case[17]
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if CALLER<=address<CALLER+11:return
        if address==0x4ac9f0:
            self.event('M',sid,*args[:3])
            if mutation==2:self.put(0x540bc4,0)
            self.ret(self.case[6],12);return
        if address==0x4b26f0:
            self.event('K',*args)
            if mutation==3:self.put(0x5412d0,159);self.put(POOL+51*50+12,0x2342,2)
            self.ret(self.case[7]);return
        if address==0x4af530:
            self.event('C',*args[:2])
            if mutation==4:self.put(POOL+51*50+12,0x3443,2)
            self.ret(POOL+52*50);return
        if address==0x4ac220:
            self.event('W',sid,args[0]);self.put(this+12,args[0],2)
            if mutation==5:self.put(this,ALT_VT)
            self.ret(0,4);return
        if address in (OWNER,ALT_OWNER):
            self.event('O' if address==OWNER else 'o',sid,args[0]);self.put(this+34,args[0],1)
            if mutation==6:self.put(POOL+51*50+12,4,1);self.put(this,ALT_VT)
            self.ret(0,4);return
        if address in (DESTROY,ALT_DESTROY):self.event('D' if address==DESTROY else 'd',sid,args[0]);self.put(this+11,(self.get(this+11)&255)|2,1);self.ret(0,4);return
        if address==UNPOP:
            self.event('U',sid,args[0]);self.put(this+11,(self.get(this+11)&255)|4,1)
            if mutation==1:self.put(this,ALT_VT);self.put(this+14,bits(42.5));self.put(this+18,bits(43.5))
            self.ret(0,4);return
        if address in (POP,ALT_POP):
            self.event('P' if address==POP else 'p',sid,*args[:3]);self.put(this+14,args[0]);self.put(this+18,args[1]);self.put(this+11,(self.get(this+11)&255)&~4,1)
            if mutation==9:self.put(this,ALT_VT);self.put(POOL+51*50,ALT_VT)
            self.ret(0,12);return
        if address in (FALL,ALT_FALL):
            self.event('F' if address==FALL else 'f',sid)
            if mutation==10:self.put(POOL+51*50,ALT_VT);self.put(POOL+50*50,ALT_VT)
            self.ret(1);return
        if address==0x4e0620:
            self.event('A',args[2]);assert self.string(args[0])==('boss' if args[2]==0x3bf else 'priestPos.isValid()') and self.string(args[1])=='\\Ns\\O\\Carrier.cpp'
            if mutation==7:self.put(0x540bc4,1)
            if mutation==8:self.mu.mem_write(PLAYERS,bytes(0x6000));self.put(POOL+52*50,ALT_VT)
            self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'캐리어 낙하 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """직접 쓰기를 z BYTE/스택/caller/SEH로 한정한다."""
        if STACK-0x7f000<=address<=STACK+0x1000-size or address==0 and size==4 or address==RESULT and size==4 or address==POOL+50*50+35 and size==1:return
        raise RuntimeError(f'캐리어 낙하 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """허용한 독립 raw/코드/hash0/Player/TYPE/보호 이미지/호출자만 읽는다."""
        regions=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+188*500),(HASH,HASH+0x20000),(CODES,CODES+0x1000),(PLAYERS,PLAYERS+0x6000),(STOP,STOP+0x1000),(STACK-0x7f000,STACK+0x1000))
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'캐리어 낙하 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,case,precision):
        """독립 입력을 제공하고 원본 반환/현재 raw/효과 순서만 기록한다."""
        self.case=case;self.events=[]
        # 세 객체는 물리 raw로 초기화하며 free/dead 필터를 대신 추가하지 않는다.
        for sid in (50,51,52):
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x50f210);raw[10]=158;raw[11]=0;struct.pack_into('<II',raw,14,*case[2:4]);self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(POOL+51*50+12,case[8],2);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x542514,HASH);self.put(0x59534c,PLAYERS);self.put(0x531928,case[11]);self.put(0x540bc4,case[4]);self.put(0x5e4794,case[5]);self.put(0x5412d0,case[18]);self.put(0,0x87654321)
        # 현재 TYPE/프레임과 고정 hash0 패턴은 입력이며 기대 좌표를 계산하지 않는다.
        for sid,kind,genus in ((60,70,case[12]),(61,71,case[12]^4)):
            self.put(POOL+sid*50+10,kind,1);self.put(POOL+sid*50+36,case[13]);self.put(TYPES+kind*500+0xec,genus);self.put(TYPES+kind*500+0x124,CODES+(kind-70)*32)
            self.mu.mem_write(CODES+(kind-70)*32,b''.join(bytes((65,0,i,case[14]^(i*17)^(32 if kind==71 else 0))) for i in range(8)))
        self.mu.mem_write(HASH,self.boards[2 if case[16] else case[15]])
        # 원본은 이전 소유자를 마스킹한 뒤 실제 b4 크기 기록을 읽는다.
        for owner in range(128):self.put(PLAYERS+owner*0xb4+0xa8,case[9]);self.put(PLAYERS+owner*0xb4+0xac,case[10])
        op=case[0];entry=ENTRIES[op] if op<3 else 0x40e800;args=struct.pack('<2I',*case[2:4]) if op==0 else struct.pack('<I',case[1]) if op==1 else b''
        if op==3:self.put(POOL+50*50,case[2]);self.put(POOL+50*50+4,case[3])
        # GP/canary/제어어/빈 x87 스택을 호출자에서 검사한다.
        if op==0:args_for_invoke=b''
        else:args_for_invoke=args
        # cdecl 좌표는 고정 코드가 메모리에서 읽는다. 입력마다 코드를 고치지 않아 번역 캐시를 재사용해도 안전하다.
        if op==0:
            self.put(INPUT,case[2]);self.put(INPUT+4,case[3]);entry=THUNK
        self.invoke(entry,args_for_invoke,precision,op==1)
        self.returns+=1;assert self.get(0)==0x87654321
        result=self.get(RESULT) if op==1 else 0 if op==2 else self.mu.reg_read(UC_X86_REG_EAX)
        return str(result),';'.join(self.events) or '-',','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,51,52)),str(self.get(0x540bc4)),str(self.get(0x5412d0))

    def invoke(self,entry,args,precision,floating):
        """실제 retN/GP/스택 canary/SEH/x87 제어어와 빈 스택을 검사한다."""
        registers={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        self.mu.mem_write(STACK,struct.pack('<I',CALLER if floating else STOP)+args+b'\xc0\xde\xc0\xde')
        # 비휘발 레지스터를 매 입력마다 독립 초기화한다.
        for register,value in registers.items():self.mu.reg_write(register,value)
        # 휘발 레지스터와 FPU 상태도 독립 초기화한다.
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_EDX,0),(UC_X86_REG_ECX,POOL+50*50),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):self.mu.reg_write(register,value)
        self.mu.emu_start(entry,STOP,count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)
        assert bytes(self.mu.mem_read(STACK+4+len(args),4))==b'\xc0\xde\xc0\xde'
        # 원본 몸체가 모든 보존 레지스터를 되돌렸는지 확인한다.
        for register,value in registers.items():assert self.mu.reg_read(register)==value
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800 and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff


def main():
    """새 원문을 생성하거나 기대값/보호 해시를 변경하지 않고 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        # 보호 원본과 현재 도구/원문을 각각 감사한다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert VTABLES.read_text('utf-8')==virtual_tables()
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'] and len(rows)*2==report['normalReturns']
        assert [i for i,r in enumerate(rows) if r[1:6]!=r[6:11]]==report['precisionDifferentInputs'];print('캐리어 낙하 원본 감사:',len(rows),'입력/',report['normalReturns'],'정상 반환');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 두 정밀도를 독립 실행하며 제품 C++를 기대값 생성에 사용하지 않는다.
    for index,case in enumerate(cases()):
        outputs=[o.sample(case,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,case)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input return53 effects53 raws53 boss53 priest53 return64 effects64 raws64 boss64 priest64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    VTABLES.write_text(virtual_tables(),encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_falling.py','tools/decomp_nugget_regular.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-falling-x86.tsv','cpppj/tests/fixtures/carrier-falling-vtables.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-17',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),controls=[0x27f,0x37f],uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,boundaries='4225b0/426ef0/426c80/40e800/TYPEgetter/_ftol 전체. 검색4ac9f0/4b26f0·생성/WORD/현재 가상 효과/복귀하는 진단은 명시 경계. 현재 TYPE/Owner/VT 재조회·캡처 좌표 검사. 실제 caller float32/GP/스택/canary/SEH/x87/코드·읽기·쓰기 범위 검사. 실제 OS/게임 창 없음.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


def virtual_tables():
    """보호 PE에서 실제 여덟 표의 슬롯 DWORD를 직접 읽으며 예상 주소를 주입하지 않는다."""
    image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image();rows=[]
    # 삭제/전후처리/Release/Unpop/Pop/낙하의 실제 주소를 모두 보존한다.
    for table in (0x507770,0x50f210,0x503d80,0x503e50,0x503f20,0x503ff0,0x5040c0,0x50e568):
        rows.append('\t'.join(str(value) for value in (table,*(struct.unpack_from('<I',image,table-0x400000+offset)[0] for offset in (0x10,0x14,0x18,0x2c,0x48,0x90,0xc8)))))
    return '# vtable destroy pre post release unpop pop fallWalker\n'+'\n'.join(rows)+'\n'


if __name__=='__main__':main()
