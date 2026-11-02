#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE00435ba0의 정상 반환·창 비트 정리·설정 뒤 갱신을 격리 관측한다. 게임/OS 장치는 실행하지 않는다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import decomp_production_shortcuts as original
from unicorn.x86_const import *

ENTRY, END = 0x435ba0, 0x435c29  # 실제 부모 전체와 정상 RET 다음 주소다.
FLAGS, REFRESH = 0x5c78d4, 0x5c85d8  # 현재 화면 DWORD와 설정 뒤 증가하는 실제 DWORD다.
STOP = original.HEAP+0x3000  # 원문 RET의 명시 외부 복귀 주소다.


class TransitionOracle(original.Oracle):
    """원본 CPU 몸체를 유지하며 화면 장치와 설정 쓰기 CALL만 명시한다."""
    def __init__(self):
        """읽기 전용 PE/스택에 원본 SEH 체인의 명시 바깥 슬롯을 더한다."""
        super().__init__()
        self.mu.mem_map(0,0x1000)
        self.original[ENTRY]=bytes(self.mu.mem_read(ENTRY,END-ENTRY))

    def guard(self,mu,access,address,size,value,data):
        """CPU 쓰기는 원문 SEH 슬롯·부모 스택·갱신 DWORD만 허용한다."""
        if address==0 and size==4 or address==REFRESH and size==4:
            return
        if original.STACK-0x1000<=address and address+size<=original.STACK+16:
            return
        raise RuntimeError(f'미명시 원문 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """정상 RET까지 실행하고 하위 장치에 전달된 원시 인자만 기록한다."""
        if address==STOP:
            self.completed=True
            mu.emu_stop()
            return
        stack=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4a4fe0:
            self.event('M',(self.number(stack+4),))
            self.number(FLAGS,self.after)
            self.returned()
            return
        if address==0x441520:
            pointer=self.number(stack+4)
            assert bytes(mu.mem_read(pointer,18))==b'startInFullScreen\0'
            self.event('S',(self.number(stack+8),))
            if self.mutation:
                self.number(REFRESH,self.mutation)
            self.returned()
            return
        if not ENTRY<=address<END or address+size>END:
            raise RuntimeError(f'미명시 모드 전환 실행 {address:08x}')
        self.instructions+=1

    def run_transition(self,requested,current,after,refresh,mutation,control):
        """입력과 실제 원문 출력/스택·비휘발·SEH 복구를 독립 관측한다."""
        self.trace,self.instructions,self.completed=[],0,False
        self.after,self.mutation=after,mutation
        self.number(FLAGS,current)
        self.number(REFRESH,refresh)
        self.number(0,0x12345678)
        self.mu.mem_write(original.STACK-0x100,b'\xa5'*0x120)
        self.number(original.STACK,STOP)
        self.number(original.STACK+4,requested)
        saved={UC_X86_REG_EBX:0x87654321,UC_X86_REG_ESI:0x31415926,UC_X86_REG_EDI:0x27182818,UC_X86_REG_EBP:0x19283746}
        for register,value in saved.items():  # 실제 호출 규약의 비휘발 입력을 확인한다.
            self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_ESP,original.STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(ENTRY,STOP+1,count=300)
        assert self.completed and self.mu.reg_read(UC_X86_REG_ESP)==original.STACK+4
        assert self.number(0)==0x12345678 and self.number(original.STACK+4)==requested
        assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPSW)==0
        assert bytes(self.mu.mem_read(ENTRY,END-ENTRY))==self.original[ENTRY]
        return ';'.join(self.trace),self.number(FLAGS),self.number(REFRESH)


def main():
    """전체 DWORD·동일 모드·설정 콜백 변경·감김 입력을 두 정밀도로 고정한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    oracle=TransitionOracle()
    inputs=list(itertools.product((0,10,94,138,171,235,0xffffffff,0x80000000),(10,138,235),(138,235),(0,0xffffffff),(0,17)))
    rows=['# requested\tcurrent\tafter\trefresh\tmutation\ttrace\tflagsout\trefreshout']
    for values in inputs:  # 출력은 기대 계산 없이 원본 실행에서만 얻는다.
        first=oracle.run_transition(*values,0x27f)
        assert first==oracle.run_transition(*values,0x37f)
        rows.append('\t'.join(map(str,(*values,*first))))
    args.fixture.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(unit=746,host=socket.gethostname(),inputs=len(inputs),executions=len(inputs)*2,range='00435ba0..00435c28',boundaries=['004a4fe0 화면 장치의 요청/명시 현재 모드','00441520 설정 쓰기/명시 갱신값 변경'],exceptionBranch='435c29 정적 대조/C++ 명시 실패 검사만; 실제 SEH 미실행',gameProcesses=0,fixtureSha256=original.digest(args.fixture),toolSha256=original.digest(Path(__file__)),peSha256=original.digest(original.PE_PATH))
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
