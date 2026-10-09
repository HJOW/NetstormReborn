#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 판본의 모든 자산/패턴 배치 접두를 독립 파싱·원본 getter/접두로 대조한다."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from decomp_canonshape_oracle import ROOT,CanonShapeOracle,CONTROLS,bits
from decomp_typehotspot_oracle import HotspotOracle
from decomp_canonplacement_oracle import CanonPlacementOracle
from cpp_assets_smoke import original_order,read_data,expected_code,run
from cpp_canontype_smoke import NAMES,COUNTS
from taff import TaffArchive
from shp import read_blocks
from typefile import parse_type


def check(executable,edition):
    """C++ 출력의 키/기준점을 기대값으로 재사용하지 않고 실제 파일과 PE를 읽는다."""
    root=ROOT/edition;archive=TaffArchive(str(root/'netstorm.tarc'));order=original_order(edition);args=['--cd'] if edition=='originalCD' else []
    actual=[json.loads(line) for line in run(executable,'--inspect-canon-placement',root,*args).stdout.splitlines()]
    data=read_data(root,archive,'d/_shapes.shp');blocks=read_blocks(data);oracle=CanonShapeOracle(edition);hot=HotspotOracle(edition);prefix=CanonPlacementOracle(edition)
    patterns=[struct.unpack('<I',hot.mu.mem_read(address,4))[0] for address in oracle.s['patterns']]
    if patterns!=[70+order.index(name) for name in NAMES[:8]]:raise AssertionError('실제 패턴 전역/타입 순서 오류')
    sources={};expected=set();type_sha={};physical=0
    # 모든 자산 정의의 코드/defaultFrame/기준점과 물리 헤더를 독립적으로 읽는다.
    for index,name in enumerate(order):
        number=70+index;source=read_data(root,archive,f'd/{name}.type');definition=parse_type(source.decode('cp1252'))
        codes=b''.join(expected_code(label,flags) for label,flags,_ in definition.clusters);default=0
        for frame,(_,flags,_) in enumerate(definition.clusters):
            if 'default' in flags.lower().split():default=frame
        props={key.lower():value for key,value in definition.props.items()}
        converted=[hot.run(('XY',bits(props.get('hotfootratiox',0)),bits(props.get('hotfootratioy',0)),0,0),control) for control in CONTROLS]
        if converted[0]!=converted[1]:raise AssertionError('실제 기준점 x87 차이')
        headers=[struct.unpack_from('<4h',data,frame.offset-12) for frame in blocks[index].frames];physical+=len(headers)
        flags=sum(value for key,value in (('shadow',0x40000),('flyershadow',0x400000)) if key in [f.lower() for f in definition.flags])
        sources[number]=(codes,default,flags,converted[0],headers,0x02000000 if any(f.lower() in ('not_real','notreal') for f in definition.flags) else 0);type_sha[name]=hashlib.sha256(source).hexdigest()
        if number in patterns:
            kind=patterns.index(number)
            for argument in range(COUNTS[kind]):
                for direction in (0,2,4,6):expected.add((number,argument,direction))
        else:expected.add((number,number,0))
    seen=set();cache={}
    # 좌표/호출자/후반 반환은 콘솔 입력과 같은 독립 고정 프로필이다.
    positions=(0,1.99999,2,20.5,255,256,-1,20.5)
    # 타입/패턴마다 실제 getter를 먼저 실행하고 그 관찰을 접두의 모양 경계에 공급한다.
    for item in actual:
        key=(item['type'],item['argument'],item['direction']);profile=item['profile'];full=(*key,profile)
        if key not in expected or full in seen or profile not in range(8):raise AssertionError(f'자산 접두 입력 중복/범위 오류: {full}')
        seen.add(full);codes,default,flags,hotspots,headers,genus=sources[key[0]]
        if key not in cache:
            observed=[oracle.observe(*key,0,codes,default,flags,hotspots,headers,patterns,(bits(16),bits(11)),control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise AssertionError('실제 자산 getter x87 차이')
            cache[key]=observed[0]
        shape=cache[key]
        case=dict(type=key[0],argument=key[1],x=bits(positions[profile]),y=bits(21.25),owner=255 if profile==7 else 0x108,
            flags=key[2],mode=0,genus=genus,force=1 if profile==6 else 0,local=-1 if profile==7 else 8,
            left=shape[0],top=shape[1],right=shape[2],bottom=shape[3],lower=int(profile%2==0),change=0)
        values=[prefix.run(case,control) for control in CONTROLS]
        if values[0]!=values[1]:raise AssertionError('실제 자산 접두 x87 차이')
        native=values[0];geometry=[event for event in native[-1].split(';') if event.startswith('G:')]
        expected_values=(native[0],int(bool(geometry)),int(geometry[0].split(':')[1]) if geometry else 0,native[2])
        if tuple(item[k] for k in ('allowed','entered','local','blocked'))!=expected_values:raise AssertionError(f'실제 자산 접두 불일치: {edition}/{full}: {item} / {native}')
    if seen!={(*key,profile) for key in expected for profile in range(8)} or oracle.assertions or hot.assertions or prefix.assertions:raise AssertionError('실제 자산 접두 누락/assert')
    return dict(types=len(order),combinations=len(expected),cases=len(actual),physical_frames=physical,
        native_calls=dict(oracle.native_calls),prefix_native_calls=dict(prefix.native_calls),prefix_substitutions=dict(prefix.stub_calls),
        hot_native_calls=dict(hot.native_calls),assertions=0,os_calls=0,
        binary_sha256=oracle.sha256,shapes_sha256=hashlib.sha256(data).hexdigest(),type_sha256=type_sha)



def main():
    """클론 콘솔 조회만 호출하고 실제 파일 대조 결과를 Git 제외 경로에 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--out',type=Path,default=ROOT/'extracted/cpp-canonplacement-assets-report.json');args=parser.parse_args();results={}
    # 두 원본 판본의 자산 전체를 별도로 확인한다.
    for edition in ('originals','originalCD'):
        results[edition]=check(args.exe,edition);print(f"{edition}: 자산 {results[edition]['types']}개·접두 관찰 {results[edition]['cases']}개 통과",flush=True)
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-09',editions=results,
        limits=['실제 TYPE/SHP 독립 파싱과 전체 getter 관찰 뒤 접두의 모양 진입에 그 출력 공급',
        'SHP 주소는 분석용 재배치; 코드/물리 헤더/기준점은 실제 자료',
        '접두의 현재 genus에서 즉시 허용 비트만 TYPE에서 공급; 나머지 비트는 접두에 영향 없음',
        'getter와 접두는 별도 원본 실행; 접두의 모양 진입/후반 정책은 명시 대체',
        '전체 MayPlace/미리보기/충돌/지형/일반 Pop/GUI/게임/OS 실행 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
