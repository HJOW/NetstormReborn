#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""StyleText의 두 원본 PE·내보낸 범위·가상 표·파서 case·실제 명령을 정적 분석 자료로 보관한다.

사용법: python -X utf8 tools/analyze_styletext.py
게임/에뮬레이터/테스트는 실행하지 않는다. Ghidra styletext 내보내기가 먼저 필요하다.
"""
import csv
import datetime
import hashlib
import json
import re
import socket
from pathlib import Path

import capstone
import pefile

# 경로는 저장소 안에서만 사용하며 원본 파일은 읽기만 한다.
ROOT=Path(__file__).resolve().parent.parent
PLAN=ROOT/'tools/ghidra/styletext-functions.json'
REPORT=ROOT/'cpppj/recovery-styletext-evidence.json'
# 실제 문자열 조각의 가상 표와 짧은 명령 파서다. 표의 다섯 DWORD는 삭제/메트릭/측정/출력/복제다.
EDITIONS={
    'originals':('originals/Netstorm.exe',0x5143cc,0x4bb430),
    'originalCD':('originalCD/NETSTORM.EXE',0x505d48,0x4b7770),
}


def digest(path):
    """분석 근거 파일의 SHA-256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_ranges(path):
    """Ghidra가 내보낸 각 함수의 시작 주소와 분리된 실제 몸체 범위를 읽는다."""
    result=[]
    with path.open(encoding='utf-8',newline='') as stream:
        # 함수 이름/범위를 재추정하지 않고 원본 TSV의 행을 그대로 사용한다.
        for row in csv.DictReader(stream,delimiter='\t'):
            ranges=[]
            # 끝 주소를 포함하는 Ghidra 범위를 Python의 끝 제외 범위로 바꾼다.
            for part in row['ranges'].split(';'):
                low,high=part.split('-')
                ranges.append((int(low,16),int(high,16)+1))
            result.append((row['entry'],ranges))
    return result


def parser_cases(source,address):
    """원본 C의 파서 case 문자 목록을 기록한다. 대괄호 내부를 구현했다고 판단하지 않는다."""
    part=next(p for p in re.split(r'(?=// ==== )',source) if f'@ {address:08x}' in p[:120])
    codes=sorted({int(value,0) for value in re.findall(r'case (0x[0-9a-f]+|[0-9]+):',part)})
    return [{'byte':value,'character':chr(value)} for value in codes]


def main():
    """두 PE의 실제 가상 표/명령을 읽어 정적 복원 근거와 코드 해시를 저장한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),PLAN]
    facts={}
    # 각 판본의 다른 바이너리와 내보내기를 독립적으로 읽는다.
    for edition,(binary,vtable,parser) in EDITIONS.items():
        path=ROOT/binary
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/styletext'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        assembly=[]
        # 내보낸 함수마다 실제 PE 바이트의 명령 주소를 보관한다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # Ghidra가 분리한 범위 사이의 데이터/다른 함수를 이어 붙이지 않는다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 실제 바이트/주소/명령을 기록하며 실행하거나 기대 출력을 만들지 않는다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        slots=[]
        # 문자열 조각의 실제 가상 함수 주소 다섯 개를 읽는다.
        for i in range(5):
            offset=vtable-base+i*4
            slots.append(f'{int.from_bytes(image[offset:offset+4],"little"):08x}')
        facts[edition]={
            'binary':binary,'exported_functions':[entry for entry,_ in functions],
            'requested_functions':plan['editions'][edition],
            'text_vtable':f'{vtable:08x}','text_virtual_slots':slots,
            'parser':f'{parser:08x}','parser_cases':parser_cases(exported.read_text(encoding='utf-8'),parser),
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 구현의 현재 상태도 근거에 묶되 동작 대조 완료를 뜻하지 않는다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/StyleText.h','cpppj/src/client/StyleText.cpp',
        'cpppj/src/client/GumpText.h','cpppj/src/client/GumpText.cpp',
        'cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE의 정적 디컴파일·가상 표·기계어 판독; 원본 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본 동작 대조·GUI 검사는 후속',
        'boundaries':['대괄호 옵션/그림 파싱','그림 등 파생 조각의 메트릭/측정/출력/삭제','클라이언트 자동 UI 연결'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('StyleText 정적 분석 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
