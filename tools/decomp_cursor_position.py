#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 정수 마우스 위치 갱신/역투영/절삭과 실제 연속 이동 사슬을 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,POOL,TYPES,DESTROY,digest
from decomp_cursor_previews import Oracle as PreviewOracle,GLOBALS as PREVIEW_GLOBALS
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/cursor-position-x86.tsv'  # 기존 커서 원문과 구별되는 새 고유 입력 자료다.
REPORT=ROOT/'cpppj/cursor-position.json'  # 실제 함수/정밀도/외부 경계와 현재 제품 SHA다.
OUT,GUMP=0x12800000,0x12900000  # 별도 출력 점과 GUI 수신 객체 입력이다.
# 화면/월드/잠금/Gump/snap/camera/그림 치수13DWORD의 원래 주소다.
GLOBALS=(0x55a2f8,0x55a2fc,0x55a300,0x55a304,0x5325c8,0x55a27c,0x55a2d8,0x59a900,0x59a904,0x55a2c0,0x55a2c4,0x55a2c8,0x55a2cc)


def bits(value):
    """단정도 입력만 저장한다. 출력 기대값을 Python 수식으로 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """정수 감김/음수 치수/정밀도/불변/NaN/잠금 재진입을 독립 입력으로 만든다."""
    # 열: operation,mouseX,mouseY,hotX,hotY,width,height,cameraX,cameraY,snap,oldX,oldY,lock,legalGump,activateMutation,createMutation,inputX,inputY.
    base=[0,320,220,8,17,31,23,bits(3.25),bits(-5.75),0,0,0,0,13,0,0,0,0];rows=[]
    shapes=((8,17,31,23),(-3,-5,-11,-13),(0x7fffffff,0x80000001,0x7fffffff,0x80000001))
    # 정수 인자는 float 캐스트를 거치지 않는다. 홀수 음수 치수의 나눗셈도 입력한다.
    for mouse,shape,snap,lock,camera in itertools.product(((0,0),(320,220),(-1,-3),(0x7fffffff,0x80000000),(0x7ffffff0,0x7ffffff5)),shapes,(0,1,0x100),(0,1,0xffffffff),((0.0,0.0),(3.25,-5.75))):
        row=base.copy();row[1:3]=mouse;row[3:7]=shape;row[7:10]=bits(camera[0]),bits(camera[1]),snap;row[12]=lock;rows.append(row)
    # Activate 이후 잠금/현재 전역을 다시 읽고 Create 뒤에는 좌표를 덮어쓰지 않는다.
    for activate,create,lock,snap in itertools.product(range(5),range(4),(0,1),(0,1)):
        row=base.copy();row[9]=snap;row[12]=lock;row[14:16]=activate,create;rows.append(row)
    # 알려진 일치점·부호0·스냅 후 일치·NaN은 비교 자체를 원문에서 관찰한다.
    for mx,my,snap,oldx,oldy in ((0,0,0,0,0),(0,0,0,0x80000000,0),(320,220,0,bits(20),bits(20)),(321,221,1,bits(20),bits(20)),(0,0,0,0x7fc01234,0)):
        row=base.copy();row[1:7]=mx,my,0,0,0,0;row[7:10]=0,0,snap;row[10:12]=oldx,oldy;rows.append(row)
    # 역투영은 int→float 선행 축소와 잘못된 double1/11 사용을 구별한다.
    for mouse,camera in itertools.product(((0,0),(-17,-12),(16777217,16777219),(0x7fffffff,0x80000000)),((0,0),(bits(3.25),bits(-5.75)),(0x4f000000,0xcf000000),(0x7fc01234,0x7f800000))):
        row=base.copy();row[0]=1;row[1:3]=mouse;row[7:9]=camera;rows.append(row)
    # 실제 ftol의 signed64→하위signed32→float를 두 축 모두 확인한다.
    values=(0,0x80000000,bits(1.75),bits(-1.75),0x4effffff,0x4f000000,0x4f800000,0xcf800000,0x5effffff,0x5f000000,0xdf000000,0x7f7fffff,0x7f800000,0xff800000,0x7fc01234,0x7fa01234)
    for x,y in itertools.product(values,values):
        row=base.copy();row[0]=2;row[16:18]=x,y;rows.append(row)
    return list(dict.fromkeys(tuple(row) for row in rows))


def chain_cases():
    """실제 정리/생성/위치 판정/표시 준비를 두 번 이어 움직이는64개 사슬이다."""
    return list(itertools.product((1,2),(0,0x400),(0,4),(0,1),(0,1),(0,1)))


def ranges(oracle,entries):
    """현재 PC 정밀 함수표의 실제 불연속 명령 범위만 허용한다."""
    with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
    # 각 함수의 마지막 명령 바이트까지 원래 범위를 그대로 포함한다.
    for entry in entries:
        for part in rows[entry]['ranges'].split(';'):
            low,high=(int(value,16) for value in part.split('-'));oracle.allowed.append((low,high+1))


class Oracle(BaseOracle):
    """실제 세 몸체와 CRT를 실행하며 Activate/Create/커서 선택만 명시 효과로 둔다."""
    def __init__(self):
        """새 출력 점과 실제 허용 범위를 준비한다. 게임/OS 진입은 없다."""
        super().__init__();self.mu.mem_map(OUT,0x1000);self.allowed=[];self.instructions=0;ranges(self,(0x446b50,0x4971e0,0x41d770,0x4e49c0))

    def instruction(self,mu,address,size,data):
        """외부 효과가 현재 전역을 바꾸는 입력도 호출 시점에 적용한다."""
        self.instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<3I',mu.mem_read(esp+4,12))
        if address==0x445aa0:
            self.event('A',args[0],self.get(0x55a300),self.get(0x55a304),self.get(0x55a2f8),self.get(0x55a2fc));mode=self.case[14]
            if mode==1:self.put(0x5325c8,3)
            if mode==2:self.put(0x5325c8,0)
            if mode==3:self.put(0x55a300,bits(99));self.put(0x55a304,bits(-77))
            if mode==4:
                # 계산된 목표점은 전역이 바뀌어도 다시 투영하지 않는다.
                for target,value in ((0x55a2d8,0),(0x59a900,bits(1.25)),(0x59a904,bits(-2.5)),(0x55a2c0,-5),(0x55a2c4,9),(0x55a2c8,13),(0x55a2cc,17)):self.put(target,value)
            self.ret();return
        if address==0x445b30:
            self.event('B',self.get(0x55a300),self.get(0x55a304),self.get(0x55a2f8),self.get(0x55a2fc),self.get(0x5325c8));mode=self.case[15];self.put(0x55a27c,77 if mode in (1,2) else 0)
            if mode==2:self.put(0x5325c8,5)
            if mode==3:self.put(0x55a300,bits(-11.25));self.put(0x55a304,bits(7.5))
            self.ret();return
        if address==0x4a4410:self.event('C',args[0]);self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'마우스 위치 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 본문의 쓰기는 커서/출력/스택만 허용한다."""
        if not any(low<=address and address+size<=high for low,high in ((0x55a2f8,0x55a308),(OUT,OUT+8),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'마우스 위치 범위 밖 쓰기 {address:08x}')

    def sample(self,case,precision):
        """독립 입력18개로 실제 반환·15DWORD·외부 효과를 관찰한다."""
        self.case=case;self.events=[]
        values=(-3,47,case[10],case[11],case[12],case[13],case[9],case[7],case[8],*case[3:7])
        for address,value in zip(GLOBALS,values):self.put(address,value)
        self.put(OUT,case[16]);self.put(OUT+4,case[17]);entry=(0x446b50,0x4971e0,0x41d770)[case[0]]
        arguments=struct.pack('<2I',case[1]&0xffffffff,case[2]&0xffffffff) if not case[0] else struct.pack('<3I',OUT,case[1]&0xffffffff,case[2]&0xffffffff) if case[0]==1 else b''
        self.call(entry,OUT if case[0]==2 else 0,arguments,0,precision);self.returns+=1
        values=[self.get(address) for address in GLOBALS]+[self.get(OUT),self.get(OUT+4)]
        return (';'.join(self.events) or '-',','.join(map(str,values)))


class ChainOracle(PreviewOracle):
    """실제 위치/Activate/미리보기/Query/Prepare를 두 번 연속 실행한다."""
    def __init__(self):
        """기존 부모 경계와 원문 입력을 재사용하고 GUI 수신 객체만 더한다."""
        super().__init__();self.mu.mem_map(GUMP,0x1000);self.put(GUMP+64+0x24,77);self.position_returns=0;ranges(self,(0x446b50,0x4971e0,0x41d770,0x445aa0))

    def call(self,entry,this,arguments,purge,precision,*args,**kwargs):
        """부모의 한 최상위 생성 호출을 실제 두 번 마우스 이동으로 바꾼다."""
        if entry!=0x445b30:return super().call(entry,this,arguments,purge,precision,*args,**kwargs)
        self.put(0x55a2d8,self.position_case[3]);self.put(0x5325c8,self.position_case[4]);self.put(0x59a900,bits(3.25));self.put(0x59a904,bits(-5.75));self.put(0x55a2e0,0x240 if self.position_case[2] else 0x40);self.put(0x55a2dc,0)
        self.move_values=[];self.move_raws=[]
        # 첫 Query가 바꾼 그림 치수와 목록을 두 번째 호출이 그대로 사용한다.
        for x,y in ((320,220),(337,243)):
            super().call(0x446b50,0,struct.pack('<2i',x,y),0,precision);assert self.get(0)==0;self.position_returns+=1;values,raws=self.snapshot_position();self.move_values.append(values);self.move_raws.append(raws)

    def snapshot_position(self):
        """두 이동 각각의63DWORD와 그 시점까지 접근한 모든 슬롯을 실제 메모리에서 읽는다."""
        values=[self.get(address) for address in PREVIEW_GLOBALS]+[self.get(0x55a1d8+4*index) for index in range(30)]+[(self.get(0x55a280)-TYPES)//500,self.get(TYPES+100*500+0xec),self.get(0x55a2a8)]
        values.extend((self.get(0x55a2f8),self.get(0x55a2fc),2 if self.get(0x55a27c) else 0,1 if self.get(0x55a278) else 0,self.get(0x5325c8),self.get(0x55a2d8),self.get(0x59a900),self.get(0x59a904),self.get(0x55a2ac),self.get(0x55a2e0),self.get(0x55a2dc)));raws=[]
        # 호스트에서 역참조하지 않는 VT 토큰만1로 정규화한다.
        for sid in sorted(self.touched):
            raw=bytearray(self.mu.mem_read(POOL+sid*50,50));struct.pack_into('<I',raw,0,1);raws.append(str(sid)+':'+raw.hex())
        return ','.join(map(str,values)),';'.join(raws)

    def instruction(self,mu,address,size,data):
        """가상 삭제/커서 선택/GUI 경계만 기록하며 다섯 부모의 계산은 대체하지 않는다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<4I',mu.mem_read(esp+4,16));ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==DESTROY:self.instructions+=1;self.event('D',(ecx-POOL)//50,args[0]);self.ret(0,4);return
        if address==0x4a4410:self.instructions+=1;self.event('K',args[0]);self.ret();return
        if address==0x465ed0 and self.position_case[5]:self.instructions+=1;self.event('G',*args[:2]);self.ret(GUMP);return
        if address==0x4651b0:self.instructions+=1;self.event('A',1,*args[:4]);self.ret(GUMP+64,16);return
        super().instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """실제 Activate의 격리 FS:0 SEH DWORD와 기존 커서/raw/스택 쓰기만 허용한다."""
        if address==0 and size==4:return
        super().write(mu,access,address,size,value,data)

    def sample_position(self,case,precision):
        """기존 명시 Factory/슬롯 입력과 실제 두 번 이동의60DWORD/전체 raw를 읽는다."""
        self.position_case=case
        parent=[case[0],1,case[1],case[2],1,2,1,0,bits(20.25),bits(21.75),0,1,0,1]
        events,_,_=super().sample(parent,precision)
        return (events,'|'.join(self.move_values),'|'.join(self.move_raws))


def main():
    """두 원문 정밀도 결과를 비교하거나 고정 원문/제품 SHA를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 이미 관찰한 기대값은 재생성하지 않는다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs']==len({line.split('\t',2)[0]+'\t'+line.split('\t',2)[1] for line in rows});print('마우스 위치 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];chains=[ChainOracle(),ChainOracle()];rows=[]
    # 수식 재구현을 기대값으로 쓰지 않고 실제 두 정밀도만 비교한다.
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append(('Point',','.join(map(str,case)),*outputs[0],'-'))
    # 두 번 연속 호출도 실제 수명/생성/표시 사슬끼리 먼저 일치해야 한다.
    for case in chain_cases():
        outputs=[oracle.sample_position(case,precision) for oracle,precision in zip(chains,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append(('Chain',','.join(map(str,case)),*outputs[0]))
    FIXTURE.write_text('# kind input events globals rawSlots\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_previews.py','tools/decomp_cursor_position.py',FIXTURE.relative_to(ROOT).as_posix(),'cpppj/src/client/RawCursorPosition.h','cpppj/src/client/RawCursorPosition.cpp','cpppj/src/client/RawCursorLifecycle.h','cpppj/src/client/RawCursorLifecycle.cpp','cpppj/src/client/RawCursorPreviews.h','cpppj/src/client/RawCursorPreviews.cpp','cpppj/src/client/RawCursorPlacement.h','cpppj/src/client/RawCursorPlacement.cpp','cpppj/tests/CursorPositionTests.cpp','cpppj/tests/CursorPreviewSupport.h','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt')
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=['00446b50','004971e0','0041d770'],uniqueInputs=len(rows),pointInputs=len(cases()),chainInputs=len(chain_cases()),normalReturns=sum(oracle.returns for oracle in oracles)+sum(oracle.position_returns for oracle in chains),instructionObservations=sum(oracle.instructions for oracle in oracles+chains),instructionObservationsIncludeBoundaryEntries=True,precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['실제 정수 위치 갱신/역투영/CRT 절삭을 실행한다. 단독 입력의 Activate/Create/선택은 명시 효과다.','사슬은 실제 위치/Activate/미리보기/Query/Prepare를 두 번 실행한다. Factory/슬롯/GUI/표시 등은 단위118의 명시 경계를 유지한다.','실제 Client 활성/건설 확정/사용자 완주/전체 픽셀 동일성은 별도다. 원본/클론 게임 또는 OS를 호출하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('마우스 위치:',len(rows),'입력/사슬',len(chain_cases()),'/정상 반환',facts['normalReturns'],'/정밀도 차이0/명령·경계',facts['instructionObservations'])


if __name__=='__main__':main()
