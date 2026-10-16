#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 플라이어의 실제 +8 WORD 대상 삭제→Damageable Pre 호출/재진입을 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_particle_gradient import Oracle as CallOracle,ROOT,STACK,STOP,digest
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL,VTABLE,DELETE=0x12000000,0x12400000,STOP+0x100  # 원본 주소와 겹치지 않는 독립 raw/가상 효과 경계다.
IDS=(50,60,61,32767)  # WORD의 높은 값과 부모/대상 변경을 보존할 슬롯이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/flyer-predestroy-x86.tsv'  # 전체 raw/이벤트 기대값이다.
REPORT=ROOT/'cpppj/flyer-predestroy.json'  # 고유 입력/원본/실행기 근거다.


class Oracle(CallOracle):
    """원본 전용 Pre만 실행하며 대상 가상 삭제와 Damageable 하위는 명시 callback이다."""
    def __init__(self):
        """현재 전체 정밀 함수표의 해당 범위만 실행 가능하게 한다."""
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000,(len(image)+4095)&~4095);self.mu.mem_write(0x400000,image)
        # 높은 WORD 슬롯까지 격리 raw 공간을 확보한다.
        for address,size in ((POOL,0x200000),(VTABLE,0x1000),(STACK-0x7f000,0x80000),(STOP,0x1000)):self.mu.mem_map(address,size)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x45e7e0)
        self.allowed=[]
        # 실제 불연속 범위를 각각 등록한다.
        for part in row['ranges'].split(';'):
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.events=[];self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def instruction(self,mu,address,size,data):
        """대상/flags와 callback 뒤 raw 변경만 외부 입력으로 공급한다."""
        if address in (DELETE,0x44b4b0):
            esp=mu.reg_read(UC_X86_REG_ESP);ret,flags=struct.unpack('<2I',mu.mem_read(esp,8));sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50
            self.events.append(('D' if address==DELETE else 'P')+f':{sid}:{flags}')
            if address==DELETE:
                assert sid==self.child and flags==0
                if self.mutation==1:mu.mem_write(POOL+2508,struct.pack('<H',61));mu.mem_write(POOL+2540,b'\x09')
                if self.mutation==2:mu.mem_write(POOL+2508,bytes(2));mu.mem_write(POOL+sid*50+11,b'\x01')
            elif self.mutation==3:mu.mem_write(POOL+2514,struct.pack('<f',9.75))
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+8);mu.reg_write(UC_X86_REG_EIP,ret);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'전용 Pre 범위 밖 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """원문 몸체는 스택 외의 raw 쓰기를 하지 않는다."""
        if not STACK-0x7f000<=address or address+size>STACK+0x1000:raise RuntimeError(f'전용 Pre 쓰기 범위 밖 {address:08x}')

    def sample(self,case,precision):
        """원문 함수 반환 뒤 네 슬롯과 외부 효과 순서를 그대로 보존한다."""
        self.child,flags,self.mutation=case;mu=self.mu;self.events=[]
        # 실제 TYPE/상태가 WORD 위쪽에 있어 DWORD 읽기로 바꿀 수 없는 입력이다.
        for sid in IDS:
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=144;raw[11]=2;struct.pack_into('<2f',raw,14,2.125,3.25)
            if sid==50:struct.pack_into('<H',raw,8,self.child)
            mu.mem_write(POOL+sid*50,bytes(raw))
        mu.mem_write(VTABLE+16,struct.pack('<I',DELETE));mu.mem_write(0x5c8464,struct.pack('<I',POOL))
        self.call(0x45e7e0,POOL+2500,struct.pack('<I',flags),4,precision)
        return ';'.join(self.events),b''.join(bytes(mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex()


def main():
    """두 정밀도 원문을 독립 보존하거나 재계산 없이 해시/고유 입력을 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 전체 근거 파일 원문 바이트를 비교한다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({tuple(row[:3]) for row in rows})==len(rows)
        print('플라이어 Pre SHA/고유 입력 감사:',len(rows));return
    cases=list(itertools.product((0,60,61,32767),(0,1,0x100000,0x200000,0x4100000,0xffffffff),range(4)));oracles=[Oracle(),Oracle()];rows=[]
    # 같은 입력의 두 정밀도 출력을 비교하며 고유 입력 수에 합산하지 않는다.
    for case in cases:
        output=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert output[0]==output[1];rows.append('\t'.join([*map(str,case),*output[0]]))
    FIXTURE.write_text('# child flags mutation events slots200\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_flyer_predestroy.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',uniqueInputs=len(cases),normalReturns=len(cases)*2,precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['실제0045e7e0의 WORD/분기/호출 순서를 실행했다. 대상의 가상 삭제와 Damageable Pre는 명시 callback이다.','전체 네 raw/flags/재진입을 보존한다. 실제 전체 삭제/폭발/사용자 완주 증거와 구별한다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('플라이어 Pre:',len(cases),'고유 입력, 두 정밀도 정상 반환')


if __name__=='__main__':main()
