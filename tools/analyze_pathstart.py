#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미복원 Path 시작 상위 몸체의 실제 x86 ABI·가상 순서·명령 필드를 정적으로 보존한다. 게임/native 실행 수는0이다."""
import argparse
import csv
import json
import struct
from decomp_reward_oracle import ROOT, digest, pefile
import capstone

# 실제 상위 함수·원본 파일·검증할 vtable 및 명령 필드 쓰기 주소다.
SPECS={
 'originals':dict(path='originals/Netstorm.exe',entry=0x489080,vtables=(0x50e284,0x50e458),
   slots=(0x48bb10,0x48b680,0x48a200),stores={0x48929a:'[esi + 0x186d0]',0x4892a7:'[esi + 0x186e4]',
     0x4892ad:'[esi + 0x186c8]',0x4892b7:'[esi + 0x186cc]',0x4892bd:'[esi + 0x186d4]',0x4892c3:'[esi + 0x186e8]',
     0x4892cd:'[esi + 0x186dc]',0x4892da:'[esi + 0x186e0]'},
   calls={0x489285:'[edx + 0x20]',0x489385:'[ebx + 0x30]',0x489398:'[edx + 0x2c]',0x4893ad:'[edx + 0x44]',0x4893b4:'[edx + 0x40]'}),
 'originalCD':dict(path='originalCD/NETSTORM.EXE',entry=0x47d150,vtables=(0x503d60,0x503eb0),
   slots=(0x47feb0,0x47fe90,0x47e710),stores={0x47d308:'[ebp + 0x2f08]',0x47d30e:'[ebp + 0x2f0c]',
     0x47d314:'[ebp + 0x2f10]',0x47d322:'[ebp + 0x2f14]',0x47d328:'[ebp + 0x2f1c]',0x47d32e:'[ebp + 0x2f20]'},
   calls={0x47d2f9:'[eax + 0x20]',0x47d3b1:'[eax + 0x30]',0x47d3c5:'[ebx + 0x2c]',0x47d3d7:'[eax + 0x44]',0x47d3e2:'[eax + 0x40]'}),
}
REPORT=ROOT/'cpppj/recovery-pathstart-analysis.json' # 정적 분석만의 근거다. 완료 native/C++ 대조에 합산하지 않는다.
DISASSEMBLY=ROOT/'extracted/pathstart/abi-disassembly.tsv' # 원본 전체 범위의 기계어 주소·실제 연산이다.
SOURCES=['tools/analyze_pathstart.py','tools/ghidra/pathstart-functions.json','docs/exe/cpp-raw-path-start-pending.md'] # 새 정적 자료의 UTF-8 SHA 대상이다.


def analyze():
    """정밀 함수 범위로부터 실제 byte를 디스어셈블하고 검토한 ABI·가상 순서·저장 위치를 확인한다."""
    results={};rows=[];exports={};pe_shas={}
    # 판본을 섞지 않고 원본 메모리 이미지와 raw 가상 표를 각각 읽는다.
    for edition,spec in SPECS.items():
        pe=pefile.PE(str(ROOT/spec['path']));base=pe.OPTIONAL_HEADER.ImageBase;image=pe.get_memory_mapped_image();pe_shas[spec['path']]=digest(ROOT/spec['path'])
        with (ROOT/f'extracted/pathstart/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            entries={int(row['entry'],16):row['ranges'] for row in csv.DictReader(stream,delimiter='\t')}
        entry=spec['entry'];instructions={}
        # 내보낸 해당 함수의 실제 범위 전체를 읽는다. C 디컴파일의 미추정 인자를 실행 근거로 삼지 않는다.
        for part in entries[entry].split(';'):
            low,high=(int(value,16) for value in part.split('-'))
            for instruction in capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32).disasm(image[low-base:high-base+1],low):
                instructions[instruction.address]=instruction;rows.append([edition,f'{instruction.address:08x}',instruction.bytes.hex(),instruction.mnemonic,instruction.op_str])
        # 의미를 판독한 주소의 실제 연산이 바뀌면 보고서 생성 자체를 거부한다.
        for address,operand in spec['stores'].items():assert instructions[address].mnemonic=='mov' and instructions[address].op_str.startswith('dword ptr '+operand)
        for address,operand in spec['calls'].items():assert instructions[address].mnemonic=='call' and instructions[address].op_str=='dword ptr '+operand
        tables=[]
        # 같은 Init/Prepare와 파생 Attach를 원본 가상 표 byte에서 확인한다.
        for vtable in spec['vtables']:
            values=[struct.unpack('<I',image[vtable-base+offset:vtable-base+offset+4])[0] for offset in (0x20,0x2c,0x30)]
            assert values[0]==spec['slots'][0] if edition=='originals' else values[0] in (0x47feb0,0x480910)
            assert values[2]==spec['slots'][2]
            tables.append(dict(address=f'{vtable:08x}',init=f'{values[0]:08x}',attach=f'{values[1]:08x}',prepare=f'{values[2]:08x}'))
        results[edition]=dict(entry=f'{entry:08x}',vtables=tables,stores={f'{a:08x}':v for a,v in spec['stores'].items()},calls={f'{a:08x}':v for a,v in spec['calls'].items()},abi_argument_count=8)
        for filename in ('creation.c','functions.tsv'):
            path=f'extracted/pathstart/{edition}/{filename}';exports[path]=digest(ROOT/path)
    DISASSEMBLY.parent.mkdir(parents=True,exist_ok=True)
    with DISASSEMBLY.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['edition','address','bytes','mnemonic','operands']);writer.writerows(rows)
    report=dict(scope='실제 Path 시작 상위 몸체의 정적 ABI/가상 순서/명령 필드 판독; 전체 몸체 미복원',implemented=False,native_cases=0,cpp_comparisons=0,game_gui_runs=0,
      last_decompile_host='VM-W11-CODEX',last_decompile_date='2026-10-13',results=results,sources={p:digest(ROOT/p) for p in SOURCES},exports=exports,protected_pe=pe_shas,disassembly=digest(DISASSEMBLY))
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('Path 시작 상위2/2개 정적 ABI·가상 표·명령 쓰기 확인; native/C++ 실행 대조 추가0')


def verify():
    """정적 기록의 현재 원본/소스/범위 SHA를 확인하며 완성된 실행 대조라고 표시하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert not report['implemented'] and report['native_cases']==report['cpp_comparisons']==0
    for group in ('sources','exports','protected_pe'):
        for path,sha in report[group].items():assert digest(ROOT/path)==sha,path
    assert digest(DISASSEMBLY)==report['disassembly'];print('Path 시작 정적 기록 SHA 확인; 상위 실제 몸체는 미복원')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else analyze()
