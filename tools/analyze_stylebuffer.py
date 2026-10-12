#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다중 줄 버퍼의 두 원본 PE·정밀 내보내기·가상 분류/복제 슬롯을 정적 자료로 보관한다.

사용법: python -X utf8 tools/analyze_stylebuffer.py
Ghidra stylebuffer 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트는 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 분석 근거는 저장소 안에 생성한다. 보호 원본은 읽기만 한다.
PLAN=ROOT/'tools/ghidra/stylebuffer-functions.json'
REPORT=ROOT/'cpppj/recovery-stylebuffer-evidence.json'
# 문자열/그림/기본 조각의 실제 가상 표다. 여섯째 슬롯이 문자열 여부를 반환한다.
EDITIONS={
    'originals':('originals/Netstorm.exe',{'text':0x5143cc,'picture':0x50c5d8,'base':0x50c5c0}),
    'originalCD':('originalCD/NETSTORM.EXE',{'text':0x505d48,'picture':0x501f88,'base':0x501f68}),
}


def virtuals(image,base,address,decoder):
    """실제 가상 주소 여섯 개와 문자열 분류기의 명령을 판독한다. 반환값을 실행하지 않는다."""
    slots=struct.unpack_from('<6I',image,address-base)
    classification=[]
    # 짧은 실제 분류기를 ret까지만 기록한다. 뒤 함수/정렬 바이트는 붙이지 않는다.
    for instruction in decoder.disasm(image[slots[5]-base:slots[5]-base+16],slots[5]):
        classification.append({'at':f'{instruction.address:08x}','bytes':instruction.bytes.hex(),
            'instruction':f'{instruction.mnemonic} {instruction.op_str}'.strip()})
        if instruction.mnemonic.startswith('ret'): break
    return {'address':f'{address:08x}','slots':[f'{slot:08x}' for slot in slots],'classification':classification}


def main():
    """버퍼 생성/수명/개행/표시 분할의 실제 주소·명령·가상 분류와 현재 구현 해시를 저장한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 서로 다른 두 원본 PE와 각 정밀 프로젝트 내보내기를 읽는다.
    for edition,(binary,tables) in EDITIONS.items():
        path=ROOT/binary
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/stylebuffer'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        assembly=[]
        # 각 함수의 실제 몸체 명령을 보관하며 원본/호스트 실행 결과를 만들지 않는다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # Ghidra의 불연속 범위 사이에 있는 다른 코드/데이터는 제외한다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 원본 바이트·주소·피연산자를 정적 판독만 한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        facts[edition]={
            'binary':binary,'requested_functions':plan['editions'][edition],
            'exported_functions':[entry for entry,_ in functions],
            'virtual_tables':{name:virtuals(image,base,address,decoder) for name,address in tables.items()},
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 같은 조각/폰트/그림 측정·복제·삭제에 연결한 현재 코드와 빌드 목록을 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/StyleTextBuffer.cpp','cpppj/src/client/StyleTextBuffer.h',
        'cpppj/src/client/StyleText.cpp','cpppj/src/client/StyleText.h',
        'cpppj/src/client/Markup.cpp','cpppj/src/client/Markup.h',
        'cpppj/src/client/GumpText.cpp','cpppj/src/client/GumpText.h','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 정적 디컴파일·가상 표/명령 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본 줄/조각/픽셀 대조·GUI 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'hard_lines':'LF에서만 완료 줄 수 증가; 마지막 미완료 칸은 표시 대상에서 제외',
            'soft_lines':'0 센티널·안정된 별도 토큰·같은 공통 표의 복제 소유 조각',
            'comparison':'전체 폭이 limit 미만일 때 유지, 동률은 분할; noWrap 비0은 분할 생략',
            'split':'space만 경계; 공백/NUL 포함 복사·첫 단어 강제·빈 표시 줄·원본 태그 유지',
            'picture':'이동 전 원래 줄의 높이 갱신; 가상 복제의 배율/색 초기화 유지',
            'lifetime':'입력/표시 소유권 분리; 원본 소멸의 미완료 끝 칸 삭제 누락 유지',
        },
        'boundaries':['TextGump/Gump 생성·재부착·표시/입력 분배·스크롤·W 명령/전체 스타일 파서',
            '그림 외 비문자/기본 조각 가상 복제','독립 원본/C++ 동작 대조','자동 raw Client UI 연결'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('StyleTextBuffer 정적 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
