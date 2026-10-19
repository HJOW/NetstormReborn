#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제157의 H=8과 한 프레임짜리162 삭제 정보를 원본 공통 pre/Destroy로 검증한다."""
import argparse
import json
import socket
import struct
import zlib
from pathlib import Path

from decomp_destroygraph_oracle import DestroyGraphOracle, case_input, TABLE, FLOOD, CODES, TYPES, POOL, CONTROLS, packed, sha, ROOT

FIXTURE=ROOT/'cpppj/tests/fixtures/island-graph-frame-x86.tsv'  # 기존 합성2프레임 fixture와 구별하는 새 원본 결과다.
REPORT=ROOT/'cpppj/island-graph-frame-native.json'  # 원본 정상 반환과 도달 경계를 고정한다.


def generate():
    """기존 기계어 실행기에 실제 프레임 수/번호만 입력하고 출력 전체를 먼저 동결한다."""
    assert not REPORT.exists()
    oracle=DestroyGraphOracle('originals')
    rows=[]
    # 두 x87 정밀도와 직접 pre/통합 Destroy를 각각 독립 입력으로 실행한다.
    for control in CONTROLS:
        oracle.start_lifecycle(control)
        # 직접 전처리와 종속 Unpop/반납까지 수행하는 통합 경로를 구별한다.
        for kind in ('Pre','Destroy'):
            # 기존 여섯 장부/관문 입력에 실제 프레임 배치만 공급한다.
            for case in (8,24,40,56,72,88):
                args,nodes,spots,records=case_input(case,kind)
                nodes[0][8]=8
                args[24]=2  # 보호 TYPE의 island 플래그다. 기존 합성 bomb 플래그8과 구별한다.
                oracle.prepare_case(args,nodes,spots,records)
                root=TYPES+157*oracle.creation['type_stride']
                fake=TYPES+162*oracle.creation['type_stride']
                oracle.mu.mem_write(root+0x114,struct.pack('<I',9))
                oracle.mu.mem_write(root+0x124,struct.pack('<I',CODES+0x300))
                oracle.mu.mem_write(CODES+0x300,b'AP\x01\0'*8+b'HP\x09\0')
                oracle.mu.mem_write(fake+0x114,struct.pack('<I',1))
                oracle.mu.mem_write(CODES+0x100,b'AP\x01\0'+b'\xcc'*64)
                oracle.invoke(oracle.d['pre' if kind=='Pre' else 'destroy'],[args[8]],POOL+oracle.root*oracle.stride,4)
                rows.append([kind,'originals',control,','.join(map(str,args)),*oracle.output(),oracle.bookkeeping_output(),oracle.context_output(),
                             packed(nodes),packed(spots),records.hex(),bytes(oracle.mu.mem_read(TABLE,255*6)).hex(),zlib.adler32(oracle.mu.mem_read(FLOOD,4096*4))])
    assert len(rows)==24 and not oracle.assertions
    content='# 실제157 H=8/9프레임, 치환162 island/1프레임; 공통 pre와 Destroy 전체 상태.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'
    # 보고서 작성이 실패한 첫 실행의 기대값이 있으면 바이트 일치만 확인하고 덮어쓰지 않는다.
    if FIXTURE.exists():
        assert FIXTURE.read_bytes()==content.encode('utf-8')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    paths={Path(__file__),FIXTURE,ROOT/'originals/Netstorm.exe'}
    # 실행기가 실제 읽은 모든 범위 표와 상속 도구를 기록한다. 사용하지 않은 과거 creation.c는 포함하지 않는다.
    paths.update(ROOT/'tools'/name for name in ('decomp_destroygraph_oracle.py','decomp_destroylifecycle_oracle.py','decomp_destroy_oracle.py','decomp_graphremove_oracle.py',
                 'decomp_rawgraph_oracle.py','decomp_graph_oracle.py','decomp_creation_oracle.py','decomp_sid_oracle.py','decomp_pop_oracle.py',
                 'decomp_display_oracle.py','decomp_postpop_oracle.py','decomp_surface_oracle.py','decomp_oracle.py','decomp_bridge_oracle.py'))
    paths.update(ROOT/f'extracted/{folder}/originals/functions.tsv' for folder in ('sid','creation','refined'))
    paths.update(path for path in oracle.body_paths if path.name=='functions.tsv')
    paths.update(ROOT/f'extracted/graphremove/originals{suffix}/functions.tsv' for suffix in ('','/geometry'))
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),cases=24,normalReturns=24,assertions=oracle.assertions,
        native=dict(oracle.native),lifecycle=dict(oracle.lifecycle_native),graph=dict(oracle.graph_native),substitutions=dict(oracle.substitutions),
        sha256={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(paths)},
        limits=['실제 공통 pre/Destroy와 Graph/finder/타입 getter; 합성 풀/이웃/장부, 실제 프레임 수9/1 및 H번호8',
                '가상 UI/소리/환급/로그는 기존 실행기의 외부 사건 경계; 게임/OS/창 실행 없음',
                '기존 destroygraph fixture는 변경하지 않음; frame8에서 임시 타입의 프레임 표를 무조건 해석해서는 안 됨']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('원본 정상 반환24/24',dict(oracle.graph_native))


def verify():
    """동결된 기대값을 다시 생성하지 않고 입력/도구/원본 바이트를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 과거 관측값을 바꾸지 않고 현재 입력 파일의 모든 핀을 대조한다.
    for path,value in report['sha256'].items():
        assert sha(ROOT/path)==value,path
    assert report['normalReturns']==24 and not report['assertions']
    print('섬 삭제 프레임 원본 근거 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    verify() if options.verify else generate()
