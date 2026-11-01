#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Shift 선택/일반 그룹 이동 입구와 WorldDestSelected 이름을 읽기 전용으로 기록한다(701)."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

def main():
    """원본 명령/호출 인자/두 수정키 비트를 정적으로 읽으며 몸체나 게임을 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();assert not args.output.exists()
    oracle=Oracle('originals',[dict(name='toggle',originals='004da6b0')],helpers=[])
    code=bytes(oracle.mu.mem_read(0x4da35c,0x11b))
    instructions=[dict(address=f'{item.address:08x}',instruction=item.mnemonic+' '+item.op_str) for item in Cs(CS_ARCH_X86,CS_MODE_32).disasm(code,0x4da35c)]  # 부분 입구의 기계어를 그대로 보존하며 Python 분기 기대값을 만들지 않는다.
    by_address={int(row['address'],16):row['instruction'] for row in instructions}
    assert by_address[0x4da35c]=='test ecx, 0x10000000' and by_address[0x4da38f]=='test eax, 0x20000000'
    assert by_address[0x4da3f5]=='call 0x4da6b0' and by_address[0x4da459]=='call 0x4d5500' and by_address[0x4da46f]=='call 0x42a360'
    pointer,glob=struct.unpack('<II',oracle.mu.mem_read(0x542da8,8));assert glob==0x542808 and oracle.read_string(pointer)==b'WorldDestSelected'
    command,=struct.unpack('<I',oracle.mu.mem_read(glob,4));assert command==0x02000044
    record=dict(unit=701,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),kind='static native instruction evidence',gameExecutions=0,nativeBodyExecutions=0,branchStart='004da35c',branchBytes=len(code),branchSha256=hashlib.sha256(code).hexdigest(),nameEntry='00542da8',globalAddress='00542808',name='WorldDestSelected',command=command,peSha256=oracle.sha256,instructions=instructions,toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.output.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(dict(instructions=len(instructions),command=command,nativeExecutions=0)))

if __name__=='__main__':
    main()
