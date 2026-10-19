#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 SHP의 실제 사제/골렘 헤더를 원본48bb40에 연결해 보행 결과를 동결한다."""
import argparse
import json
import socket
import struct
from pathlib import Path

from decomp_pathruntime_oracle import PathOracle, ROOT, ARENA, PATH, POOL, PARENT, TYPES, dbits, bits, digest
from shp import read_blocks, load_type_texts, read_clusters

FIXTURE=ROOT/'cpppj/tests/fixtures/walk-shape-x86.tsv'  # 기존 합성 보행 기대값을 유지하는 독립 실제 자산 대조다.
REPORT=ROOT/'cpppj/walk-shape-native.json'  # 사용한 원본과 관측 도구의 바이트를 보존한다.


def main():
    """게임이나 창을 열지 않고 실제 프레임의 보행과 가상 공간 요청을 관측한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if options.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        # 저장 기대값은 다시 쓰지 않고 출처 파일 전체와 대조한다.
        for name,value in record['sha256'].items():
            assert digest(ROOT/name)==value,name
        assert record['inputs']==1170 and record['normalReturns']==1170
        print('실제 사제/골렘 SHP 보행1170입력 출처 통과')
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    data=(ROOT/'originals/d/_shapes.shp').read_bytes()
    blocks=read_blocks(data)
    texts=load_type_texts(str(ROOT/'originals'))
    oracle=PathOracle('originals')
    rows=[]
    # 원본 두 타입의 모든 논리 프레임을 각각 초기 입력으로 사용한다.
    for kind,name in ((158,'priest'),(133,'sunwalker')):
        clusters=read_clusters(texts[name])
        block=blocks[kind-70]
        # 한 걸음보다 작은 거리부터 충분히 진행한 거리까지 원본 판정을 관측한다.
        for frame in range(len(clusters)):
            # 시각은 명시 입력이며 원본 Step/프레임 계산을 대체하지 않는다.
            for now in (0.01,0.03,0.1,0.25,0.5):
                oracle.reset(1)
                raw=POOL+PARENT*oracle.s['stride']
                oracle.put(raw+10,kind,1)
                oracle.put(raw+36,frame)
                shape=ARENA+0x18000
                codes=ARENA+0x19000
                typ=TYPES+kind*oracle.s['type_stride']
                oracle.put(typ+0xdc,shape)
                oracle.put(typ+0x114,len(clusters))
                oracle.put(typ+0x124,codes)
                oracle.put(typ+0x6c,321)
                # 실제36바이트 추가 헤더를 통째로 공급하며 +8/+c와 +10/+14를 교환하지 않는다.
                for index,cluster in enumerate(clusters):
                    header=shape+0x2000+index*0x40
                    offset=block.frames[index].offset
                    oracle.mu.mem_write(header,data[offset-36:offset])
                    oracle.put(shape+8+index*8,header-shape+36)
                    oracle.put(codes+index*4,ord(cluster[0][0]),1)
                    oracle.put(codes+index*4+2,int(cluster[0][1:]),1)
                # 원본 letterRun의 첫 번호와 개수는 실제 TYPE 클러스터에서 읽은 입력이다.
                for direction in range(8):
                    indices=[i for i,c in enumerate(clusters) if c[0][0]==chr(ord('A')+direction)]
                    oracle.put(typ+0x130+direction*4,indices[0])
                    oracle.put(typ+0x170+direction*4,len(indices))
                oracle.path([(10,20),(15,20),(17,22)],[0,0,2],[0,1,3])
                oracle.put(PATH+oracle.m['current'],1)  # 첫 꼭짓점 선택을 끝낸 구간 안의 보행 판정을 실행하는 입력이다.
                oracle.put(oracle.m['now'],dbits(now),8)
                oracle.invoke(oracle.m['run'][1],this=PATH)
                fields,events=oracle.observed()
                rows.append([kind,frame,dbits(now),fields,events])
    assert len(rows)==1170
    FIXTURE.write_text('# type frame nowBits observedFields observedEvents\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n','utf-8')
    paths={Path(__file__),FIXTURE,ROOT/'tools/decomp_pathruntime_oracle.py',ROOT/'tools/decomp_reward_oracle.py',ROOT/'tools/shp.py',
           ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'originals/d/_shapes.shp'}
    paths.update(ROOT/f'extracted/{name}/originals/functions.tsv' for name in ('pathruntime','pathruntimehelpers','pathmath'))
    record=dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',inputs=len(rows),normalReturns=len(rows),
                scope='실제48bb40/Step/프레임과 보호SHP 전체36바이트. 경로/시각은 명시 입력이며 공간 가상 효과/음향/할당은 기존 관측 경계다. CD와 실제 게임 화면은 별도다.',
                sha256={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8')
    print(json.dumps(dict(inputs=len(rows),normalReturns=len(rows))))


if __name__=='__main__':
    main()
