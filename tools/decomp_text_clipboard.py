#!/usr/bin/env python3
"""보호 PE의 실제 CF_TEXT 부모와 strncpy/rep 복사를 명시 Win32 장치에서 관찰한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
import sys
from pathlib import Path

# 현재 프로젝트의 기존 격리 Oracle만 재사용하며 원본 PE는 읽기 전용이다.
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle, SCRATCH
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 모두 가상 주소이며 실제 클립보드나 GlobalAlloc을 호출하지 않는다.
SOURCE,OUTPUT,MEMORY,DEVICE=SCRATCH+0x1000,SCRATCH+0x3000,SCRATCH+0x5000,SCRATCH+0x10000
HANDLE=0x12345678
FIXTURE=ROOT/'cpppj/tests/fixtures/text-clipboard-x86.tsv'
REPORT=ROOT/'cpppj/text-clipboard-native-20261026.json'
IMPORTS=((0x500290,'available',1),(0x500294,'open',1),(0x500298,'get',1),(0x500130,'lock',1),(0x50012c,'unlock',1),(0x5002b4,'empty',0),(0x500134,'allocate',2),(0x50028c,'set',2),(0x50037c,'close',0))


class ClipboardOracle(Oracle):
    """함수의 분기/복사/끝 NUL을 CPU가 수행하고 Win32 IAT만 명시 장치로 연결한다."""
    def __init__(self):
        """실제 두 부모와 CRT strncpy 범위 및 읽기 전용 원문 바이트를 보관한다."""
        super().__init__('originals',[dict(name='read',originals='004dd810'),dict(name='write',originals='004dd870')],[0x4e51f0])
        self.devices={}
        for index,(iat,name,count) in enumerate(IMPORTS):
            address=DEVICE+index*16;self.devices[address]=(name,count);self.mu.mem_write(iat,struct.pack('<I',address))
        self.protected={start:bytes(self.mu.mem_read(start,end-start)) for start,end in self.allowed}

    def on_instruction(self,mu,address,size,data):
        """OS 진입만 반환/인자 관찰로 대체하고 본체와 CRT는 실제 명령을 실행한다."""
        if address not in self.devices:return super().on_instruction(mu,address,size,data)
        name,count=self.devices[address];sp=mu.reg_read(UC_X86_REG_ESP);words=struct.unpack('<'+str(count+1)+'I',mu.mem_read(sp,4*(count+1)));args=words[1:]
        self.trace.append(name+(':'+','.join(map(str,args)) if args else ''))
        result=0
        if name in ('available','open','empty','unlock','close'):result=self.flags.get(name,1)
        elif name in ('get','allocate'):result=HANDLE if self.flags.get(name,1) else 0
        elif name=='lock':result=MEMORY if self.flags.get(name,1) else 0
        elif name=='set':result=HANDLE if self.flags.get(name,1) else 0
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4*(count+1));mu.reg_write(UC_X86_REG_EIP,words[0])

    def run(self,action,payload,capacity,flags):
        """명시 장치 입력으로 정상 RET 또는 원문의 NULL 쓰기 실패까지 관찰한다."""
        self.flags=flags;self.trace=[];self.mu.mem_write(SOURCE,payload+b'\0'+bytes([0xa5])*512);self.mu.mem_write(OUTPUT,bytes([0xa5])*272);self.mu.mem_write(MEMORY,bytes([0xa5])*512)
        if action=='read':self.mu.mem_write(MEMORY,payload+b'\0'+bytes([0xa5])*256)
        completed=1;fault=0
        try:self.call(action,[OUTPUT,capacity] if action=='read' else [SOURCE,len(payload)])
        except UcError:
            if action!='write' or not flags['open'] or not flags['allocate'] or flags['lock']:raise
            completed=0;fault=self.mu.reg_read(UC_X86_REG_EIP)
        for start,original in self.protected.items():
            if bytes(self.mu.mem_read(start,len(original)))!=original:raise RuntimeError('읽기 전용 clipboard/CRT 본체 변경')
        result=bytes(self.mu.mem_read(OUTPUT,capacity+8)) if action=='read' else bytes(self.mu.mem_read(MEMORY,len(payload)+9))
        return completed,hex(fault),result.hex(),';'.join(self.trace) or '-'


def cases():
    """출력 capacity와 NUL/고위BYTE·모든 OS 실패 반환을 독립 입력으로 교차한다."""
    payloads=(b'',b'Alpha',b'A\0B\r\n',bytes([0x80,0xff,13,10,9]),b'X'*260)
    for values,payload,capacity in itertools.product(itertools.product((0,1),repeat=4),payloads,(1,8,256)):
        yield 'read',payload,capacity,dict(zip(('available','open','get','lock'),values))
    for values,payload in itertools.product(itertools.product((0,1),repeat=7),payloads):
        yield 'write',payload,0,dict(zip(('open','empty','allocate','lock','unlock','set','close'),values))


def main():
    """최초 자료 생성과 원문 재실행 검증을 분리하고 기존 fixture는 덮지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if FIXTURE.exists() and not args.verify:raise RuntimeError('최초 clipboard fixture 덮어쓰기 금지')
    oracle=ClipboardOracle();rows=[];faults=0
    for action,payload,capacity,flags in cases():
        result=oracle.run(action,payload,capacity,flags);faults+=not result[0]
        rows.append('\t'.join(map(str,(action,payload.hex() or '-',capacity,','.join(k+'='+str(v) for k,v in flags.items()),*result))))
    encoded=('# action/inputhex/capacity/flags/completed/faultPC/outputhex/trace\n'+'\n'.join(rows)+'\n').encode('utf8')
    if args.verify:
        if FIXTURE.read_bytes()!=encoded:raise RuntimeError('clipboard 최초 fixture와 재관찰 불일치')
    else:FIXTURE.parent.mkdir(parents=True,exist_ok=True);FIXTURE.write_bytes(encoded)
    report=dict(host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',observations=len(rows),nullWriteFaults=faults,gameRuns=0,osCalls=0,binarySha256=oracle.sha256,fixtureSha256=hashlib.sha256(encoded).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),nativeCode={hex(k):v.hex() for k,v in oracle.protected.items()},limits=['실제 read/write 부모와 CRT strncpy/rep 복사 실행; Win32 IAT만 명시 장치','write GlobalLock0은 원문 NULL 접근 실패이며 정상 반환/Close 아님','실제 Windows clipboard/HWND/권한 성공을 실행한 자료 아님','원문 부모의 독립 관측이며 실제 OS/제품 GUI 검증은 통합 보고서에서 구별한다'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8');print(json.dumps({k:report[k] for k in ('observations','nullWriteFaults','fixtureSha256')},ensure_ascii=False))


if __name__=='__main__':main()
