#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다중 줄 TextGump의 생성/버퍼 소유/부착/태그·스크롤 입력·프레임 통지를 정적 자료로 보관한다.

사용법: python -X utf8 tools/analyze_scrolltext.py
scrolltext Ghidra 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트를 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 정적 자료는 저장소 안에 생성한다. 두 보호 PE는 읽기만 한다.
PLAN=ROOT/'tools/ghidra/scrolltext-functions.json'
REPORT=ROOT/'cpppj/scrolltext.json'
# 실제 다중 줄 가상 표·signed 정렬 BYTE와 입력 사건 전역의 판본별 주소다.
EDITIONS={
    'originals':('originals/Netstorm.exe',0x51454c,0x540449,
        {'change':0x5427d0,'step':0x5427b0,'refresh':0x542764,'cursor':0x5427e4,'left_tag':0x54278c,'right_tag':0x542790}),
    'originalCD':('originalCD/NETSTORM.EXE',0x505d80,0x535f60,
        {'change':0x512958,'step':0x512938,'refresh':0x5128ec,'cursor':0x51296c,'left_tag':0x512914,'right_tag':0x512918}),
}


def main():
    """실제 함수/명령/가상 표/정렬/사건 전역과 현재 공유 버퍼·Slider·입력·수명 연결 해시를 저장한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 두 판본의 같은 PC 프로젝트 내보내기를 차례로 읽는다.
    for edition,(binary,table,alignment,events) in EDITIONS.items():
        path=ROOT/binary
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/scrolltext'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        if [entry for entry,_ in functions]!=plan['editions'][edition]:
            raise RuntimeError(f'{edition}: 내보낸 함수 주소/순서가 계획과 다름')
        assembly=[]
        # 실제 함수별 범위만 디코드하며 원본/호스트 몸체나 입력을 실행하지 않는다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # 불연속 범위 사이의 다른 코드/데이터는 이어 붙이지 않는다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 원본 x86 바이트/주소/명령/피연산자를 보관한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        slots=struct.unpack_from('<12I',image,table-base)
        facts[edition]={
            'binary':binary,'requested_functions':plan['editions'][edition],
            'exported_functions':[entry for entry,_ in functions],
            'virtual_table':{'address':f'{table:08x}','slots':[f'{slot:08x}' for slot in slots],
                'scalar_delete':f'{slots[0]:08x}','attach':f'{slots[8]:08x}','draw':f'{slots[9]:08x}',
                'handle':f'{slots[10]:08x}','broadcast':f'{slots[11]:08x}'},
            'initial_alignment':struct.unpack_from('<b',image,alignment-base)[0],
            'event_globals':{name:f'{address:08x}' for name,address in events.items()},
            'event_values':'사건 ID는 런타임 문자열 초기화 뒤 공급; PE 초기 0을 실제 ID로 사용하지 않음',
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 같은 실제 생성/버퍼/표시/Slider/트리/통지/캡처/프레임/수명 구현과 빌드 대상을 기록한다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/ScrollingTextGumpWidgets.h','cpppj/src/client/ScrollingTextGumpWidgets.cpp',
        'cpppj/src/client/TextGump.h','cpppj/src/client/TextGump.cpp',
        'cpppj/src/client/StyleTextBuffer.h','cpppj/src/client/StyleTextBuffer.cpp',
        'cpppj/src/client/StyleText.h','cpppj/src/client/StyleText.cpp',
        'cpppj/src/client/ScrollGumpWidgets.h','cpppj/src/client/ScrollGumpWidgets.cpp',
        'cpppj/src/client/ScrollGump.h','cpppj/src/client/ScrollGump.cpp',
        'cpppj/src/client/GumpCreate.h','cpppj/src/client/GumpCreate.cpp',
        'cpppj/src/client/GumpTree.h','cpppj/src/client/GumpTree.cpp',
        'cpppj/src/client/GumpLifetime.h','cpppj/src/client/GumpLifetime.cpp',
        'cpppj/src/client/GumpEvents.h','cpppj/src/client/GumpEvents.cpp',
        'cpppj/src/client/GumpInput.h','cpppj/src/client/GumpInput.cpp',
        'cpppj/src/client/GumpFrame.h','cpppj/src/client/GumpFrame.cpp','cpppj/src/client/CMakeLists.txt',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 읽기 전용 정적 내보내기·가상 표/기계어/초기 정렬 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본/C++ 생성/소유/부착/태그/드래그/픽셀 대조·GUI/게임 실행 없음',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'store':'0x38 별도 토큰의 실제 128칸 버퍼·안정된 같은 state/handler·소멸/free 순서·Gump 등록 제외·원본 미완료 끝 칸 누락 유지',
            'create':'기본 등록/partial fields·문맥/저장 전 배율·overlay→현재 전역 Slider 9×100/숨김/우측 mode 1→Bottom 요청→18 비트',
            'lifetime':'현재 owned buffer만 소멸/free/포인터 0→기본 scalar·borrowed 포인터 유지·자식 우선 Slider 삭제',
            'attach':'기본 Attach→현재 Slider Push/높이/visible(2&4)→우측 mode 1→-1일 때 기존 scroll 요청→폭 dirty',
            'input':'Slider change 발신 검사/현재 position·±5/±높이·refresh·태그 왼쪽/오른쪽·빈 영역 캡처/release·커서 1/15/16',
            'frame':'event/x 무시·poll 없음·bit 4·전달 y-이전 y의 반대 상대 요청·현재 scroll 기준',
            'editions':'폭 비교 unordered: 패치 dirty/저장, CD 생략; 반환/요청/현재 next·필드 조회는 기존 처리기 재사용',
            'binding':'같은 버퍼/Core 표시·트리/재귀/부모 Bubble/직접 캡처/프레임/수명 분배에 연결',
        },
        'boundaries':['독립 PE/C++ 생성/소유/대기 요청/현재 Slider/태그/커서/캡처/드래그/픽셀 대조',
            'W 명령/전체 다중 줄 스타일 파서·버퍼 교체/추가 설정·메뉴/Form·자동 raw Client UI·건설/미션 완주',
            '현재 실제 Cursor 선택/문자열 ID·타입 전역/allocator 토큰 공급',
            '생성자 예외/EH 정리·극단 x87 80비트 값·콜백 도중 현재 표시 줄/조각 반납 또는 버퍼 교체'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('다중 줄 TextGump 정적 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
