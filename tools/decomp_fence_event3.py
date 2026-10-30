#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 펜스 사건3/실제 Regular 호출자와 생성자를 별도32비트 CPU 도우미로 관측한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 원본 파일은 읽기만 하고 관측은 별도 경로에 쓴다.
FIXTURE=ROOT/'cpppj/tests/fixtures/fence-event3-x86.tsv'  # 기존 관측을 변경하지 않는 최초 CPU 결과다.
DEFAULT_HELPER=ROOT/'extracted/cpp-fence-event3-613-20261030/native_fence_event3.exe'  # 게임이 아닌 전용 콘솔 검사 도우미다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM

ORIGINAL_BASE,MAPPED_BASE=0x400000,0x30000000  # 선택된 함수의 명령/제어를 유지하는 논리주소와 검사 주소다.
ENTRIES=(0x496cb0,0x457a50,0x496e80,0x41bfb0)  # 실제 호출자/Handle/생성자/부모 getter만 실행한다.


def digest(path):
    """전체 바이트를 실제 관측/빌드 산출물의 근거로 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def double_bits(value):
    """독립 double 입력만 비트로 옮기며 반환값을 계산하지 않는다."""
    return struct.unpack('<Q',struct.pack('<d',value))[0]


def cases():
    """네 방향 스냅샷/할당/변이/캐시 보존과 bit20의 별도 위임을 구분한다."""
    result=[]
    for mask,allocation,mutation,cache in itertools.product(range(16),range(2),(0,2,3,4),(0,0x1000001)):  # 실제 사건3은 두 캐시를 쓰지 않는다.
        values=(3,0,7,cache,0x12345678,2,3,mask,allocation,mutation,0x3e000000,0)
        result.append((values,double_bits(16777217.5 if mask&1 else 10.25)))
    for payload,count,extra,mutation in itertools.product((0,0x80000000,0x3e000000,0xbf800000,0x7fc12345),(0,7,0xffffffff),(0,32),(0,8)):  # 실제 호출자가 값/NaN을 소비한 뒤의 상태도 관측한다.
        if not extra and mutation: continue
        result.append(((3,extra,7,0x12345678,0x87654321,2,3,5,1,mutation,payload,count),double_bits(10.25)))
    return list(dict.fromkeys(result))


def prepare_image(output):
    """선택 함수의 절대주소 피연산자와 두 함수 포인터 표만 옮기고 그 내역을 보존한다."""
    source=ROOT/'originals/Netstorm.exe';pe=pefile.PE(str(source));image=bytearray(source.read_bytes());delta=MAPPED_BASE-ORIGINAL_BASE;changes=[]
    with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
        facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
    disassembler=Cs(CS_ARCH_X86,CS_MODE_32);disassembler.detail=True
    for entry in ENTRIES:  # 현재 PC 정밀 범위의 모든 명령을 빠짐없이 방문한다.
        for part in facts[entry]['ranges'].split(';'):
            low,high=(int(value,16) for value in part.split('-'));raw=pe.get_data(low-ORIGINAL_BASE,high-low+1);consumed=0
            for instruction in disassembler.disasm(raw,low):  # 상대분기는 옮기지 않고 동일 이미지 내 상대거리를 유지한다.
                consumed+=instruction.size;offsets=set()
                for operand in instruction.operands:  # 절대주소가 저장된32비트 피연산자만 기계적으로 변경한다.
                    if operand.type==X86_OP_MEM and not operand.mem.base and ORIGINAL_BASE<=operand.mem.disp<ORIGINAL_BASE+pe.OPTIONAL_HEADER.SizeOfImage:
                        assert instruction.disp_size==4;offsets.add(instruction.disp_offset)
                    if operand.type==X86_OP_IMM and not instruction.group(CS_GRP_CALL) and not instruction.group(CS_GRP_JUMP) and ORIGINAL_BASE<=operand.imm<ORIGINAL_BASE+pe.OPTIONAL_HEADER.SizeOfImage:
                        assert instruction.imm_size==4;offsets.add(instruction.imm_offset)
                for offset in sorted(offsets):  # opcode/분기/수치상수는 보존하고 주소DWORD만 변경한다.
                    at=pe.get_offset_from_rva(instruction.address-ORIGINAL_BASE)+offset;value=struct.unpack_from('<I',image,at)[0];struct.pack_into('<I',image,at,value+delta)
                    changes.append({'instruction':hex(instruction.address),'mnemonic':instruction.mnemonic,'offset':offset,'original':hex(value),'mapped':hex(value+delta)})
            assert consumed==len(raw),(hex(entry),hex(low),consumed,len(raw))
    for address in (*range(0x457e08,0x457e18,4),0x5094cc):  # Handle switch4개와 부모 vtable+5c만 실제 선택한다.
        at=pe.get_offset_from_rva(address-ORIGINAL_BASE);value=struct.unpack_from('<I',image,at)[0];assert ORIGINAL_BASE<=value<ORIGINAL_BASE+pe.OPTIONAL_HEADER.SizeOfImage;struct.pack_into('<I',image,at,value+delta)
        changes.append({'pointer':hex(address),'original':hex(value),'mapped':hex(value+delta)})
    destination=output/'selected-image.bin';destination.write_bytes(image)
    (output/'address-relocations.json').write_text(json.dumps(changes,indent=2)+'\n',encoding='utf-8',newline='\n')
    return destination


def observe(helper,image,values,control,output,label):
    """게임/창을 시작하지 않고 선택된 원문 함수만 실행하는 콘솔 도우미를 숨겨 호출한다."""
    source=output/(label+'-inputs.tsv');destination=output/(label+'-native.tsv')
    source.write_text(''.join(','.join(map(str,value))+'\t'+str(now)+'\t'+str(control)+'\n' for value,now in values),encoding='utf-8',newline='\n')
    command=[str(helper),str(image),str(source),str(destination)]
    start=time.monotonic();result=subprocess.run(command,cwd=ROOT,capture_output=True,timeout=60,creationflags=subprocess.CREATE_NO_WINDOW)
    (output/(label+'-stdout.log')).write_bytes(result.stdout);(output/(label+'-stderr.log')).write_bytes(result.stderr)
    (output/(label+'-process.json')).write_text(json.dumps({'command':command,'returnCode':result.returncode,'seconds':time.monotonic()-start,'helperSha256':digest(helper)},ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    if result.returncode: raise RuntimeError('실제 CPU 도우미 실패: '+str(result.returncode)+' '+result.stderr.decode('utf-8',errors='replace'))
    rows=[line.split('\t') for line in destination.read_text('utf-8').splitlines()]
    assert len(rows)==len(values) and all(len(row)==9 for row in rows)
    for row in rows:  # 실행 주소와 독립적인 C++ raw 가상표 기록값으로 첫 DWORD5개만 되돌린다.
        raw=bytearray.fromhex(row[2])
        for at in range(0,250,50):
            value=struct.unpack_from('<I',raw,at)[0];assert value==0x509470+MAPPED_BASE-ORIGINAL_BASE;struct.pack_into('<I',raw,at,0x509470)
        row[2]=raw.hex()
    (output/(label+'.json')).write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8',newline='\n')
    return rows


def main():
    """원본 CPU의 두 정밀도 결과를 먼저 보존하고 명시 요청에서만 새 fixture를 만든다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True);parser.add_argument('--helper',type=Path,default=DEFAULT_HELPER);parser.add_argument('--fixture',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    output=ROOT/args.output;output.mkdir(parents=True,exist_ok=False);helper=args.helper.resolve();binary=helper.read_bytes();pe=struct.unpack_from('<I',binary,0x3c)[0]
    assert binary[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',binary,pe+4)[0]==0x14c and struct.unpack_from('<H',binary,pe+24)[0]==0x10b
    assert struct.unpack_from('<I',binary,pe+24+28)[0]==0x10000000
    image=prepare_image(output);values=cases();first=observe(helper,image,values,0x27f,output,'first');replay=observe(helper,image,values,0x37f,output,'replay');assert first==replay
    payload='# 입력12DWORD/현재시각/원문250BYTE/FST결과/효과/현재Regular/새Regular/FST직전태그/동일객체재호출\n'+''.join('\t'.join(row)+'\n' for row in first)
    if args.verify: assert FIXTURE.read_bytes()==payload.encode('utf-8')
    elif args.fixture:
        with FIXTURE.open('xb') as stream: stream.write(payload.encode('utf-8'))
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','extracted/refined/originals/Netstorm.c','tools/decomp_fence_event3.py','tools/native_fence_event3.cpp']
    if args.fixture or args.verify: paths.append(FIXTURE.relative_to(ROOT).as_posix())
    report={'unit':613,'host':socket.gethostname(),'lastFullDecompilation':'HJOW-Athlon / 2026-10-23','oracle':'실제 Windows x86 CPU / 주소참조38DWORD를0x30000000으로 옮긴 private PE / 원래 CALL+FST7BYTE 관찰 trampoline','cases':len(values),'observations':2*len(values),'differences':[],'fixtureWritten':args.fixture,'verificationOnly':args.verify,'helperSha256':digest(helper),'sourceSha256':{name:digest(ROOT/name) for name in paths},'evidenceSha256':{path.name:digest(path) for path in output.iterdir() if path.is_file()},'addressNormalization':'명령의 절대주소33개/함수표5개만 재배치하고 opcode·상대분기·수치상수를 보존한다. 출력은 raw 가상표 첫DWORD5개만 원본 논리주소로 정규화한다.', 'scope':'실제496cb0→457a50→496e80과 빈 x87 결과의 FST/현재 Regular 예약을 실행한다. 이웃/표식/선·할당·공통 부착/삭제·Damageable은 명시 경계다. 정상 호출의 빈 x87 입력을 쓰며 원본 파일 수정/게임GUI/특정적공격/포획/희생/승리/신규MP는 없다. 최초 Unicorn의 빈 FST를0으로 처리한 결과는 기준값에서 제외하고 원시 실패를 별도 보존했다.'}
    destination=ROOT/'cpppj/fence-event3-native-20261030.json' if args.fixture or args.verify else output/'report.json';destination.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'cases':len(values),'observations':2*len(values),'differences':[]}))


if __name__=='__main__':
    main()
