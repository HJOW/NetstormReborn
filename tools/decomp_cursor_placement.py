#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 커서 위치/연관 지역/갱신 사각형의 실제 세 몸체를 격리 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,POOL,TYPES,VTABLE,IMAGE,PROCESS,SOURCE,EVENT,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/cursor-placement-x86.tsv'  # 새 원문이며 기존 수명/내려놓기 관찰을 바꾸지 않는다.
REPORT=ROOT/'cpppj/cursor-placement.json'  # 실제 PC/명령 범위/경계와 제품 SHA를 기록한다.
MAP,GUMP=0x12500000,0x12600000  # OS/게임 주소와 분리한 표면 지도와 Gump다.
GLOBALS=(0x55a28c,0x55a290,0x55a294,0x55a298,0x55a29c,0x55a278,0x55a27c,0x55a2b0,0x55a2b4,0x55a2b8,0x55a2bc,0x55a2c0,0x55a2c4,0x55a2c8,0x55a2cc)


def bits(value):
    """입력 단정도 비트를 기대값 계산 없이 저장한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cells(profile):
    """외부 decoder 경계에만 공급하는 명시 프레임/상대 좌표다."""
    if profile==2:return []
    if profile==4:return [(0,bits(float('nan')),bits(1.1))]
    if profile==5:return [(0,bits(-1.9998),bits(-0.9999)),(1,bits(254.0002),bits(255.0002))]
    return [(0,0,0)] if profile==0 else [(0,0,0),(1,bits(1.25),bits(-0.5)),(2,bits(-2.75),bits(3.5))]


def image(profile,frame):
    """SHP 경계의 signed WORD 네 개다. 음수 치수와 기준점도 별도 입력이다."""
    return ((-11,-17,23,-7),(19,29,-3,31),(47,13,7,-5))[frame] if profile==3 else ((31,23,8,17),(19,29,-3,31),(47,13,7,-5))[frame]


def cases():
    """월드/Gump 전이·문구/다리 여백·같은 지역·표시 조건을 독립 입력으로 교차한다."""
    # 열: op,profile,flags1,flags2,footX,footY,spacing,describe,text,oldGump,newGump,accept,place,obstruction,sourceRegion,inputFlags,costFlags,mana,x,y,match,mutation,rectVariant.
    base=[0,1,0,0,2,3,16,7,0,0,0,0,9,0,4,0,0x160,1,bits(20.5),bits(21.5),1,0,0];rows=[]
    # 실제 세 표시 여백과 빈 모양의 기존 치수 보존을 검사한다.
    for profile,flags1,flags2,text,spacing in itertools.product(range(6),(0,0x400),(0,4),(0,1,2),(0,16,-3)):
        row=base.copy();row[1:4]=profile,flags1,flags2;row[6]=spacing;row[8]=text;rows.append(row)
    # GUI 전이는 description이 거짓이어도 수신 부모가 있으면 허용한다. 공간 상태는 GUI에서 보존된다.
    for old,new,accept,describe,place,blocked in itertools.product((0,1),(0,1),(0,1),(0,7),(0,9),(0,1)):
        row=base.copy();row[7]=describe;row[9:14]=old,new,accept,place,blocked;rows.append(row)
    # 연관 지역은 적어도 한 유효 표면 칸이 같은 지역일 때만 성공한다. flags20이 없으면 조회하지 않는다.
    for profile,flags,match,x,y in itertools.product((0,1,2,5),(0,0x20,0x60),(0,1),(-1.25,0.00001,20.0002,255.9),(0.0,20.5)):
        row=base.copy();row[1]=profile;row[15]=flags;row[18:21]=bits(x),bits(y),match;rows.append(row)
    # 준비의 네 독립 경고 조건 및 signed 나눗셈/감김을 관찰한다.
    for flags,legal,blocked,cost,variant in itertools.product((0,0x20,0x40,0x60),(0,1),(0,1),(0,0x160),(0,1,2)):
        row=base.copy();row[0]=1;row[7]=legal;row[12]=legal;row[13]=blocked;row[15:17]=flags,cost;row[20]=legal;row[22]=variant;rows.append(row)
    # 명시 외부 효과 이후 현재 값 재조회와 standalone source를 확인한다.
    for operation,mutation in itertools.product((0,2),(0,1,2)):
        row=base.copy();row[0]=operation;row[15]=0x20;row[21]=mutation;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """실제445440/445340/4458e0와 _ftol만 실행하며 decoder/GUI/공간/표시를 기록한다."""
    def __init__(self):
        """현재 정밀 함수표 범위를 더하고 지도/Gump 입력을 격리한다."""
        super().__init__();self.mu.mem_map(MAP,0x20000);self.mu.mem_map(GUMP,0x1000);self.instructions=0;self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # floor(float+0.5)의 실제 본체/CRT도 실행한다. OS 경계는 허용하지 않는다.
        for entry in (0x445440,0x445340,0x4458e0,0x4e49c0,0x43fa60,0x4e62a0,0x4ec4c1,0x4f5bac):
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def token(self,value):
        """격리 Gump 포인터만 호스트 식별값으로 정규화한다."""
        return (value-GUMP)//64+1 if GUMP<=value<GUMP+128 else value

    def decode_cell(self):
        """외부 decoder의 현재 칸만 입력한다. 커서의 후속 산술은 원본이 수행한다."""
        if self.ordinal>=len(self.decoded):self.put(self.decoder+20,0);return
        frame,x,y=self.decoded[self.ordinal];self.put(self.decoder+16,frame);self.put(self.decoder+20,1)
        self.put(self.decoder+32,x);self.put(self.decoder+36,y)

    def instruction(self,mu,address,size,data):
        """경계의 실제 호출 규약과 현재 전역 변경을 보존하며 전체 효과 순서를 기록한다."""
        self.instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<7I',mu.mem_read(esp+4,28));ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4734d0:
            self.event('D',*args[:4]);mu.mem_write(ecx,(b'AB' if self.case[8]==0 else b'' if self.case[8]==1 else b'ABCDE')+b'\0')
            if self.case[21]==1:self.put(0x55a280,TYPES+101*500)
            self.ret(self.case[7],16);return
        if address==0x425c20:
            self.event('W',*args[:6]);self.decoder=ecx;self.ordinal=0;origin=struct.unpack('<2f',struct.pack('<2I',*args[3:5]));self.decoded=[]
            # 실제 decoder 대신 명시 상대 좌표만 공급하며 float 저장 경계를 보존한다.
            for frame,x,y in cells(self.case[1]):
                px,py=struct.unpack('<2f',struct.pack('<2I',x,y));self.decoded.append((frame,bits(origin[0]+px),bits(origin[1]+py)))
            self.decode_cell();self.ret(ecx,24);return
        if address==0x425860:self.event('N');self.ordinal+=1;self.decode_cell();self.ret();return
        if address==0x419850:
            self.event('I',(ecx-TYPES)//500,*args[:2]);mu.mem_write(IMAGE+24,struct.pack('<4h',*image(self.case[1],args[1])));self.ret(IMAGE,8);return
        if address==0x465ed0:
            self.event('G',*args[:2],self.get(0x55a278),self.get(0x55a27c));self.ret(GUMP if self.case[10] else 0);return
        if address==0x4651b0:self.event('A',self.token(ecx),*args[:4]);self.ret(GUMP+64 if self.case[11] else 0,16);return
        if address==0x49b510:
            self.event('P',(ecx-TYPES)//500,*args[:6])
            if self.case[21]==2:self.put(0x59ab2c,1)
            self.ret(self.case[12],24);return
        if address==SOURCE:self.event('U',(ecx-POOL)//50);self.ret(77);return
        if address==0x462c80:self.event('R',args[0]);self.ret(self.case[14] if args[0]==77 or self.case[20] else self.case[14]+1);return
        if address==EVENT:self.event('E',1,*args[:3]);self.ret(0,12);return
        if address==0x497220:
            self.event('J',*args[1:3]);mu.mem_write(ecx,struct.pack('<2I',*self.projected));mu.mem_write(args[0],b'\0'*8);self.ret(0,12);return
        if address==0x497580:self.event('V',*struct.unpack('<4I',mu.mem_read(args[0],16)),args[1]);self.ret(0,8);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'커서 위치 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """커서/격리 스택 밖의 실제 본문 쓰기를 거부한다. 경계의 입력 쓰기는 별도다."""
        if not any(low<=address and address+size<=high for low,high in ((0x55a198,0x55a440),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'커서 위치 범위 밖 쓰기 {address:08x}')

    def sample(self,case,precision):
        """새 입력으로 실제 정상 반환·허용/치수 전역과 효과를 관찰한다."""
        self.case=case;self.events=[];self.mu.mem_write(0x55a198,b'\0'*(0x55a440-0x55a198));self.mu.mem_write(MAP,b'\0'*0x20000)
        # 표면 칸 네 곳은 서로 다른 경계/유효 위치에 놓인 독립 입력이다.
        for x,y,sid in ((0,0,80),(21,22,81),(20,21,82),(255,255,83)):self.put(MAP+2*(y*256+x),sid,2)
        for address,value in zip(GLOBALS,(5,case[12],case[7],1-case[13],case[20],GUMP if case[9] else 0,GUMP+64,3,7,43,51,44,45,91,93)):self.put(address,value)
        for address,value in ((0x55a280,TYPES+100*500),(0x55a284,100),(0x55a288,7),(0x55a2a4,3),(0x55a2a8,50),(0x55a2e0,case[15]),(0x55a2dc,case[16]),(0x55a274,PROCESS if case[17] else 0),(0x55a300,case[18]),(0x55a304,case[19]),(0x540c70,0x123401),(0x5325e0,case[6]),(0x59ab2c,case[13]),(0x5427f4,0x123456),(0x542514,MAP),(0x5c8464,POOL),(0x55a2f8,-3),(0x55a2fc,47),(0x55a1d8,100)):
            self.put(address,value)
        # mutation의 TYPE101은 footprint/genus가 다르므로 현재 값 재조회가 관찰된다.
        for number,fx,fy,flags2 in ((100,case[4],case[5],case[3]),(101,7,9,4)):
            self.put(TYPES+number*500+0x1d4,fx);self.put(TYPES+number*500+0x1d8,fy);self.put(TYPES+number*500+0xe8,case[2]);self.put(TYPES+number*500+0xec,flags2)
        variant=case[22];sizes=((35,17,49,23),(-3,-7,-11,-13),(0x7fffffff,0x80000001,0x7ffffffd,0x80000003))[variant]
        for address,value in zip((0x55a268,0x55a198,0x55a270,0x55a250),sizes):self.put(address,value)
        self.projected=((101,203),(0xfffffffd,0xfffffff9),(0x7ffffffe,0x80000002))[variant]
        for address,value in ((0x59a91c,-17),(0x59a920,31)):self.put(address,value)
        self.put(POOL+50*50,VTABLE);self.put(POOL+100*50+22,0xfff9,2);self.put(POOL+100*50+24,0x8001,2);self.put(GUMP+64+0x24,77)
        self.mu.mem_write(0x55a308,b'old\0');entry=(0x445440,0x4458e0,0x445340)[case[0]];self.call(entry,0,b'',0,precision);self.returns+=1
        values=[self.get(address) for address in GLOBALS];values[5]=self.token(values[5]);values[6]=self.token(values[6]);values.append((self.get(0x55a280)-TYPES)//500)
        values.append(self.mu.reg_read(UC_X86_REG_EAX) if case[0]==2 else 0)
        return (';'.join(self.events) or '-',','.join(map(str,values)),self.string(0x55a308))


def main():
    """두 실제 정밀도 실행을 대조하거나 저장된 원문과 제품 SHA를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 저장된 fixture/제품은 다시 생성하지 않고 실제 SHA만 감사한다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[row for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('커서 위치 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[]
    # 두 원문 실행끼리 먼저 비교하며 C++나 Python 판정으로 기대값을 만들지 않는다.
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    FIXTURE.write_text('# input23 events globals17 description\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_placement.py',FIXTURE.relative_to(ROOT).as_posix(),'cpppj/src/client/RawCursorPlacement.h','cpppj/src/client/RawCursorPlacement.cpp','cpppj/tests/CursorPlacementTests.cpp','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt')
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=['00445440','00445340','004458e0'],uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['세 커서 몸체와 실제 CRT 절삭/floor만 실행한다. decoder/SHP/설명/GUI/지역/투영/화면 갱신은 명시 경계다.','445627..445736 표시 여백은 정밀 C 출력에서 빠져 실제 기계어로 교정했다. source flags20은 SP 부족 판정이 아니다.','실제 Client 세션/미리보기 생성445b30/일반 건설 배치/사용자 플레이와 전체 화면 동일성은 별도다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('커서 위치:',len(rows),'고유 입력/두 정밀도 차이0/명령',facts['instructions'])


if __name__=='__main__':main()
