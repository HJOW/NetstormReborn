#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Gump 부착의 signed GT/상한 진단 접두와 실제 상세 writer를 원문에서 관측한다."""
import argparse
import itertools
import json
from pathlib import Path
import socket
import struct
import capstone
import pefile
from decomp_priestmenu import Oracle as MemoryOracle,ROOT,TEXT,STACK,STOP
from decomp_file_name_leaf import add_ranges
from decomp_basefile_whole_read import BoundaryFailure
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_FPCW

OBJECT,COPY=TEXT+0x1000,TEXT+0x8000  # 진단 전 객체와 strdup의 명시 소유 복사 저장소다.
CHECKING,LIMIT,DETAIL=0x5e4794,0x54045e,0x5e4798  # 각각 현재DWORD/signed BYTE/nullable 상세 포인터다.

class GumpAttachCheckingOracle(MemoryOracle):
    """부착 첫 쓰기 전까지와436d20 전체만 실행하며 포맷/할당은 명시 장치다."""
    def __init__(self):
        """실제 writer의 strdup 호출 주소를 PE에서 확인해 좁은 경계를 연결한다."""
        super().__init__();add_ranges(self,(0x466750,0x436d20));image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image();instructions=list(capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32).disasm(image[0x36d20:0x36d40],0x436d20));calls=[int(item.op_str,16) for item in instructions if item.mnemonic=='call'];assert len(calls)==1;self.duplicate=calls[0]

    def instruction(self,mu,address,size,data):
        """포맷 인자/실제 상세 복사와 로그 뒤 현재 필드 변경을 구별한다."""
        if address==0x46678f:self.events.append('B');raise BoundaryFailure('body')
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,3)
        if address==0x4a89c0:
            assert self.string(args[1])==b'gt = %d' and args[2]==(self.case[0]&0xffffffff);signed=struct.unpack('<i',struct.pack('<I',args[2]))[0];mu.mem_write(args[0],('gt = '+str(signed)).encode()+b'\0');self.ret(args[0]);return
        if address==0x436d20:
            self.events.append('L:'+self.string(args[0]).hex())
            if self.case[3]:self.put(CHECKING,0);self.put(OBJECT+32,2);self.put(OBJECT+4,0x200009)
        if address==self.duplicate:
            mu.mem_write(COPY,self.string(args[0])+b'\0');self.ret(COPY);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError('미확인 Gump 진단 명령 '+hex(address))

    def observe(self,case,precision):
        """GT는signedDWORD, 상한은signedBYTE로 주고 첫 부착 쓰기 전 전체 객체/상세를 읽는다."""
        self.case=case;self.events=[];raw=bytearray((index*17+3)&255 for index in range(160));struct.pack_into('<I',raw,32,case[0]&0xffffffff);self.mu.mem_write(OBJECT,bytes(raw));self.put(CHECKING,case[1]);self.put(LIMIT,case[2],1);self.put(DETAIL,0);self.mu.mem_write(STACK,struct.pack('<5I',STOP,0,0,0,1));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,OBJECT);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='return'
        try:self.mu.emu_start(0x466750,STOP,count=10000)
        except BoundaryFailure as boundary:outcome=str(boundary)
        assert outcome=='body' and self.mu.reg_read(UC_X86_REG_FPCW)==precision;detail=self.words(DETAIL,1)[0];assert detail in (0,COPY)
        return (';'.join(self.events),bytes(self.mu.mem_read(OBJECT,160)).hex(),str(self.words(CHECKING,1)[0]),self.string(detail).hex() if detail else '-',outcome)

def main():
    """독립 두 정밀도의 실제 원문 결과를 비교하고 첫 출력 바이트를 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();oracles=[GumpAttachCheckingOracle(),GumpAttachCheckingOracle()];rows=[]
    for case in itertools.product((-2147483648,-1,0,1,2,66,67,68,2147483647),(0,1,0x80000000,0xffffffff),(0,1,67,127,128,255),(0,1)):  # BYTE 상한의0/양수/음수와 signed GT 경계를 교차한다.
        outputs=[oracle.observe(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1],case;rows.append((','.join(map(str,case)),*outputs[0]))
    assert len(rows)==432;args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text('# input4 events raw160 checking detailHex outcome\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');report=dict(host=socket.gethostname(),entries=['00466750 prefix','00436d20'],checkpoint='0046678f first flags write',rows=len(rows),observations=len(rows)*2,details=sum(row[-2]!='-' for row in rows),gameExecutions=0,boundaries=['부착은 첫 쓰기 전 접두·기존 전체 부착은 이전 fixture로 유지','436d20은 전체 실제 실행·4a89c0 포맷과 strdup 확보/복사는 명시 장치','현재 GT signedDWORD/상한 signedBYTE·원문 PE 초기상한67','실제CRT/할당실패/진단UI/전체SEH/게임 관측 아님']);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
