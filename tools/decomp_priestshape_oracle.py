#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 비패턴 픽셀 범위/기준점과 SHP 주소 조회를 세 실제 PE로 대조한다.

전체 getter·decoder·패치 프레임 helper를 정상 반환까지 실행한다. 대체 호출/OS/게임 실행은 없다.
특수 패턴 번호와 타입/SHP/기준점/화면 배율은 분석용 입력이며 자산 로더 몸체는 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS,TYPES,STACK,STOP,digest
from decomp_priestgeometry_oracle import GeometryOracle,GEOMETRY,OUT,CODES
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import UC_X86_REG_ECX

# 분석 전용 SHP와 출력/기준점 저장소다. 실제 PE 파일을 바꾸지 않는다.
SHP=0x14500000
FIXTURE=ROOT/'cpppj/tests/fixtures/priestshape-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestshape-evidence.json'
# 실제 getter/픽셀 배율/타입 기준점 필드다.
SHAPE={'originals':dict(entry=0x43cbd0,scale=0x59a92c,hot=0x1dc,header=0x419850),
       'originalCD':dict(entry=0x4ed7c0,scale=0x565c44,hot=0x1bc,header=0)}
SHAPE['original1037']=dict(SHAPE['originalCD'])
# 입력과 관찰의 고정 열 순서다.
KEYS=('type','argument','direction','explicit','count','default','flags1','width','height','hotX','hotY','typeHotX','typeHotY','scaleX','scaleY')


def inputs(edition):
    """signed short/정수 넘침/float 반올림·빈/명시/이중 프레임·회전을 교차한다."""
    metrics=((16,11,8,5,0,0),(0,0,0,0,0,0),(-16,-11,-8,-5,0,0),
        (32767,-32768,32767,-32768,0,0),(16,11,8,5,8,5),(16,11,8,5,-1000,-1000),
        (16,11,8,5,1000,1000),(16,11,0,0,16777217,-16777217),
        (32767,32767,0,0,-16777217,16777217),(1,1,32767,-32768,2147483647,-2147483648),
        (16,11,0,0,-999,-1000),(16,11,0,0,-1000,-1001),(16,11,-1,1,-2147483648,2147483647),
        (300,220,150,110,48,33))
    profiles=((5,0,158,0,0),(5,4,158,0,0),(1,-1,158,0,0),(0,-1,158,0,0),
        (5,0,0,1,0),(5,0,4,1,0),(5,7,158,0,0x400000),(5,7,158,0,0x40000))
    if edition!='originals':profiles+=((1,4,158,0,0),)
    scales=((16,11),(-16,-11),(0,-0.0),(0.5,1.25))
    directions=(0,1,2,3,4,5,6,7,0xffffffff) if edition=='originals' else (0,2,4,6)
    result=[]
    # 원본 assert/표 밖 접근에 닿는 입력은 별도 C++ 진단 검사로 남긴다.
    for metric,profile,scale,direction in itertools.product(metrics,profiles,scales,directions):
        width,height,hx,hy,tx,ty=metric;count,default,argument,explicit,flags=profile
        result.append(dict(type=158,argument=argument,direction=direction,explicit=explicit,count=count,default=default,
            flags1=flags,width=width,height=height,hotX=hx,hotY=hy,typeHotX=tx,typeHotY=ty,scaleX=bits(scale[0]),scaleY=bits(scale[1])))
    return result


def short(value):
    """분석 입력의 프레임별 signed WORD를 원본 저장 폭으로 만든다."""
    return (value+32768)%65536-32768


class ShapeOracle(GeometryOracle):
    """기존 ABI 검사와 메모리 감시를 재사용하고 새 getter 허용 범위만 실행한다."""
    def __init__(self,edition):
        """읽기 전용 내보내기와 비연속 SHP 프레임 주소 표를 준비한다."""
        super().__init__(edition);self.shape=SHAPE[edition];self.mu.mem_map(SHP,0x1000)
        self.exports=[ROOT/f'extracted/priestshape/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 새 함수 목록의 실제 불연속 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 원본 범위를 반열린 구간으로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def run(self,case,control):
        """전체 cdecl getter의 정상 반환/보존 레지스터/x87와 여섯 float 비트를 관찰한다."""
        mu=self.mu;s=self.shape;typ=TYPES+case['type']*self.type_stride;self.instructions=0;self.write_ranges=[(OUT,OUT+24)]
        mu.mem_write(typ,bytes(self.type_stride));mu.mem_write(typ+0xdc,struct.pack('<I',SHP))
        mu.mem_write(typ+0xe8,struct.pack('<I',case['flags1']));mu.mem_write(typ+0x114,struct.pack('<Ii',case['count'],case['default']))
        mu.mem_write(typ+0x124,struct.pack('<I',CODES));mu.mem_write(typ+s['hot'],struct.pack('<2i',case['typeHotX'],case['typeHotY']))
        mu.mem_write(s['scale'],struct.pack('<2I',case['scaleX'],case['scaleY']));mu.mem_write(SHP,bytes(0x1000))
        # 프레임 주소 순서를 뒤집고 서로 다른 헤더를 넣어 실제 offset 표 조회를 검증한다.
        for frame in range(8):
            header=0x100+(7-frame)*0x40
            mu.mem_write(SHP+8+frame*8,struct.pack('<II',header+0x24,0xdeadbeef))
            mu.mem_write(SHP+header+0x18,struct.pack('<4h',short(case['width']+frame*2),short(case['height']-frame),short(case['hotX']+frame),short(case['hotY']-frame)))
        mu.mem_write(OUT,bytes([0xa5])*24);self.prepare_registers(control);mu.reg_write(UC_X86_REG_ECX,0)
        args=[OUT,case['type'],case['argument'],case['direction'],OUT+16,OUT+20,case['explicit']]
        mu.mem_write(STACK,struct.pack('<8I',STOP,*[value&0xffffffff for value in args]))
        mu.emu_start(s['entry'],STOP,count=20000);self.check(control,STACK+4)
        return list(struct.unpack('<6I',mu.mem_read(OUT,24)))


def generate(smoke=False):
    """독립 기계어 관찰만 기대값으로 저장하고 모든 근거 파일 SHA를 기록한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestgeometry_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/priestshape-functions.json',FIXTURE}
    # 세 실제 PE와 두 정밀도에서 독립 입력을 실행한다.
    for edition in SPECS:
        oracle=ShapeOracle(edition);cases=inputs(edition)
        if smoke:cases=cases[:100]
        # 기대 사각형/기준점은 Python에서 계산하지 않는다.
        for case in cases:
            observations=[oracle.run(case,control) for control in CONTROLS]
            if observations[0]!=observations[1]:raise RuntimeError(f'x87 정밀도 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*observations[0]])
        editions[edition]=dict(cases=len(cases),native_calls=dict(oracle.native_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: 실제 사제 픽셀 모양 {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 전체 비패턴 픽셀 모양 getter/decoder/SHP 조회의 실제 x86 관찰. 자산 로더/MayPlace/지형/OS 미실행.\n# edition '+' '.join(KEYS)+' left top right bottom anchorX anchorY\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limits=['비패턴 픽셀 범위 getter·decoder ctor/Advance·패치 SHP/frameCheck helper 전체 정상 반환 및 cdecl/보존 레지스터/x87 검사','특수 패턴 번호=0·프레임 디버그 전역=0; 타입/SHP 주소 표/메타/기준점/배율은 분석 입력','SHP 자산 로더/특수 패턴/전체 MayPlace/후보별 지형/게임/OS 미실행; assert 오류 입력은 C++ 진단으로 구분']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/행 수/실제 getter·ctor·Advance·프레임 helper 횟수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 모든 생성 근거를 다시 검사한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('사제 픽셀 모양 근거 오류')
    # 빈 프레임은 ctor의 첫 Advance까지만 실행하며 SHP를 조회하지 않는다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];n=len(selected);valid=sum(int(row[4])!=0 or int(row[6])!=-1 for row in selected)
        calls=item['native_calls'];s=SHAPE[edition];g=GEOMETRY[edition]
        if n!=item['cases'] or n!=len(inputs(edition)) or item['assertions']:raise RuntimeError('사제 픽셀 모양 입력 누락')
        if calls.get(f'{s["entry"]:08x}')!=2*n or calls.get(f'{g["begin"]:08x}')!=2*n or calls.get(f'{g["predicate"]:08x}')!=2*n:raise RuntimeError('실제 getter/생성/비패턴 검사 누락')
        if calls.get(f'{g["next"]:08x}')!=2*(n+valid):raise RuntimeError('실제 Advance 횟수 오류')
        if s['header'] and calls.get(f'{s["header"]:08x}')!=2*valid:raise RuntimeError('실제 SHP helper 누락')
    print(f'priestshape 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
