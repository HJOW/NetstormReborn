#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 판본의 모든 자산/패턴 모양 순회와 finder 사각형을 원본 명령에 대조한다."""
import argparse
import hashlib
import json
from pathlib import Path
from decomp_canongeometry_oracle import ROOT,CanonGeometryOracle,CONTROLS,bits
from cpp_assets_smoke import original_order,read_data,expected_code,run
from cpp_canontype_smoke import NAMES,COUNTS
from taff import TaffArchive
from typefile import parse_type


def check(executable,edition):
    """C++ 값과 독립적인 실제 TYPE parser 자료를 동일 원본 계산 구간에 공급한다."""
    root=ROOT/edition;archive=TaffArchive(str(root/'netstorm.tarc'));order=original_order(edition);args=['--cd'] if edition=='originalCD' else []
    actual=[json.loads(line) for line in run(executable,'--inspect-canon-geometry',root,*args).stdout.splitlines()]
    patterns=[70+order.index(name) for name in NAMES[:8]];sources={};expected=set();type_sha={};oracle=CanonGeometryOracle(edition)
    # 모든 자산의 실제 논리 코드/defaultFrame/발자국을 원래 로딩 순서로 읽는다.
    for index,name in enumerate(order):
        number=70+index;source=read_data(root,archive,f'd/{name}.type');definition=parse_type(source.decode('cp1252'))
        codes=b''.join(expected_code(label,flags) for label,flags,_ in definition.clusters);default=0
        for frame,(_,flags,_) in enumerate(definition.clusters):
            if 'default' in flags.lower().split():default=frame
        props={key.lower():value for key,value in definition.props.items()};wx,wy=(int(props.get(key,1)) for key in ('foot_x','foot_y'))
        if wy==6:wy=8
        sources[number]=(codes,default,wx,wy);type_sha[name]=hashlib.sha256(source).hexdigest()
        if number in patterns:
            kind=patterns.index(number)
            for argument in range(COUNTS[kind]):
                for direction in (0,2,4,6):expected.add((number,argument,direction))
        else:expected.add((number,number,0))
    seen=set();cells=0
    # 최초 범위와 모든 칸의 원래/절삭 좌표·프레임/라벨/방향·finder 인자를 대조한다.
    for item in actual:
        key=(item['type'],item['argument'],item['direction'])
        if key not in expected or key in seen:raise AssertionError(f'자산 모양 입력 중복/범위 오류: {key}')
        seen.add(key);codes,default,wx,wy=sources[key[0]];kind=patterns.index(key[0]) if key[0] in patterns else 8
        case=dict(kind=kind,type=key[0],argument=key[1],direction=key[2],profile=0,x=bits(20.5),y=bits(21.25),footX=wx,footY=wy,explicit=0,alias=0,default=default,mutation=0)
        observed=[oracle.run_geometry(case,control,codes) for control in CONTROLS]
        if observed[0]!=observed[1]:raise AssertionError('실제 자산 모양 x87 차이')
        bounds,sequence=observed[0];expected_cells=[] if sequence=='-' else [[int(value) for value in cell.split(',')] for cell in sequence.split(';')]
        if item['bounds']!=[int(value) for value in bounds.split(',')] or item['cells']!=expected_cells:raise AssertionError(f'실제 자산 모양/finder 인자 불일치: {edition}/{key}')
        cells+=len(expected_cells)
    if seen!=expected or oracle.assertions:raise AssertionError('실제 자산 모양 누락/assert')
    return dict(types=len(order),cases=len(actual),valid_cells=cells,native_calls=dict(oracle.native_calls),
        area_entries=oracle.area_entries,finder_boundaries=oracle.finder_boundaries,assertions=0,os_calls=0,
        binary_sha256=oracle.sha256,type_sha256=type_sha)


def main():
    """콘솔 조회만 호출하고 실제 자산 대조 결과를 Git 제외 경로에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--out',type=Path,default=ROOT/'extracted/cpp-canongeometry-assets-report.json');args=parser.parse_args();results={}
    # 실제 두 원본 자료를 각각 검사한다.
    for edition in ('originals','originalCD'):
        results[edition]=check(args.exe,edition);print(f"{edition}: 자산 {results[edition]['types']}개·모양 조합 {results[edition]['cases']}개·유효 칸 {results[edition]['valid_cells']}개 통과",flush=True)
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-09',editions=results,
        limits=['모든 실제 자산의 기본 모양과 특수 패턴/네 정상 회전의 전체 모양/finder 인자 대조',
        '전체 decoder와 모양별 중간 사각형/snap/CRT 계산; finder 진입 전에 관찰 종료',
        '전체 MayPlace/finder 몸체/후보별 지형·관계/일반 Pop/GUI/게임/OS 실행 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
