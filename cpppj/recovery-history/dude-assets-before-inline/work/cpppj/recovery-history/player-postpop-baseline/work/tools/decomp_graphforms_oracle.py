#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""내부 Form이 함께 있는 전체 그래프 재구성을 두 실제 원본 PE에서 독립 관찰한다."""
import argparse
import json
import struct
from pathlib import Path
from decomp_graphrebuild_oracle import RebuildOracle, REBUILD, PARENTS, verify as verify_parent
from decomp_graph_oracle import records_for
from decomp_rawgraph_oracle import TABLE, FLOOD
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL
from decomp_oracle import ROOT
from decomp_graphremove_oracle import digest

FIXTURE=ROOT/'cpppj/tests/fixtures/graphforms-x86.tsv'  # 전체 raw 슬롯/표/스택을 직접 대조할 독립 결과다.
REPORT=ROOT/'cpppj/recovery-graphforms-evidence.json'  # 양판본의 고유 입력과 두 정밀도 정상 반환을 분리한다.
FORM_TYPES=(6,14,24,46)  # 내용물/Burning/Path/Process의 내부 타입 번호를 합성 비표면 자료로 준비한다.
STATES=(0,2,4,8)  # live/dead/void/contained 상태를 각각 전체순회에 넣는다.
EDITIONS=('originals','originalCD')  # 각 실제 PE를 서로 독립 실행한다.
SEED=137  # 미사용 flood 스택 DWORD까지 같은 초기 값을 재생한다.


def input_case(oracle,number,state):
    """표면 하나와 내부 Form 여섯 개의 입력만 준비하며 그래프 정답은 계산하지 않는다."""
    nodes=[[20+i,20,1,1,0x800 if i==0 else 0,2,65,80,0,0 if i==0 else 4,0,254,0 if i==0 else 1] for i in range(7)]
    records=records_for([(oracle.ids[0],254,0)],0x7a00)
    oracle.prepare_delete(nodes,[],records,SEED,0,1)
    typ=TYPES+number*oracle.creation['type_stride']
    oracle.mu.mem_write(typ+0xe8,bytes(8))
    # 내부 Form의 좌표/프레임은 그래프가 읽을 필드가 아니다. NaN/stale 값으로 조기 생략을 확인한다.
    for sid in oracle.ids[1:]:
        address=POOL+sid*oracle.stride
        oracle.mu.mem_write(address+10,bytes((number,state)))
        oracle.mu.mem_write(address+14,struct.pack('<2I',0x7fc00123,0xff800000))
        oracle.mu.mem_write(address+(30 if oracle.edition=='originals' else 28),b'\xfa')
    return records


def slots(oracle):
    """할당한 일곱 슬롯의 전체 바이트를 SID 순서로 반환한다."""
    return ';'.join(bytes(oracle.mu.mem_read(POOL+sid*oracle.stride,oracle.stride)).hex() for sid in oracle.ids)


def generate(smoke):
    """동일 입력의 두 x87 결과를 확인하고 전체 raw 관찰만 저장한다."""
    verify_parent()
    rows=[]
    editions={}
    # 각 실제 PE의 Reset/Create와 전체풀 순회를 직접 실행한다.
    for edition in EDITIONS:
        oracle=RebuildOracle(edition)
        reference=[]
        controls=(0x027f,0x037f)
        # 정밀도를 달리한 반환은 같은 고유 입력 수에 다시 더하지 않는다.
        for control in controls:
            oracle.start_graph(control)
            observed=[]
            # 각 내부 타입/상태/reset을 동일 표면과 함께 독립 준비한다.
            for number in FORM_TYPES[:1] if smoke else FORM_TYPES:
                # 현재 live/dead/void/contained의 합성 raw 상태를 구분한다.
                for state in STATES[:1] if smoke else STATES:
                    # 부분 재구성과 전체 초기화 재구성 모두 같은 입력에서 실행한다.
                    for reset in (0,1):
                        before=input_case(oracle,number,state)
                        raw_before=slots(oracle)
                        oracle.invoke(REBUILD[edition],[reset])
                        row=[edition,str(reset),str(number),str(state),','.join(map(str,oracle.ids)),raw_before,slots(oracle),
                             before.hex(),bytes(oracle.mu.mem_read(TABLE,255*6)).hex(),str(SEED),
                             bytes(oracle.mu.mem_read(FLOOD,4096*4)).hex()]
                        observed.append('\t'.join(row))
            if control==controls[0]:reference=observed
            else:assert reference==observed,edition
            print(edition,hex(control),'정상 반환',len(observed),flush=True)
        assert oracle.assertions==0
        rows.extend(reference)
        editions[edition]=dict(normal_cases=len(reference),returns=len(reference)*2,assertions=oracle.assertions,
                               actual_calls=dict(oracle.rebuild_calls),capacity=oracle.capacity)
    if smoke:
        target=ROOT/'extracted/graphforms-smoke.json'
        target.write_text(json.dumps(dict(cases=len(rows),editions=editions),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        print('별도 smoke',len(rows),'입력; 완료 고유 수 추가0')
        return
    assert not REPORT.exists() and not FIXTURE.exists()
    assert len(rows)==64
    FIXTURE.write_text('# 실제 전체풀 Rebuild: 표면1/비표면 내부Form6, 두x87 동일반환, 대체효과0.\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__).resolve(),FIXTURE,ROOT/'cpppj/src/o/RawGraph.cpp',ROOT/'tools/decomp_graphrebuild_oracle.py',
           ROOT/'cpppj/recovery-graphrebuild-evidence.json',*[ROOT/'tools'/name for name in PARENTS]]
    # 원본 PE와 실제 실행한 재구성 범위는 현재 전체 바이트에 고정한다.
    for edition in EDITIONS:
        paths.append(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
        paths.extend(ROOT/f'extracted/graphrebuild/{edition}'/name for name in ('creation.c','functions.tsv'))
    data=dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',total=64,normal_total=64,assert_total=0,
              game_runs=0,external_boundaries=0,editions=editions,
              files={p.relative_to(ROOT).as_posix():digest(p) for p in paths},
              limitations=['내부Form raw 입력/타입flag0과 비공간 상태의 전체순회만 대조한다. 실제Form 생성/등록 완료 근거로 확대하지 않는다.',
                           '표면1과 내부Form6·정수표면/합성FrameCode·32768전체풀·비전투 null 큐이며 실제 캠페인 월드 전체는 후속이다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    verify()


def verify():
    """실행한 전체 원본/도구/출력 바이트와 고유 입력 수를 읽기 전용으로 감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'))
    assert data['total']==data['normal_total']==64 and data['assert_total']==data['external_boundaries']==data['game_runs']==0
    # 모든 실행 출처와 결과 전체 바이트를 확인한다.
    for name,value in data['files'].items():assert digest(ROOT/name)==value,name
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==64 and len({tuple(row[:4]) for row in rows})==64
    # 정밀도 반복 두 번은 정상반환 수이며 고유 입력 수는 판본별32다.
    for edition,values in data['editions'].items():
        assert values['normal_cases']==32 and values['returns']==64 and values['assertions']==0
    print('내부 Form 그래프 원본 정상64/assert0·전체 raw/표/스택 SHA 감사 통과')


# smoke의 입력은 별도 기록하며 완성 고유 입력이나 fixture에 합산하지 않는다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--smoke',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate(arguments.smoke)
