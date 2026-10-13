#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""한 줄/다중 줄 TextGump의 실제 가상 표·정밀 내보내기·상수·명령을 정적 근거로 기록한다.

사용법: python -X utf8 tools/analyze_textgump.py
Ghidra textgump 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트를 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 정적 분석 계획/보고서 경로다. 보호 원본은 읽고 생성 자료는 프로젝트 안에 쓴다.
PLAN=ROOT/'tools/ghidra/textgump-functions.json'
REPORT=ROOT/'cpppj/textgump.json'
# 한 줄/다중 줄 실제 가상 표·정렬 BYTE·스크롤 허용 폭/임시 배치 폭의 ROM 주소다.
EDITIONS={
    'originals':('originals/Netstorm.exe',{'single':0x5144d4,'scrolling':0x51454c},0x540448,0x500fc8,0x514480),
    'originalCD':('originalCD/NETSTORM.EXE',{'single':0x505dc0,'scrolling':0x505d80},0x535f5c,0x505d78,0x505d70),
}


def main():
    """두 PE의 함수 몸체/슬롯/상수를 읽고 소유권·스크롤·선택 경계와 현재 구현 해시를 저장한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 두 배포판의 현재 정밀 Ghidra 프로젝트 내보내기를 차례로 읽는다.
    for edition,(binary,tables,alignment,tolerance,fit_width) in EDITIONS.items():
        path=ROOT/binary
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/textgump'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        requested=plan['editions'][edition]
        if [entry for entry,_ in functions]!=requested:
            raise RuntimeError(f'{edition}: 함수 내보내기 순서/주소가 계획과 다름')
        assembly=[]
        # Ghidra의 함수별 불연속 범위만 판독한다. 범위 사이의 다른 함수/데이터를 포함하지 않는다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # 각 실제 범위의 마지막 주소는 TSV에서 inclusive로 변환된 high-1이다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 실제 x86 바이트를 실행하지 않고 피연산자/명령만 디코드한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        virtuals={}
        # 실제 첫 11슬롯에서 scalar 삭제/Redraw/Draw/Input 주소를 보관한다.
        for name,address in tables.items():
            slots=struct.unpack_from('<11I',image,address-base)
            virtuals[name]={'address':f'{address:08x}','slots':[f'{slot:08x}' for slot in slots],
                'scalar_delete':f'{slots[0]:08x}','redraw':f'{slots[2]:08x}','draw':f'{slots[9]:08x}','input':f'{slots[10]:08x}'}
        facts[edition]={
            'binary':binary,'requested_functions':requested,'exported_functions':[entry for entry,_ in functions],
            'virtual_tables':virtuals,
            'initial_single_alignment':struct.unpack_from('<b',image,alignment-base)[0],
            'bottom_tolerance':struct.unpack_from('<f',image,tolerance-base)[0],
            'fit_width':struct.unpack_from('<f',image,fit_width-base)[0],
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 공유 생성/수명/클립/조각·버퍼와 빌드 대상의 현재 해시를 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/TextGump.h','cpppj/src/client/TextGump.cpp',
        'cpppj/src/client/GumpDraw.h','cpppj/src/client/GumpDraw.cpp',
        'cpppj/src/client/GumpCreate.h','cpppj/src/client/GumpCreate.cpp',
        'cpppj/src/client/GumpLifetime.h','cpppj/src/client/GumpLifetime.cpp',
        'cpppj/src/client/GumpCollection.h','cpppj/src/client/GumpCollection.cpp',
        'cpppj/src/client/StyleText.h','cpppj/src/client/StyleText.cpp',
        'cpppj/src/client/StyleTextBuffer.h','cpppj/src/client/StyleTextBuffer.cpp','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 정적 디컴파일·가상 표/ROM/명령 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본/C++ 동작/픽셀 대조·게임/GUI 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'single':'기본 등록 생성·signed 정렬 BYTE·소유 head·0 길이 생략·정수 여백/저장 전 배율·내부 클립·Clear 두 번/scalar 삭제',
            'layout':'dirty 판본별 저장·slider 폭 차감·CD float32 중간 저장·강제 높이/내용 맞춤',
            'scroll':'viewport>=0·range/page/position/current 재조회·요청 저장/소비·Bottom 인자 미사용·10px ordered 허용',
            'draw':'시작 줄 strict skip·x87 viewport 비교·높이 누적·두 번 ascent 측정·측정 폭 전진·현재 next·클립 정상/예외 복구',
            'hit':'half-open 화면 경계·줄 폭 조기 거부·누적 조각 폭·rowHeight-fragmentHeight의 strict 세로 경계',
            'editions':'CD NaN viewport는 줄 순회, 패치 unordered는 제외; AtBottom의 NaN은 두 판본 false',
        },
        'boundaries':['다중 줄 전체 생성/소멸·Slider range/page/position 실제 몸체·가상 Attach/Input 통지',
            'W 명령/전체 다중 줄 스타일 파서·메뉴/Form 생성/장식/제목 분배·Client 자동 raw UI',
            '조각/표시 줄을 출력 콜백 도중 반납하거나 버퍼를 교체하는 무효 수명 입력',
            '독립 PE/C++ 동작·픽셀 대조·극단 x87 80비트 입력'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('TextGump 정적 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
