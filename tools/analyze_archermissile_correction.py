#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사전 tool 출력의 실제 SHA와 ctor/충돌 사운드 교정의 역변환을 감사한다."""
import argparse
import hashlib
import json
from pathlib import Path
from decomp_owner_oracle import ROOT, digest

# root가 첫 C++ 대조 전에 읽은 실제 보고서의 SHA다. 이전 전체 보고서는 보존하지 못했다.
OBSERVED={
    'cpppj/tests/fixtures/archermissile-x86.tsv':'3ac77dbd052c7a7fd7df1be9ed18fa4ce3f8f1b22ceaf1fbfc311a8b85d2be3f',
    'cpppj/src/o/RawMissile.cpp':'176436c391308b86ddbe9d9595501da16cd21eb8825515d3836c49031a8d0427',
    'cpppj/src/o/RawMissile.h':'a48c2833f5463df9fdaf25634c440805f5edce224e30b3725cea142fa21b3487',
}
# 현재와 교정 전의 실제 ctor 두 분기/충돌 사운드 조건 및 새 설명 줄을 역변환한다.
CHANGES=(
    ('    // 패치의 +32/+36 초기화는 group1 코너 분기에만 있으며 group0의 기존 바이트는 보존한다.\n',''),
    ('if (patch) { if (corner && !archer) {','if (patch) { if (corner) {'),
    ('    // group0 코너의 종점은 패치에서 코너이며 CD10.72에서는 표적 치수로 보정한 중심이다.','    // group0은원본targetCenter/corner를그대로쓰며group1만사정선축정렬을호출한다.'),
    ('archer ? (patch && corner ? request.targetCenter : targetCenter)','archer ? (corner ? request.targetCenter : targetCenter)'),
    ('            // CD10.72는 사거리 종료와 충돌이 겹쳐도 nonzero 표적 충돌 사운드를 요청한다.\n',''),
    ('missileTypes_[asset[10]].impactSound && (!patch || !exhausted)','missileTypes_[asset[10]].impactSound && !exhausted'),
)
# 독립 교정 경로 보고서이며 이전1888행 보고서를 복원했다고 주장하지 않는다.
REPORT=ROOT/'cpppj/archermissile-correction.json'


def previous_source():
    """현재 C++의 명시한 ctor/사운드 분기만 되돌려 사전 관찰 SHA와 일치하는지 검사한다."""
    raw=(ROOT/'cpppj/src/o/RawMissile.cpp').read_bytes();text=raw.decode('utf-8');newline='\r\n' if '\r\n' in text else '\n';text=text.replace('\r\n','\n')
    # 예상한 현재 분기가 각각 한 번만 나타날 때만 역변환한다.
    for current,previous in CHANGES:
        if text.count(current)!=1:raise RuntimeError('Archer Missile 교정 분기 역변환 오류')
        text=text.replace(current,previous,1)
    return text.replace('\n',newline).encode('utf-8')


def audit():
    """기존 fixture/header 및 역변환한 교정 전 C++ 바이트를 실제 사전 SHA와 비교한다."""
    previous=previous_source()
    if hashlib.sha256(previous).hexdigest()!=OBSERVED['cpppj/src/o/RawMissile.cpp']:raise RuntimeError('Archer Missile 사전 C++ SHA 불일치')
    for path in ('cpppj/tests/fixtures/archermissile-x86.tsv','cpppj/src/o/RawMissile.h'):
        if digest(ROOT/path)!=OBSERVED[path]:raise RuntimeError('Archer Missile 사전 fixture/header SHA 불일치')


def generate():
    """현재 source와 초기 실패로그에 교정 경로 및 같은1888개 재방문을 연결한다."""
    audit();previous=ROOT/'cpppj/hist/archermissil-d6700/RawMissile-before-correction.cpp';previous.parent.mkdir(parents=True,exist_ok=True);previous.write_bytes(previous_source());paths=(Path(__file__).relative_to(ROOT).as_posix(),previous.relative_to(ROOT).as_posix(),'cpppj/src/o/RawMissile.cpp','cpppj/src/o/RawMissile.h','cpppj/tests/fixtures/archermissile-x86.tsv','cpppj/archermissile.json','extracted/run7-ArcherMissile_-first.log')
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',observed_before=OBSERVED,observed_before_source='root가 첫 C++ 대조 전에 읽은 실제 report의 tool 출력',changes=['10.78 group0 corner의 +32/+36 기존 raw 바이트 보존','10.72 group0 corner의 endpoint는 표적 치수 중심 보존','10.72는 충돌/사거리 종료가 겹쳐도 nonzero impactSound 요청'],native_rows=1888,new_unique_rows=0,previous_report_snapshot=False,previous_fixture_sha_verified=True,reconstructed_previous_cpp_sha_verified=True,files={path:digest(ROOT/path) for path in paths},limitations=['이전1888행 전체 report/fixture의 독립 바이트 snapshot은 보존하지 못했다.','사전 실제 tool 출력의 SHA와 현재 fixture의 동일성 및 명시 ctor/사운드 분기의 역변환 SHA를 감사한다.','05 baseline/296행 근거는 별도 보고서와 git commit blob으로 보존한다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(verified=True,native_rows=1888,new_unique_rows=0),ensure_ascii=False))


def verify():
    """현재 교정 경로 SHA와 사전 관찰 SHA를 다시 검사한다."""
    audit();report=json.loads(REPORT.read_text(encoding='utf-8'))
    for path,expected in report['files'].items():
        if digest(ROOT/path)!=expected:raise RuntimeError('Archer Missile 교정 SHA 불일치 '+path)
    print(json.dumps(dict(verified=True,native_rows=report['native_rows'],new_unique_rows=report['new_unique_rows']),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
