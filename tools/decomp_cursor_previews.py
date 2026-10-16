#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제10.78 커서 미리보기 생성과 위치 판정/갱신 사슬을 격리 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,POOL,TYPES,VTABLE,IMAGE,digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/cursor-previews-x86.tsv'  # 이전 수명/위치 원문과 구별되는 새 자료다.
REPORT=ROOT/'cpppj/cursor-previews.json'  # 실제 PC/명령 범위/외부 경계/제품 SHA다.
CODES,OWNER=0x12700000,STOP+0x300  # 프레임 코드 입력과 가상 Owner 효과 경계다.
# 비교할 원본 전역19개의 순서다. 그 뒤30SID·현재 TYPE/플래그/Form을 붙여52DWORD로 관찰한다.
GLOBALS=(0x55a1b8,0x55a284,0x55a288,0x540c70,0x55a300,0x55a304,0x55a28c,0x55a290,0x55a294,0x55a298,0x55a29c,0x55a2b0,0x55a2b4,0x55a2b8,0x55a2bc,0x55a2c0,0x55a2c4,0x55a2c8,0x55a2cc)


def bits(value):
    """입력 단정도 저장 비트다. 기대 출력의 산술은 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def codes(profile):
    """세 기본 프레임과 cracked/hard 변형의 실제4바이트 입력 코드다."""
    base=[(65,49,0,0),(66,50,1,0),(67,51,2,0)] if not profile else [(193,192,0,128),(127,49,1,255),(65,255,2,16)]
    return bytes(value for mask in (0,32,64) for side,variant,number,flags in base for value in (side,variant,number,flags|mask))


def cells(profile):
    """명시 decoder 경계의 상대 좌표다. 생성/다음 호출은 원문이 한다."""
    return [] if not profile else [(0,0.0,0.0)] if profile==1 else [(0,0.0,0.0),(1,1.25,-0.5),(2,-2.75,3.5)]


def cases():
    """생성/재사용·품질·추가 섬·연결 수·화면 절삭·현재 전역 재조회를 교차한다."""
    # 열: profile,quality,flags1,flags2,existing,connectors,hp,codeProfile,x,y,mutation,repeat,prefix,chain.
    base=[1,1,0,4,0,2,1,0,bits(20.25),bits(21.75),0,1,0,0];rows=[]
    # 원본 첫 SID 유무를 생성 조건으로 사용한다. HP0/1과 빈 decoder도 개별 입력이다.
    for profile,quality,flags1,flags2,existing,connectors,hp in itertools.product(range(3),(0,1,3,255),(0,0x400),(0,4),(0,1),(0,2),(0,1)):
        row=base.copy();row[:7]=profile,quality,flags1,flags2,existing,connectors,hp;rows.append(row)
    # signed 코드 바이트와 최대6연결 및 세부 좌표 절삭 경계를 관찰한다.
    for profile,quality,existing in itertools.product((1,2),(0,1,3,255),(0,1)):
        row=base.copy();row[0]=profile;row[1]=quality;row[4:6]=existing,6;row[7]=1;rows.append(row)
    for (x,y),profile,existing,flags2 in itertools.product(((-1.25,-0.75),(0.03125,0.045454543083906174),(0.99999,1.00001),(2048.5,-2979.5),(float('nan'),float('inf'))),(1,2),(0,1),(0,4)):
        if flags2 and x!=x:continue
        row=base.copy();row[0]=profile;row[3:5]=flags2,existing;row[8:10]=bits(x),bits(y);rows.append(row)
    # 현재 전역 변경과 연속 두 번 재사용을 원문에서 관찰한다.
    for mutation,profile,existing,flags2 in itertools.product(range(1,5),(1,2),(0,1),(0,4)):
        row=base.copy();row[0]=profile;row[3:5]=flags2,existing;row[10]=mutation;rows.append(row)
    for profile,flags1,existing,connectors in itertools.product((1,2),(0,0x400),(0,1),(0,2)):
        row=base.copy();row[0]=profile;row[2]=flags1;row[4:6]=existing,connectors;row[11]=2;rows.append(row)
    for profile,flags1 in itertools.product((0,1,2),(0,0x400)):
        row=base.copy();row[0]=profile;row[2]=flags1;row[12]=3;rows.append(row)
    # 외부 Query/Prepare 대체 없이 실제 세 몸체를 한 사슬로 실행한다.
    for profile,flags1,flags2,existing,connectors in itertools.product((1,2),(0,0x400),(0,4),(0,1),(0,2)):
        row=base.copy();row[0]=profile;row[2:6]=flags1,flags2,existing,connectors;row[13]=1;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


class Oracle(BaseOracle):
    """실제 생성/FindFlags/CRT와 선택한 Query/Prepare 사슬만 허용한다."""
    def __init__(self):
        """현재 정밀 함수표를 읽고 실제 몸체의 쓰기를 격리한다."""
        super().__init__();self.mu.mem_map(CODES,0x1000);self.put(VTABLE+0x74,OWNER);self.allowed=[];self.instructions=0
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 이전 수명 몸체는 실행 허용 범위에 넣지 않는다.
        for entry in (0x445b30,0x445440,0x445340,0x4458e0,0x49aa30,0x4e49c0,0x43fa60,0x4e62a0,0x4ec4c1,0x4f5bac):
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def raw(self,sid,kind=100):
        """Factory 경계/기존 슬롯의 독립 바이트 입력이다. 실제 생성자 관찰을 주장하지 않는다."""
        raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=kind;raw[11]=0;raw[34]=7;struct.pack_into('<I',raw,26,99999);self.mu.mem_write(POOL+sid*50,bytes(raw));self.touched.add(sid)

    def query_mutation(self):
        """위치 조회가 바꾼 현재 전역을 원본 부모가 다시 읽는 입력이다."""
        if self.case[10]==1:
            for address,value in ((0x55a280,TYPES+101*500),(0x55a284,101),(0x55a288,99),(0x540c70,2),(0x55a300,bits(-3.25)),(0x55a304,bits(4.75))):self.put(address,value)
        if self.case[10]==4:
            # Query가 재사용으로 전환할 때 유효한 전체 미리보기 목록도 함께 공급한다.
            for index in range(5):self.put(0x55a1d8+4*index,100+index)
            self.put(0x55a1b8,5)

    def decode_cell(self):
        """명시 decoder의 현재 칸만 공급한다. 모든 raw 쓰기는 생성 부모가 수행한다."""
        if self.ordinal>=len(self.decoded):self.put(self.decoder+20,0);return
        frame,x,y=self.decoded[self.ordinal];self.put(self.decoder+16,frame);self.put(self.decoder+20,1);self.put(self.decoder+32,x);self.put(self.decoder+36,y)

    def instruction(self,mu,address,size,data):
        """가상 효과의 실제 인자·원본 연결 호출 시 임시 좌표·전체 순서를 기록한다."""
        self.instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<7I',mu.mem_read(esp+4,28));ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50
        if address==0x445440 and not self.case[13]:self.event('Q');self.query_mutation();self.ret();return
        if address==0x4458e0 and not self.case[13]:self.event('P');self.ret();return
        if address==0x425c20:
            self.event('W',*args[:6]);self.decoder=ecx;self.ordinal=0;ox,oy=struct.unpack('<2f',struct.pack('<2I',*args[3:5]));self.decoded=[(frame,bits(ox+x),bits(oy+y)) for frame,x,y in cells(self.case[0])];self.decode_cell();self.ret(ecx,24);return
        if address==0x425860:self.event('N');self.ordinal+=1;self.decode_cell();self.ret();return
        if address==0x4af530:
            sid=self.next_main;self.next_main+=1;self.event('C',*args[:2],sid);self.raw(sid,args[0])
            if self.case[10]==2 and self.next_main==101:
                for address,value in ((0x55a284,102),(0x55a288,0xface9999),(0x540c70,2),(0x55a300,bits(-7.75)),(0x55a304,bits(6.5))):self.put(address,value)
            self.ret(POOL+sid*50);return
        if address==0x4ac220:self.event('S',sid,args[0]);self.put(ecx+12,args[0],2);self.ret(0,4);return
        if address==OWNER:
            self.event('O',sid,args[0]);self.put(ecx+34,args[0],1);self.owner_calls+=1
            if self.case[10]==3 and self.owner_calls==1:self.put(TYPES+100*500+0xec,4);self.put(0x55a2a8,51)
            self.ret(0,4);return
        if address==0x4adc60:self.event('H',sid);self.ret(self.case[6]);return
        if address==0x4adca0:self.event('Z',sid,args[0]);self.put(ecx+26,args[0]);self.ret(0,4);return
        if address==0x49a840:self.event('F',args[0]);self.ret(TYPES+args[0]*500);return
        if address==0x4213b0:
            raw=POOL+args[0]*50;x,y=struct.unpack('<2f',mu.mem_read(raw+14,8));self.event('B',args[0],self.get(raw+14),self.get(raw+18),*struct.unpack('<2H',mu.mem_read(raw+22,4)),self.get(raw+36),self.get(0x55a1b8))
            # 실제 연결 객체 생성은 명시 경계다. 단정도 위치와 결과 SID만 공급한다.
            for index in range(self.case[5]):
                target=self.next_connector;self.next_connector+=1;self.raw(target,90);mu.mem_write(POOL+target*50+14,struct.pack('<2f',x+(index%2*2-1),y+(index%3-1)));self.put(args[1]+index*4,target)
            self.put(args[2],self.case[5]);self.ret();return
        if address==0x4734d0:self.event('D',*args[:4]);mu.mem_write(ecx,b'Cursor\0');self.ret(1,16);return
        if address==0x419850:self.event('I',(ecx-TYPES)//500,*args[:2]);mu.mem_write(IMAGE+24,struct.pack('<4h',31,23,8,17));self.ret(IMAGE,8);return
        if address==0x465ed0:self.event('G',*args[:2]);self.ret();return
        if address==0x49b510:self.event('M',(ecx-TYPES)//500,*args[:6]);self.ret(1,24);return
        if address==0x497220:self.event('J',*args[1:3]);mu.mem_write(ecx,struct.pack('<2i',501,603));mu.mem_write(args[0],b'\0'*8);self.ret(0,12);return
        if address==0x497580:self.event('V',*struct.unpack('<4I',mu.mem_read(args[0],16)),args[1]);self.ret(0,8);return
        if address==0x4e0620:raise RuntimeError('원본 진단: '+self.string(args[0]))
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미리보기 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """원본 본체가 커서/raw/스택 외부에 쓰면 중단한다."""
        if not any(low<=address and address+size<=high for low,high in ((0x55a198,0x55a440),(POOL,POOL+0x10000),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'미리보기 범위 밖 쓰기 {address:08x}')

    def sample(self,case,precision):
        """매 입력에서 전체 목록/현재 전역/접근 슬롯50바이트와 효과를 독립 관찰한다."""
        self.case=case;self.events=[];self.touched=set();self.next_main=100;self.next_connector=200;self.owner_calls=0;mu=self.mu;mu.mem_write(0x55a198,b'\0'*(0x55a440-0x55a198))
        # 재사용 입력도 Factory 경계와 같은 바이트 표식을 갖는다.
        for sid in (50,51,100,101,102,103,104):self.raw(sid)
        self.put(POOL+50*50+30,case[1],1);self.put(POOL+51*50+30,3,1)
        for index in range(5):self.put(0x55a1d8+4*index,100+index if case[4] else 0)
        for address,value in ((0x5c8464,POOL),(0x55a1b8,5 if case[4] else case[12]),(0x55a280,TYPES+100*500),(0x55a284,100),(0x55a288,7),(0x55a2a4,3),(0x55a2a8,50),(0x540c70,1),(0x55a300,case[8]),(0x55a304,case[9]),(0x5411a0,99),(0x5411d0,91),(0x5411d4,92),(0x55a2e0,0x40 if case[13] else 0),(0x55a2dc,0),(0x55a268,35),(0x55a198,17),(0x55a270,49),(0x55a250,23),(0x59a91c,-17),(0x59a920,31),(0x55a2f8,-3),(0x55a2fc,47),(0x5325e0,16),(0x59ab2c,0)):
            self.put(address,value)
        for number in (100,101,102):self.put(TYPES+number*500+0xe8,case[2]);self.put(TYPES+number*500+0xec,case[3]);self.put(TYPES+number*500+0x1d4,2);self.put(TYPES+number*500+0x1d8,3)
        mu.mem_write(CODES,codes(case[7]));self.put(TYPES+99*500+0x124,CODES);self.put(TYPES+99*500+0x114,9)
        # 두 호출은 같은 결과 목록을 이어 사용한다. 재초기화하지 않는다.
        for _ in range(case[11]):self.call(0x445b30,0,b'',0,precision);self.returns+=1
        values=[self.get(address) for address in GLOBALS]+[self.get(0x55a1d8+4*index) for index in range(30)]+[(self.get(0x55a280)-TYPES)//500,self.get(TYPES+100*500+0xec),self.get(0x55a2a8)]
        raws=[]
        # 가상 표만 호스트 토큰1로 정규화하고 나머지46바이트는 그대로 보존한다.
        for sid in sorted(self.touched):
            raw=bytearray(mu.mem_read(POOL+sid*50,50));struct.pack_into('<I',raw,0,1);raws.append(str(sid)+':'+raw.hex())
        return (';'.join(self.events) or '-',','.join(map(str,values)),';'.join(raws))


def main():
    """두 원문 실행끼리 먼저 대조하거나 저장된 근거의 SHA만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 기존 원문을 다시 생성하지 않고 입력 고유성과 모든 핀을 확인한다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs']==len({line.split('\t')[0] for line in rows});print('미리보기 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[]
    # Python으로 커서 판정을 계산하지 않고 실제 두 정밀도 결과만 비교한다.
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    FIXTURE.write_text('# input14 events globals52 rawSlots\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_previews.py',FIXTURE.relative_to(ROOT).as_posix(),'cpppj/src/client/RawCursorPreviews.h','cpppj/src/client/RawCursorPreviews.cpp','cpppj/src/o/RawBridgeConnect.h','cpppj/src/o/RawBridgeConnect.cpp','cpppj/tests/BridgeConnectTests.cpp','cpppj/tests/CursorPreviewTests.cpp','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt')
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entry='00445b30',uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),chainInputs=sum(int(case[13]) for case in cases()),instructionObservations=sum(oracle.instructions for oracle in oracles),instructionObservationsIncludeBoundaryEntries=True,precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['실제445b30/FindFlags/CRT를 실행하며 chain입력은445440/445340/4458e0도 실행한다.','decoder/Factory/WORD/Owner/HP/연결 객체·설명/공간/GUI/투영/표시 효과는 명시 경계다.','실제 Client 활성·일반 건설 확정/사용자 완주/전체 그래픽 동일성은 별도다. 원본 또는 클론 게임/OS 호출은 없다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('미리보기:',len(rows),'고유 입력/',facts['normalReturns'],'정상 반환/사슬',facts['chainInputs'],'/두 정밀도 차이0/명령·경계 관찰',facts['instructionObservations'])


if __name__=='__main__':main()
