#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다이스의 별도 TYPE 추가 그림 요청을 제품 C++ 구현 전에 격리 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, TYPES, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT / 'cpppj/tests/fixtures/dais-draw-x86.tsv'  # 두 정밀도의 실제 그림 요청을 보존한다.
REPORT = ROOT / 'cpppj/dais-draw-native.json'  # 게임 실행과 구별한 원본 몸체의 관측 기록이다.


def cases():
    """signed 프레임 나머지·현재 프레임 수·좌표와 자체 필터 부재를 교차한다."""
    return list(itertools.product((0,1,2,3,47,63,64,119,120,179,180,181,0x7fffffff,0xffffffff,0xffffff4c,0x80000000),
        (1,3,48,180),(0,2,0x1002),((0,0),(0xffffffff,0x80000000)),(0x12345000,0x12346000)))


class Oracle(BaseOracle):
    """실제4484b0/TYPE getter를 실행하고 일반497280 그림 몸체만 효과 경계로 둔다."""
    def __init__(self):
        """원본 가상 표·타입 전역을 확인하고 허용한 기계어 범위만 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,(0x4484b0,0x49a840));self.instructions=0
        assert (self.get(0x507950+0xa0),self.get(0x507950+0xa4)) == (0x41c0f0,0x4484b0)
        assert self.get(0x5412fc)==169

    def instruction(self,mu,address,size,data):
        """그림의 현재 this와 전체 인자를 관측하며 게임·OS 주소로 넘어가지 않는다."""
        self.instructions+=1
        if address==0x497280:
            sp=mu.reg_read(UC_X86_REG_ESP);self.event('draw',(mu.reg_read(UC_X86_REG_ECX)-POOL)//50,*struct.unpack('<4I',mu.mem_read(sp+4,16)));self.ret(purge=16);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'다이스 미허용 실행 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """이 작은 추가 그림은 호출 스택 외의 원본 상태를 쓰지 않는다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000:
            return
        raise RuntimeError(f'다이스 미허용 쓰기 {address:x}+{size}')

    def sample(self,case,precision):
        """독립 입력의 슬롯과 TYPE를 설치해 동일 기계어의 정상 반환을 확인한다."""
        frame,count,flags,point,sheet=case;self.events=[];self.mu.mem_write(POOL,bytes(0x10000));self.mu.mem_write(TYPES,bytes(0x20000))
        self.put(POOL+50*50,0x507950);self.put(POOL+50*50+10,165,1);self.put(POOL+50*50+11,flags,1);self.put(POOL+50*50+40,flags>>8,1);self.put(POOL+50*50+36,frame)
        self.put(TYPES+169*500+0x114,count);self.put(TYPES+169*500+0xdc,sheet);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0,0x87654321)
        InvokeOracle.invoke(self,0x4484b0,struct.pack('<2I',*point),precision,False);assert self.get(0)==0x87654321;self.returns+=1
        return ';'.join(self.events)


def key(case):
    """입력의 전체 DWORD 비트를 TSV 한 열에 손실 없이 보존한다."""
    frame,count,flags,point,sheet=case
    return ','.join(map(str,(frame,count,flags,*point,sheet)))


def main():
    """새 원문을 최초 동결하거나 기존 원본/도구/기대값의 해시만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        for name,wanted in record['sha256'].items():
            assert digest(ROOT/name)==wanted,name
        assert [row[0] for row in rows]==[key(case) for case in cases()] and all(row[1]==row[2] for row in rows)
        assert len(rows)==record['uniqueInputs']==768 and record['normalReturns']==1536
        print('다이스 추가 그림: 원본768입력/1536정상 반환 원문 보존 확인');return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 원문은 덮어쓰지 않는다'
    oracles=[Oracle(),Oracle()];rows=[]
    # 두 독립 정밀도의 같은 원본 함수를 제품 코드와 무관하게 실행한다.
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1];rows.append((key(case),*outputs))
    FIXTURE.write_text('# input draw53 draw64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_dais_draw.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())  # 原본 실행 의존성과 새 원문만 해시한다.
    record={'unit':'242','host':socket.gethostname(),'date':'2026-10-20','lastDecompileHost':'HJOW-Athlon','lastDecompileDate':'2026-10-20',
        'uniqueInputs':len(rows),'normalReturns':sum(oracle.returns for oracle in oracles),'instructions':sum(oracle.instructions for oracle in oracles),
        'boundary':'실제4484b0/49a840 실행,497280 일반 그림은 this/인자 경계다. 전체 RLE/픽셀·게임/OS/창 실행은 없다.',
        'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({name:record[name] for name in ('uniqueInputs','normalReturns','instructions')}))


if __name__=='__main__':
    main()
