#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 타입별 패턴 선택/생성/전체 순회/범위/방향 getter를 대체 없이 실행한다."""
import argparse
import csv
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS,TYPES,digest
from decomp_priestgeometry_oracle import GeometryOracle,GEOMETRY,DEC,OUT,CODES
from decomp_damageablepredestroy_oracle import CONTROLS,bits

# 실제 원본 전역 타입 번호와 새 결과 경로다. 기본 사제 158은 마지막 비패턴 분류다.
TYPE_IDS=(107,82,94,157,131,129,140,142,158)
COUNTS=(68,26,2,1,1,1,1,1)
FIXTURE=ROOT/'cpppj/tests/fixtures/canontype-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canontype-evidence.json'
# 패턴 표 주소와 실제 방향 getter 진입점이다.
TABLES={'originals':(0x5300f0,0x52f998,0x531410,0x5314a0,0x5314e8),
        'originalCD':(0x5151d8,0x514a80,0x5164f8,0x516588,0x5165d0)}
TABLES['original1037']=TABLES['originalCD']
SIDES={'originals':0x425c00,'originalCD':0x420370,'original1037':0x420370}
# 입력 열은 기대값과 무관한 패턴 종류/번호/회전/프레임 순서/좌표/발자국/명시 모드/별칭이다.
KEYS=('kind','argument','direction','profile','x','y','footX','footY','explicit','alias','default')


def frame_codes(profile):
    """모든 방향/번호와 역순/중복 프레임 입력을 만든다. 기대 프레임은 계산하지 않는다."""
    codes=[]
    # 원본 회전 변형 글자는 P다. 빈 표는 프레임 조회 없는 비패턴 입력에만 쓴다.
    for side in range(65,81):
        for number in range(11):
            if profile==2:continue
            codes.append(bytes((side,80,number,0)))
    if profile==1:codes.reverse()
    if profile!=2:codes.append(codes[0])
    return b''.join(codes)


def inputs(edition):
    """모든 패턴 번호/유효 회전/프레임 순서와 비패턴·명시 모드/별칭 우선순위를 교차한다."""
    cases=[];directions=(0,1,2,3,4,5,6,7,-1) if edition=='originals' else (0,2,4,6)
    # 모든 패턴의 기본 경로와 역순 프레임 표를 관찰한다. 누락 검색은 원본 assert 경로다.
    for kind,count in enumerate(COUNTS):
        for argument in range(count):
            for direction in directions:
                for profile in (0,1):
                    cases.append(dict(kind=kind,argument=argument,direction=direction,profile=profile,x=bits(20.5),y=bits(-1.25),footX=3,footY=2,explicit=argument%2,alias=0,default=3))
    # 좌표 반올림/발자국과 타입 전역 별칭은 적은 패턴으로 별도 교차한다.
    for kind in range(8):
        for direction in directions:
            for x,y in ((0,0),(16777216,16777216),(255.99999,254.5)):
                cases.append(dict(kind=kind,argument=0,direction=direction,profile=0,x=bits(x),y=bits(y),footX=1,footY=4,explicit=1,alias=0,default=3))
    for alias,kind in ((1,1),(2,4)):
        for direction in directions:
            cases.append(dict(kind=kind,argument=1,direction=direction,profile=0,x=bits(20),y=bits(21),footX=2,footY=3,explicit=1,alias=alias,default=3))
    # 비패턴의 음수 기본 프레임과 명시 입력은 정상 계약 범위에서 검사한다.
    for direction in directions:
        for default,argument,explicit,profile in ((3,158,0,0),(-1,158,0,2),(3,2,1,0)):
            cases.append(dict(kind=8,argument=argument,direction=direction,profile=profile,x=bits(20.25),y=bits(21.75),footX=2,footY=3,explicit=explicit,alias=0,default=default))
    return cases


class CanonTypeOracle(GeometryOracle):
    """전체 생성/진행/helper 몸체와 원본 PE의 패턴 표만 실행/참조한다."""
    def __init__(self,edition):
        """새 내보내기 범위를 읽고 합성 프레임 자료 입력을 준비한다."""
        super().__init__(edition);self.exports=[ROOT/f'extracted/canontype/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.steps=0;self.valid_cells=0
        with self.exports[1].open(encoding='utf-8') as fp:
            # 각 실제 함수의 불연속 몸체를 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))

    def run(self,case,control,codes=None):
        """생성부터 마지막 상태/범위를 관찰한다. 선택 입력은 실제 자산의 프레임 코드 표다."""
        mu=self.mu;s=self.s;number=TYPE_IDS[case['kind']];target=TYPES+number*self.type_stride
        if codes is None:codes=frame_codes(case['profile'])
        ids=list(TYPE_IDS[:8])
        if case['alias']==1:ids[0]=ids[1]
        if case['alias']==2:ids[2]=ids[4]
        # 실제 비교 순서와 별칭 우선순위를 원본 코드에 맡긴다.
        for address,value in zip(s['patterns'],ids,strict=True):mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(target,bytes(self.type_stride));mu.mem_write(target+0x114,struct.pack('<Ii',len(codes)//4,case['default']))
        mu.mem_write(target+0x124,struct.pack('<I',CODES));mu.mem_write(target+s['foot'],struct.pack('<2i',case['footX'],case['footY']))
        mu.mem_write(CODES,bytes(0x1000));mu.mem_write(CODES,codes);mu.mem_write(DEC,bytes(84));self.write_ranges=[]
        if self.call(s['begin'],[number,*[case[k]&0xffffffff for k in ('argument','direction','x','y','explicit')]],control)!=DEC:raise RuntimeError('타입 decoder 생성 반환 오류')
        pointer=struct.unpack('<I',mu.mem_read(DEC+0x34,4))[0];selected='plain'
        # 선택된 원본 패턴 포인터를 종류/번호로 기록한다. 선택 규칙을 기대값으로 계산하지 않는다.
        for group,address in enumerate(TABLES[self.edition]):
            count=(68,26,2,1,1)[group]
            if address<=pointer<address+count*72 and (pointer-address)%72==0:selected=f'{group}:{(pointer-address)//72}'
        if pointer and selected=='plain':raise RuntimeError('기록할 수 없는 원본 패턴 포인터')
        self.call(s['bounds'],[OUT],control);bounds=','.join(map(str,struct.unpack('<4i',mu.mem_read(OUT,16))))
        sequence=[]
        # 유효 칸과 끝 상태를 모두 저장하며 원본의 건너뛰기를 C++에서 대조한다.
        while struct.unpack('<I',mu.mem_read(DEC+20,4))[0]:
            frame,valid,x,y=self.snapshot();label=struct.unpack('<i',mu.mem_read(DEC+24,4))[0]
            side=self.call(SIDES[self.edition],[],control)&0xff
            sequence.append(','.join(map(str,(frame,x,y,label,side))));self.valid_cells+=1
            self.call(s['next'],[],control);self.steps+=1
            if len(sequence)>15:raise RuntimeError('원본 패턴의 칸 수 초과')
        end=self.snapshot();label=struct.unpack('<i',mu.mem_read(DEC+24,4))[0]
        self.call(s['bounds'],[OUT],control);ended=','.join(map(str,struct.unpack('<4i',mu.mem_read(OUT,16))))
        return [selected,bounds,';'.join(sequence) or '-',*end,label,ended]


def generate(smoke=False):
    """실제 명령의 두 정밀도 관찰이 같을 때 fixture와 SHA/호출 기록을 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/canontype-functions.json',ROOT/'tools/cpp_canon_type_tables.py',ROOT/'cpppj/src/o/CanonTypePatterns.inc',FIXTURE}
    # 세 PE 모두 전체 생성/조회/진행을 실행한다.
    for edition in SPECS:
        oracle=CanonTypeOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[:25]+cases[-25:]
        for case in cases:
            observed=[oracle.run(case,c) for c in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'타입 패턴 x87 차이: {edition} {case}')
            rows.append([edition,*[case[k] for k in KEYS],*observed[0]])
        editions[edition]=dict(cases=len(cases),steps=oracle.steps,valid_cells=oracle.valid_cells,native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 타입별 decoder {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 타입별 decoder 전체 생성/진행/범위/방향. 자산/MayPlace/게임/OS 미실행.\n# edition '+' '.join(KEYS)+' selected bounds sequence endFrame endValid endX endY endLabel endBounds\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['패턴/비패턴 생성·전체 진행·셀 해석·프레임 검색·범위·방향 getter 대체 없이 정상 반환/ABI/x87 대조','프레임/타입 발자국은 분석 입력, 패턴 표는 실제 PE 바이트','미러/진행 간격은 원본 생성 초기값 0/1만 관찰, 전체 MayPlace/일반 Pop/GUI/게임 미실행']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """근거 SHA/입력 개수/실제 생성·전체 진행·범위·방향 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 입력과 내보내기의 변경을 모두 탐지한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('타입 패턴 근거 오류')
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];n=len(selected);valid=sum(0 if r[14]=='-' else len(r[14].split(';')) for r in selected)
        spec=GEOMETRY[edition];calls=item['native_calls']
        if n!=item['cases'] or n!=len(inputs(edition)) or item['assertions'] or item['steps']!=valid*2 or item['valid_cells']!=valid*2:raise RuntimeError('타입 패턴 입력/진행 횟수 오류')
        if calls.get(f'{spec["begin"]:08x}')!=2*n or calls.get(f'{spec["next"]:08x}')!=2*(n+valid) or calls.get(f'{spec["bounds"]:08x}')!=4*n or calls.get(f'{SIDES[edition]:08x}',0)!=2*valid:raise RuntimeError('실제 생성/조회/helper 횟수 오류')
    print(f"타입별 decoder 감사 통과: {report['total']}개")


def main():
    """전체 생성/짧은 무저장 실행/저장된 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
