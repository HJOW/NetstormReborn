#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""커서의 임시0/음수/경계 좌표에서 원본 발자국과 첫 이웃의 정상 반환을 관측한다."""
import argparse,json,socket,struct
from pathlib import Path
from decomp_neighbor_oracle import NeighborOracle,BRIDGE,BRIDGE_TYPE,TYPES,FINDER,base_case,float_bits,ROOT,digest
from unicorn.x86_const import UC_X86_REG_EAX

FIXTURE=ROOT/'cpppj/tests/fixtures/neighbor-boundary-x86.tsv' # 새 기하 수정 전에 동결할 출력이다.
REPORT=ROOT/'cpppj/neighbor-boundary.json' # 실제 호출/정밀도와 원본 출처다.

# 정상지도·미리보기 초기점·부분0·소수·지도 밖의 유한 입력을 구별한다.
def inputs():return [(x,y,width) for x,y in ((0,0),(0,20),(20,0),(.5,.5),(.5,20),(20.75,21.9),(-2.5,20),(300,20),(255.5,255.5)) for width in (1,3)]

# 원본 기존 탐색 실행기에 발자국 진입점만 추가한다. 계산/현재TYPE/조회는 원문이다.
def generate():
    oracle=NeighborOracle('originals');oracle.spec['entries']['Footprint']=0x4ade70;rows=[]
    for x,y,width in inputs():
        outputs=[]
        # 같은 입력을 두 정밀도에서 새로 설치하고 모든 출력 바이트를 읽는다.
        for control in (0x27f,0x37f):
            case=base_case();nodes=[(BRIDGE,BRIDGE_TYPE,0,8,float_bits(20),float_bits(21),width,width,0x800,4,0,0)]
            oracle.prepare_space(case,nodes,[]);oracle.mu.mem_write(oracle.slot(BRIDGE)+14,struct.pack('<ff',x,y));destination=FINDER+0x180
            oracle.call('Footprint',[destination,0],oracle.slot(BRIDGE),8,control,returns_float=False);foot=bytes(oracle.mu.mem_read(destination,16))
            oracle.mu.mem_write(FINDER,bytes(0x200));oracle.call('Find',[BRIDGE,0],FINDER,8,control,returns_float=False);first=struct.unpack('<I',oracle.mu.mem_read(FINDER+0x34,4))[0]
            outputs.append((foot.hex(),str(first)))
        assert outputs[0]==outputs[1],(x,y,width);rows.append([str(float_bits(x)),str(float_bits(y)),str(width),*outputs[0]])
    FIXTURE.write_text('# x비트 y비트 발자국크기 원본사각형16BYTE 첫이웃\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_neighbor_oracle.py',ROOT/'tools/decomp_bridgeevent_oracle.py',ROOT/'originals/Netstorm.exe',FIXTURE,*oracle.exports]
    report=dict(host=socket.gethostname(),date='2026-10-18',rows=len(rows),normalReturns=len(rows)*4,controls=[0x27f,0x37f],precisionDifferences=0,sha256={p.relative_to(ROOT).as_posix():digest(p) for p in paths},limits=['10.78 실제4ade70/49ae80과4b23e0 전체의 유한 임시 좌표 입력이다. 원래20/21에 준비한 같은 raw source의 좌표만 덮어쓰며 다른 후보는 없다. 미등록 커서 미리보기의 발자국/빈 첫 이웃 경계이며 비유한 좌표나 모든 탐색 후보 조합의 증거는 아니다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('임시 좌표 원본 관측',len(rows),report['normalReturns'])

# 관측 출력을 재생성하지 않고 모든 원문 SHA/행 수를 확인한다.
def verify():
    report=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
    assert len(rows)==report['rows']==len(inputs()) and report['normalReturns']==len(rows)*4 and not report['precisionDifferences']
    print('임시 좌표 동결/SHA 감사',len(rows))

# 명시 CLI 요청에서만 새 관측 파일을 쓴다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
