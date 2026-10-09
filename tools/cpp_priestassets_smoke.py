#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 원본 자산을 독립 parser로 읽고 C++ 배치 자료와 실제 PE의 사제 픽셀 getter를 대조한다."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT, TYPES, STACK, STOP
from decomp_typehotspot_oracle import HotspotOracle, CONTROLS, bits
from decomp_priestshape_oracle import ShapeOracle, SHP
from decomp_priestgeometry_oracle import OUT, CODES
from cpp_assets_smoke import read_data, original_order, expected_code, run
from taff import TaffArchive
from shp import read_blocks
from typefile import parse_type
from unicorn.x86_const import UC_X86_REG_ECX


class AssetShapeOracle(ShapeOracle):
    """이전 getter 실행기를 재사용하되 모든 물리 헤더와 코드를 실제 자산 자료로 공급한다."""
    def run_asset(self,number,codes,default,flags,hotspots,headers,patterns,control):
        """전체 getter의 정상 반환/ABI/x87와 여섯 출력 비트를 관찰한다. 픽셀 데이터는 조회하지 않는다."""
        mu=self.mu;s=self.shape;typ=TYPES+number*self.type_stride;self.instructions=0;self.write_ranges=[(OUT,OUT+24)]
        # 실제 자산의 offset 표를 대신하는 분석용 주소 표다. 각 헤더 값/물리 순서는 그대로 유지한다.
        start=8+len(headers)*8;size=(start+len(headers)*0x30+4095)&~4095
        if size>0x1000:mu.mem_map(SHP+0x1000,size-0x1000)
        mu.mem_write(SHP,bytes(size));mu.mem_write(CODES,codes)
        mu.mem_write(typ,bytes(self.type_stride));mu.mem_write(typ+0xdc,struct.pack('<I',SHP))
        mu.mem_write(typ+0xe8,struct.pack('<I',flags));mu.mem_write(typ+0x114,struct.pack('<Ii',len(codes)//4,default))
        mu.mem_write(typ+0x124,struct.pack('<I',CODES));mu.mem_write(typ+s['hot'],struct.pack('<2i',*hotspots))
        mu.mem_write(s['scale'],struct.pack('<2f',16,11))
        # 패턴 식별 전역도 PE에 저장된 실제 타입 번호를 공급한다.
        for address,value in zip(self.s['patterns'],patterns,strict=True):mu.mem_write(address,struct.pack('<I',value))
        # 기존 합성 헤더 대신 모든 실제 물리 프레임의 signed WORD 메타 자료를 넣는다.
        for index,header in enumerate(headers):
            offset=start+index*0x30;mu.mem_write(SHP+8+index*8,struct.pack('<II',offset+0x24,0))
            mu.mem_write(SHP+offset+0x18,struct.pack('<4h',*header))
        mu.mem_write(OUT,bytes([0xa5])*24);self.prepare_registers(control);mu.reg_write(UC_X86_REG_ECX,0)
        mu.mem_write(STACK,struct.pack('<8I',STOP,OUT,number,number,0,OUT+16,OUT+20,0))
        mu.emu_start(s['entry'],STOP,count=20000);self.check(control,STACK+4)
        result=list(struct.unpack('<6I',mu.mem_read(OUT,24)))
        if size>0x1000:mu.mem_unmap(SHP+0x1000,size-0x1000)
        return result


def check(executable,edition):
    """원본 파일을 읽기만 하고 C++ CLI와 원본 숫자 변환/픽셀 getter에 동일 자료를 공급한다."""
    root=ROOT/edition;archive=TaffArchive(str(root/'netstorm.tarc'));data=read_data(root,archive,'d/_shapes.shp')
    blocks=read_blocks(data);order=original_order(edition);args=['--cd'] if edition=='originalCD' else []
    actual=json.loads(run(executable,'--inspect-priest-assets',root,*args).stdout);hot=HotspotOracle(edition);shape=AssetShapeOracle(edition)
    patterns=[struct.unpack('<I',hot.mu.mem_read(address,4))[0] for address in shape.s['patterns']]
    if actual['patterns']!=patterns or len(actual['types'])!=len(order):raise AssertionError('패턴 전역/자산 개수 불일치')
    physical=0;measurements=[]
    # 각 타입의 프레임 코드/기본 프레임/기준점은 독립 Python parser와 실제 숫자 변환으로 대조한다.
    for index,(name,item) in enumerate(zip(order,actual['types'],strict=True)):
        source=read_data(root,archive,f'd/{name}.type');definition=parse_type(source.decode('cp1252'));props={k.lower():v for k,v in definition.props.items()}
        codes=b''.join(expected_code(cluster,flags) for cluster,flags,_ in definition.clusters)
        default=0
        # 원본은 마지막 default 클러스터를 사용한다.
        for frame,(_,flags,_) in enumerate(definition.clusters):
            if 'default' in flags.lower().split():default=frame
        converted=[hot.run(('XY',bits(props.get('hotfootratiox',0)),bits(props.get('hotfootratioy',0)),0,0),control) for control in CONTROLS]
        if converted[0]!=converted[1] or item['hotspot']!=list(converted[0]):raise AssertionError(f'기준점 불일치: {edition}/{name}')
        if (item['number'],item['name'],item['default'],item['codes'],item['physical'])!=(70+index,name,default,codes.hex(),len(blocks[index].frames)):
            raise AssertionError(f'프레임 메타 자료 불일치: {edition}/{name}')
        physical+=len(blocks[index].frames)
        if 'priest' in [flag.lower() for flag in definition.flags]:
            headers=[struct.unpack_from('<4h',data,frame.offset-12) for frame in blocks[index].frames]
            flags=sum(value for key,value in (('shadow',0x40000),('flyershadow',0x400000)) if key in [f.lower() for f in definition.flags])
            observed=[shape.run_asset(70+index,codes,default,flags,converted[0],headers,patterns,c) for c in CONTROLS]
            if observed[0]!=observed[1] or item.get('shape_bits')!=observed[0]:raise AssertionError(f'실제 사제 픽셀 getter 불일치: {edition}/{name}')
            measurements.append(dict(type=70+index,name=name,default=default,hotspot=list(converted[0]),shape_bits=observed[0]))
    if not measurements or hot.assertions or shape.assertions:raise AssertionError('사제 실제 관찰 누락/assert 도달')
    return dict(types=len(order),physical_frames=physical,patterns=patterns,priests=measurements,
        hot_native_calls=dict(hot.native_calls),shape_native_calls=dict(shape.native_calls),assertions=0,os_calls=0,
        binary_sha256=hot.sha256,shapes_sha256=hashlib.sha256(data).hexdigest())


def main():
    """두 판본의 실제 자산 검사를 실행하고 Git 제외 경로에 결과를 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,default=ROOT/'cpppj/build/bin/Release/NetstormCpp.exe')
    parser.add_argument('--out',type=Path,default=ROOT/'extracted/cpp-priestassets-report.json');args=parser.parse_args();results={}
    # C++ 실행 파일은 콘솔 자료 조회 모드만 호출한다.
    for edition in ('originals','originalCD'):
        results[edition]=check(args.exe,edition);print(f"{edition}: 자산 {results[edition]['types']}개·사제 getter 대조 통과",flush=True)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-09',editions=results,
        limits=['실제 파일의 독립 파싱과 원본 숫자 변환 구간/전체 사제 getter 대조','SHP 주소 표는 분석용 재배치, 물리 헤더/프레임 순서는 실제 자산 그대로','전체 자산 로더/MayPlace/일반 Pop/GUI/게임/OS 실행 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
