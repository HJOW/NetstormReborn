#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다중 줄 스타일 파서/설정의 실제 PE·정규식·SHP 높이 문턱과 현재 구현 해시를 정적으로 저장한다.

사용법: python -X utf8 tools/analyze_scrollmarkup.py
scrollmarkup Ghidra 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트는 실행하지 않는다.
"""
import datetime
import json
import socket
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges,parser_cases
from analyze_stylemarkup import string,number

# 계획/기록은 저장소 안에서 생성하며 두 보호 PE는 읽기만 한다.
PLAN=ROOT/'tools/ghidra/scrollmarkup-functions.json'
REPORT=ROOT/'cpppj/recovery-scrollmarkup-evidence.json'
# 실제 파서/정규식·맞춤 전역과 복사된 36바이트 SHP 헤더의 높이 비교 명령이다.
EDITIONS={
    'originals':{
        'binary':'originals/Netstorm.exe','parser':0x4ba8f0,
        'patterns':{'I':0x514588,'S':0x5145a8,'T':0x514608,'C':0x5145bc,'B':0x5145d0,'E':0x5145e4},
        'fit':{'width':0x540cbc,'height':0x540cc0},
        'height_compare':0x4babe7,'height_maximum':16,'header_copy_stack_offset':0xb0,'compare_stack_offset':0xca,
        'command_order':['I','S','T','C','B','E'],
    },
    'originalCD':{
        'binary':'originalCD/NETSTORM.EXE','parser':0x4b6c80,
        'patterns':{'I':0x53c3e8,'W':0x53c408,'T':0x53c428},
        'fit':{'width':0x5119c4,'height':0x5119c8},
        'height_compare':0x4b6fd4,'height_maximum':12,'header_copy_stack_offset':0xb8,'compare_stack_offset':0xd2,
        'command_order':['I','W','T'],
    },
}


def main():
    """두 내보낸 실제 함수/명령·ROM 형식/초기 맞춤 값·현재 연결 코드를 실행 없이 기록한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),PLAN,ROOT/'tools/analyze_styletext.py',ROOT/'tools/analyze_stylemarkup.py']
    facts={}
    # 각 판본은 같은 PC의 읽기 전용 정밀 프로젝트에서 내보낸 함수만 판독한다.
    for edition,source in EDITIONS.items():
        path=ROOT/source['binary'];pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/scrollmarkup'/edition
        exported=folder/'creation.c';ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        if [entry for entry,_ in functions]!=plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 내보낸 함수 주소/순서가 계획과 다름')
        assembly=[]
        # 실제 함수 범위만 디코드하며 원본/호스트 결과값이나 기대 조각을 생성하지 않는다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # Ghidra의 불연속 범위 사이 다른 코드/데이터는 읽지 않는다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 원본 주소/바이트/명령/피연산자를 보관한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt';assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        address=source['height_compare']
        compare=next(decoder.disasm(image[address-base:address-base+16],address))
        facts[edition]={
            'binary':source['binary'],'functions':[entry for entry,_ in functions],
            'short_commands':parser_cases(exported.read_text(encoding='utf-8'),source['parser']),
            'command_order':source['command_order'],
            'patterns':{key:string(image,base,value) for key,value in source['patterns'].items()},
            'fit_defaults':{key:number(image,base,value) for key,value in source['fit'].items()},
            'restricted_picture':{
                'height_compare':{'address':f'{address:08x}','bytes':compare.bytes.hex(),
                    'instruction':f'{compare.mnemonic} {compare.op_str}'},
                'header_offset':source['compare_stack_offset']-source['header_copy_stack_offset'],
                'height_maximum_signed':source['height_maximum'],'fit_width':99999,'fit_height':12,
            },
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 같은 문맥/태그/실제 버퍼·그림/옵션/설정 연결과 라이브러리 편입을 현재 해시로 보관한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/ScrollingTextGumpMarkup.h','cpppj/src/client/ScrollingTextGumpMarkup.cpp',
        'cpppj/src/client/StyleText.h','cpppj/src/client/StyleText.cpp',
        'cpppj/src/client/StyleTextMarkup.h','cpppj/src/client/StyleTextMarkup.cpp',
        'cpppj/src/client/TextGump.h','cpppj/src/client/TextGump.cpp',
        'cpppj/src/client/ScrollingTextGumpWidgets.h','cpppj/src/client/ScrollingTextGumpWidgets.cpp',
        'cpppj/src/client/StyleTextBuffer.h','cpppj/src/client/StyleTextBuffer.cpp',
        'cpppj/src/client/Markup.h','cpppj/src/client/Markup.cpp','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'실제 두 PE 읽기 전용 정적 내보내기/기계어/ROM 형식/SHP 높이 비교 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본/C++ 파서/콜백/소유/설정/픽셀 대조·GUI/게임 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'parser':'현재 기본 문맥 복사·구간별 Append/dirty/Redraw·T 태그 +c8 호출 간 유지·~./~n 현재 기본 문맥 재조회·~~ 길이 1·빈 마지막 입력',
            'markup':'패치 I/S/T/C/B/E·CD I/W/T; W 두 캡처 중 첫 소리 이름·CD zone 허용 검사 없음·원본 부분 검색/greedy/80바이트 캡처',
            'picture':'다중 줄 *와 # 기본 프레임·소비 후 suffix # 맞춤·& 색 치환 없음·패치 숫자 타입 상한·count까지 클램프·현재 +d4 제한',
            'restriction':'패치 0/1/3/4 글꼴 명령은 +d4 비0에서 소비만 함·CD 변경; 실제 SHP signed 높이 +1a<=16/12 후 99999×12 맞춤',
            'replace':'완료 입력 줄만 제거/미완료 끝 줄 보존·현재 소유 버퍼만 소멸/free/포인터 0→새 buffer/owns DWORD; 태그/스크롤/dirty 자동 재설정 없음',
            'settings':'selectable 전체 0/비0·slider/scroll 최하위 비트·slider 켜면 scroll 켜기·scroll 끄면 slider 끄기·Push 후 현재 위치 자기 Attach·wrap는 noWrap 저장만',
        },
        'boundaries':['독립 PE/C++ 파서/통지/그림/설정/버퍼 교체/수명/픽셀 대조',
            '콜백이 입력 source 바이트를 수정하는 포인터 alias·원본 프레임 지역 배열 80바이트 이상 쓰기·끝 ~의 버퍼 밖 읽기',
            '원본 그림 색 표/회전 VFX·Form/메뉴 생성·OS 큐/폴링/overlay·자동 raw Client UI·건설/미션 완주',
            '현재 실제 allocator/커서/사건 전역 공급·생성 EH·극단 x87'],
        'editions':facts,'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('다중 줄 스타일 정적 기록 완료:',', '.join(f'{edition} {len(value["functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
