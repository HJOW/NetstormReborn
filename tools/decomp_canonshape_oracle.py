#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 자산과 특수 패턴의 전체 픽셀 getter를 세 실제 PE에서 대체 없이 실행한다."""
import argparse
import csv
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS,TYPES,STACK,STOP,digest
from decomp_priestshape_oracle import ShapeOracle,SHP,SHAPE,short
from decomp_priestgeometry_oracle import OUT,CODES,GEOMETRY
from decomp_canontype_oracle import TYPE_IDS,frame_codes,inputs as decoder_inputs
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_ECX

# 새 독립 관찰의 출력 경로와 입력 열 순서다.
FIXTURE=ROOT/'cpppj/tests/fixtures/canonshape-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canonshape-evidence.json'
KEYS=('kind','argument','direction','profile','explicit','alias','default','metric','scaleX','scaleY','hotX','hotY')


def headers(count,metric):
    """주소나 기대 사각형과 무관한 서로 다른 signed WORD 입력을 만든다."""
    result=[]
    # 각 논리 프레임의 헤더를 구별하여 선택/회전/전체 칸 합산을 확인한다.
    for frame in range(count):
        if metric==0:value=(24+frame%7*3,19+frame%5*2,12+frame%11,7-frame%13)
        elif metric==1:value=(short(32767+frame*37),short(-32768+frame*29),short(32767-frame*31),short(-32768+frame*17))
        else:value=(0,0,0,0)
        result.append(value)
    return result


def inputs(edition):
    """모든 패턴/회전/역순/별칭과 배율·기준점 넘침·동률을 교차한다."""
    cases=[];seen=set()
    # 앞 단계의 입력에서 픽셀 getter가 사용하는 독립 키만 취한다.
    for source in decoder_inputs(edition):
        case={key:source[key] for key in KEYS[:7]}
        case.update(metric=source['profile']%2,scaleX=bits(16),scaleY=bits(11),hotX=8,hotY=2)
        key=tuple(case[k] for k in KEYS)
        if key not in seen:cases.append(case);seen.add(key)
    directions=(0,1,2,3,4,5,6,7,-1) if edition=='originals' else (0,2,4,6)
    scales=((0,-0.0),(-16,-11),(0.1,1/3),(16777216,0.5),(0.5,1.25))
    # 작은 대표 패턴에서도 칸별 최소/최대와 기준점 갱신이 경쟁하도록 만든다.
    for kind in range(9):
        for direction in directions:
            for metric in range(3):
                for scale in scales:
                    cases.append(dict(kind=kind,argument=0 if kind<8 else 158,direction=direction,profile=0,
                        explicit=0,alias=0,default=3,metric=metric,scaleX=bits(scale[0]),scaleY=bits(scale[1]),
                        hotX=-2147483648 if metric==1 else 0,hotY=2147483647 if metric==1 else 0))
    return cases


class CanonShapeOracle(ShapeOracle):
    """이전 ABI/메모리 감시와 새 getter 전체/helper 범위만 사용한다."""
    def __init__(self,edition):
        """실제 내보내기와 충분한 비연속 SHP 헤더 공간을 준비한다."""
        super().__init__(edition);self.mu.mem_map(SHP+0x1000,0x3f000)
        self.exports=[ROOT/f'extracted/canonshape/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 실제 함수의 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def observe(self,number,argument,direction,explicit,codes,default,flags,hotspots,metrics,patterns,scales,control):
        """전체 cdecl getter의 정상 반환/ABI/x87와 여섯 float 비트를 관찰한다."""
        mu=self.mu;s=self.shape;typ=TYPES+number*self.type_stride;self.instructions=0;self.write_ranges=[(OUT,OUT+24)]
        if len(codes)>0x1000 or len(metrics)*0x40+0x1000>0x40000:raise RuntimeError('분석 입력 공간 부족')
        mu.mem_write(typ,bytes(self.type_stride));mu.mem_write(typ+0xdc,struct.pack('<I',SHP))
        mu.mem_write(typ+0xe8,struct.pack('<I',flags));mu.mem_write(typ+0x114,struct.pack('<Ii',len(codes)//4,default))
        mu.mem_write(typ+0x124,struct.pack('<I',CODES));mu.mem_write(typ+s['hot'],struct.pack('<2i',*hotspots))
        mu.mem_write(s['scale'],struct.pack('<2I',*scales));mu.mem_write(CODES,bytes(0x1000));mu.mem_write(CODES,codes)
        # 각 SHP 주소는 역순이고 원본/합성 헤더의 물리 순서는 보존한다.
        for frame,metric in enumerate(metrics):
            offset=0x1000+(len(metrics)-1-frame)*0x40
            mu.mem_write(SHP+8+frame*8,struct.pack('<II',offset+0x24,0xdeadbeef))
            mu.mem_write(SHP+offset+0x18,struct.pack('<4h',*metric))
        # 선택 규칙은 실제 타입 비교/원본 PE 패턴 표/FindFrame 몸체에 맡긴다.
        for address,value in zip(self.s['patterns'],patterns,strict=True):mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(OUT,bytes([0xa5])*24);self.prepare_registers(control);mu.reg_write(UC_X86_REG_ECX,0)
        args=[OUT,number,argument,direction,OUT+16,OUT+20,explicit]
        mu.mem_write(STACK,struct.pack('<8I',STOP,*[value&0xffffffff for value in args]))
        mu.emu_start(s['entry'],STOP,count=200000);self.check(control,STACK+4)
        return list(struct.unpack('<6I',mu.mem_read(OUT,24)))

    def run(self,case,control):
        """기대값을 계산하지 않고 합성 코드/헤더/기준점 자료만 공급한다."""
        codes=frame_codes(case['profile']);patterns=list(TYPE_IDS[:8])
        if case['alias']==1:patterns[0]=patterns[1]
        if case['alias']==2:patterns[2]=patterns[4]
        return self.observe(TYPE_IDS[case['kind']],case['argument'],case['direction'],case['explicit'],codes,
            case['default'],0,(case['hotX'],case['hotY']),headers(len(codes)//4,case['metric']),patterns,
            (case['scaleX'],case['scaleY']),control)


def generate(smoke=False):
    """두 정밀도의 실제 관찰이 같을 때 fixture와 모든 근거 SHA를 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestshape_oracle.py',
        ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_canontype_oracle.py',
        ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/canonshape-functions.json',FIXTURE}
    # 세 실제 PE를 각각 읽고 전체 함수 정상 반환까지 실행한다.
    for edition in SPECS:
        oracle=CanonShapeOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[:30]
        for case in cases:
            observations=[oracle.run(case,control) for control in CONTROLS]
            if observations[0]!=observations[1]:raise RuntimeError(f'x87 정밀도 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*observations[0]])
        calls=dict(oracle.native_calls);cells=calls.get(f'{GEOMETRY[edition]["next"]:08x}',0)//2-len(cases)
        editions[edition]=dict(cases=len(cases),valid_cells=cells,native_calls=calls,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 전체 픽셀 getter {len(cases)}개·유효 칸 {cells}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 전체 일반/패턴 픽셀 getter·SHP·decoder helper의 실제 x86 관찰.\n# edition '+' '.join(KEYS)+' left top right bottom anchorX anchorY\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,
        editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['전체 getter·ctor/Advance·cell/FindFrame·패치 SHP helper 정상 반환/보존 레지스터/x87 검사',
        '타입/프레임/SHP 헤더/기준점/배율은 분석 입력; 패턴 표는 실제 PE 데이터',
        '대체 함수/OS/게임 실행 없음; 자산 로더/전체 MayPlace/지형/일반 Pop/GUI 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/행 수/실제 getter와 전체 순회/helper 실행 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('일반 픽셀 관찰 근거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];n=len(selected);calls=item['native_calls'];g=GEOMETRY[edition];s=SHAPE[edition]
        if n!=item['cases'] or n!=len(inputs(edition)) or item['assertions']:raise RuntimeError('일반 픽셀 입력/assert 오류')
        if calls.get(f'{s["entry"]:08x}')!=2*n or calls.get(f'{g["begin"]:08x}')!=2*n:raise RuntimeError('전체 getter/생성 실행 누락')
        if calls.get(f'{g["next"]:08x}')!=2*(n+item['valid_cells']):raise RuntimeError('전체 Advance 횟수 오류')
        if s['header'] and calls.get(f'{s["header"]:08x}')!=2*item['valid_cells']:raise RuntimeError('전체 SHP 조회 누락')
        find=0x49a940 if edition=='originals' else 0x4442f0
        if not calls.get(f'{find:08x}',0):raise RuntimeError('패턴 프레임 검색 미실행')
    print(f'canonshape 검증 통과: {len(rows)}개')


def main():
    """전체 생성/무저장 표본/저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
