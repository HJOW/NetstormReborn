#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 대표 TYPE/SHP 요청자의 전체 MayPlace를 원본과 C++로 독립 대조한다. 주변은 합성 장면이다."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from decomp_canonmayplace_oracle import ROOT,MayPlaceOracle,CONTROLS,bits,inputs
from decomp_typehotspot_oracle import HotspotOracle
from cpp_assets_smoke import original_order,read_data,expected_code,run
from cpp_canontype_smoke import NAMES,COUNTS
from taff import TaffArchive
from shp import read_blocks
from typefile import parse_type
from fort import FLAG1_BITS,FLAG2_BITS

# 제한 검사의 대표 요청자와 원본 그룹 이름 표다.
SELECTED=('sunArcher','sunBlocker','sunFactory','priest','bridge','noIsland','island')
GROUPS=('archer','cannon','blocker','aviary','flyer','battery','fence','walker','balloon','misc')


def metadata(definition):
    """기존 독립 플래그 표와 원본 후처리 규칙으로 입력 필드를 파싱한다. 배치 결과는 계산하지 않는다."""
    props={k.lower():v for k,v in definition.props.items()};f1=f2=0
    # 파일의 플래그 단어를 기존 Python 표로 해석한다.
    for word in definition.flags:
        f1|=next((v for k,v in FLAG1_BITS.items() if k.lower()==word.lower()),0)
        f2|=next((v for k,v in FLAG2_BITS.items() if k.lower()==word.lower()),0)
    group=GROUPS.index(props['group'].lower()) if props.get('group','').lower() in GROUPS else 10 if 'group' not in props else -1
    wx,wy=int(props.get('foot_x',props.get('footx',1))),int(props.get('foot_y',props.get('footy',1)))
    if 'maxhitpoints' in props:f1|=0x10
    if any(props.get(k,0)!=0 for k in ('minusage','maxusage','maxrainbattleusage','maxthunderbattleusage','maxwindbattleusage')):f1|=0x4000
    if group==5:f2|=0x800
    if group in (0,1):f2|=0x8000000
    if group==2:f2|=0x4000000
    if f2&0x34200:f1|=0x20000
    if f2&0x2000:f1|=0x1000
    if f2&0xa00:f1|=0x10000000
    if f2&0x4000:f1|=0x4000000
    if f2&0x200:f1|=0x84000
    if not(f1&4):f1|=2
    if f2&0x40000:f2|=0x10
    if f2&0x40000 and not(f2&0x4200) and wx==3 and wy==3:f1|=6
    if f2&0x408100:f1|=2
    if f2&0x70000 and not(f2&0x4000):f1|=0x8000
    if wy==6:wy=8
    return f1,f2,group,(wx,wy),props


def check(executable,edition):
    """C++ 출력은 관찰 결과 비교에만 사용하고 원본 입력은 파일에서 별도로 읽는다."""
    root=ROOT/edition;archive=TaffArchive(str(root/'netstorm.tarc'));order=original_order(edition);args=['--cd'] if edition=='originalCD' else []
    actual=[json.loads(line) for line in run(executable,'--inspect-canon-mayplace',root,*args).stdout.splitlines()]
    data=read_data(root,archive,'d/_shapes.shp');blocks=read_blocks(data);oracle=MayPlaceOracle(edition);hot=HotspotOracle(edition)
    patterns=[struct.unpack('<I',oracle.mu.mem_read(address,4))[0] for address in oracle.gm['patterns']]
    if patterns!=[70+order.index(name) for name in NAMES[:8]]:raise AssertionError('실제 패턴 번호 불일치')
    expected=set();sources={};type_sha={}
    # 대표 7개 요청자의 모든 물리 프레임/코드/기준점과 배치 필드를 독립 파싱한다.
    for name in SELECTED:
        number=70+order.index(name);source=read_data(root,archive,f'd/{name}.type');definition=parse_type(source.decode('cp1252'))
        flags,genus,group,foot,props=metadata(definition);codes=b''.join(expected_code(label,flags) for label,flags,_ in definition.clusters);default=0
        # 논리 기본 프레임은 원문 선언 순서의 마지막 default를 따른다.
        for i,(_,words,_) in enumerate(definition.clusters):
            if 'default' in words.lower().split():default=i
        observed=[hot.run(('XY',bits(props.get('hotfootratiox',0)),bits(props.get('hotfootratioy',0)),0,0),control) for control in CONTROLS]
        if observed[0]!=observed[1]:raise AssertionError('실제 기준점 정밀도 차이')
        sources[number]=dict(number=number,codes=codes,default=default,group=group,foot=foot,hotspots=observed[0],
            metrics=[struct.unpack_from('<4h',data,f.offset-12) for f in blocks[number-70].frames],patterns=patterns,flags=flags,genus=genus)
        type_sha[name]=hashlib.sha256(source).hexdigest()
        if number in patterns:
            for argument in range(COUNTS[patterns.index(number)]):
                for direction in (0,2,4,6):
                    for profile in range(4):expected.add((number,argument,direction,profile))
        else:
            for profile in range(4):expected.add((number,number,0,profile))
    seen=set()
    # 동일 합성 장면에서 원본 전체 함수의 두 정밀도 관찰만 기대값으로 삼는다.
    for item in actual:
        key=tuple(item[k] for k in ('type','argument','direction','profile'));number,argument,direction,profile=key
        if key not in expected or key in seen:raise AssertionError('실제 자산 전체 배치 입력 중복/누락: '+str(key))
        seen.add(key);asset=sources[number];case=dict(inputs()[1],argument=argument,direction=direction,flags=asset['flags'],genus=asset['genus'],local=2 if profile==3 else 1,
            scene=2 if profile==3 else 4 if profile==1 else 0,ready=1,editor=int(profile==3),allies=int(profile!=3),relation=int(profile!=3),bypass=int(profile==3),restricted=int(profile==3),mode=0,
            x=bits(250 if profile==2 else 20.25 if profile==1 else 20),y=bits(250 if profile==2 else 21.5 if profile==1 else 21))
        values=[oracle.run(case,control,asset) for control in CONTROLS]
        if values[0]!=values[1]:raise AssertionError('실제 자산 전체 배치 x87 차이: '+str(key))
        native=values[0]
        if (item['allowed'],item['blocked'],item['preview'],item['regions'])!=tuple(native[:4]):raise AssertionError(f'실제 자산 전체 배치 불일치: {edition}/{key}: C++={item} PE={native[:4]}')
    if seen!=expected or oracle.assertions or hot.assertions:raise AssertionError('실제 자산 전체 배치 키/assert 오류')
    return dict(types=len(sources),cases=len(actual),native_calls=dict(oracle.native_calls),substitutions=oracle.substitutions,assertions=0,os_calls=0,
        binary_sha256=oracle.sha256,shapes_sha256=hashlib.sha256(data).hexdigest(),type_sha256=type_sha)


def main():
    """두 원본 판본의 제한 대표 자산 검사를 창 없이 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--out',type=Path,default=ROOT/'extracted/cpp-canonmayplace-assets-report.json');args=parser.parse_args();editions={}
    # 추가 PE의 자산은 별도 검증하지 않고 두 실제 자산 판본만 대조한다.
    for edition in ('originals','originalCD'):
        editions[edition]=check(args.exe,edition);print(f"{edition}: 실제 요청자 {editions[edition]['types']}개·전체 배치 {editions[edition]['cases']}개 통과",flush=True)
    args.out.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-09',editions=editions,
        limits=['대표 7개 실제 요청자 TYPE/프레임/SHP/기준점/패턴과 전체 MayPlace 정상 반환 대조','주변 지형/작업장/충돌자는 예약 내장 타입 10/11/12의 합성 장면',
            '임시 160바이트 확보/반납만 대체; 실제 자산 전수/맵 로더/일반 Pop/GUI 건설/게임/OS 미검증']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
