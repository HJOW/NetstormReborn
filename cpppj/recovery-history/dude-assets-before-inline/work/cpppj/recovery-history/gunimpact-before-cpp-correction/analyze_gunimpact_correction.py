#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초2274전체 근거와 실제 좌표 하한1/두 판본 unordered 비교 교정을 연결한다."""
import argparse
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,digest

# 최초 정상2274 source/test/oracle/fixture/전체보고서의 실제 수정 전 바이트다.
BEFORE=ROOT/'cpppj/recovery-history/gunimpact-initial'
NATIVE=ROOT/'cpppj/recovery-gunimpact-evidence.json'
REPORT=ROOT/'cpppj/recovery-gunimpact-correction-evidence.json'
CHANGES={
 'cpppj/src/o/RawGunImpact.cpp':(
    ('// 실제 하한float1이며 unordered 상한 비교에서 패치는NaN을보존하고CD는255를선택한다.\nstd::array<float,2> RawGunImpact::Clamp(float x,float y,OriginalEdition edition) {\n    float a=x<1.0f ? 1.0f : x,b=y<1.0f ? 1.0f : y;const bool patch=edition==OriginalEdition::Patch1078;if (patch ? a>255.0f : !(a<=255.0f)) a=255.0f;if (patch ? b>255.0f : !(b<=255.0f)) b=255.0f;return {a,b};',
     '// 원본은 NaN에서도 첫 비교가 false이므로 하한0.5를 선택한다.\nstd::array<float,2> RawGunImpact::Clamp(float x,float y) {\n    float a=0.5f<=x ? x : 0.5f,b=0.5f<=y ? y : 0.5f;if (255.0f<a) a=255.0f;if (!(b<=255.0f)) b=255.0f;return {a,b};'),
    ('const auto position=Clamp(x,y,pool_.Edition());','const auto position=Clamp(x,y);'),
 ),
 'cpppj/src/o/RawGunImpact.h':(
    ('// 0041d0d0/CD0043f950: 좌표를1..255에 제한하며 패치NaN보존/CDNaN255를 구분한다.','// 0041d0d0/CD0043f950: 원본 비교 순서대로 각 좌표를0.5..255에 제한한다.'),
    ('static std::array<float,2> Clamp(float x,float y,OriginalEdition edition=OriginalEdition::Patch1078);','static std::array<float,2> Clamp(float x,float y);'),
 ),
 'cpppj/tests/GunImpactTests.cpp':(('logic.Clamp(F(2),F(3),pool.Edition())','logic.Clamp(F(2),F(3))'),),
}


def audit():
    """당시 전체 SHA와 각 명시 좌표 교정의 역변환 및 동일2274fixture를 검사한다."""
    previous=json.loads((BEFORE/NATIVE.name).read_text(encoding='utf-8'))
    for name,expected in previous['files'].items():
        saved=BEFORE/Path(name).name;path=saved if saved.exists() else ROOT/name
        if digest(path)!=expected:raise RuntimeError('충돌 이전 전체 SHA 불일치 '+name)
    for name,changes in CHANGES.items():
        current=(ROOT/name).read_text(encoding='utf-8')
        for now,old in changes:
            expected=2 if now=='const auto position=Clamp(x,y,pool_.Edition());' else 1
            if current.count(now)!=expected:raise RuntimeError('충돌 좌표 교정 분기 누락')
            current=current.replace(now,old)
        if current!=(BEFORE/Path(name).name).read_text(encoding='utf-8'):raise RuntimeError('충돌 좌표 교정 외 source 변경 '+name)
    if (BEFORE/'gunimpact-x86.tsv').read_bytes()!=(ROOT/'cpppj/tests/fixtures/gunimpact-x86.tsv').read_bytes():raise RuntimeError('충돌 동일2274fixture 변경')
    from decomp_gunimpact_oracle import ImpactOracle
    result={}
    for edition,address in (('originals',0x5012d0),('originalCD',0x501764)):
        oracle=ImpactOracle(edition);raw=bytes(oracle.mu.mem_read(address,4));value=struct.unpack('<f',raw)[0]
        if value!=1.0:raise RuntimeError('충돌 실제 하한float 상수 불일치')
        result[edition]=dict(address=f'{address:08x}',bytes=raw.hex(),value=value,nan='보존' if edition=='originals' else '255')
    return result


def generate():
    """동일 입력 재방문을 신규 고유0으로 기록하고 현재 source/원본보고서 SHA를 연결한다."""
    native=audit();paths=[Path(__file__),NATIVE,*[ROOT/name for name in CHANGES],ROOT/'cpppj/tests/fixtures/gunimpact-x86.tsv',*[p for p in BEFORE.iterdir() if p.is_file()]]
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',native_rows=2274,new_unique_rows=0,initial_full_snapshot=True,change='실제 하한float1과 패치NaN보존/CDNaN255 비교 순서 적용',pe_clamp=native,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(verified=True,native_rows=2274,new_unique_rows=0),ensure_ascii=False))


def verify():
    """두 실제 PE의 상수/fixture와 이전·현재 전체 source lineage를 다시 검사한다."""
    native=audit();report=json.loads(REPORT.read_text(encoding='utf-8'))
    if report['pe_clamp']!=native:raise RuntimeError('충돌 실제 좌표 비교 근거 불일치')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('충돌 교정 SHA 불일치 '+name)
    print(json.dumps(dict(verified=True,native_rows=2274,new_unique_rows=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
