#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호484f90의 전체 조회/독립 할당/난수/두 예약/기본 위임을 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, TYPES, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

POOL, FLOAT0, VALUE = 0x13b00000, STOP + 0x700, STOP + 0x780  # 격리 부모와 실제FPU 반환 경계다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/nugget-postpop-x86.tsv'  # 새 독립 원문이다.
REPORT = ROOT / 'cpppj/nugget-postpop.json'  # 실제 몸체/VT/보호SHA 근거다.


def cases():
    """현재 예약/각 할당 실패/flags 상위비트/float 반환 입력이다."""
    return list(itertools.product((0, 1, 2, 4, 14, 15, 0x80000001, 0xfffffffe, 0xffffffff), (0, 1), range(4), range(4)))


class Oracle(BaseOracle):
    """484f90은 전체 실행하고 조회/할당/난수/Regular/기본 몸체만 경계로 둔다."""
    def __init__(self):
        """실제 ret까지 허용하고 난수의ST0는 fld 명령으로 전달한다."""
        super().__init__();self.mu.mem_map(POOL, 0x10000);self.allowed=[];ranges(self,(0x484f90,));self.instructions=0;self.calls=collections.Counter();self.mu.mem_write(FLOAT0,b'\xd9\x05'+struct.pack('<I',VALUE)+b'\xc3')

    def instruction(self,mu,address,size,data):
        """관측 경계의 호출 순서/인자를 기록하며 원본 부모 분기를 계산하지 않는다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<3I',mu.mem_read(sp+4,12));this=mu.reg_read(UC_X86_REG_ECX)
        if address==0x484f90:self.calls['00484f90']+=1
        if FLOAT0<=address<FLOAT0+7:return
        if address==0x4afb40:self.event('H',(this-POOL)//50,args[0]);self.ret(self.case[1],4);return
        if address==0x4e4391:
            self.event('A',args[0]);success=(self.case[2]>>self.allocations)&1;self.allocations+=1;self.ret(0x13c00000 if success else 0);return
        if address==0x455930:
            self.event('R',*args[:2]);values=((0x3dcccccd,0x3e4ccccd),(0,0),(0x3e99999a,0x3f000000),(0x7fc01234,0x3e800000))[self.case[3]];self.put(VALUE,values[self.randoms]);self.randoms+=1;mu.reg_write(UC_X86_REG_EIP,FLOAT0);return
        if address==0x496e80:self.event('C',*args);self.ret(this,12);return
        if address==0x44bf80:self.event('D',(this-POOL)//50,args[0]);self.ret(0,4);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'너겟 범위밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 부모는SEH와스택 이외의 메모리에 쓰지 않는다."""
        if STACK-0x7f000<=address<=STACK+0x1000-size or address==0 and size==4:return
        raise RuntimeError(f'너겟 범위밖 쓰기 {address:08x}+{size}')

    def sample(self,case,precision):
        """입력마다 경계/SEH를 새로 주고 실제 정상 반환 효과만 읽는다."""
        self.case=case;self.events=[];self.allocations=self.randoms=0;self.put(0x5c8464,POOL);self.put(0,0x87654321)
        self.call(0x484f90,POOL+50*50,struct.pack('<I',case[0]),4,precision);self.returns+=1;assert self.get(0)==0x87654321
        return ';'.join(self.events) or '-'


def main():
    """새 원문을 저장하거나 변경 없이 입력/SHA/VT를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));vt={str(offset):struct.unpack('<I',pe.get_data(0x50dc30+offset-0x400000,4))[0] for offset in (0x10,0x14,0x18,0x20)};thunk=pe.get_data(vt['20']-0x400000,5).hex()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text('utf-8').splitlines() if r and not r.startswith('#')]
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert report['vtable']==vt and report['preThunk']==thunk and [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()]
        assert len(rows)==report['uniqueInputs']==288 and report['normalReturns']==576 and all(r[1]==r[2] for r in rows)
        print('너겟PostPop 원본 감사:288입력/576정상반환');return
    oracles=[Oracle(),Oracle()];rows=[]
    # 제품 코드 없이 실제 부모의 두 정밀도 반환을 각각 보존한다.
    for case in cases():
        outputs=[o.sample(case,p) for o,p in zip(oracles,(0x27f,0x37f))];assert outputs[0]==outputs[1];rows.append((','.join(map(str,case)),*outputs))
    FIXTURE.write_text('# input events53 events64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_nugget_postpop.py','tools/decomp_cursor_lifecycle.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/nugget-postpop-x86.tsv')
    report=dict(host=socket.gethostname(),date='2026-10-17',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),controls=[0x27f,0x37f],uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),vtable=vt,preThunk=thunk,precisionDifferentInputs=[],boundaries='조회4afb40/할당/게임난수455930/Regular496e80/Damageable44bf80은명시입력. 부모484f90전체/SEH/스택/GP/x87검사. 원본/OS창실행없음.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','vtable','preThunk')},ensure_ascii=False))


if __name__=='__main__':main()
