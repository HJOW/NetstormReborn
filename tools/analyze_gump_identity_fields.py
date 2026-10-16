#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 원본의 UI 생성자 종류 BYTE→+20 DWORD 실제 두 대입 명령을 격리 실행한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 보호 PE와 검증 산출물의 저장소 기준이다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
import capstone
from capstone.x86 import X86_OP_MEM,X86_OP_REG
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
import unicorn.x86_const as registers

OBJECT=0x12000000  # 실제 PE 주소와 겹치지 않는 검사 객체다.
REPORT=ROOT/'cpppj/gump-identity-fields.json'  # 명령 주소/바이트/필드와 실제 고유 관찰이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/gump-identity-fields-x86.tsv'  # 생성 전체와 구별한 대입 명령 전용 원문이다.
SPECS={
    'originals':('originals/Netstorm.exe',[(0x4bb170,0x540448),(0x4bc060,0x540449),(0x4a64b0,0x540442),(0x4a63b0,0x54041c),(0x4a60e0,0x54044e)]),
    'originalCD':('originalCD/NETSTORM.EXE',[(0x4b7280,0x535f5c),(0x4b5c30,0x535f60),(0x417bd0,0x535f44),(0x417bd0,0x535eac),(0x4178f0,0x535f74)])
}  # 각 판본의 실제 single/scrolling/slider/arrow/thumb 진입점과 종류 BYTE다.
NAMES=('single','scrolling','slider','arrow','thumb')  # 위 원본 진입 순서의 명칭이다.
BYTES=(0,1,26,39,45,46,51,57,127,128,255)  # 부호 확장·실제 초기 종류를 구별하는 고유 입력이다.


def digest(path):
    """파일 전체 원문 SHA-256을 계산한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def observe():
    """생성자 전체 대신 실제 종류 로드/필드 저장 명령만 실행하고 그 한계를 기록한다."""
    rows=[];facts=[];decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);decoder.detail=True
    # 두 판본은 서로 독립된 PE/가상 주소 공간에서 관찰한다.
    for edition,(path,constructors) in SPECS.items():
        pe=pefile.PE(str(ROOT/path));base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image();mu=Uc(UC_ARCH_X86,UC_MODE_32);mu.mem_map(base,(len(image)+4095)&~4095);mu.mem_write(base,image);mu.mem_map(OBJECT,0x1000)
        # 현재 생성자 안의 실제 MOVSX와 그 결과를 저장하는 MOV를 확인한다.
        for name,(entry,source) in zip(NAMES,constructors):
            with (ROOT/f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as stream:ranges={int(row['entry'],16):row['ranges'] for row in csv.DictReader(stream,delimiter='\t')}
            instructions=[]
            # 실제 함수의 불연속 범위만 디코딩한다.
            for part in ranges[entry].split(';'):
                low,high=(int(value,16) for value in part.split('-'));instructions.extend(decoder.disasm(image[low-base:high+1-base],low))
            loads=[(index,ins) for index,ins in enumerate(instructions) if ins.mnemonic=='movsx' and ins.operands[1].type==X86_OP_MEM and ins.operands[1].mem.disp==source]
            assert len(loads)==(4 if edition=='originalCD' and name=='arrow' else 1),(edition,name,'종류 로드',loads)
            # CD의 화살표는 Slider 생성자 네 분기에 인라인되어 각각의 실제 저장을 관찰한다.
            for branch,(index,load) in enumerate(loads):
                destination=load.operands[0].reg;stores=[ins for ins in instructions[index+1:index+15] if ins.mnemonic=='mov' and ins.operands[0].type==X86_OP_MEM and ins.operands[1].type==X86_OP_REG and ins.operands[1].reg==destination and ins.operands[0].mem.disp==0x20]
                assert stores,(edition,name,'+20 저장',stores);store=stores[0];object_register=store.operands[0].mem.base;assert object_register and not store.operands[0].mem.index
                kind=name if len(loads)==1 else f'{name}-{branch}';initial=image[source-base];facts.append(dict(edition=edition,kind=kind,entry=f'{entry:08x}',source=f'{source:08x}',initialByte=initial,load=f'{load.address:08x}',loadBytes=bytes(load.bytes).hex(),store=f'{store.address:08x}',storeBytes=bytes(store.bytes).hex(),field=32))
                # 두 실제 명령의 효과만 실행한다. 사이의 생성/할당/가상 효과를 실행했다고 합산하지 않는다.
                for value in BYTES:
                    mu.mem_write(OBJECT,b'\xa5'*0x180);mu.mem_write(source,bytes([value]));mu.reg_write(getattr(registers,'UC_X86_REG_'+decoder.reg_name(object_register).upper()),OBJECT)
                    mu.emu_start(load.address,load.address+load.size,count=1);mu.emu_start(store.address,store.address+store.size,count=1)
                    rows.append((edition,kind,str(value),str(struct.unpack('<I',mu.mem_read(OBJECT+0x20,4))[0]),str(struct.unpack('<I',mu.mem_read(OBJECT+0x28,4))[0])))
    return rows,facts


def main():
    """관찰을 저장하거나 기존 원문/고유 입력과 근거 SHA만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 기대값을 C++ 결과에 맞춰 다시 생성하지 않는다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({tuple(row.split('\t')[:3]) for row in rows})==len(rows);print('UI 종류 대입 SHA/고유 입력 감사:',len(rows));return
    rows,instructions=observe();FIXTURE.write_text('# edition kind inputByte command20 alignFlags28\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=['tools/analyze_gump_identity_fields.py',FIXTURE.relative_to(ROOT).as_posix(),*[spec[0] for spec in SPECS.values()],*[f'extracted/refined/{edition}/functions.tsv' for edition in SPECS]]
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',uniqueInputs=len(rows),executedInstructions=len(rows)*2,instructions=instructions,sha256={path:digest(ROOT/path) for path in paths},limitations=['실제 MOVSX와 +20 MOV 두 명령만 실행한다. 생성자 전체·기본 생성/할당/가상 효과 관찰이 아니다.','+28 표식 보존은 두 대입 명령의 효과다. 기본 생성자는 별도 기존 몸체로 +28을 초기화한다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('UI 종류 대입:',len(rows),'고유 입력 / 실제',len(rows)*2,'명령')


if __name__=='__main__':main()
