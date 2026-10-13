#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""StyleText 대괄호의 실제 ROM 정규식·옵션·명령/범위와 현재 연결 해시를 정적 자료로 저장한다.

사용법: python -X utf8 tools/analyze_stylemarkup.py
Ghidra stylemarkup 내보내기를 먼저 수행한다. 게임/에뮬레이터/테스트는 실행하지 않는다.
"""
import datetime
import json
import socket
import struct
from pathlib import Path

import capstone
import pefile
from analyze_styletext import ROOT,digest,read_ranges

# 경로는 저장소 안에서만 사용하며 원본 PE는 읽기 전용이다.
PLAN=ROOT/'tools/ghidra/stylemarkup-functions.json'
REPORT=ROOT/'cpppj/stylemarkup.json'
# 판본별 실제 형식 문자열과 맞춤 전역 주소다. CD의 W는 TextGump용이며 StyleText에서 사용하지 않는다.
EDITIONS={
    'originals':{
        'binary':'originals/Netstorm.exe','patterns':{'I':0x514588,'S':0x5145a8,'C':0x5145bc,'B':0x5145d0,'E':0x5145e4,'T':0x514608},
        'fit':{'width':0x540cbc,'height':0x540cc0},
        'options':{'allowzonesounds':0x53186c,'zonebgcolor':0x531870,'zonefgcolor':0x531874,'zoneoffset':0x53187c},
    },
    'originalCD':{
        'binary':'originalCD/NETSTORM.EXE','patterns':{'I':0x53c3e8,'T':0x53c428,'W_TextGump':0x53c408},
        'fit':{'width':0x5119c4,'height':0x5119c8},'options':{},
    },
}


def string(image,base,address):
    """실제 ROM에서 NUL까지 바이트/문자열을 읽는다. CR 중복도 정규화하지 않는다."""
    begin=address-base
    end=image.index(b'\0',begin)
    raw=image[begin:end]
    return {'address':f'{address:08x}','bytes':raw.hex(),'text':raw.decode('cp1252')}


def number(image,base,address):
    """초기 DWORD 전역의 실제 signed 값과 바이트를 기록한다."""
    raw=image[address-base:address-base+4]
    return {'address':f'{address:08x}','bytes':raw.hex(),'value':struct.unpack('<i',raw)[0]}


def main():
    """두 PE·내보낸 몸체/명령과 현재 문자열/그림/타입/옵션 연결을 실행 없이 기록한다."""
    plan=json.loads(PLAN.read_text(encoding='utf-8'))
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    files=[Path(__file__),ROOT/'tools/analyze_styletext.py',PLAN]
    facts={}
    # 각 판본은 별도 원본과 별도 정밀 프로젝트 내보내기를 읽는다.
    for edition,source in EDITIONS.items():
        path=ROOT/source['binary']
        pe=pefile.PE(str(path))
        image=pe.get_memory_mapped_image()
        base=pe.OPTIONAL_HEADER.ImageBase
        folder=ROOT/'extracted/stylemarkup'/edition
        exported=folder/'creation.c'
        ranges_path=folder/'functions.tsv'
        functions=read_ranges(ranges_path)
        assembly=[]
        # 실제 함수 범위의 PE 바이트를 판독하며 기대 조각/픽셀이나 재생 결과를 만들지 않는다.
        for entry,ranges in functions:
            assembly.append(f'; 함수 {entry}')
            # Ghidra가 분리한 범위 사이의 다른 함수/데이터는 제외한다.
            for low,high in ranges:
                assembly.append(f'; 범위 {low:08x}-{high-1:08x}')
                # 현재 원본의 명령 주소/바이트와 피연산자를 보관한다.
                for instruction in decoder.disasm(image[low-base:high-base],low):
                    assembly.append(f'{instruction.address:08x}  {instruction.bytes.hex():<24} {instruction.mnemonic} {instruction.op_str}')
        assembly_path=folder/'assembly.txt'
        assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8')
        facts[edition]={
            'binary':source['binary'],'requested_functions':plan['editions'][edition],
            'exported_functions':[entry for entry,_ in functions],
            'patterns':{key:string(image,base,address) for key,address in source['patterns'].items()},
            'fit':{key:number(image,base,address) for key,address in source['fit'].items()},
            'option_defaults':{key:number(image,base,address) for key,address in source['options'].items()},
        }
        files.extend([path,exported,ranges_path,assembly_path])
    # 공통 태그/체인·그림 선택·실제 자산/소리/옵션 경계와 기존 기대값 수정까지 현재 해시를 남긴다.
    files.extend(ROOT/name for name in [
        'cpppj/src/client/StyleTextMarkup.cpp','cpppj/src/client/StyleTextMarkup.h',
        'cpppj/src/client/StyleText.cpp','cpppj/src/client/StyleText.h',
        'cpppj/src/client/Markup.cpp','cpppj/src/client/Markup.h','cpppj/src/client/CMakeLists.txt',
        'cpppj/src/o/TypeParser.cpp','cpppj/src/o/RiftType.h','cpppj/src/o/ConfigInterface.cpp',
        'cpppj/tests/AssetTests.cpp',
    ])
    report={
        'last_decompile_host':socket.gethostname(),'recorded_at':datetime.datetime.now().astimezone().isoformat(),
        'method':'두 실제 PE 정적 디컴파일·원본 정규식/전역/명령 판독; 게임/에뮬레이터 실행 없음',
        'cpp_comparison':'미수행: 새 테스트 추가/실행·독립 원본 동작 대조·GUI 실행 없음; 기존 AssetTests 기대값 한 곳 수정',
        'new_original_observations':0,'new_cpp_comparisons':0,
        'implementation':{
            'patch_order':['I','S','T','C','B','E'],'cd_order':['I','T'],
            'tag':'Parse마다 0, T 이후 문자열/그림/리터럴/마지막 빈 조각에 전달, ~.로 초기화하지 않음',
            'patterns':'부분 검색/대소문자 구분/greedy 캡처/ASCII space/점 wildcard/탭 캡처 유지',
            'picture':'이름 또는 번호·글자 구간·*/#/&·count 포함 상한·현재 태그·색 치환/상자 맞춤',
            'gump_frame_default':'두 원본 .type 로딩의 -1 초기값 반영, 미지정은 default로 복귀',
        },
        'boundaries':['전체 Regexp 모듈','TextGump W 명령/다중 줄/줄바꿈/스크롤',
            '원본 그림 변환 표 공급','자동 raw 메뉴/Form·입력/클라이언트 UI 연결','독립 원본/C++ 동작 대조'],
        'editions':facts,
        'files':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in files},
    }
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('StyleText 대괄호 정적 기록 완료:',', '.join(f'{edition} {len(value["exported_functions"])}개' for edition,value in facts.items()))


if __name__=='__main__': main()
