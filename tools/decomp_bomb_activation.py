#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본5c/41e860의 원격 비용 처리와 주문 Regular 예약을 독립 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,PROCESS,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as InvokeOracle,POOL
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/bomb-activation-x86.tsv' # 제품 구현 전에 보존하는 독립 원문이다.
REPORT=ROOT/'cpppj/bomb-activation-native.json' # 비용/할당 경계와 후속 피해 미검증을 구별한다.
ENTRIES=(0x41e860,0x4ac200,0x49a840,0x40eb80,0x4e49c0) # 실제 실행할5개 원본 몸체다.
PACKET,INPUT,THUNK=STOP+0x600,STOP+0x640,STOP+0x700 # cdecl 수신의 실제 호출자 입력이다.


def cases():
    """free/dead·로컬/원격·전체 소유자DWORD·비용·할당 실패와 효과 뒤 현재TYPE를 입력한다."""
    rows=[]
    # 원본 비용은 정수 인코딩에서 직접 복호화한다.
    for flags,owner,local,cost,genus,allocate in itertools.product((0,1,2,3,4,8),(1,2),(1,2,257),(0,100,1001),(0,0x100),(0,1)):
        rows.append((flags,owner,local,cost,genus,allocate,0))
    # 비용 효과 뒤 현재TYPE 변경과 복귀 진단 뒤에도 계속하는 경로를 관측한다.
    for mutation in (1,2):
        rows.append((0,1,2,100,0x100,1,mutation))
    return rows


class Oracle(BaseOracle):
    """원본 수신/TYPE/ftol을 실행하며 실제 경제·힙·Regular 생성만 선언 경계로 둔다."""
    def __init__(self):
        """기존 보호PE/SEH/스택과 격리 부모/호출자를 한 번 설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.calls=collections.Counter();self.instructions=0
        code=b'\x68'+struct.pack('<I',PACKET)+b'\x6a\x00\xff\x35'+struct.pack('<I',INPUT)+b'\xe8'+struct.pack('<i',0x41e860-(THUNK+18))+b'\x83\xc4\x0c\xc3'
        assert len(code)==22
        self.mu.mem_write(THUNK,code);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def instruction(self,mu,address,size,data):
        """실제 비용·진단·할당·생성 호출의 인자와 순서를 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<4I',mu.mem_read(sp+4,16));c=self.case
        if address in ENTRIES:
            self.calls[f'{address:08x}']+=1
        if address==0x41df80:
            self.event('subtract',*args[:2])
            if c[6]==1:
                self.put(POOL+50*50+10,169,1)
            self.ret();return
        if address==0x4e0620:
            self.event('assert',args[2])
            if c[6]==2:
                self.put(POOL+50*50+10,169,1)
            self.ret();return
        if address==0x4e4391:
            self.event('allocate',args[0]);self.ret(PROCESS if c[5] else 0);return
        if address==0x496f00:
            assert mu.reg_read(UC_X86_REG_ECX)==PROCESS
            self.event('construct',*args[:3]);self.ret(PROCESS,12);return
        if THUNK<=address<THUNK+22:
            return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'주문 활성화 미허용 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """원본 수신 몸체는 스택과 격리 SEH만 직접 변경한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4:
            return
        raise RuntimeError(f'주문 활성화 미허용 쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호PE·격리 슬롯/TYPE/프로세스·호출자 밖 읽기를 거부한다."""
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(PROCESS,PROCESS+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(low<=address and address+size<=high for low,high in spans):
            raise RuntimeError(f'주문 활성화 미허용 읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """동일 명시 입력에서 실제 raw/효과/정상 반환만 읽으며 예상 비용을 계산하지 않는다."""
        self.case=c;self.events=[];raw=bytearray(50);raw[10]=168;raw[11]=c[0];raw[34]=c[1];self.mu.mem_write(POOL+50*50,bytes(raw));self.mu.mem_write(TYPES,bytes(0x20000))
        self.put(TYPES+168*500+0xec,c[4]);self.put(TYPES+169*500+0xec,0)
        for address,value in ((0x5c8464,POOL),(0x59ab20,TYPES),(0x540c70,c[2]),(0x59ab50+168*4,c[3]+168*23)):
            self.put(address,value)
        self.mu.mem_write(PACKET,struct.pack('<BBHH',1,0x5c,6,50));self.put(INPUT,9);self.put(0,0x87654321)
        InvokeOracle.invoke(self,THUNK,b'',precision,False);self.returns+=1;assert self.get(0)==0x87654321
        return ';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+50*50,50)).hex()


def main():
    """원본 기대를 최초 동결하거나 기존 출처/입력/두 정밀도를 읽기 전용 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');options=parser.parse_args()
    if options.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 원본 기대값을 재생성하지 않고 전체 바이트를 확인한다.
        for path,wanted in report['sha256'].items():
            assert digest(ROOT/path)==wanted,path
        rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int,row[0].split(','))) for row in rows]==cases() and len(rows)==report['uniqueInputs'] and len(rows)*2==report['normalReturns']
        assert all(row[1:3]==row[3:5] for row in rows)
        print('주문 활성화 원본 감사',len(rows),report['normalReturns']);return
    assert not FIXTURE.exists() and not REPORT.exists(),'동결 원문을 덮어쓰지 않는다'
    oracles=[Oracle(),Oracle()];rows=[]
    # 제품과 독립적으로 두 원본 x87 정밀도의 전체 결과를 보존한다.
    for c in cases():
        try:
            results=[oracle.sample(c,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
        except Exception as error:
            raise RuntimeError(str(c)) from error
        assert results[0]==results[1],c
        rows.append((','.join(map(str,c)),*results[0],*results[1]))
    FIXTURE.write_text('# input effects53 raw53 effects64 raw64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n','utf-8',newline='\n')
    paths=('originals/Netstorm.exe','tools/decomp_bomb_activation.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/bomb-activation-x86.tsv')
    record=dict(unit=229,host=socket.gethostname(),date='2026-10-19',lastPreciseDecompile=dict(host='VM-W11-Codex',date='2026-10-19'),uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),nativeCalls=dict(sum((oracle.calls for oracle in oracles),collections.Counter())),
        boundaries='41e860/TYPE/getter/ftol 전체. 경제41df80·복귀 진단·new40·Regular496f00 생성은 명시 효과 경계다. 원격 비용/현재TYPE 재조회/정상반환과GP/retN/SEH/스택canary/x87/실행·읽기·쓰기범위를 대조한다. 실제 Regular41e6a0의 내용물 소비/피해·게임 시전 성공과 별도다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','normalReturns','instructions','nativeCalls')}))


if __name__=='__main__':
    main()
