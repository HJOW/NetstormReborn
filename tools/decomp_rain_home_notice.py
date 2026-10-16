#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""맨 오 워 사망 시 실제 풀 가상 통지의 상태/시각/정수 지연/갱신을 관찰한다."""
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

OBJECT,POOL=0x12000000,0x12100000  # 현재 프로세스48바이트와 홈 raw 입력의 독립 주소다.
ENTRIES={0x45b5b0,0x4558f0}  # 실제 홈 통지와 정수 게임 난수 몸체다.
FIXTURE=ROOT/'cpppj/tests/fixtures/rain-home-notice-x86.tsv'  # 입력과 전체 상태/요청 원문이다.
REPORT=ROOT/'cpppj/rain-home-notice.json'  # 현재 원본/정밀도/경계 근거다.


class Oracle(CallOracle):
    """실제 가상 통지를 실행하며 finder/Host lookup/갱신만 명시 경계로 둔다."""
    def __init__(self):
        """현재 정밀 범위와 격리 입력/스택을 준비한다."""
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image();assert struct.unpack_from('<I',image,0x5099e8-0x400000+0x34)[0]==0x45b5b0
        self.mu.mem_map(0x400000,(len(image)+4095)&~4095);self.mu.mem_write(0x400000,image)
        # 원본 주소나 서로의 버퍼와 겹치지 않는다.
        for address,size in ((OBJECT,0x1000),(POOL,0x10000),(STACK-0x7f000,0x80000),(STOP,0x1000)):self.mu.mem_map(address,size)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:rows={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[];self.events=[];self.returns=0
        # 함수의 실제 불연속 범위만 허용한다.
        for entry in ENTRIES:
            # 각 구간의 마지막 명령 바이트를 포함한다.
            for part in rows[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def instruction(self,mu,address,size,data):
        """현재 실제 lookup/갱신 인자를 기록하고 계산은 원문에 맡긴다."""
        if address in (0x4b1a90,0x4ae2d0,0x41c010):
            esp=mu.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',mu.mem_read(esp,4))[0];value=0
            if address==0x4b1a90:
                args=struct.unpack('<4I',mu.mem_read(esp+4,16));assert args==(50,60,0,0);self.events.append('F:50:60:0:0');value=80
            elif address==0x4ae2d0:
                assert mu.reg_read(UC_X86_REG_ECX)==POOL+80*50;self.events.append('L:80');value=OBJECT+32
            else:
                assert mu.reg_read(UC_X86_REG_ECX)==OBJECT+32;self.events.append('R:'+bytes(mu.mem_read(OBJECT,48)).hex())
            mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'홈 통지 범위 밖 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """실제 프로세스/게임 난수/스택만 쓸 수 있다."""
        if not any(low<=address and address+size<=high for low,high in ((OBJECT,OBJECT+48),(0x532710,0x532714),(STACK-0x7f000,STACK+0x1000))):raise RuntimeError(f'홈 통지 범위 밖 쓰기 {address:08x}')

    def sample(self,case,precision):
        """원문을 호출해 전체48바이트/난수/요청을 그대로 읽는다."""
        server,seed,now,dirty=case;mu=self.mu;raw=bytearray(b'\xa5'*48);struct.pack_into('<I',raw,0,0x509e20);raw[8]=dirty;struct.pack_into('<d',raw,16,-77.125);struct.pack_into('<I',raw,24,3);struct.pack_into('<3I',raw,32,0x509e00,80,50)
        mu.mem_write(OBJECT,bytes(raw));mu.mem_write(0x5c8464,struct.pack('<I',POOL));mu.mem_write(0x540bc0,struct.pack('<I',server));mu.mem_write(0x541160,struct.pack('<I',60));mu.mem_write(0x55b4d0,struct.pack('<d',now));mu.mem_write(0x532710,struct.pack('<I',seed));self.events=[]
        self.call(0x45b5b0,POOL+2500,bytes(8),8,precision);self.returns+=1
        return (bytes(raw).hex(),bytes(mu.mem_read(OBJECT,48)).hex(),str(struct.unpack('<I',mu.mem_read(0x532710,4))[0]),';'.join(self.events) or '-')


def main():
    """고유 입력의 원문을 저장하거나 기존 SHA/고유 입력을 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 원본 기대값을 다시 계산하지 않는다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({tuple(row[:4]) for row in rows})==len(rows);print('맨 오 워 홈 통지 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()];rows=[]
    # 서버 관문/원래 변경 비트/0시드와 분수 시각을 교차한다.
    for case in itertools.product((0,1),(0,1,0xdeadbeef),(0,2.025),(0,1,255)):
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1];server,seed,now,dirty=case;rows.append((str(server),str(seed),str(struct.unpack('<Q',struct.pack('<d',now))[0]),str(dirty),*outputs[0]))
    FIXTURE.write_text('# server seed nowBits dirty fieldsBefore48 fieldsAfter48 seedAfter events\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_rain_home_notice.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',uniqueInputs=len(rows),normalReturns=sum(oracle.returns for oracle in oracles),precisionDifferentInputs=[],sha256={path:digest(ROOT/path) for path in paths},limitations=['실제0045b5b0의 stage/time/dirty와 정수 난수/1000 지연을 실행한다.','finder/Host lookup/refresh는 명시 외부 경계다. 원본 NULL process 역참조는 정상 입력에 합산하지 않는다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('맨 오 워 홈 통지:',len(rows),'고유 입력, 두 정밀도 차이0')


if __name__=='__main__':main()
