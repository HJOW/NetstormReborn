#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의4287d0/426db0/41d7a0/_ftol을 독립 실행해 자연 낙하 시작을 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import zlib
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL,PLAYERS
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

PROCESS=0x14400000  # 실제40바이트 Regular의 명시 외부 생성/기존 객체다.
FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-fall-start-x86.tsv'  # 새 원문이며 커밋된 이전 기대값은 바꾸지 않는다.
REPORT=ROOT/'cpppj/carrier-fall-start.json'  # 실제 두 정밀도/정상 반환/NULL 쓰기 근거다.
ENTRIES=(0x4287d0,0x426db0,0x41d7a0,0x4e49c0)  # 명령 범위를 벗어난 계산은 허용하지 않는다.


def double_words(value):
    """현재 double 시각의 원문 두 DWORD다."""
    return struct.unpack('<2I',struct.pack('<d',value))


def cases():
    """현재 task/할당/선택/소유자/좌표/권한과 효과 뒤 재조회를 교차한다."""
    base=[0,0x5105,bits(.1),*double_words(12.345),1,0x5101,7,bits(.75),1,0,0,51,0x1281,bits(20.25),bits(21.5),158,35,0,1];rows=[]
    # op/task/delay/nowLo/nowHi/existing/event/count/payload/boss/extra/hasFall/kind/word/x/y/priest/movement/mutation/allocation다.
    for task,existing,event,delay,now,mutation,allocation in itertools.product((0x5105,0x5101,0xffffffff),(0,1),(0x5105,0x5101),(0,bits(.1),bits(-.25),0x7fc01234),(12.345,2**30+.125,2**53,float('inf'),float('nan')),(0,5,6,7),(0,1)):
        row=base.copy();row[1:7]=task,delay,*double_words(now),existing,event;row[18:20]=mutation,allocation;rows.append(tuple(row))
    points=((20.25,21.5),(254.99999,255.00001),(-2,-.5),(2**32,2**32+512),(0.00001,.99999))
    # 원문 몸체의 결과를 대신 계산하지 않고 독립 입력만 제공한다.
    for boss,extra,existing,has_fall,kind,point,mutation in itertools.product((0,1),(0,4),(0,1),(0,1),(0,51),points,range(5)):
        row=base.copy();row[0]=1;row[5]=existing;row[9:13]=boss,extra,has_fall,kind;row[14:16]=map(bits,point);row[18]=mutation;rows.append(tuple(row))
    # 무효 변환과 owner BYTE 마스크를 독립 입력으로 관측한다.
    for point,word in itertools.product(((0x7fc01234,bits(20)),(bits(20),0x7f800000),(0xff800000,0x7fc01234)),(0,0x7f,0x80,0xffff)):
        row=base.copy();row[0]=1;row[5]=0;row[13:16]=word,*point;rows.append(tuple(row))
    # 양쪽 NaN의 부호/서로 다른 payload와 음의0/무한 시각도 독립 관측한다.
    for delay,clock,existing in itertools.product((0,0x80000000,0x7fc00000,0x7fc01234,0xffc01234,0x7f800000,0xff800000),(0x7ff8000000000001,0x7fffffffffffffff,0xfff8000000000001,0x8000000000000000,0xfff0000000000000),(0,1)):
        row=base.copy();row[2]=delay;row[3:5]=clock&0xffffffff,clock>>32;row[5]=existing;rows.append(tuple(row))
    # 같은 NaN payload의 반대 부호와 signaling 입력의 실제 quiet 변환도 별도로 보존한다.
    for delay,clock,existing in itertools.product((bits(.1),0x7fc00000,0x7fc01234,0xffc01234,0x7f800001,0xff800001),(0x7ff8024680000000,0xfff8024680000000,0x7ff0000000000001,0xfff0000000000001),(0,1)):
        row=base.copy();row[2]=delay;row[3:5]=clock&0xffffffff,clock>>32;row[5]=existing;rows.append(tuple(row))
    return list(dict.fromkeys(rows))


class NullProcessWrite(RuntimeError):
    """원본 new 실패 뒤 실제 NULL+10 쓰기를 정상 반환과 분리한다."""


class Oracle(BaseOracle):
    """부모 두 몸체/실제 거의 올림/CRT를 실행하며 큰 외부 효과만 기록한다."""
    def __init__(self):
        """독립40바이트 process와 Player 기록을 추가한다."""
        super().__init__()
        # 원본은 읽기 전용이고 이 세 입력 영역만 새로 추가한다.
        for address,size in ((POOL,0x10000),(PLAYERS,0x6000),(PROCESS,0x1000)):self.mu.mem_map(address,size)
        self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.faults=0;self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.player_input=bytes((i*17+29)&255 for i in range(128*0xb4))

    def instruction(self,mu,address,size,data):
        """기존 task/생성/이동 삭제/선택/해방이 바꾼 현재 상태도 보존한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<4I',mu.mem_read(sp+4,16));this=mu.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50;mutation=self.case[18]
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4afbe0:
            self.event('Q',sid,*args[:2])
            if mutation==5:self.mu.mem_write(0x55b4d0,struct.pack('<d',42.5))
            self.ret(PROCESS if self.case[5] else 0,8);return
        if address==0x4afb40:self.event('H',sid,args[0]);self.ret(PROCESS if self.case[11] else 0,4);return
        if address==0x4e4391:self.event('A',args[0]);self.ret(PROCESS if self.case[19] else 0);return
        if address in (0x496e80,0x496f00):
            self.event('C',46 if address==0x496e80 else 61,*args[:3]);self.put(PROCESS+24,args[0]);self.put(PROCESS+28,args[2]);self.put(PROCESS+32,0);self.mu.mem_write(PROCESS+16,bytes(8))
            if mutation==6:self.put(PROCESS+24,0x515a);self.put(PROCESS+32,99);self.put(PROCESS+28,bits(2.5));self.mu.mem_write(0x55b4d0,struct.pack('<d',123.25))
            self.ret(PROCESS,12);return
        if address==0x4e0620:
            self.event('!',args[2]);assert self.string(args[0])=='task == taskFALL' and self.string(args[1])=='\\Ns\\O\\Carrier.cpp' and args[2]==0x407
            if mutation==7:self.mu.mem_write(0x55b4d0,struct.pack('<d',64.25));self.put(PROCESS+24,0x6161);self.put(PROCESS+32,77);self.put(PROCESS+28,bits(.5))
            self.ret();return
        if address==0x488fe0:self.event('M',*args[:3]);self.ret(self.case[17]);return
        if address==0x4b1a90:
            self.event('D',*args)
            if mutation==1:self.put(0x540bc4,0);self.put(POOL+50*50+40,4,1);self.put(0x5412d0,159);self.put(POOL+50*50+14,bits(42.125));self.put(POOL+50*50+18,bits(43.25))
            self.ret(0);return
        if address==0x4d5c90:
            self.event('S',*args[:3]);self.put(POOL+50*50+40,(self.get(POOL+50*50+40)&255)&~4,1)
            if mutation==2:self.put(0x540bc4,1);self.put(0x5412d0,159);self.put(POOL+50*50+14,bits(43.5));self.put(POOL+50*50+18,bits(44.5));self.put(POOL+51*50+12,0x2342,2)
            self.ret(0);return
        if address==0x4b26f0:
            self.event('K',*args)
            if mutation==3:self.put(0x540bc4,1);self.put(0x5412d0,160);self.put(POOL+50*50+14,bits(.00001));self.put(POOL+50*50+18,bits(254.99999));self.put(POOL+51*50+12,0x3443,2)
            self.ret(self.case[12]);return
        if address==0x426c80:
            self.event('R',sid)
            if mutation==4:self.mu.mem_write(0x55b4d0,struct.pack('<d',88.875));self.put(POOL+51*50+12,0,2);self.put(POOL+50*50+14,bits(55));self.put(POOL+50*50+18,bits(66))
            self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'캐리어 시작 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 task/Player 위치/스택/SEH만 쓰며 NULL은 별도 실패로 기록한다."""
        if address in (0x10,0x14):raise NullProcessWrite('원본 new28 실패 뒤 NULL 기한 쓰기')
        if STACK-0x7f000<=address<=STACK+0x1000-size or address==0 and size==4 or address in (PROCESS+16,PROCESS+20,PROCESS+24,PROCESS+32) and size==4:return
        if PLAYERS<=address<PLAYERS+128*0xb4 and (address-PLAYERS)%0xb4 in (0xa8,0xac) and size==4:return
        raise RuntimeError(f'캐리어 시작 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 이미지와 현재 raw/process/Player/스택만 읽는다."""
        regions=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(PLAYERS,PLAYERS+0x6000),(PROCESS,PROCESS+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'캐리어 시작 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,case,precision):
        """계산한 기대 시각/위치를 주입하지 않고 실제 결과와 전체 Player 체크섬을 수집한다."""
        self.case=case;self.events=[]
        # 부모와 Form은 원본의 물리 입력이며 상태 필터를 추가하지 않는다.
        for sid in (50,51):
            raw=bytearray(b'\xa5'*50);raw[10]=135;raw[40]=case[10];struct.pack_into('<II',raw,14,*case[14:16]);self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(POOL+51*50+12,case[13],2);self.mu.mem_write(PLAYERS,self.player_input);self.mu.mem_write(PROCESS,b'\xa5'*40);self.put(PROCESS+24,case[6]);self.put(PROCESS+28,case[8]);self.put(PROCESS+32,case[7]);self.mu.mem_write(PROCESS+16,struct.pack('<d',-100.5));self.put(0x5c8464,POOL);self.put(0x59534c,PLAYERS);self.put(0x540bc4,case[9]);self.put(0x5412d0,case[16]);self.mu.mem_write(0x55b4d0,struct.pack('<2I',*case[3:5]));self.put(0,0x87654321)
        status='ok'
        try:
            PreviousOracle.invoke(self,0x426db0 if case[0]==0 else 0x4287d0,struct.pack('<2I',*case[1:3]) if case[0]==0 else b'',precision,False);self.returns+=1;assert self.get(0)==0x87654321
        except NullProcessWrite:status='null';self.faults+=1
        result=0 if case[0]==0 or status!='ok' else self.mu.reg_read(UC_X86_REG_EAX)
        state=(result,self.get(PROCESS+24),self.get(PROCESS+32),self.get(PROCESS+28),self.get(PROCESS+16),self.get(PROCESS+20),self.get(0x55b4d0),self.get(0x55b4d4),self.get(0x540bc4),self.get(0x5412d0),zlib.adler32(bytes(self.mu.mem_read(PLAYERS,128*0xb4))))
        return status,';'.join(self.events) or '-',','.join(map(str,state)),','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in (50,51))


def main():
    """두 정밀도의 독립 원문을 기록하거나 기존 바이트를 변경하지 않고 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        # 도구/원문/보호 원본의 현재 해시를 각각 검사한다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'];assert sum(r[1]=='ok' for r in rows)*2==report['normalReturns'];assert sum(r[1]=='null' for r in rows)*2==report['nullWrites']
        assert [i for i,r in enumerate(rows) if r[1:5]!=r[5:9]]==report['precisionDifferentInputs'];print('캐리어 자연 시작 원본 감사:',len(rows),'입력/',report['normalReturns'],'정상 반환/',report['nullWrites'],'NULL 쓰기');return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 실제 몸체를 각 정밀도에서 실행하며 C++ 결과로 기대값을 생성하지 않는다.
    for index,case in enumerate(cases()):
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,case)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input status53 effects53 state53 raws53 status64 effects64 state64 raws64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_fall_start.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-fall-start-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-17',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),controls=[0x27f,0x37f],uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),nullWrites=sum(o.faults for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,boundaries='4287d0/426db0/41d7a0/_ftol 전체. 현재 조회/이동 삭제/선택/사제 해방/할당/두 생성자는 명시 경계다. 생성 뒤 event/count 보존, 현재 double 시각/콜백 뒤 재조회, 실제 Player 쓰기 확인. 정상 반환은 GP/retN/SEH/canary/x87/코드·읽기·쓰기 범위를 검사한다. NULL+10 쓰기는 실제 별도 실패이며 정상 반환으로 세지 않는다. OS/게임 창 실행 없음.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','nullWrites','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
