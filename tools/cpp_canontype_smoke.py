#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 타입 프레임/발자국/defaultFrame으로 C++ 전체 패턴 순회와 원본 PE 전체 decoder를 대조한다."""
import argparse
import hashlib
import json
from pathlib import Path
from decomp_canontype_oracle import ROOT,CanonTypeOracle,TYPE_IDS,COUNTS,CONTROLS,bits
from cpp_assets_smoke import original_order,read_data,expected_code,run
from taff import TaffArchive
from typefile import parse_type

# 실제 타입 전역의 파일 이름이다. 마지막 사제는 비패턴 경로다.
NAMES=('puzzlePiece','bridge','island','noIsland','thunderCannon','rainCannon','windArcher','windBlocker','priest')


def check(executable,edition):
    """독립 parser 자료를 원본 코드에 공급하고 C++의 모든 칸/끝 상태/범위를 비교한다."""
    root=ROOT/edition;archive=TaffArchive(str(root/'netstorm.tarc'));order=original_order(edition);args=['--cd'] if edition=='originalCD' else []
    actual=[json.loads(line) for line in run(executable,'--inspect-canon-patterns',root,*args).stdout.splitlines()]
    oracle=CanonTypeOracle(edition);sources={};types=[]
    # C++ 타입 번호/프레임 코드를 재사용하지 않고 실제 로딩 순서/정의에서 입력을 만든다.
    for name in NAMES:
        number=70+order.index(name);source=read_data(root,archive,f'd/{name}.type');definition=parse_type(source.decode('cp1252'))
        codes=b''.join(expected_code(label,flags) for label,flags,_ in definition.clusters);default=0
        # 마지막 default 지정이 원본의 현재 기본 프레임이다.
        for frame,(_,flags,_) in enumerate(definition.clusters):
            if 'default' in flags.lower().split():default=frame
        props={key.lower():value for key,value in definition.props.items()};wx,wy=(int(props.get(key,1)) for key in ('foot_x','foot_y'))
        if wy==6:wy=8
        types.append((number,codes,default,wx,wy));sources[name]=hashlib.sha256(source).hexdigest()
    if tuple(t[0] for t in types)!=TYPE_IDS:raise AssertionError('실제 타입 로딩 번호 차이')
    expected_count=(sum(COUNTS)+1)*4
    if len(actual)!=expected_count:raise AssertionError('실제 패턴 검사 행 수 오류')
    cells=0;seen=set()
    # 원본 getter/생성/전체 진행은 합성 fixture와 동일한 실행 감시로 두 정밀도에서 실행한다.
    for item in actual:
        kind=item['kind'];number,codes,default,wx,wy=types[kind];key=(kind,item['argument'],item['direction'])
        if key in seen or item['type']!=number or item['direction'] not in (0,2,4,6):raise AssertionError('실제 패턴 입력 중복/타입/방향 오류')
        seen.add(key)
        if not (0<=item['argument']<(COUNTS[kind] if kind<8 else 0)) and not (kind==8 and item['argument']==158):raise AssertionError('실제 패턴 인자 오류')
        case=dict(kind=kind,argument=item['argument'],direction=item['direction'],profile=0,x=bits(20.5),y=bits(21.25),footX=wx,footY=wy,explicit=0,alias=0,default=default)
        observed=[oracle.run(case,c,codes) for c in CONTROLS]
        if observed[0]!=observed[1]:raise AssertionError('실제 자산 decoder x87 차이')
        value=observed[0];sequence=[] if value[2]=='-' else [[int(x) for x in cell.split(',')] for cell in value[2].split(';')]
        if item['bounds']!=[int(x) for x in value[1].split(',')] or item['sequence']!=sequence or item['end']!=[value[3],value[5],value[6],value[7]] or item['end_bounds']!=[int(x) for x in value[8].split(',')]:
            raise AssertionError(f'실제 자산 decoder 불일치: {edition}/{key}')
        cells+=len(sequence)
    if oracle.assertions:raise AssertionError('실제 자산의 누락 프레임/assert 도달')
    return dict(cases=len(actual),valid_cells=cells,types=list(TYPE_IDS),native_calls=dict(oracle.native_calls),assertions=0,os_calls=0,
        binary_sha256=oracle.sha256,type_sha256=sources,limits=['짝수 방향/현재 프레임·발자국의 실제 자산 대조','게임/OS/전체 MayPlace/일반 Pop/GUI 미실행'])


def main():
    """클론의 콘솔 모드만 호출하고 두 판본 원본 자료 대조 결과를 Git 제외 경로에 쓴다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--out',type=Path,default=ROOT/'extracted/cpp-canontype-assets-report.json');args=parser.parse_args();results={}
    # 두 판본의 실제 자산 전체 패턴을 각각 검증한다.
    for edition in ('originals','originalCD'):
        results[edition]=check(args.exe,edition);print(f"{edition}: 실제 자산 decoder {results[edition]['cases']}개·칸 {results[edition]['valid_cells']}개 통과",flush=True)
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-09',editions=results),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
