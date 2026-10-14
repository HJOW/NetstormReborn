#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""신전 가상8c와 사각형/표시 기계어를 활성 표시 상태에서 독립 실행한다."""
import csv
import itertools
import json
import struct
from decomp_display_oracle import DisplayOracle, ROOT, POOL, packed, bits
from decomp_campaign_frame import sha
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

SPEC = {  # 실제 신전 표/8c와 lazy 초기화 저장소다.
    'originals': ((0x5196f0,0x519860,0x519918),(0x4dcda0,0x4dcf80,0x4dd150),(0x5e1834,0x5e1848,0x5e1880),None),
    'originalCD': ((0x501d28,0x501de0,0x501e98),(0x44feb0,0x450140,0x450320),(0x5bbc00,0x5bbbe0,0x5bbbc4),0x4f1560),
}
FIXTURE = ROOT / 'cpppj/tests/fixtures/vortex-display-x86.tsv'  # 이번 활성 표시의 새 입력/dirty 기대값이다.
REPORT = ROOT / 'cpppj/vortex-display.json'  # 원본 함수와 정상 반환을 별도로 기록한다.


class VortexDisplayOracle(DisplayOracle):
    """lazy 저장과 CD 종료 등록만 허용하며 실제 사각형/dirty 몸체는 대체하지 않는다."""

    def __init__(self, edition):
        """같은 PC export의 실제 파생 및 사각형 보조 범위를 추가한다."""
        super().__init__(edition)
        self.paths, self.registrations = [], 0  # 추가 근거와 CD 외부 종료 등록 요청 수다.
        # Ghidra의 불연속 몸체 사이/OS 진입은 허용하지 않는다.
        for folder in ('vortex-update','campaign-frame'):
            self.paths += [ROOT/f'extracted/{folder}/{edition}/{name}' for name in ('creation.c','functions.tsv')]
            with self.paths[-1].open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'))
                        self.allowed.append((low,high+1))

    def on_instruction(self, mu, address, size, data):
        """CD atexit은 함수 포인터의 외부 등록만 관찰하고 callback을 실행하지 않는다."""
        if address == SPEC[self.edition][3]:
            self.registrations += 1
            sp = mu.reg_read(UC_X86_REG_ESP)  # cdecl caller의 스택이다.
            ret = int.from_bytes(mu.mem_read(sp,4),'little')
            mu.reg_write(UC_X86_REG_EAX,0)
            mu.reg_write(UC_X86_REG_ESP,sp+4)
            mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)


def main():
    """동일 입력의 정상 활성 표시 반환과 변경표 전체를 새 fixture로 고정한다."""
    rows, editions = [], {}  # 이번 원본 관찰만 기록한다.
    cases = list(itertools.product(range(3),(0,1,0x8000),(0,4),(0,0x40000),(32768,65536,98304),range(3)))
    # 각 보호 PE에서 실제 파생/사각형/dirty 명령을 실행한다.
    for edition in SPEC:
        oracle = VortexDisplayOracle(edition)
        oracle.start(0x037f)
        tables, entries, lazy, _ = SPEC[edition]
        oracle.write_ranges.extend((a,a+40) for a in lazy)
        for kind, word, extra, flags, zoom, position in cases:
            x,y=((20.,20.),(0.25,0.5),(38.,40.))[position]
            oracle.shape(flags,2,True,91,[(100,120,-3,5),(130,140,7,-9),(50,70,2,3),(80,90,4,6)])
            oracle.view(640,480,7,9,zoom,(12,9,620,450),0)
            oracle.slot(1,extra,4,x,y)
            raw = POOL+oracle.sid*oracle.stride  # 같은 실제 확보 슬롯이다.
            oracle.mu.mem_write(raw,struct.pack('<I',tables[kind]))
            oracle.mu.mem_write(raw+12,struct.pack('<H',word))
            before = oracle.raw()
            oracle.invoke(entries[kind],this=raw)
            assert before == oracle.raw()
            rows.append((edition,','.join(map(str,(kind,word,extra,flags,zoom,position))),before,*oracle.dirty_output()))
        paths = oracle.paths  # 새 export의 원문 SHA다.
        editions[edition] = dict(samples=len(cases),returns=len(cases),assertions=oracle.assertions,registrations=oracle.registrations,
            exports={p.relative_to(ROOT).as_posix():sha(p) for p in paths},displayCalls=dict(oracle.display_calls))
        print(edition,len(cases),'표시 정상 반환',flush=True)
    FIXTURE.write_text('# 실제 신전8c/사각형/dirty 명령, 합성SHP/카메라, CD atexit 등록은 명시 경계.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),editions=editions,
        fixtureSha=sha(FIXTURE),toolSha=sha(ROOT/'tools/decomp_vortex_display.py'),
        limitations=['실제 파생8c/부분사각형/공통88/bounds/dirty 명령. 합성TYPE/SHP와 유한 카메라 입력이다.',
            'CD 종료 등록은 관찰 경계이며 callback은 실행하지 않는다. GUI/실제 게임 플레이 검증은 별도다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__ == '__main__':
    main()
