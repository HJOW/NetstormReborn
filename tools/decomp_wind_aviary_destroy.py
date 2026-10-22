#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""생산 프로세스 Form 삭제45c090의 권한/현재DWORD주소/생존 검사를 독립 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import pefile
from decomp_arc_spire import Oracle as BaseOracle,ROOT,STOP,HEAP,VTABLE,STACK
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

POOL=0x15000000  # 실제24000개를 별도 주소에 설치해 TYPE 표와 겹치지 않게 한다.
FIELDS=HEAP+0x800  # 원본80BYTE와 inner+38의 동일한 주소 관계다.
DESTROY=STOP+0x700  # 현재 가상10 효과의 명시 경계다.
IDS=(1,60,127,23999)  # 최초 허용/정상 임시/중간/마지막 허용 슬롯이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/wind-aviary-destroy-x86.tsv'  # 기존 생산1639행과 분리한다.
REPORT=ROOT/'cpppj/wind-aviary-destroy-20261023.json'  # 보호 PE·호스트·정상 반환·SHA 근거다.


def cases():
    """주소 상·하한/비트 감김/생존·현재TYPE/사용하지 않는flags를 교차한다."""
    rows=list(itertools.product((0,1),(0,1,60,127,23999,24000,65535,65536,0x7fffffff,0x80000000,0x8000003c,0xffffffff),(0,1,2,3,4,8),(138,139),(138,),(0,0x200000,0xffffffff),(0,)))
    for mutation,kind,wanted in itertools.product((0,1,2,3),(138,139),(138,139)):rows.append((1,60,0,kind,wanted,0x500,mutation))  # 진단 뒤 현재 필드/가상 표/권한 변경도 관찰한다.
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """큰 객체 삭제와 개발자 진단만 경계로 두고 실제45c090 전체를 실행한다."""
    def __init__(self):
        """분리된 단일 풀과 정확한 코드 범위/메모리 가드를 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x200000);self.allowed=[];ranges(self,(0x45c090,))

    def read(self,mu,access,address,size,value,data):
        """원본 이미지·명시 슬롯·원문·스택 밖 읽기는 즉시 거부한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0x400000,0x600000),(POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'Wind삭제 범위 밖 읽기 {address:x}+{size}')

    def write(self,mu,access,address,size,value,data):
        """객체/프로세스/스택 외 쓰기는 허용하지 않는다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((POOL,POOL+0x200000),(HEAP,HEAP+0x10000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'Wind삭제 범위 밖 쓰기 {address:x}+{size}')

    def instruction(self,mu,address,size,data):
        """진단 뒤 현재 가상 표 재조회와 최초 대상 포인터 보존을 기록한다."""
        self.instructions+=1
        if address==0x45c090:self.calls['0045c090']+=1
        if address==0x4e0620:
            sp=mu.reg_read(UC_X86_REG_ESP);self.event('A',self.get(sp+12))
            if self.mutation==1:self.put(FIELDS+48,0);self.put(POOL+60*50+11,2,1)
            if self.mutation==2:self.put(POOL+60*50,VTABLE+0x100)
            if self.mutation==3:self.put(0x540bc4,0)
            self.ret();return
        if address in (DESTROY,DESTROY+32):
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;self.event('K2' if address!=DESTROY else 'K',sid,self.get(mu.reg_read(UC_X86_REG_ESP)+4));self.put(POOL+sid*50+11,mu.mem_read(POOL+sid*50+11,1)[0]|3,1);self.ret(purge=4);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'Wind삭제 미검토 명령 {address:x}')

    def sample(self,values,precision):
        """원문80BYTE와 네 슬롯을 설치하고 효과와 변경 바이트 전체를 반환한다."""
        boss,temporary,state,kind,wanted,flags,self.mutation=values;self.events=[];self.mu.mem_write(FIELDS,bytes([0xa5])*80);self.put(FIELDS+48,temporary)
        for sid in IDS:  # 허용 주소의 모든 후보를 동일한 입력으로 설치한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=kind;raw[11]=state;raw[34]=2;self.mu.mem_write(POOL+sid*50,bytes(raw))
        for address,value in ((0x5c8464,POOL),(0x5c847c,24000),(0x540bc4,boss),(0x541280,wanted),(VTABLE+0x10,DESTROY),(VTABLE+0x110,DESTROY+32)):self.put(address,value)  # 현재 공유 전역과 두 가상 효과 주소를 공급한다.
        self.call(0x45c090,FIELDS+56,struct.pack('<I',flags),4,precision);self.returns+=1
        return ';'.join(self.events) or '-',bytes(self.mu.mem_read(FIELDS,80)).hex(),''.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in IDS)


def main():
    """새 원문 생성과 읽기 전용 바이트 검사를 구별한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 기대값을 다시 계산하지 않는다.
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'];print('Wind Form삭제 SHA:',len(rows));return
    first,second=Oracle(),Oracle();rows=[]
    for values in cases():  # 두x87 정밀도에서 전체80BYTE/네 슬롯/효과가 같아야 한다.
        a=first.sample(values,0x27f);b=second.sample(values,0x37f);assert a==b,values;rows.append((','.join(map(str,values)),*a))
    FIXTURE.write_text('# input effects fields slots1/60/127/23999\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_wind_aviary_destroy.py','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));entries={f'{table:08x}':{f'{off:02x}':f'{int.from_bytes(pe.get_data(table-0x400000+off,4),"little"):08x}' for off in offsets} for table,offsets in ((0x509ddc,(0xc,0x10)),(0x509aa0,(0x14,0x18,0x78,0xa0,0xa4)))}  # Form 수명과139의 실제 공통 캡처 진입이다.
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=first.returns+second.returns,nativeInstructions=first.instructions+second.instructions,precisionDifferentInputs=[],nativeCalls=dict(first.calls+second.calls),vtableEntries=entries,sha256={p:digest(ROOT/p) for p in paths},limitations=['45c090 전체 몸체를 실행하고 가상10 객체 삭제와 개발자 진단은 명시 효과 경계로 둔다.','32비트 주소 곱셈의 bit31 별칭을 포함한다. 슬롯 경계 밖 비정렬 손상 포인터의 임의 가상 호출은 관측하지 않는다.','주문 부모의 원주 배치/139 및 모든25종 전체 동일성 완료 증거가 아니다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Wind Form삭제:',len(rows),facts['normalReturns'],facts['nativeInstructions'])


if __name__=='__main__':main()
