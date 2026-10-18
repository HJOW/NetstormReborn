#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 보호 SHP 헤더의 표시 기준점과 의식 목표점 WORD를 구별해 원본 Anchor/Point를 관측한다."""
import argparse,hashlib,itertools,json,socket,struct
from decomp_templeritual_oracle import TempleOracle,ROOT,TYPES,OUT,bits
from shp import read_blocks

FIXTURE=ROOT/'cpppj/tests/fixtures/ritual-shape-offsets-x86.tsv' # 기존 합성 의식 원문과 분리한 실제 헤더 입력이다.
REPORT=ROOT/'cpppj/ritual-shape-offsets.json' # 원본 실행 범위/보호 실제 SHP 출처다.
ACTUAL=0x1e000000 # 기존 합성 SHP/TYPE/풀과 겹치지 않는 두 실제 헤더 블록 저장소다.

def sha(path):
    """현재 파일 원문의 SHA256이다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    """기대 수식 없이 보호 헤더를 원본419850과 두 상위 helper에 공급한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    oracle=TempleOracle('originals');oracle.mu.mem_map(ACTUAL,0x20000);shape=ROOT/'originals/d/_shapes.shp';raw=shape.read_bytes();blocks=read_blocks(raw);rows=[];headers={}
    for type in (165,166): # 실제 로딩 순서의 두 블록에서 전체36바이트 헤더를 그대로 복사한다.
        block=blocks[type-70];base=ACTUAL+(type-165)*0x10000
        oracle.put(base,0x30312e31);oracle.put(base+4,len(block.frames))
        for frame,source in enumerate(block.frames): # 픽셀/RLE는 이 두 좌표 helper가 읽지 않는다.
            offset=0x1000+frame*64;oracle.put(base+8+frame*8,offset);oracle.mu.mem_write(base+offset-36,raw[source.offset-36:source.offset])
            headers[f'{type}:{frame}']=raw[source.offset-36:source.offset].hex()
    for stage,spot,position in itertools.product(range(5),(0,6,8),((90.0,108.0),(40.75,41.25))): # 단계/지면보정/정수·소수 부모 입력을 교차한다.
        outputs=[]
        for precision in (0x27f,0x37f): # 같은 실제 헤더를 두 x87 정밀도로 실행한다.
            oracle.prepare(('main',3,'A',0,1,1,spot,0,0),precision)
            oracle.put(oracle.slot(5)+14,bits(position[0]));oracle.put(oracle.slot(5)+18,bits(position[1]))
            for type in (165,166): # TYPE의 SHP 포인터만 실제 헤더 블록으로 교체한다. 나머지 입력 범위는 기존 선언과 같다.
                oracle.put(TYPES+type*oracle.type_stride+0xdc,ACTUAL+(type-165)*0x10000)
            oracle.call('anchor',[OUT,stage],oracle.slot(5),8,precision);anchor=bytes(oracle.mu.mem_read(OUT,8)).hex()
            oracle.call('point',[OUT,stage],oracle.slot(5),8,precision);point=bytes(oracle.mu.mem_read(OUT,8)).hex();outputs.append((anchor,point))
        assert outputs[0]==outputs[1];rows.append('\t'.join((str(stage),str(spot),str(bits(position[0])),str(bits(position[1])),*outputs[0])))
    fixture=('# stage\tspot\txBits\tyBits\tanchorFloatBytes\tpointFloatBytes\n'+'\n'.join(rows)+'\n').encode('utf-8')
    paths={shape,ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'tools/decomp_ritual_shape_offsets.py',ROOT/'tools/decomp_templeritual_oracle.py',ROOT/'tools/shp.py',*oracle.paths}
    report=dict(edition='10.78',host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',uniqueInputs=30,normalAnchorPointReturns=120,precisionDifferences=0,headers=headers,scope='실제 보호 SHP36바이트/419850/447aa0/447b30. 원본 TYPE parser/픽셀/전체 게임 검증은 아니다. 기존 입력 준비용 호출 수는 새120정상반환에 합산하지 않는다.',sha256={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(paths)})
    report['sha256'][FIXTURE.relative_to(ROOT).as_posix()]=hashlib.sha256(fixture).hexdigest()
    if args.verify:assert FIXTURE.read_bytes()==fixture and json.loads(REPORT.read_text('utf-8'))==report
    else:FIXTURE.write_bytes(fixture);REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n')
    print('실제 의식 SHP',len(rows),'입력/120정상 반환, 기준점과목표점필드 분리')
    print('Dais3 표시/목표',struct.unpack_from('<hh',bytes.fromhex(headers['165:3']),28),struct.unpack_from('<hh',bytes.fromhex(headers['165:3']),32))

if __name__=='__main__':main()
