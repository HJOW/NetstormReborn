#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Devastation의 추가 그림41e3d0이 실제 전달하는 SHP/프레임/좌표를 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, TYPES, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL
from unicorn.x86_const import UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/devastation-draw-x86.tsv'  # 추가 그림 요청의 독립 인자 원문이다.
REPORT=ROOT/'cpppj/devastation-draw-native.json'  # 실제 가상 표와 보호PE 출처를 고정한다.


def cases():
    """현재 프레임/입력 좌표의 전체 비트와 TYPE의 실제 그림 토큰을 교차한다."""
    return list(itertools.product((0,1,17,0xffffffff),((0,0),(15,23),(0xffffffff,0x80000000),(0x7fffffff,0xfffffff1)),(0x12345000,0x12346000)))


class Oracle(BaseOracle):
    """실제 추가 그림·TYPE 몸체를 실행하고 큰 RLE 그리기만 경계로 둔다."""
    def __init__(self):
        """가상 주소와 격리 현재 슬롯/TYPE를 검증한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,(0x41e3d0,0x4ac550,0x49a840));self.instructions=0
        assert (self.get(0x502370+0xa0),self.get(0x502370+0xa4))==(0x41c0f0,0x41e3d0)

    def instruction(self,mu,address,size,data):
        """대상 그리기의 네 실제 인자를 원문 순서대로 관측한다."""
        self.instructions+=1
        if address==0x4daa70:
            sp=mu.reg_read(UC_X86_REG_ESP);self.event('draw',*struct.unpack('<4I',mu.mem_read(sp+4,16)));self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(hex(address))

    def write(self,mu,access,address,size,value,data):
        """그림 요청은 스택 밖의 상태를 쓰지 않는다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000:return
        raise RuntimeError(f'그림 미허용 쓰기 {address:x}+{size}')

    def sample(self,case,precision):
        """현재 raw/TYPE 입력을 설치하고 실제 그리기 호출만 반환한다."""
        frame,point,sheet=case;self.events=[];self.mu.mem_write(POOL,bytes(0x10000));self.mu.mem_write(TYPES,bytes(0x20000));self.put(POOL+50*50,0x502370);self.put(POOL+50*50+10,168,1);self.put(POOL+50*50+36,frame);self.put(TYPES+168*500+0xdc,sheet);self.put(0x5c8464,POOL);self.put(0x59ab20,TYPES);self.put(0,0x87654321)
        InvokeOracle.invoke(self,0x41e3d0,struct.pack('<2I',*point),precision,False);assert self.get(0)==0x87654321;self.returns+=1;return ';'.join(self.events)


def main():
    """기대값을 최초 동결하거나 기존 보호 해시/입력/두 정밀도만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if options.verify:
        record=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        for name,wanted in record['sha256'].items():assert digest(ROOT/name)==wanted,name
        assert [row[0] for row in rows]==[','.join(map(str,(f,*p,s))) for f,p,s in cases()] and all(row[1]==row[2] for row in rows)
        assert len(rows)==record['uniqueInputs']==32 and record['normalReturns']==64;print('Devastation 추가 그림 원문 감사32/64');return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 원문은 덮어쓰지 않는다'
    oracles=[Oracle(),Oracle()];rows=[]
    for case in cases():
        outputs=[o.sample(case,p) for o,p in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1];frame,point,sheet=case;rows.append((','.join(map(str,(frame,*point,sheet))),*outputs))
    FIXTURE.write_text('# input draw53 draw64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n','utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_devastation_draw.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    record=dict(unit=236,host=socket.gethostname(),date='2026-10-20',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-19'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),boundaries='실제41e3d0/TYPE는 전체 실행하고4daa70 RLE만 경계다. 보호VT+a0=빈41c0f0/+a4=41e3d0 확인. GP/ret8/canary/SEH/x87/실행/쓰기범위 확인. 캡처 전체 픽셀의 원본 대조나 게임 성공을 증명하지 않는다.',sha256={name:digest(ROOT/name) for name in paths})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','normalReturns','instructions')}))


if __name__=='__main__':main()
