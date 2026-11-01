#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""HTML Zsend 원문을 읽기 전용으로 추출한다. 기계어·제품·통신을 실행하지 않는다(709)."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import socket
import struct
import sys

ROOT=Path(__file__).resolve().parent.parent  #현재보호PE와같은PC의정밀소스를조회한다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))  #기존읽기/디스어셈블라이브러리만사용한다.
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

def digest(path):
    """실제파일바이트의SHA-256을읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    """검토범위의명령/정밀C와실제패킷초기값을남긴다.에뮬레이터는호출하지않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));base=pe.OPTIONAL_HEADER.ImageBase;memory=pe.get_memory_mapped_image();decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    for name,start,end in (('construct',0x40a150,0x40a184),('metadata',0x4c8acf,0x4c8b47),('send',0x4e2dc0,0x4e2e29)):  #선택한원문제어/복사/송신관문은실행하지않고명령으로만읽는다.
        lines=[f'{item.address:08x} {item.mnemonic} {item.op_str}' for item in decoder.disasm(memory[start-base:end-base],start)];(args.output/(name+'.asm')).write_text('\n'.join(lines)+'\n','utf-8',newline='\n')
    source=ROOT/'extracted/refined/originals/Netstorm.c';text=source.read_text('utf-8');functions={}
    with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream: table={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
    for address in (0x40a150,0x4e2dc0):  #원문헤더와현재전송장치선택의정밀몸체를보관한다.
        begin=text.index(f'// ==== FUN_{address:08x} @');end=text.find('// ==== ',begin+10);value=text[begin:end if end>=0 else len(text)];(args.output/f'{address:08x}.c').write_text(value,'utf-8',newline='\n');functions[f'{address:08x}']=dict(ranges=table[address]['ranges'],sha256=hashlib.sha256(value.encode()).hexdigest())
    read=lambda address,length: memory[address-base:address-base+length]  #보호PE의가상주소를읽기전용오프셋으로변환한다.
    packet=dict(constructor='0040a150',metadataRange='004c8acf..004c8b46',protocolAddress='0052ed59',protocol=read(0x52ed59,1)[0]&0x7f,messageAddress='0052ed58',message=read(0x52ed58,1)[0],lengthAddress='0052ed5c',length=struct.unpack('<H',read(0x52ed5c,2))[0],declaredTableLength=struct.unpack('<H',read(0x52ec40+0x87*2,2))[0],stringOffsets=[0x22,0x72,0xc2],stringBytes=80,untouchedPrefix=[4,34],target=11,observer='0040b380',version=1,mismatchPolicy=0,clientReceiver=None)
    report=dict(unit=709,date='2026-11-02',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),method='읽기 전용 PE 메타데이터/디스어셈블과 기존 정밀 C. 에뮬레이션/런타임 검사 아님',packet=packet,functions=functions,peSha256=digest(ROOT/'originals/Netstorm.exe'),decompileSha256=digest(source),toolSha256=digest(Path(__file__)),nativeRuntimeInputs=0,unitTests=0,gameExecutions=0,networkTests=0,limits=['정적 사전 구현이며 동작 검증 완료 아님','원문 미정 스택 prefix/문자 꼬리의 물리 동일성 보장 안 함','CD10.72 대응 생성자/메타 부모는 미확인','현재 제품에 root서버11 새 접속/원격 수신기를 추가하지 않음'])
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps(dict(unit=709,method=report['method'],packet=packet),ensure_ascii=False))

if __name__=='__main__': main()
