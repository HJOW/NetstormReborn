#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""4d5840의 후보 순위/거부/근접 대체를 실제 x86으로 관측한다. 검색과 픽셀 hit은 명시 경계다."""
import argparse
import hashlib
import itertools
import json
import random
import socket
import struct
from decomp_oracle import Oracle,ROOT,SCRATCH,STOP,STACK
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP

POOL=0x12000000  # 격리된50BYTE 물리 풀이다.
TABLE=0x11000000  # 합성 hit 경계의 가상 표이며 실제 유닛을 만들지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/user-pick-x86.tsv'  # 제품과 독립된 원문 결과다.
REPORT=ROOT/'cpppj/user-pick-native-20261028.json'  # 경계/해시/입력 범위를 기록한다.


def bits(value):
    """입력float의 원문32비트를 직렬화한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def digest(path):
    """실행한 입력의 전체 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases():
    """합성 TYPE80..87로 순위/동률/숨김/강제/근접 경계를 교차한다. 공중3종 전용 관측은 없다."""
    result=[]
    for owner_a,owner_b,mobile_a,mobile_b,z_a,z_b in itertools.product((0,1,2),(0,1,2),(0,0x10000),(0,0x20000),(-30,-20,0),(-30,-20,0)):  # 모든 두 후보 소유/이동/깊이 순위를 관측한다.
        objects=[(50,80,0,mobile_a,owner_a,z_a,bits(51),bits(51),0,1,1,1),(51,81,0,mobile_b,owner_b,z_b,bits(51),bits(51),0,1,1,1)]
        result.append(((0,1,0,0,0,bits(50),bits(50)),objects))
    rng=random.Random(0x5221078)
    for index in range(768):  # 인접float/동률 좌표·마지막 표면/근접 보정과 독립 거부 조건이다.
        header=(int(index%97==0),rng.choice((0,1,2)),rng.randrange(2),rng.randrange(2),rng.choice((0,0,1)),bits(50),bits(50))
        objects=[]
        for number in range(rng.randrange(1,9)):  # 검색이 반환한 순서를 그대로 입력한다.
            genus=rng.choice((0,2,4,0x80000,0x10000,0x20000,0x2000000,0x80000000,0x4000))
            objects.append((50+number,80+number,rng.choice((0,0,0x8000000)),genus,rng.choice((0,1,2)),rng.choice((-127,-30,-20,0,30,127)),bits(rng.choice((49.5,50,50.5,51,51.5,52.099998,52.100002,53))),bits(rng.choice((50,50.5,51,51.5,53))),rng.choice((0,0,0x10)),rng.randrange(2),rng.randrange(1,4),rng.randrange(1,4)))
        result.append((header,objects))
    return result


class PickOracle(Oracle):
    """원문 순위/필터/중심/거리를 실행하고 UI/검색/픽셀 hit만 합성 입력으로 분리한다."""
    def __init__(self):
        """보호PE와 실제 작은 하위의 명시 범위를 읽는다."""
        super().__init__('originals',[dict(name='Pick',originals='004d5840')],[0x4ac200,0x4ac1e0,0x4ac550,0x49a840,0x49a900,0x4adea0,0x41cfc0])
        self.mu.mem_map(POOL,0x10000);self.mu.mem_map(TABLE,0x1000);self.mu.mem_write(TABLE+0xb0,struct.pack('<I',STOP+0x100));self.instructions=0

    def word(self,address):
        """에뮬레이터의 현재DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def finish(self,value,purge=0):
        """명시한 경계만 원래 호출 규약에 맞춰 반환한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_EIP,self.word(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def on_instruction(self,mu,address,size,data):
        """선택 계산을 대신하지 않으며 경계의 인자와 호출 순서를 함께 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        if address==0x460de0:self.trace.append('paused');self.finish(self.header[0]);return
        if address==0x4971e0:
            assert self.word(sp+8)==480 and self.word(sp+12)==330;self.trace.append('project');mu.mem_write(self.word(sp+4),struct.pack('<II',*self.header[5:]));self.finish(0);return
        if address==0x422230:
            args=[self.word(sp+4+i*4) for i in range(5)];assert args[:2]==list(self.header[5:]) and args[4]==0
            self.trace.append('begin:'+str(args[2])+','+str(args[3]));self.index=0;mu.mem_write(this+0x34,struct.pack('<I',50));self.finish(this,20);return
        if address==0x4b1810:
            self.index+=1;sid=50+self.index if self.index<len(self.objects) else 0;mu.mem_write(this+0x34,struct.pack('<I',sid));self.finish(sid);return
        if address==STOP+0x100:
            sid=(this-POOL)//50;assert self.word(sp+4)==480 and self.word(sp+8)==330;self.trace.append('hit:'+str(sid));self.finish(self.objects[sid-50][9],8);return
        return super().on_instruction(mu,address,size,data)

    def observe(self,case):
        """현재 raw/TYPE만 설치하고 실제 함수 반환·콜백 순서·불변성을 읽는다."""
        self.header,self.objects=case;self.trace=[]
        for address,value in ((0x5c8464,POOL),(0x5c847c,240),(0x59ab20,SCRATCH),(0x540c70,self.header[1]),(0x594fc8,self.header[2]),(0x59a8ac,self.header[3]),(0x5424b4,2421574144),(0x5412e8,164),(0x5411a8,84)):
            self.mu.mem_write(address,struct.pack('<I',value))  # 원래 UI가 공급한 현재 전역이다.
        self.mu.mem_write(POOL,bytes(0x10000));self.mu.mem_write(SCRATCH,bytes(188*500))
        for sid,kind,flags,genus,owner,depth,x,y,extra,hit,foot_x,foot_y in self.objects:  # 합성 슬롯을 원문과 같은 비정렬 위치에 설치한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,TABLE);raw[10]=kind;raw[34]=owner;raw[35]=depth&255;raw[40]=extra;struct.pack_into('<II',raw,14,x,y);self.mu.mem_write(POOL+sid*50,bytes(raw));base=SCRATCH+kind*500
            self.mu.mem_write(base+0xe8,struct.pack('<II',flags,genus));self.mu.mem_write(base+0x1d4,struct.pack('<II',foot_x,foot_y))
        before=bytes(self.mu.mem_read(POOL,0x10000));value=self.call('Pick',[480,330,self.header[4]]);assert bytes(self.mu.mem_read(POOL,0x10000))==before
        return value,';'.join(self.trace)


def main():
    """새 결과만 작성하거나 원래fixture를 보존하면서 기계어를 재실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args();oracle=PickOracle();rows=[]
    for header,objects in cases():  # 기대값 생성에는 C++ 출력이나 Python 순위 모델을 사용하지 않는다.
        value,trace=oracle.observe((header,objects));rows.append('\t'.join((','.join(map(str,header)),';'.join(','.join(map(str,obj)) for obj in objects),str(value),trace)))
    raw='# header=paused,owner,meta,hide,force,worldXbits,worldYbits objects=sid,type,flags1,flags2,owner,z,xBits,yBits,extra,hit,footX,footY result trace\n'+'\n'.join(rows)+'\n'
    if options.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name  # 보호 원문과 도구도 식별한다.
        assert FIXTURE.read_text('utf-8')==raw;print('선택 순위 원문 재실행',len(rows),'차이0');return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 관측은 덮어쓰지 않는다';FIXTURE.write_text(raw,'utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_oracle.py','tools/decomp_user_pick.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(unit=522,host=socket.gethostname(),date='2026-10-28',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23',outputsModified=False),uniqueInputs=len(rows),instructions=oracle.instructions,assertions=oracle.assertions,boundaries='원문4d5840/4ac200/4ac1e0/4ac550/49a840/49a900/4adea0/41cfc0. 460de0/4971e0/422230/4b1810와 가상+b0는 명시 입력 경계. 합성TYPE80..87만 사용하며 실제3공중 유닛 생성/게임/OS/통신은 실행하지 않음.',sha256={name:digest(ROOT/name) for name in names})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(dict(inputs=len(rows),instructions=oracle.instructions,assertions=oracle.assertions)))


if __name__=='__main__':main()
