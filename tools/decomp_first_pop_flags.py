#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""양판본 공통 firstPop 가상 함수의 모든 BYTE 입력과 단일 읽기를 원본 CPU에서 동결한다."""
import hashlib
import json
import socket
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent  # 보호PE/새fixture/이번 원시만 접근하는 저장소 루트다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS

RAW, STACK, STOP = 0x10000000,0x20000800,0x30000000  # 원문 this/복귀 스택/종료 위치이며 호스트 메모리와 무관하다.
FIXTURE = ROOT/'cpppj/tests/fixtures/first-pop-flags-x86.tsv'  # 기존 공간/Pop 원문 기대값을 덮어쓰지 않는 독립512입력이다.
WORK = ROOT/'extracted/cpp-resume-975'  # 현재 호스트 전용 실행/불변 증거를 보관한다.


class FirstPopOracle:
    """실제 두 PE 함수의 다음 함수 전까지0x30바이트만 실행하고 this의 단일 BYTE 읽기를 검사한다."""
    def __init__(self,edition):
        """판본별 원문 코드와 실제 extra 오프셋만 설치한다."""
        self.edition=edition  # 결과의 판본 식별자이며 게임 모드 값이 아니다.
        self.entry,self.offset=(0x4ad470,40) if edition=='originals' else (0x4ae0f0,35)  # 두판본의실제base VT+30이가리키는시작/필드다.
        binary=ROOT/edition/'Netstorm.exe';pe=pefile.PE(str(binary))
        self.sha256=hashlib.sha256(binary.read_bytes()).hexdigest()  # 관측한 원본 바이트의 해시다.
        self.code=bytes(pe.get_data(self.entry-pe.OPTIONAL_HEADER.ImageBase,0x30))  # 다음 함수4ad4a0/4ae120을 포함하지 않는다.
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)  # OS/게임 창 없이 실행되는 단일 정수 CPU다.
        for address in (self.entry&~4095,RAW,STACK&~4095,STOP):self.mu.mem_map(address,4096)  # 코드/입력/스택/복귀 영역만 분리한다.
        self.mu.mem_write(self.entry,self.code)
        self.reads=[]  # 현재 원문 호출이 this에서 읽은 실제 주소/폭이다.
        self.mu.hook_add(UC_HOOK_CODE,self.on_instruction);self.mu.hook_add(UC_HOOK_MEM_READ,self.on_read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.on_write)

    def on_instruction(self,_mu,address,_size,_data):
        """검토한 두 정수 함수의 몸체 밖 명령을 거부한다."""
        if not self.entry<=address<self.entry+len(self.code):raise RuntimeError(f'firstPop 허용 밖 명령 {address:08x}')

    def on_read(self,_mu,_access,address,size,_value,_data):
        """this의 모든 읽기를 기록하여 TYPE/상태/VT의 선행 조회가 없음을 확인한다."""
        if RAW<=address<RAW+4096:self.reads.append((address-RAW,size))

    def on_write(self,_mu,_access,address,size,_value,_data):
        """공통 firstPop은 코드/입력/스택 어디에도 쓰지 않는 원문 함수다."""
        raise RuntimeError(f'firstPop의 예상 밖 쓰기 {address:08x}+{size}')

    def sample(self,extra):
        """독립 입력 슬롯을 오염 패턴으로 채우고 지정 BYTE만 바꿔 실제 반환값을 얻는다."""
        raw=bytearray((i*37+extra)&255 for i in range(64));raw[self.offset]=extra;self.mu.mem_write(RAW,bytes(raw));self.mu.mem_write(STACK,struct.pack('<I',STOP))
        self.mu.reg_write(UC_X86_REG_ECX,RAW);self.mu.reg_write(UC_X86_REG_EAX,0xaabbccdd);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.reads=[]
        self.mu.emu_start(self.entry,STOP,count=64)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.reads==[(self.offset,1)] and bytes(self.mu.mem_read(RAW,64))==raw and bytes(self.mu.mem_read(self.entry,len(self.code)))==self.code
        return self.mu.reg_read(UC_X86_REG_EAX)


def main():
    """제품 계산을 사용하지 않고 두 원문의256BYTE 입력을 고정한다."""
    WORK.mkdir(parents=True,exist_ok=True);rows=['# edition\textra\tflags'];proof=[]
    for edition in ('originals','originalCD'):  # 두 PE/필드 오프셋을 독립적으로 읽는다.
        oracle=FirstPopOracle(edition)
        for extra in range(256):rows.append(f'{edition}\t{extra}\t{oracle.sample(extra)}')  # 모든 비트 조합의 원문 값을 그대로 기록한다.
        proof.append(dict(edition=edition,entry=f'{oracle.entry:08x}',offset=oracle.offset,codeHex=oracle.code.hex(),binarySha256=oracle.sha256,inputs=256,singleByteReads=True,unchanged=True))
    content=('\n'.join(rows)+'\n').encode('utf-8')
    if FIXTURE.exists():assert FIXTURE.read_bytes()==content,'기존 firstPop fixture와 원문 불일치'
    else:FIXTURE.write_bytes(content)
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-11-03',uniqueInputs=512,nativeObservations=512,proof=proof,fixtureSha256=hashlib.sha256(content).hexdigest(),boundary='실제4ad470/4ae0f0의0x30바이트만 실행. 매입력extra BYTE1개읽기/무쓰기/코드·입력불변/정상복귀 확인. PATCH4ad470은canonical 함수 누락이어서보호PE/VT+30 근거를사용.')
    (WORK/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False),flush=True)


if __name__=='__main__':main()
