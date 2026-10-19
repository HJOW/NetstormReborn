#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Devastation 효과 패킷의 송신 길이 확정40aca0을 독립 실행한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle, POOL

FIXTURE = ROOT/'cpppj/tests/fixtures/bomb-packet-x86.tsv'  # 송신 전210BYTE 전체의 독립 원문이다.
REPORT = ROOT/'cpppj/bomb-packet-native.json'  # 보호PE와 실제 입력/두 정밀도를 고정한다.


def cases():
    """실제0..100 대상수와 비영 송신 인자·기존 선언 길이를 교차한다."""
    return list(itertools.product(range(101), (0, 1, 0xffffffff), (0, 10, 65535)))


class Oracle(BaseOracle):
    """실제 길이 확정 함수만 격리 raw/스택에서 실행한다."""
    def __init__(self):
        """호출자this와210바이트 패킷을 같은 격리 저장소에 둔다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,(0x40aca0,));self.instructions=0

    def instruction(self,mu,address,size,data):
        """보호 함수의 실제 명령 범위 밖으로 실행하지 않는다."""
        self.instructions+=1
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(hex(address))

    def write(self,mu,access,address,size,value,data):
        """스택과 패킷 길이WORD만 변경할 수 있다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==POOL+50*50+2 and size==2:return
        raise RuntimeError(f'송신 길이 미허용 쓰기 {address:x}+{size}')

    def sample(self,case,precision):
        """출력은 원본 함수가 쓴210바이트 전체이며 수식으로 재생성하지 않는다."""
        packet=bytearray(b'\xa7'*210);struct.pack_into('<BBHHI',packet,0,1,0x5f,case[2],15000,case[0]);self.mu.mem_write(POOL+50*50,bytes(packet))
        self.put(0,0x87654321);InvokeOracle.invoke(self,0x40aca0,struct.pack('<I',case[1]),precision,False);assert self.get(0)==0x87654321;self.returns+=1
        return bytes(self.mu.mem_read(POOL+50*50,210)).hex()


def main():
    """제품 구현 전에 한 번 동결하거나 기존 입력/기대값/해시만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if options.verify:
        record=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        for name,wanted in record['sha256'].items():assert digest(ROOT/name)==wanted,name
        assert [tuple(map(int,row[0].split(','))) for row in rows]==cases() and all(row[1]==row[2] for row in rows)
        assert len(rows)==record['uniqueInputs']==909 and record['normalReturns']==1818
        print('주문 송신 길이 원문 감사909/1818');return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 원문은 덮어쓰지 않는다'
    oracles=[Oracle(),Oracle()];rows=[]
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1];rows.append((','.join(map(str,case)),*outputs))
    FIXTURE.write_text('# input packet53 packet64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n','utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_bomb_packet.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    record=dict(unit=236,host=socket.gethostname(),date='2026-10-20',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-19'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),boundaries='40aca0 실제 길이 확정 전체다. 유효0..100 대상수/송신 인자/기존 길이와210BYTE 전체를 두 정밀도로 관측한다. GP/ret4/canary/SEH/x87/실행/쓰기범위 확인. 실제 전송/게임 성공을 증명하지 않는다.',sha256={name:digest(ROOT/name) for name in paths})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','normalReturns','instructions')}))


if __name__=='__main__':main()
