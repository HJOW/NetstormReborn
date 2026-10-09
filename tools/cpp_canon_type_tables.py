#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미복원 섬/받침/공유 전투 패턴 네 레코드를 읽기만 하여 C++ 포함 파일로 만든다."""
import struct
from decomp_owner_oracle import ROOT, SPECS
import pefile

# 기존 영역/다리 표와 분리하여 이전 감사 파일을 바꾸지 않는다.
OUTPUT=ROOT/'cpppj/src/o/CanonTypePatterns.inc'
TABLES=(('kIslandPatterns',0x531410,0x5164f8,2),('kSupportPatterns',0x5314a0,0x516588,1),('kBattlePatterns',0x5314e8,0x5165d0,1))


def generate():
    """세 PE의 패턴 바이트가 같음을 확인한 뒤 원래 레코드 전체를 저장한다."""
    images={name:pefile.PE(str(ROOT/spec['binary'])) for name,spec in SPECS.items()}
    lines=['// 자동 생성: python -X utf8 tools/cpp_canon_type_tables.py. 직접 수정하지 않는다.',
           '// 마지막 디컴파일 PC: HJOW-Athlon, 2026-10-09. 섬/받침/공유 전투 표는 세 PE에서 바이트가 같다.','']
    # 표와 사용하지 않는 셀까지 72바이트 레코드 전체를 보존한다.
    for name,patch,cd,count in TABLES:
        values=[pe.get_data((patch if edition=='originals' else cd)-pe.OPTIONAL_HEADER.ImageBase,count*72) for edition,pe in images.items()]
        if len(set(values))!=1:raise RuntimeError(f'판본의 특수 패턴 차이: {name}')
        lines.extend([f'// 패치 0x{patch:x}, CD/10.37 0x{cd:x}: 원본 정수 세 개와 셀 15개다.',f'constexpr std::array<CanonPattern,{count}> {name}{{{{'])
        # 개별 패턴의 정수/셀을 C++ 배열로 변환한다.
        for index in range(count):
            record=values[0][index*72:(index+1)*72];first,width,height=struct.unpack_from('<3i',record)
            cells=['{'+','.join(f'0x{b:02x}' for b in record[12+i*4:16+i*4])+'}' for i in range(15)]
            lines.append(f'    {{{first},{width},{height},{{{{'+', '.join(cells)+'}}},')
        lines.extend(['}};',''])
    OUTPUT.write_text('\n'.join(lines),encoding='utf-8',newline='\n');print(f'특수 패턴 4개 → {OUTPUT.relative_to(ROOT)}')


if __name__=='__main__':generate()
