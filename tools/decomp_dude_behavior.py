#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude의 정지·지면 판정·도착·삭제 후처리를 제품 구현 전에 원본 PE로 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/dude-behavior-x86.tsv'  # 원본 상위 함수와 효과 경계의 독립 입력/결과다.
REPORT=ROOT/'cpppj/dude-behavior-native.json'  # 관측 범위·원본·기존 호출 도구의 해시를 보존한다.
SPOTS,LIST=0x16000000,0x16020000  # 원본 지도와 population의 격리 저장소다.
ENTRIES=(0x450e60,0x450e80,0x450f50,0x450960,0x450920)  # Halt/CheckSpace/Fall/Arrival/PostDestroy의 실제 주소다.


def cases():
    """부모 상태·작업·지면·진단과 population의 중복·stale 바이트를 교차한다."""
    result=[]
    # 절삭 칸과 거의 올림 칸을 반대 비트로 채워 실제450e80의+500edc 조회를 구별한다.
    for operation,status,task,spot in itertools.product((0,1),(0,1,2,3),(0x5101,0x5102,0x5105,0x5106,0x5107),(0,2,4,6,8,10,12,14)):
        result.append((operation,status,task,spot,0,0))
    for status in (0,2):
        result.append((2,status,0x5102,6,0,0))
    # 도착은 현재 task와 free/dead/진단 조건만 읽는다. 알려지지 않은 task의 진단 후 반환도 보존한다.
    for status,task,checking in itertools.product((0,1,2,3),(0x5100,0x5101,0x5102,0x5103,0x5104,0x5105,0x5106,0x5107),(0,1)):
        result.append((3,status,task,6,checking,0))
    for extra,pattern in itertools.product((0,1,2,8,9,16),range(4)):
        result.append((4,2,0x5107,6,0,(extra<<8)|pattern))
    return result


class Oracle(BaseOracle):
    """여섯 실제 작은 몸체를 실행하며 큰 예약·삭제·공통 후처리만 명시 효과 경계로 둔다."""
    def __init__(self):
        """보호 PE와 격리 지도/list를 준비하고 함수/가상 표를 검증한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.mu.mem_map(SPOTS,0x10000);self.mu.mem_map(LIST,0x1000)
        self.allowed=[];ranges(self,(0x450e80,0x450f50,0x450960,0x450920,0x40ea00,0x4e49c0));self.allowed.append((0x450e60,0x450e7a));self.instructions=0
        assert (self.get(0x508cc8+0x38),self.get(0x508cc8+0x3c),self.get(0x508cc8+0x18))==(0x450e60,0x450960,0x450920)

    def instruction(self,mu,address,size,data):
        """가상 효과의 실제 this/인자를 기록하며 OS·게임 진입점은 허용하지 않는다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        if address==0x450670:
            self.event('task',(this-POOL)//50,*struct.unpack('<2I',mu.mem_read(sp+4,8)));self.ret(purge=8);return
        if address==0x488fe0:
            self.event('type',self.get(sp+4));self.ret(24);return
        if address==0x4b1a90:
            self.event('lookup',*struct.unpack('<4I',mu.mem_read(sp+4,16)));self.ret();return
        if address==0x4af780:
            self.event('destroy',(this-POOL)//50,self.get(sp+4));self.ret(1,4);return
        if address==0x4b0840:
            self.event('post',(this-POOL)//50,self.get(sp+4));self.ret(purge=4);return
        if address==0x4e0620:
            self.event('assert',self.get(sp+12));self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'Dude 미허용 실행 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """실제 list 압축과 스택 이외의 쓰기를 거부한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000:
            return
        if LIST<=address and address+size<=LIST+0x1000 or 0x55a4f0<=address and address+size<=0x55a4fc:
            return
        raise RuntimeError(f'Dude 미허용 쓰기 {address:x}+{size}')

    def sample(self,case,precision):
        """독립 현재 raw/list를 설치하고 정상 반환·효과·전체 population 바이트를 관측한다."""
        operation,status,task,spot,checking,tail=case;self.events=[];self.mu.mem_write(POOL,bytes(0x10000));raw=POOL+50*50
        self.put(raw,0x508cc8);self.put(raw+10,70,1);self.put(raw+11,status,1);self.put(raw+12,task,2);self.put(raw+14,struct.unpack('<I',struct.pack('<f',20.75))[0]);self.put(raw+18,struct.unpack('<I',struct.pack('<f',21.25))[0]);self.put(raw+40,tail>>8,1)
        self.mu.mem_write(SPOTS,bytes([spot^6])*65536);self.put(SPOTS+21*256+20,spot,1);self.put(0x5c7c44,SPOTS);self.put(0x5c8464,POOL);self.put(0x5410d0,24);self.put(0x5e4794,checking)
        patterns=((50,51,50,52,99),(51,52,53,54,99),(50,50,50,50,99),(51,50,52,50,99));values=patterns[tail&3];self.mu.mem_write(LIST,struct.pack('<5I',*values));self.put(0x55a4f0,LIST);self.put(0x55a4f4,5);self.put(0x55a4f8,4);self.put(0,0x87654321)
        before=bytes(self.mu.mem_read(raw,50));args=struct.pack('<2I',17,23) if operation==3 else struct.pack('<I',0x12345678) if operation==4 else b''
        InvokeOracle.invoke(self,ENTRIES[operation],args,precision,False);assert self.get(0)==0x87654321 and bytes(self.mu.mem_read(raw,50))==before;self.returns+=1
        return '|'.join((str(self.mu.reg_read(UC_X86_REG_EAX)) if operation in (0,1,2,3) else '-', ';'.join(self.events) or '-', str(self.get(0x55a4f8)), ','.join(map(str,struct.unpack('<5I',self.mu.mem_read(LIST,20))))))


def main():
    """최초 관측만 생성하거나 기존 원본/도구/fixture 해시를 변경 없이 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        for name,wanted in record['sha256'].items():
            assert digest(ROOT/name)==wanted,name
        assert [row[0] for row in rows]==[','.join(map(str,case)) for case in cases()] and all(row[1]==row[2] for row in rows)
        assert len(rows)==record['uniqueInputs'] and len(rows)*2==record['normalReturns'];print(f'Dude 부모 원본 {len(rows)}입력 보존 확인');return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 원문은 덮어쓰지 않는다'
    oracles=[Oracle(),Oracle()];rows=[]
    # 실제 PE의 작은 몸체만 두 정밀도에서 독립 실행한다.
    for case in cases():
        results=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert results[0]==results[1],case;rows.append((','.join(map(str,case)),*results))
    FIXTURE.write_text('# input result53 result64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_dude_behavior.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())  # 원본과 실제 실행 도구·새 관측만 고정한다.
    record={'unit':'243','date':'2026-10-20','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon','lastDecompileDate':'2026-10-20','uniqueInputs':len(rows),'normalReturns':sum(oracle.returns for oracle in oracles),'instructions':sum(oracle.instructions for oracle in oracles),
        'boundaries':'실제 Halt/지면/Fall/Arrival/PostDestroy/list압축/ftol 실행. task 예약·이동 종류 선택·lookup/삭제·공통 post·진단은 인자 경계다. 게임/OS/창 실행 없음.',
        'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({name:record[name] for name in ('uniqueInputs','normalReturns','instructions')}))


if __name__=='__main__':
    main()
