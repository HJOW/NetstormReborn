#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""일반 피해 검사 명칭/double 경고/CD 지면 입력 교정과 두 전체5846근거를 연결한다."""
import argparse
import json
from pathlib import Path
from decomp_owner_oracle import ROOT,digest

# 첫 공유 빌드 전 source/test/oracle/fixture/report와 실패 로그의 실제 전체 snapshot이다.
BEFORE=ROOT/'cpppj/hist/damageablehi-1d1d7'
CHECKPOINT=ROOT/'cpppj/hist/damageablehi-8cde8'
TEST=ROOT/'cpppj/tests/DamageableHitTests.cpp'
CPP=ROOT/'cpppj/src/o/RawDamageableHit.cpp'
NATIVE=ROOT/'cpppj/damageablehit.json'
REPORT=ROOT/'cpppj/damageablehit-correction.json'
# 첫 native/C++ 대조 뒤 원본 fadd qword와 동일 CD map 입력만 교정한 역변환이다.
CPP_CHANGES=(
    ('// 현재 실제 주 섬에 대한 경고만 표시하고 원본 double 시계+float 간격의 저장 순서를 유지한다.','// 현재 실제 주 섬에 대한 경고만 표시하고 PATCH float wall-clock 저장 순서를 유지한다.'),
    ('state_.warningTime=now+static_cast<double>(state_.warningDelay);','state_.warningTime=patch ? static_cast<double>(static_cast<float>(now)+state_.warningDelay) : now+static_cast<double>(state_.warningDelay);'),
)
TEST_CHANGES=(
    ('#include "o/RawPathSearch.h"\n',''),
    ('    SquidHash hash; // CD native가 셀(20,21)에만 넣은 현재 지면 SID 입력이며 실제 조회의 보정을 실행한다.\n',''),
    ('events.clear();hash.Reset();hash.Cell(0,20,21)=70;types.assign','events.clear();types.assign'),
    ('if (patch) { events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+\':\'+std::to_string(std::bit_cast<std::uint32_t>(y)));return Sid{70}; }return RawPathSurfaceAt(hash,{x,y});','if (patch) events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+\':\'+std::to_string(std::bit_cast<std::uint32_t>(y)));return Sid{70};'),
)


def reverse_changes(current,changes):
    """각 실제 교정 구간을 정확히 한 번 역변환한다."""
    for now,old in changes:
        if current.count(now)!=1:raise RuntimeError('일반 피해 명시 교정 구간 누락')
        current=current.replace(now,old,1)
    return current


def audit():
    """생산 코드/원본 입력/관찰은 그대로이며 검사 타입 한 줄만 바뀌었는지 검사한다."""
    previous=json.loads((BEFORE/NATIVE.name).read_text(encoding='utf-8'))
    for snapshot in (BEFORE,CHECKPOINT):
        saved_report=json.loads((snapshot/NATIVE.name).read_text(encoding='utf-8'))
        for name,expected in saved_report['files'].items():
            saved=snapshot/Path(name).name;path=saved if saved.exists() else ROOT/name
            if digest(path)!=expected:raise RuntimeError('일반 피해 이전 전체 SHA 불일치 '+name)
    current=reverse_changes(TEST.read_text(encoding='utf-8'),TEST_CHANGES)
    if current!=(CHECKPOINT/TEST.name).read_text(encoding='utf-8'):raise RuntimeError('일반 피해 CD 지면 입력 외 검사 변경')
    if current.count('    GameRandom random;')!=1 or current.replace('    GameRandom random;','    Random random;',1)!=(BEFORE/TEST.name).read_text(encoding='utf-8'):raise RuntimeError('일반 피해 검사 명칭 외 변경')
    current_cpp=reverse_changes(CPP.read_text(encoding='utf-8'),CPP_CHANGES)
    if current_cpp!=(CHECKPOINT/CPP.name).read_text(encoding='utf-8') or current_cpp!=(BEFORE/CPP.name).read_text(encoding='utf-8'):raise RuntimeError('일반 피해 double 경고 외 production 변경')
    for relative in ('cpppj/src/o/RawDamageableHit.h','tools/decomp_damageablehit_oracle.py','cpppj/tests/fixtures/damageablehit-x86.tsv'):
        if (ROOT/relative).read_bytes()!=(BEFORE/Path(relative).name).read_bytes():raise RuntimeError('일반 피해 production/oracle/fixture 변경 '+relative)
    return previous


def generate():
    """새 native0에서 동일 관찰 보고서에 현재 검사 SHA만 연결하고 원래 전체 보고서를 보존한다."""
    previous=audit();current=json.loads(json.dumps(previous))
    for path in (TEST,CPP,ROOT/'cpppj/src/o/RawPathSearch.cpp',ROOT/'cpppj/src/o/RawPathSearch.h'):current['files'][path.relative_to(ROOT).as_posix()]=digest(path)
    current['source_correction']=dict(kind='검사 명칭/실제double 경고/동일 CD 지면 입력',previous_full_report=(BEFORE/NATIVE.name).relative_to(ROOT).as_posix(),previous_sha256=digest(BEFORE/NATIVE.name),compile_corrected_report=(CHECKPOINT/NATIVE.name).relative_to(ROOT).as_posix(),compile_corrected_sha256=digest(CHECKPOINT/NATIVE.name),new_native_rows=0,new_unique_rows=0,cd_warning_active_observations=0);NATIVE.write_text(json.dumps(current,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    paths=[Path(__file__),TEST,CPP,NATIVE,ROOT/'extracted/run9-build-initial.log',ROOT/'extracted/run9-DamageableHit_first.log',*[p for snapshot in (BEFORE,CHECKPOINT) for p in snapshot.iterdir() if p.is_file()]]
    REPORT.write_text(json.dumps(dict(schema=2,host='HJOW-Athlon',native_rows=5846,new_native_rows=0,new_unique_rows=0,initial_full_snapshot=True,compile_corrected_full_snapshot=True,change='검사 GameRandom·원본 double 시계+float 간격·동일 CD map/실제 보정 조회',cd_warning_active_observations=0,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(verified=True,native_rows=5846,new_native_rows=0),ensure_ascii=False))


def verify():
    """당시 전체 보고서/한 줄 역변환과 현재 native/lineage의 실제 SHA를 감사한다."""
    previous=audit();current=json.loads(NATIVE.read_text(encoding='utf-8'));copy=json.loads(json.dumps(current));copy.pop('source_correction')
    for path in (TEST,CPP):copy['files'][path.relative_to(ROOT).as_posix()]=previous['files'][path.relative_to(ROOT).as_posix()]
    for name in ('cpppj/src/o/RawPathSearch.cpp','cpppj/src/o/RawPathSearch.h'):copy['files'].pop(name)
    if copy!=previous or current['source_correction']['previous_sha256']!=digest(BEFORE/NATIVE.name):raise RuntimeError('일반 피해 원본 관찰 또는 보고서 변경')
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('일반 피해 교정 SHA 불일치 '+name)
    print(json.dumps(dict(verified=True,native_rows=5846,new_native_rows=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
