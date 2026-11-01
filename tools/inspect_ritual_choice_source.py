#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""707의 의식 선택 원문을 읽기 전용으로 추출한다. 기계어/제품/네트워크를 실행하지 않는다."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parent.parent  #보호 원문과 같은 PC의 정밀 자료를 조회하는 저장소다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))  #기존 읽기/디스어셈블 라이브러리를 재사용하며 에뮬레이터는 호출하지 않는다.
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

def digest(path):
    """실제 파일 바이트의 SHA-256만 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    """검토 범위의 바이트/정밀C를 추출하고 실제 헤더·명령 초기값을 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));base=pe.OPTIONAL_HEADER.ImageBase;memory=pe.get_memory_mapped_image();decoder=Cs(CS_ARCH_X86,CS_MODE_32);ranges=[('construct',0x40af50,0x40af84),('input',0x4d82ed,0x4d83fc),('receive',0x448e90,0x448f45),('menu',0x4cfc10,0x4cfc79)]  #정적으로 검토한 몸체만 추출한다.
    # 각 원문 범위는 실행하지 않고 명령과 주소만 문자열로 남긴다.
    for name,start,end in ranges:
        lines=[f'{item.address:08x} {item.mnemonic} {item.op_str}' for item in decoder.disasm(memory[start-base:end-base],start)]  #OS/게임/프로세스/통신을호출하지않는다.
        (args.output/(name+'.asm')).write_text('\n'.join(lines)+'\n','utf-8',newline='\n')
    source=ROOT/'extracted/refined/originals/Netstorm.c';text=source.read_text('utf-8');functions={}
    with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream: table={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
    for address in (0x40af50,0x448e90,0x4cfc10,0x4e2dc0):  #원문 부모·수신·메뉴·송신장치의같은정밀C를보관한다.
        begin=text.index(f'// ==== FUN_{address:08x} @');end=text.find('// ==== ',begin+10);value=text[begin:end if end>=0 else len(text)];(args.output/f'{address:08x}.c').write_text(value,'utf-8',newline='\n');functions[f'{address:08x}']=dict(ranges=table[address]['ranges'],sha256=hashlib.sha256(value.encode()).hexdigest())
    read=lambda address,length: memory[address-base:address-base+length]  #PE의가상주소를읽기전용오프셋으로바꾼다.
    report=dict(unit=707,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),method='Read-only PE metadata/disassembly and existing refined C; no emulation/runtime test',peSha256=digest(ROOT/'originals/Netstorm.exe'),decompileSha256=digest(source),toolSha256=digest(Path(__file__)),packet=dict(constructor='0040af50',protocolAddress='0052ee65',protocol=read(0x52ee65,1)[0]&0x7f,messageAddress='0052ee64',message=read(0x52ee64,1)[0],lengthAddress='0052ee68',length=struct.unpack('<H',read(0x52ee68,2))[0],daisOffset=4,daisBytes=2,kindOffset=6,kindBytes=4,target=9,receiver='00448e90',observer='0040b380',version=1,mismatchPolicy=0),commands={name:dict(address=f'{address:08x}',value=struct.unpack('<I',read(address,4))[0]) for name,address in (('UpgradeAltar',0x542a44),('GetNewTech',0x542a48),('RequestStormpower',0x542ad8))},functions=functions,nativeRuntimeInputs=0,unitTests=0,gameExecutions=0,networkTests=0,limits=['Static preimplementation only, not verified behavior','Original CD10.72 referenced counterpart only; no new CD implementation','Remaining early-sacrifice/late rank/death MP UI and TCP/IP remain'])
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(dict(unit=707,method=report['method'],packet=report['packet']),ensure_ascii=False))

if __name__=='__main__': main()
