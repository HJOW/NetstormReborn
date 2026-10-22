#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 원주 계산과 세 Wind 주문의 자격 검사 중 생산기 생성 부수 효과를 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_arc_spire import Oracle as BaseOracle,ROOT,STOP,HEAP,TYPES,STACK
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

POOL=0x15000000  # 격리한50BYTE 주문 풀이다.
OUTPUT=HEAP+0x100  # 두 float 원주 출력이다.
STORAGE=HEAP+0x1000  # 최대6개의 서로 다른 미정80BYTE 생성 입력이다.
MATH=ROOT/'cpppj/tests/fixtures/wind-circle-point-x86.tsv'  # 원본CRT 기본53비트와 추가64비트 관측을 함께 보존한다.
SPAWN=ROOT/'cpppj/tests/fixtures/wind-spell-x86.tsv'  # 세 신규root의 전체생성필드/순서다.
REPORT=ROOT/'cpppj/wind-spell-20261023.json'  # 현재 원본/실행·호스트 근거다.
ENTRIES=(0x41e020,0x41ecf0,0x41fb60,0x41fcc0,0x45d730)  # 계산/조건/생성자는 실제 명령이다.


def bits(value):
    """명시 float 입력의 비트를 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def math_cases():
    """정수 나눗셈과 원점 근처 상쇄·음수/큰 반경의 독립 입력이다."""
    rows=[]
    for count in (1,2,3,4,5,6,7,8,17,31,359,360,361,-1,-7):
        for index,point in itertools.product((-1,0,1,count-1),((0.,0.),(float(-count),float(count)),(50.25,50.5),(255.99,.000001))):rows.append((count,index,bits(point[0]),bits(point[1])))  # 결과는 원문 명령에서만 구한다.
    return list(dict.fromkeys(rows))


def spawn_cases():
    """서버/기존59/한도 및 조회·확보·부착 뒤 현재 자료 변화를 교차한다."""
    return list(itertools.product(range(3),(0,1),(0,1),(-1,0,1,2,6),range(5)))


class Oracle(BaseOracle):
    """전체 root를 실행하며 힙/현재TYPE/실제부착만 명시 큰 경계로 둔다."""
    def __init__(self):
        """정확한 함수 범위와 격리 메모리 가드를 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x200000);self.allowed=[];ranges(self,ENTRIES);self.returns=0

    def number(self,address,width=4):
        """현재 비정렬 원문 필드를 정확한 폭으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def read(self,mu,access,address,size,value,data):
        """원본/SEH/명시 자료/스택 밖 접근을 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,0x1000),(0x400000,0x600000),(POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(TYPES,TYPES+0x20000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'Wind 주문 읽기 범위 {address:x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """SEH/생성필드/스택 외 원문 쓰기는 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,0x1000),(HEAP,HEAP+0x10000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'Wind 주문 쓰기 범위 {address:x}+{size}')

    def instruction(self,mu,address,size,data):
        """고정TYPE 포인터/현재 QA·종류·count 재조회와 생성자 쓰기를 실제 명령으로 실행한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=[self.number(sp+4+i*4) for i in range(4)]
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4b1a90:
            self.event('F',*args)
            if self.mutation==1:self.put(POOL+50*50+12,61,2)
            self.ret(77 if self.found else 0);return
        if address==0x49a840:self.ret(TYPES+args[0]*500);return
        if address==0x4e4391:
            assert args[0]==80;self.event('N',args[0]);at=STORAGE+self.created*0x100;self.created+=1;self.mu.mem_write(at,b'\xa5'*80)
            if self.mutation==2:self.put(0x540bc0,0);self.put(0x54115c,60);self.mu.mem_write(0x55b4d0,struct.pack('<d',101.25))
            self.ret(at);return
        if address==0x41bd20:
            self.event('A',*args);self.images.append(bytes(mu.mem_read(ecx-56,80)).hex())
            if self.mutation==3:self.put(TYPES+self.kind*500+0x1c8,1)
            if self.mutation==4:self.put(POOL+50*50+12,62,2);self.put(0x54115c,60)
            self.ret(purge=16);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'Wind 주문 미검토 명령 {address:x}')

    def point(self,row,control):
        """41e020의 signed 정수/float 입력만 공급한다."""
        count,index,x,y=row;self.mu.mem_write(OUTPUT,b'\xa5'*8);self.call(0x41e020,0,struct.pack('<5I',OUTPUT,x,y,count&0xffffffff,index&0xffffffff),0,control);self.returns+=1
        return self.number(OUTPUT),self.number(OUTPUT+4)

    def spawn(self,row,control):
        """새 힙 토큰을 정규화하되 생성80BYTE와 부착의 현재 인자는 그대로 기록한다."""
        variant,server,self.found,count,self.mutation=row;self.kind=171+variant;self.created=0;self.images=[];self.events=[]
        self.mu.mem_write(POOL+50*50,bytes(50));self.put(POOL+50*50+12,60,2);self.put(0x540bc0,server);self.put(0x54115c,59);self.mu.mem_write(0x55b4d0,struct.pack('<d',100.0))
        for i in range(3):self.put(0x541304+i*4,171+i);self.put(TYPES+(171+i)*500+0x1c8,count)  # 고정레코드의 현재 한도다.
        self.call((0x41ecf0,0x41fb60,0x41fcc0)[variant],POOL+50*50,struct.pack('<I',999),4,control);self.returns+=1
        return str(self.mu.reg_read(UC_X86_REG_EAX)),';'.join(self.events) or '-',','.join(self.images) or '-'


def main():
    """독립 관측과 현재 SHA의 읽기 전용 검증을 구별한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for p,h in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/p)==h,p  # C++ 출력으로 fixture를 다시 만들지 않는다.
        print('Wind 주문/원주 원문 SHA 확인');return
    first,second=Oracle(),Oracle();rows=[];different=[]
    for row in math_cases():
        a=first.point(row,0x27f);b=second.point(row,0x37f)
        if a!=b:different.append(','.join(map(str,row)))
        rows.append((','.join(map(str,row)),','.join(map(str,a)),','.join(map(str,b))))  # 원본CRT의 기본53비트가 C++ 비교 기준이며64비트 차이도 숨기지 않는다.
    MATH.write_text('# input point53 point64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n');spawns=[]
    for row in spawn_cases():
        a=first.spawn(row,0x27f);b=second.spawn(row,0x37f);assert a==b,row;spawns.append((','.join(map(str,row)),*a))  # 생성자/부착 필드는 두 모드에서 같아야 한다.
    SPAWN.write_text('# input result effects fields80\n'+'\n'.join('\t'.join(r) for r in spawns)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_wind_spell.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',MATH.relative_to(ROOT).as_posix(),SPAWN.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',mathInputs=len(rows),spawnInputs=len(spawns),normalReturns=first.returns+second.returns,nativeInstructions=first.instructions+second.instructions,nativeCalls=dict(first.calls+second.calls),precisionDifferentMathInputs=different,originalDefaultPrecision='004e81da __setdefaultprecision -> __controlfp(0x10000,0x30000), PC53',sha256={p:digest(ROOT/p) for p in paths},limitations=['41e020 계산/세 소환/45d730 생성자는 원문 명령이다. 힙/TYPE 조회/첫59 조회·부착은 명시 큰 경계다.','원본CRT 기본53비트를 기준으로 하며64비트 차이는 별도로 기록한다. 전체 게임 GUI/25종 전체 수명 완료 근거는 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Wind 원주',len(rows),'소환',len(spawns),'정상반환',facts['normalReturns'],'정밀도 차이',len(different))


if __name__=='__main__':main()
