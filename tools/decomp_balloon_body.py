#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""벌룬 그림자42ac90/42ae80의 실제 재독·좌표·표시 깊이를 독립 관측한다."""
import argparse,itertools,json,socket,struct,collections
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,POOL,TYPES,digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_ECX

SPOTS=0x14600000 #기존 raw 슬롯과 겹치지 않는 현재 칸 지도다.
FIXTURE=ROOT/'cpppj/tests/fixtures/balloon-body-x86.tsv' #새 그림자 원문이다.
REPORT=ROOT/'cpppj/balloon-body.json' #정상 반환/실제 경계/해시 근거다.

def bits(value):
    """명시 float 입력을 원문 비트로 전달한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

def cases():
    """정답 없이 최초flag/프레임/그림자/고도/권한/칸/효과를 교차한다."""
    result=[]
    # op,flags,frame,shadow,shadowFlag,height,boss,spot,mutation,head,xZero다.
    for flags,frame,shadow,enabled,height,boss,spot in itertools.product((0,1),(0,4),(0,51),(0,0x400000),(0,22),(0,1),(0,16)):result.append((0,flags,frame,shadow,enabled,height,boss,spot,0,0,0))
    for mutation in range(1,6):result.append((0,1,0,0,0x400000,22,1,0,mutation,0,0))
    for flags,shadow in itertools.product((0,1),(0,51)):result.append((0,flags,0,shadow,0,0,0,0,0,0,1))
    for flags,shadow,boss,head,mutation in itertools.product((0,0x20,0xffffffff),(0,51),(0,1),(0,53),range(4)):result.append((1,flags,0,shadow,0,0,boss,0,mutation,head,0))
    return list(dict.fromkeys(result))

class Oracle(BaseOracle):
    """원문 두 몸체와 TYPE/CRT만 실행하고 큰 월드 효과는 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE와 별도 지도/TYPE/슬롯만 준비한다."""
        super().__init__();self.mu.mem_map(SPOTS,0x10000);self.allowed=[];ranges(self,(0x42ac90,0x42ae80,0x4ac550,0x49a840,0x4e49c0));self.instructions=0;self.counts=collections.Counter();self.pop=self.get(0x5040c0+0x90);self.check=self.get(0x5040c0+0xcc)

    def instruction(self,mu,address,size,data):
        """효과 뒤 현재 자료 변경은 입력으로만 제공한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);raw=POOL+50*50;sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;v=self.value
        if address==0x4e0620:raise RuntimeError(f'벌룬 그림자 원본 진단 {self.string(self.get(sp+4))} line{self.get(sp+12):x} 입력{v}')
        if address==0x4af530:
            self.event('C',self.get(sp+4),self.get(sp+8));born=52 if v[3] else 51
            if v[8]==1:self.put(raw+36,1);self.put(TYPES+111*500+0x114,5)
            self.ret(POOL+born*50);return
        if address==self.pop:
            self.event('P',sid,self.get(sp+4),self.get(sp+8),self.get(sp+12));self.popCount+=1
            if v[8]==2 and self.popCount==1:self.put(raw+14,bits(98.25));self.put(raw+18,bits(121.75));self.put(raw+30,30,1)
            if v[8]==4 and self.popCount==2:self.put(raw+8,52,2)
            self.ret(0,12);return
        if address==0x4afe50:
            self.event('U',sid,self.get(sp+4))
            if v[8]==3:self.put(raw+14,bits(13.5));self.put(raw+18,bits(23.25));self.put(raw+40,16,1);self.put(TYPES+111*500+0x114,6)
            self.ret(0,4);return
        if address==self.check:
            self.event('K',sid)
            if v[8]==5:self.put(0x540bc4,1)
            self.ret();return
        if address==0x44bf80:self.event('B',sid,self.get(sp+4));self.ret(0,4);return
        if address==0x4af780:
            self.event('X',sid,self.get(sp+4));self.put(POOL+sid*50+11,1,1)
            if v[8]==1:self.put(raw+8,0,2)
            if v[8]==2:self.put(0x540bc4,0)
            self.ret(0,4);return
        if address==0x44b4b0:
            self.event('D',sid,self.get(sp+4))
            if v[8]==3:self.put(0x540bc4,0)
            self.ret(0,4);return
        if address==0x4ac9f0:self.event('F',sid,self.get(sp+4),self.get(sp+8),self.get(sp+12));self.ret(POOL+53*50 if self.get(raw+6)&0xffff else 0,12);return
        if address==0x485e40:self.event('R');self.ret();return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'벌룬 그림자 범위밖 명령 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """원문 직접 쓰기는 세 raw 슬롯과 스택뿐이다."""
        if POOL+50*50<=address and address+size<=POOL+53*50 or STACK-0x7f000<=address and address+size<=STACK+0x1000:return
        raise RuntimeError(f'벌룬 그림자 범위밖 쓰기 {address:x}+{size}')

    def sample(self,value,precision):
        """두 전체 raw 슬롯/현재 전역/효과를 원문 그대로 보존한다."""
        self.value=value;self.events=[];self.popCount=0;op,flags,frame,shadow,enabled,height,boss,spot,mutation,head,xzero=value
        # 부모와 두 가능한 그림자는 서로 구별되는 명시 초기 자료다.
        for sid in (50,51,52,53):
            raw=bytearray(50);struct.pack_into('<I',raw,0,0x5040c0);raw[10]=111;raw[35]=17;raw[40]=0x10 if sid==50 else 2;struct.pack_into('<ff',raw,14,0. if sid==50 and xzero else 98.,122.);struct.pack_into('<I',raw,36,frame if sid==50 else 7)
            if sid==50:struct.pack_into('<HH',raw,6,head,shadow);raw[30]=height
            if sid==53:raw[10]=6;struct.pack_into('<HHHH',raw,18,111,1,0,50)
            self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(TYPES+111*500+0x114,4);self.put(TYPES+111*500+0xe8,enabled);self.put(0x540bc4,boss);self.put(0x5c7c44,SPOTS);self.mu.mem_write(SPOTS,bytes([spot])*65536);self.put(0,0x87654321)
        entry=0x42ae80 if op else 0x42ac90;self.call(entry,POOL+50*50,struct.pack('<I',flags),4,precision);assert self.get(0)==0x87654321;self.counts[f'{entry:08x}']+=1
        return ';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+50*50,150)).hex(),','.join(map(str,(self.get(0x540bc4),self.get(TYPES+111*500+0x114))))

def main():
    """독립 원문 생성 또는 저장 SHA/입력 계획만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text('utf-8').splitlines() if s and not s.startswith('#')]
        for p,h in report['sha256'].items():assert digest(ROOT/p)==h,p
        assert len(rows)==report['uniqueInputs'] and [r[0] for r in rows]==[','.join(map(str,v)) for v in cases()] and all(r[1:4]==r[4:7] for r in rows);print('벌룬 그림자 독립 감사',len(rows));return
    oracle=Oracle();rows=[];different=[]
    # 두 정밀도의 기대값은 실제 두 원문 몸체에서만 얻는다.
    for index,value in enumerate(cases()):
        result=[oracle.sample(value,p) for p in (0x27f,0x37f)]
        if result[0]!=result[1]:different.append(index)
        rows.append((','.join(map(str,value)),*result[0],*result[1]))
    FIXTURE.write_text('# input effects53 raw53 globals53 effects64 raw64 globals64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','tools/decomp_balloon_body.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/balloon-body-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=dict(oracle.counts),instructions=oracle.instructions,precisionDifferentInputs=different,boundaries='42ac90/42ae80와 실제 TYPE getter/CRT 몸체. 생성·가상Pop/Unpop/삭제/check·큰Damageable후처리/전처리·contained finder/485e40은 명시효과 경계다. C++는 기존 실제 Carrier/contained finder를 재사용한다. GP/retN/canary/SEH/x87/코드·쓰기범위/150BYTE·현재전역·효과 전체를 대조한다. 게임·OS·전체수명 검증이 아니다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','precisionDifferentInputs')},ensure_ascii=False))

if __name__=='__main__':main()
