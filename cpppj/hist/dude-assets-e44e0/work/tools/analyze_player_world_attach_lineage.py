#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""같은 Player1680 입력의 실제 FormAttach 후속 관찰과 최초 전체 자료를 연결한다."""
import argparse
import ast
import hashlib
import json
from pathlib import Path

# 최초 실제 Player4몸체/Regularctor와 명시 attach 경계의 전체 출처를 보존한 경로다.
ROOT=Path(__file__).resolve().parent.parent
HISTORY='cpppj/hist/player-world-f6101'
REPORT=ROOT/'cpppj/player-world-attach-lineage.json'
NATIVE='cpppj/player-world.json'
FIXTURE='cpppj/tests/fixtures/player-world-x86.tsv'
TOOL='tools/decomp_player_world_oracle.py'


def digest(path):
    """출처 전체 바이트의 SHA256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pin(name):
    """경로/길이/전체 SHA를 보존한다."""
    path=ROOT/name;return dict(path=name,bytes=path.stat().st_size,sha256=digest(path))


def rows(path):
    """기대값을 계산하지 않고 원본 관찰 행을 그대로 읽는다."""
    return [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]


def audit():
    """최초 출처/고유 입력/보존 필드와 실제 Form 연결에 따른 부모 WORD 하나만 대조한다."""
    manifest=json.loads((ROOT/HISTORY/'manifest.json').read_text(encoding='utf-8'))
    # 최초 whole6 자료를 다시 해시하며 당시 source/fixture/report 출처를 덮어쓰지 않는다.
    for name,expected in manifest['files'].items():assert digest(ROOT/HISTORY/name)==expected,name
    old=json.loads((ROOT/HISTORY/NATIVE).read_text(encoding='utf-8'));current=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'))
    # 최초 보고서의 모든 PE/자산/정밀 범위/도구를 당시 전체 파일 또는 불변 현재 파일로 연결한다.
    for name,expected in old['files'].items():
        path=ROOT/HISTORY/name if name in manifest['files'] else ROOT/name;assert digest(path)==expected,name
    # 현재 전체 actualAttach 관찰 출처도 정확히 감사한다.
    for name,expected in current['files'].items():assert digest(ROOT/name)==expected,name
    before=rows(ROOT/HISTORY/FIXTURE);after=rows(ROOT/FIXTURE);assert len(before)==len(after)==1680
    stable=attached=0
    # 같은 입력의 추가 Kernel/Form 상태는 최초 fixture에 없었으며 새 고유 입력으로 세지 않는다.
    for left,right in zip(before,after,strict=True):
        assert len(left)==18 and len(right)==22 and left[:2]==right[:2] and left[-1]==right[-1]
        if 'B:' not in left[3]:assert left[:-1]==right[:17];stable+=1
        else:
            attached+=1;assert left[:5]==right[:5] and left[6:17]==right[6:17]
            oldRaw=bytes.fromhex(left[5]);newRaw=bytes.fromhex(right[5]);assert oldRaw[:6]==newRaw[:6] and oldRaw[8:]==newRaw[8:]
            assert oldRaw[6:8]==b'\0\0' and int.from_bytes(newRaw[6:8],'little')==int(right[17])==6 and right[19]=='1'
            assert bytes.fromhex(right[18])[10]==46
    assert (stable,attached)==(1600,80) and current['normal_total']==old['normal_total']==1452 and current['assert_total']==old['assert_total']==228
    # 입출력 격자의 함수 AST 자체가 최초 도구와 같음을 확인한다.
    def input_ast(path):
        """성능/실제 부착 변경과 별도로 고정된 독립 입력 함수를 읽는다."""
        tree=ast.parse(path.read_text(encoding='utf-8'));return ast.dump(next(node for node in tree.body if isinstance(node,ast.FunctionDef) and node.name=='inputs'),include_attributes=False)
    assert input_ast(ROOT/HISTORY/TOOL)==input_ast(ROOT/TOOL)
    # 새 상위 부모의 두 읽기 접근자를 제거하면 최초 Player 생산 전체 파일로 정확히 돌아간다.
    header='cpppj/src/o/RawPlayerWorld.h';cpp='cpppj/src/o/RawPlayerWorld.cpp'
    addedHeader='    // 상위 실제 캠페인 부모가 같은9기록을 이름/지역 초기화에 직접 사용한다.\n    std::span<std::uint8_t> Records() const;\n    // 상위 부모가 매 하위 호출 직전 같은 현재 두owner/모드를 다시 조회한다.\n    PlayerWorldState& State() const;\n'
    addedCpp='// 실제 Player 저장소의 같은 BYTE 뷰이며 새 vector/기록 사본을 만들지 않는다.\nstd::span<std::uint8_t> RawPlayerWorld::Records() const { return players_; }\n// 최초예약/getter가 읽는 현재 전역을 상위 부모에도 같은 참조로 제공한다.\nPlayerWorldState& RawPlayerWorld::State() const { return state_; }\n'
    for name,added in ((header,addedHeader),(cpp,addedCpp)):
        text=(ROOT/name).read_text(encoding='utf-8');assert text.count(added)==1 and text.replace(added,'').encode()==(ROOT/HISTORY/name).read_bytes(),name
    return manifest,current


def record():
    """현재 후속 관찰과 최초 자료6개/기존 전체 PE 범위를 별도 보고서로 최초 연결한다."""
    if REPORT.exists():raise RuntimeError('Player 실제 attach lineage 덮어쓰기 금지')
    manifest,current=audit();saved=[pin((path.relative_to(ROOT)).as_posix()) for path in sorted((ROOT/HISTORY).rglob('*')) if path.is_file()]
    proof=dict(host='HJOW-Athlon',date='2026-10-14',preserved_unique=1680,normal_unique=1452,assert_unique=228,new_unique=0,
        same_non_attach_rows=1600,same_attach_inputs=80,changed_native_boundary='명시 FormAttach를 실제 BaseProcess/Kernel/FormFactory/부모 attach/Pop로 실행',
        prior_whole_files=saved,current_files=[pin(name) for name in (TOOL,NATIVE,FIXTURE,'cpppj/src/o/RawPlayerWorld.h','cpppj/src/o/RawPlayerWorld.cpp')],tool=pin('tools/analyze_player_world_attach_lineage.py'),
        limitations=['기존 네Player몸체/Regular 초기시각·event·payload·count/전체Player/공간/장부/진단은 동일하다. 실제부착80행의부모WORD6만새Form6으로바뀐다.',
            '현재 fixture는 실제Form 전체raw·Kernel PID·rotation을 추가한다. 최초fixture 전체 바이트와 당시 source/report를 보존한다.',
            '힙예약/진단보고는명시경계다. 새로운CPP현재9기록/두owner 접근자만정확역제거로첫source에연결한다. 게임/GUI/소켓실행0이다.',
            '전수재관찰/두x87/중단된미완료실행은새고유수에합산하지않는다. CPP전수검사는공유빌드뒤별도확인한다.'])
    REPORT.write_text(json.dumps(proof,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """최초 whole자료와 현재 exact successor/동일 입력 분류를 감사한다."""
    proof=json.loads(REPORT.read_text(encoding='utf-8'));audit()
    # 모든 보존/현재 전체 파일의 실제 바이트를 연결한다.
    for item in [*proof['prior_whole_files'],*proof['current_files'],proof['tool']]:assert pin(item['path'])==item,item['path']
    print('Player actualAttach:1680 동일입력·1600행불변·80행부모WORD6·생산접근자역변환 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true')
    if parser.parse_args().record:record()
    verify()
