#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반/패턴 전체 decoder와 각 유효 모양의 finder 인자 계산 구간을 실제 PE로 대조한다."""
import argparse
import csv
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS,TYPES,STACK,STOP,digest
from decomp_canontype_oracle import CanonTypeOracle,TYPE_IDS,frame_codes,inputs as decoder_inputs,SIDES
from decomp_priestgeometry_oracle import DEC,OUT,CODES,GEOMETRY
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_EBX

# 독립 입력/출력의 열 순서와 저장 경로다.
KEYS=('kind','argument','direction','profile','x','y','footX','footY','explicit','alias','default','mutation')
FIXTURE=ROOT/'cpppj/tests/fixtures/canongeometry-x86.tsv'
REPORT=ROOT/'cpppj/recovery-canongeometry-evidence.json'
# 실제 snap이 복사 좌표에 저장하는 스택 오프셋이다. 원래 decoder 좌표와 구분한다.
SNAPPED={'originals':0x3c,'originalCD':0x24,'original1037':0x24}


def inputs(edition):
    """모든 패턴/회전/별칭과 칸 사이 현재 발자국 변경·절삭 경계를 교차한다."""
    cases=[dict(case,mutation=case['argument']%2) for case in decoder_inputs(edition)]
    directions=(0,1,2,3,4,5,6,7,-1) if edition=='originals' else (0,2,4,6)
    # 각 대표 모양에 서로 다른 소수 좌표/발자국을 입력한다. 기대 사각형은 계산하지 않는다.
    for kind in range(9):
        for direction in directions:
            for x,y in ((0.99999,1.00001),(19.999998092651367,20.000001907348633),(-1.25,-0.75),(255.99999,254.5)):
                for wx,wy in ((1,1),(2,4),(10,10)):
                    cases.append(dict(kind=kind,argument=0 if kind<8 else 158,direction=direction,profile=0,x=bits(x),y=bits(y),
                        footX=wx,footY=wy,explicit=0,alias=0,default=3,mutation=1))
    # 실제 자산 dude의 0 발자국과 축 하나만 0인 입력도 순수 사각형 산술로 관찰한다.
    for direction in directions:
        for x,y in ((20.5,21.5),(0.99999,1.00001),(-1.25,-0.75),(255.99999,254.5)):
            for wx,wy in ((0,0),(0,1),(1,0)):
                cases.append(dict(kind=8,argument=158,direction=direction,profile=0,x=bits(x),y=bits(y),footX=wx,footY=wy,explicit=0,alias=0,default=3,mutation=0))
    return cases


class CanonGeometryOracle(CanonTypeOracle):
    """실제 생성/전체 진행과 finder 직전 계산만 실행하고 실제 후보/지형은 실행하지 않는다."""
    def __init__(self,edition):
        """새 내보내기의 불연속 함수 범위를 허용한다."""
        super().__init__(edition);self.exports=[ROOT/f'extracted/canongeometry/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 실제 함수 몸체 밖의 호출/OS/assert는 기존 감시가 거부한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def run_geometry(self,case,control,codes=None):
        """전체 칸 좌표와 각 finder 인자/절삭 좌표를 실제 명령에서 관찰한다."""
        mu=self.mu;s=self.s;number=case.get('type',TYPE_IDS[case['kind']]);typ=TYPES+number*self.type_stride
        if codes is None:codes=frame_codes(case['profile'])
        ids=list(TYPE_IDS[:8])
        if case['alias']==1:ids[0]=ids[1]
        if case['alias']==2:ids[2]=ids[4]
        for address,value in zip(s['patterns'],ids,strict=True):mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(typ,bytes(self.type_stride));mu.mem_write(typ+0x114,struct.pack('<Ii',len(codes)//4,case['default']))
        mu.mem_write(typ+0x124,struct.pack('<I',CODES));mu.mem_write(typ+s['foot'],struct.pack('<2i',case['footX'],case['footY']))
        mu.mem_write(CODES,bytes(0x1000));mu.mem_write(CODES,codes);mu.mem_write(DEC,bytes(84));self.write_ranges=[]
        self.call(s['begin'],[number,*[case[key]&0xffffffff for key in ('argument','direction','x','y','explicit')]],control)
        self.call(s['bounds'],[OUT],control);bounds=','.join(map(str,struct.unpack('<4i',mu.mem_read(OUT,16))))
        sequence=[]
        # 모든 유효 칸의 계산을 종료할 때마다 실제 다음 decoder를 실행한다.
        while struct.unpack('<I',mu.mem_read(DEC+20,4))[0]:
            frame,valid,x,y=self.snapshot();label=struct.unpack('<i',mu.mem_read(DEC+24,4))[0]
            side=self.call(SIDES[self.edition],[],control)&0xff;ordinal=len(sequence)
            # 발자국은 계산 입력이다. 순회 시작 뒤의 변경도 실제 타입 필드에 공급한다.
            wx=case['footX']+(ordinal%3 if case['mutation'] and ordinal else 0)
            wy=case['footY']+((ordinal+1)%3 if case['mutation'] and ordinal else 0)
            mu.mem_write(typ+s['foot'],struct.pack('<2i',wx,wy));self.area_values=None;self.prepare_registers(control)
            mu.mem_write(STACK+s['point'],struct.pack('<2I',x,y))
            if self.edition=='originals':mu.mem_write(STACK+s['self'],struct.pack('<I',typ))
            else:mu.reg_write(UC_X86_REG_EBX,typ)
            # MayPlace 전체 호출이 아니다. 구간의 지역 입력을 공급하고 finder 몸체 진입 전에 종료한다.
            mu.emu_start(s['area'],STOP,count=20000);self.check(control,STACK,False)
            snapped=struct.unpack('<2I',mu.mem_read(STACK+SNAPPED[self.edition],8))
            if self.area_values is None:raise RuntimeError('실제 finder 인자 관찰 누락')
            sequence.append(','.join(map(str,(frame,x,y,label,side,*snapped,*self.area_values))));self.valid_cells+=1
            self.call(s['next'],[],control)
            if len(sequence)>15:raise RuntimeError('패턴의 실제 칸 수 초과')
        return [bounds,';'.join(sequence) or '-']


def generate(smoke=False):
    """두 x87 정밀도 관찰과 모든 근거 SHA를 별도 fixture에 저장한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_canontype_oracle.py',
        ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',
        ROOT/'tools/ghidra/canongeometry-functions.json',FIXTURE}
    # 세 실제 PE를 독립 실행하고 OS/후보/finder 몸체는 호출하지 않는다.
    for edition in SPECS:
        oracle=CanonGeometryOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[:30]
        for case in cases:
            observed=[oracle.run_geometry(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'x87 정밀도 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*observed[0]])
        editions[edition]=dict(cases=len(cases),valid_cells=oracle.valid_cells//2,native_calls=dict(oracle.native_calls),
            area_entries=oracle.area_entries,finder_boundaries=oracle.finder_boundaries,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 패턴 배치 모양 {len(cases)}개·칸 {oracle.valid_cells//2}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 전체 decoder와 모든 유효 칸의 finder 직전 계산. 전체 MayPlace/finder/후보/지형 미실행.\n# edition '+' '.join(KEYS)+' bounds sequence(frame,x,y,label,side,snappedX,snappedY,left,top,right,bottom)\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,
        editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['전체 decoder 생성/진행/Bounds/셀/검색/방향 getter 정상 반환/ABI/x87 대조',
        '각 칸의 snap/CRT/충돌 사각형은 MayPlace 중간 구간에 실제 좌표/타입을 공급하고 finder 진입에서 관찰 종료',
        '타입/코드/발자국은 분석 입력; 패턴 표는 실제 PE 데이터; 전체 MayPlace/finder/후보/지형/OS/게임 미실행']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/행 수/모든 칸의 실제 계산과 생성/진행/helper 호출 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('일반 배치 모양 근거 오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];n=len(selected);cells=sum(0 if row[14]=='-' else len(row[14].split(';')) for row in selected)
        calls=item['native_calls'];s=GEOMETRY[edition]
        if n!=item['cases'] or n!=len(inputs(edition)) or cells!=item['valid_cells'] or item['assertions']:raise RuntimeError('입력/칸/assert 근거 오류')
        if item['area_entries']!=2*cells or item['finder_boundaries']!=2*cells:raise RuntimeError('모든 칸의 실제 finder 인자 누락')
        if calls.get(f'{s["begin"]:08x}')!=2*n or calls.get(f'{s["next"]:08x}')!=2*(n+cells) or calls.get(f'{s["bounds"]:08x}')!=2*n:raise RuntimeError('생성/전체 진행/범위 횟수 오류')
        if calls.get(f'{s["snap"]:08x}',0)!=2*cells or calls.get(f'{s["finder"]:08x}',0):raise RuntimeError('실제 snap/미실행 finder 구분 오류')
        # Bounds는 x/y 절삭 2회, 한 모양은 snap 2회와 사각형 변환 4회다. 두 제어값의 합산 횟수다.
        if calls.get(f'{s["ftol"]:08x}')!=4*n+12*cells:raise RuntimeError('실제 CRT 횟수 오류')
    print(f'canongeometry 검증 통과: {len(rows)}개')


def main():
    """전체 생성/무저장 표본/저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
