#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 웜40 생성자 전체와 사제54/부모의 실제 가상 슬롯을 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import zlib
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP

HEAP,SIZE=0x14500000,0x186f0  # 원본 new와 같은100080바이트 영역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/worm-path-x86.tsv'  # 할당 실패도 포함한 새 생성자 원문이다.
VTABLES=ROOT/'cpppj/tests/fixtures/worm-path-vtables.tsv'  # 두 경로의 모든13슬롯과 두 부모의 의미 있는 슬롯이다.
REPORT=ROOT/'cpppj/worm-path.json'  # 실제 실행/정상 반환/범위/보호 해시다.


def tables():
    """보호 PE의 슬롯을 직접 읽으며 제품 C++의 예상값을 사용하지 않는다."""
    image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image();rows=[]
    # 경로40/54의 모든 가상 함수와 서로 다른 부착 함수도 보존한다.
    for kind,table in ((40,0x50e4c4),(54,0x50e458)):
        rows.append('\t'.join(map(str,(kind,table,*(struct.unpack_from('<I',image,table-0x400000+offset)[0] for offset in range(0,52,4))))))
    # 부모133/187의 다른 Halt를 같은 것으로 취급하지 않는다.
    for kind,table in ((133,0x50e568),(187,0x503ff0)):
        rows.append('\t'.join(map(str,(kind,table,*(struct.unpack_from('<I',image,table-0x400000+offset)[0] for offset in (0x38,0x3c,0x40,0x44,0xc8))))))
    return '# kind vtable path13slots_OR_parent_halt_arrival_started40_started44_fall\n'+'\n'.join(rows)+'\n'


class Oracle(BaseOracle):
    """48bf90과48b2c0을 실제 실행하고 new만 명시 경계로 둔다."""
    def __init__(self):
        """원본을 변경하지 않고 별도 생성자 메모리를 준비한다."""
        super().__init__();self.mu.mem_map(HEAP,0x19000);self.allowed=[];ranges(self,(0x48bf90,0x48b2c0));self.instructions=0;self.calls=collections.Counter();self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def instruction(self,mu,address,size,data):
        """실제 할당 요청과 두 몸체 진입만 기록한다."""
        self.instructions+=1
        if address in (0x48bf90,0x48b2c0):self.calls[f'{address:08x}']+=1
        if address==0x4e4391:
            assert self.get(mu.reg_read(UC_X86_REG_ESP)+4)==SIZE;self.ret(HEAP if self.allocation else 0);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'웜 생성자 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 쓰기는 생성자 전체 영역과 스택으로 제한한다."""
        if HEAP<=address and address+size<=HEAP+SIZE or STACK-0x7f000<=address and address+size<=STACK+0x1000:return
        raise RuntimeError(f'웜 생성자 범위 밖 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 이미지/스택/격리 heap만 읽는다."""
        if any(low<=address and address+size<=high for low,high in ((0x400000,0x600000),(HEAP,HEAP+SIZE),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))):return
        raise RuntimeError(f'웜 생성자 범위 밖 읽기 {address:08x}+{size}')

    def sample(self,fill,allocation,precision):
        """현재 raw100080바이트 전체의 체크섬과 정상 반환을 기록한다."""
        self.allocation=allocation;self.mu.mem_write(HEAP,bytes([fill])*SIZE);PreviousOracle.invoke(self,0x48bf90,b'',precision,False);self.returns+=1
        result=self.mu.reg_read(UC_X86_REG_EAX);assert result in (0,HEAP)
        return str(bool(result)*1),str(zlib.adler32(bytes(self.mu.mem_read(HEAP,SIZE))))


def main():
    """독립 생성자 원문을 수집하거나 기존 자료/보호 해시만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        # 원문/도구/원본의 바이트를 각각 확인한다.
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert len(rows)==6 and report['normalReturns']==12 and all(r[2:4]==r[4:6] for r in rows);assert VTABLES.read_text('utf-8')==tables();print('웜40 원본 감사:6입력/12정상반환/가상표4행');return
    oracles=[Oracle(),Oracle()];rows=[]
    # 미초기화 영역을 지우는 변경도 잡도록 서로 다른 heap 입력과 할당 실패를 관측한다.
    for fill,allocation in itertools.product((0,0xa5,0xff),(0,1)):
        outputs=[oracle.sample(fill,allocation,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))];rows.append((str(fill),str(allocation),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# fill allocation result53 adler53 result64 adler64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');VTABLES.write_text(tables(),encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_worm_path.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/worm-path-x86.tsv','cpppj/tests/fixtures/worm-path-vtables.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-17',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=6,normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),controls=[0x27f,0x37f],boundaries='48bf90/48b2c0 전체. new만 명시 경계이며 raw100080바이트 전체를 비교한다. GP/retN/canary/x87/코드·읽기·쓰기 범위 검사.13개 가상 슬롯 중40/54는부착+2c만 다르다. 부모187의42ac60은133의42a5f0과 다르며 공통 Halt로 바꾸면 안 된다.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls')},ensure_ascii=False))


if __name__=='__main__':main()
