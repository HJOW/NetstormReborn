#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 운반물 위치/발자국 탐색/거의 올림을 실제 명령으로 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_object_menu import Oracle as BaseOracle,ROOT,POOL,TYPES,TEXT,STACK,STOP,digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG

FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-drop-point-x86.tsv'  # 기존 메뉴 관측과 별도인 완성 입력/출력이다.
REPORT=ROOT/'cpppj/carrier-drop-point.json'  # 원문 명령/경계/제품 의존 SHA를 보존한다.
OUT=TEXT+0x4000  # 가드 안의 두 출력 DWORD다.
ENTRIES=(0x426a70,0x49bfe0,0x41d7a0,0x49a840,0x4e49c0)  # 실제 실행하며 응답을 대체하지 않는 몸체다.


def bits(value):
    """단정도 입력을 산술 없이 DWORD로 저장한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """주변/칸 순서·float 경계·현재 전역 재조회에 필요한 입력만 구성한다."""
    # 열: 동작,x,y,소유자,타입flags2,footX,footY,중심x/y,지도한계,실패x/y,성공호출번호,반환DWORD,변이,argument,flags,queryOwner.
    base=[0,bits(40.25),bits(50.75),1,0,2,3,bits(12.75),bits(18.125),256,0,0,0,1,0,7,3,0xfedcba98];rows=[]
    # 사방향 네 개를 모두 거친 뒤 대각선 네 개로 넘어가는 순서와 완전 실패다.
    for ordinal,mutation in itertools.product(range(51),range(10)):
        row=base.copy();row[12]=ordinal;row[14]=mutation;rows.append(row)
    # 독립 발자국 helper의 x 우선/y 후순위와 빈/음수 치수·인자 폭을 관찰한다.
    for shape,ordinal in itertools.product(((-1,3),(0,3),(3,0),(1,1),(2,3),(3,2),(4,4)),range(19)):
        row=base.copy();row[0]=1;row[5:7]=shape;row[12]=ordinal;rows.append(row)
    # x87 절삭의 하위 signed DWORD와 fild32 재변환 경계다.
    values=(0,0x80000000,bits(-1.75),bits(-.99999),bits(.00002),bits(1.00001),0x4effffff,0x4f000000,0x4f800000,0xcf800000,0x5effffff,0x5f000000,0x7f800000,0xff800000,0x7fc01234,0x7fa01234)
    for x,y in itertools.product(values,values):
        row=base.copy();row[0]=2;row[1:3]=x,y;rows.append(row)
    # 초기 즉시 허용과 현재 중심 분기, 엄격한0/한계·NaN 비교를 따로 공급한다.
    for flag,value,ordinal,limit in itertools.product((0,0x100),(bits(-1),0,bits(.25),bits(63.25),bits(255),bits(256),0x7fc01234,0x7f800000),(0,1,8),(64,256)):
        row=base.copy();row[4]=flag;row[1]=value;row[7]=value;row[12]=ordinal;row[9]=limit;rows.append(row)
    for result in (0,1,0x80000000,0xffffffff):
        row=base.copy();row[12]=1;row[13]=result;rows.append(row)
    return list(dict.fromkeys(tuple(v&0xffffffff for v in row) for row in rows))


class Oracle(BaseOracle):
    """공통 격리 메모리를 빌리고 큰 전체 배치 검사와 중심 조회만 명시 경계로 둔다."""
    def __init__(self):
        """현재 정밀 함수표의 불연속 범위와 쓰기 감시를 준비한다."""
        super().__init__('originals')
        self.allowed=[]  # 현재 정밀 함수표에서 얻은 실행 허용 구간이다.
        self.entries=Counter()  # 실제 허용 몸체별 진입 횟수다.
        self.normal_returns=0  # ABI·가드 검사를 통과한 루트 정상 반환 수다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as fp:table={int(r['entry'],16):r for r in csv.DictReader(fp,delimiter='\t')}
        # 내보낸 실제 마지막 명령 바이트까지 포함한다.
        for entry in ENTRIES:
            for part in table[entry]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        # 제품이 재사용하는 기존 방향 표와 실제 PE가 일치하는지 확인한다.
        assert struct.unpack('<8i',self.mu.mem_read(0x52f83c,32))==(0,1,1,1,0,-1,-1,-1)
        assert struct.unpack('<8i',self.mu.mem_read(0x52f85c,32))==(-1,-1,0,1,1,1,0,-1)
        assert self.word(0x502310)==bits(.9999899864196777)

    def write(self,mu,access,address,size,value,data):
        """실제 코드의 출력8바이트와 스택 쓰기만 허용한다. 경계 효과는 별도 입력이다."""
        assert OUT<=address and address+size<=OUT+8 or STACK-0x10000<=address and address+size<=STACK+0x1000,(hex(address),size)

    def instruction(self,mu,address,size,data):
        """실제 호출 인수를 기록하며 원문의 위치/반복/비교 결과를 계산해 대신하지 않는다."""
        self.instructions+=1;esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x49b510:
            args=struct.unpack('<6I',mu.mem_read(esp+4,24));self.queries+=1;self.emit('P',(ecx-TYPES)//500,*args)
            if self.queries==1:
                mutation=self.case[14]
                if mutation in (1,8):self.put(self.raw+34,(self.case[3]+3)&255,1)
                if mutation==2:self.put(self.typ+0x1d4,0);self.put(self.typ+0x1d8,0)
                if mutation in (3,8):self.put(self.typ+0x1d4,3);self.put(self.typ+0x1d8,3)
                if mutation in (4,8):self.put(0x531928,64)
                if mutation in (5,8):self.put(0x545370,bits(17.25));self.put(0x545374,bits(18.5))
                if mutation==6:self.put(self.raw+14,bits(99));self.put(self.raw+18,bits(-77))
                if mutation==7:self.put(self.typ+0xec,0x100)
                if mutation==9:self.put(self.typ+0x1d8,1)
            self.ret(self.case[13] if self.queries==self.case[12] else 0,24);return
        if address==0x4adea0:
            target=self.word(esp+4);self.emit('C',(ecx-POOL)//50);self.put(target,self.case[7]);self.put(target+4,self.case[8]);self.ret(target,4);return
        assert any(a<=address and address+size<=b for a,b in self.allowed),f'미등록 실행 {address:08x}'
        if address in ENTRIES:self.entries[f'{address:08x}']+=1

    def sample(self,case,precision):
        """전체 입력/출력 가드와 ABI·x87 복구 및 접근한 raw50바이트를 관찰한다."""
        self.case=case  # 현재 독립 입력18DWORD다.
        self.events=[]  # 현재 명시 배치·중심 경계의 인수 순서다.
        self.queries=0  # 현재 MayPlace 외부 경계의 호출 횟수다.
        self.raw=POOL+50*50  # 실제 carrier50의 격리 슬롯 주소다.
        self.typ=TYPES+123*500  # 실제 target123의 격리 TYPE 주소다.
        self.mu.mem_write(POOL,b'\0'*0x10000);self.mu.mem_write(TYPES,b'\0'*0x20000);self.mu.mem_write(self.raw,b'\xa5'*50)
        self.put(0x59ab20,TYPES);self.put(0x531928,case[9]);self.put(0x545370,case[10]);self.put(0x545374,case[11])
        self.put(self.raw+10,158,1);self.put(self.raw+14,case[1]);self.put(self.raw+18,case[2]);self.put(self.raw+34,case[3],1)
        self.put(self.typ+0xec,case[4]);self.put(self.typ+0x1d4,case[5]);self.put(self.typ+0x1d8,case[6])
        self.mu.mem_write(OUT-16,b'\xa5'*40)
        saved={UC_X86_REG_EBX:0x123456,UC_X86_REG_ESI:0x234567,UC_X86_REG_EDI:0x345678,UC_X86_REG_EBP:0x456789}
        # 호출마다 이전 x87 스택과 플래그를 이어받지 않는다.
        for register,value in saved.items():self.mu.reg_write(register,value)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EDX):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.put(STACK,STOP)
        operation=case[0]
        if operation==0:entry,purge,owner=0x426a70,8,self.raw;args=(OUT,123)
        elif operation==1:entry,purge,owner=0x49bfe0,24,self.typ;args=(OUT,case[15],case[1],case[2],case[16],case[17])
        else:entry,purge,owner=0x41d7a0,0,OUT;args=();self.put(OUT,case[1]);self.put(OUT+4,case[2])
        self.mu.reg_write(UC_X86_REG_ECX,owner)
        # 결과를 만들지 않고 실제 스택 인수를 공급한다.
        for i,value in enumerate(args):self.put(STACK+4+i*4,value)
        self.mu.emu_start(entry,STOP,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        assert all(self.mu.reg_read(reg)==value for reg,value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        assert bytes(self.mu.mem_read(OUT-16,16))==b'\xa5'*16 and bytes(self.mu.mem_read(OUT+8,16))==b'\xa5'*16
        self.normal_returns+=1
        state=[self.word(a) for a in (0x531928,0x545370,0x545374,self.typ+0xec,self.typ+0x1d4,self.typ+0x1d8)]
        return ','.join(str(self.word(OUT+i*4)) for i in range(2)),';'.join(self.events) or '-',','.join(map(str,state)),bytes(self.mu.mem_read(self.raw,50)).hex()


def main():
    """독립 두 정밀도를 비교해 새 자료를 쓰거나 기존 고유 입력/SHA를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,wanted in facts['sha256'].items():assert digest(name)==wanted,name
        rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
        assert len(rows)==facts['uniqueInputs']==len({r[0] for r in rows});assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()]
        assert facts['normalReturns']==len(rows)*2 and not facts['osCalls']
        observed=[];operations=Counter()
        # 고정 관측 폭과 두 정밀도 차이를 기존 출력 자료에서 감사한다.
        for row in rows:
            assert len(row)==6 and len(row[0].split(','))==18 and len(row[1].split(','))==2 and len(row[3].split(','))==6 and len(bytes.fromhex(row[4]))==50
            operation=int(row[0].split(',',1)[0]);operations[operation]+=1
            if row[5]!='-':
                assert row[1]!=row[5] and operation==2 and len(row[5].split(','))==2
                observed.append(dict(input=row[0],precision53=row[1],precision64=row[5]))
        assert observed==facts['precisionDifferentInputs']
        entries=facts['entryObservations']
        assert entries['00426a70']==operations[0]*2 and entries['0049a840']==operations[0]*2
        assert entries['0049bfe0']>=operations[1]*2 and entries['0041d7a0']>=(operations[0]+operations[2])*2
        assert entries['004e49c0']==entries['0041d7a0']*2
        print('운반물 위치 SHA/고유 입력 감사:',len(rows),'/정밀도 차이',len(observed));return
    oracles=[Oracle(),Oracle()];rows=[];differences=[]
    # 기대값은 Python 수식이나 복원 C++에서 얻지 않는다.
    for case in cases():
        outputs=[o.sample(case,p) for o,p in zip(oracles,(0x027f,0x037f))]
        if outputs[0]!=outputs[1]:
            assert outputs[0][1:]==outputs[1][1:],case
            differences.append(dict(input=','.join(map(str,case)),precision53=outputs[0][0],precision64=outputs[1][0]))
        rows.append((','.join(map(str,case)),*outputs[0],outputs[1][0] if outputs[0]!=outputs[1] else '-'))
    FIXTURE.write_text('# input output53 events globals raw50 output64IfDifferent\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_object_menu.py','tools/decomp_carrier_drop_point.py',FIXTURE.relative_to(ROOT).as_posix()]
    paths+=['cpppj/src/client/RawCarrierDropPoint.h','cpppj/src/client/RawCarrierDropPoint.cpp','cpppj/tests/CarrierDropPointTests.cpp','cpppj/src/client/CMakeLists.txt','cpppj/tests/CMakeLists.txt','cpppj/src/client/RawCarrierMenu.h','cpppj/src/client/RawCarrierMenu.cpp','cpppj/src/o/RawCanonPlacementPipeline.h','cpppj/src/o/RawCanonPlacementPipeline.cpp','cpppj/src/o/RawGunTarget.h','cpppj/src/o/RawGunTarget.cpp','cpppj/src/o/TerrainBuilder.h']
    facts=dict(osCalls=0,host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',entries=[f'{a:08x}' for a in ENTRIES],uniqueInputs=len(rows),normalReturns=sum(o.normal_returns for o in oracles),instructionObservations=sum(o.instructions for o in oracles),instructionObservationsIncludeBoundaryEntries=True,entryObservations=dict(sum((o.entries for o in oracles),Counter())),precisionDifferentInputs=differences,sha256={p:digest(p) for p in paths},limitations=['실제 위치/전체 발자국 탐색/거의 올림/TYPE lookup/CRT를 실행한다. 기본53비트 결과와64비트 차이를 별도 보존한다.','전체 MayPlace와 현재 중심은 명시 경계다. ABI/출력 가드/허용 실행 및 쓰기를 감사한다.','게임/OS 호출은 없으며 실제 Client 건설/사용자 완주를 입증하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('운반물 위치:',len(rows),'/정상 반환',facts['normalReturns'],'/정밀도 차이',len(differences),'/명령·경계',facts['instructionObservations'])


if __name__=='__main__':main()
