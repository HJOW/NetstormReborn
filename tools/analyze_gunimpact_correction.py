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
# 첫 C++ 전수 실패 직전의 전체 현재자료와 당시 Clamp 도구/보고서다.
CPP_BEFORE=ROOT/'cpppj/recovery-history/gunimpact-before-cpp-correction'
# fixture 문자열 해석/실제 표시 viewport/복합 단언 분리만 이전 검사로 역변환한다.
CPP_CHANGES=(
 ('state.suppressed=value[10]=="True" ? 1U : (value[10]=="False" ? 0U : Number(value[10]));','state.suppressed=Number(value[10]);'),
 ('SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,1024,768}});','SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});'),
 ('CHECK(books.globalCounts[160]==0);CHECK(books.totalCost==0);CHECK(books.depth==0);CHECK(destroy.PreDepth()==0);CHECK(destroy.PostDepth()==0);CHECK(!sink.rectangles.empty());',
  'CHECK(books.globalCounts[160]==0 && books.totalCost==0 && books.depth==0 && destroy.PreDepth()==0 && destroy.PostDepth()==0 && !sink.rectangles.empty());'),
)
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
        if name.endswith('GunImpactTests.cpp'):
            # 두 번째 수정은 최초 Clamp 교정 뒤의 전체 검사로 먼저 역변환한다.
            for now,old in CPP_CHANGES:
                if current.count(now)!=1:raise RuntimeError('충돌 C++ 입력 교정 범위 누락')
                current=current.replace(now,old)
            if current!=(CPP_BEFORE/'GunImpactTests.cpp').read_text(encoding='utf-8'):raise RuntimeError('충돌 C++ 입력 교정 외 전체검사 변경')
        for now,old in changes:
            expected=2 if now=='const auto position=Clamp(x,y,pool_.Edition());' else 1
            if current.count(now)!=expected:raise RuntimeError('충돌 좌표 교정 분기 누락')
            current=current.replace(now,old)
        if current!=(BEFORE/Path(name).name).read_text(encoding='utf-8'):raise RuntimeError('충돌 좌표 교정 외 source 변경 '+name)
    if (BEFORE/'gunimpact-x86.tsv').read_bytes()!=(ROOT/'cpppj/tests/fixtures/gunimpact-x86.tsv').read_bytes():raise RuntimeError('충돌 동일2274fixture 변경')
    checkpoint=json.loads((CPP_BEFORE/NATIVE.name).read_text(encoding='utf-8'))
    # 첫 C++ 직전 report가 가리킨 전체6자료의 SHA를 실제 snapshot으로 검사한다.
    for name in ('RawGunImpact.cpp','RawGunImpact.h','GunImpactTests.cpp','decomp_gunimpact_oracle.py','gunimpact-x86.tsv'):
        relative=next(path for path in checkpoint['files'] if Path(path).name==name)
        if digest(CPP_BEFORE/name)!=checkpoint['files'][relative]:raise RuntimeError('충돌 C++ 이전 전체자료 SHA 오류 '+name)
    prior_clamp=json.loads((CPP_BEFORE/REPORT.name).read_text(encoding='utf-8'))
    for name,expected in prior_clamp['files'].items():
        saved=CPP_BEFORE/Path(name).name
        path=ROOT/name if name.startswith('cpppj/recovery-history/') or not saved.exists() else saved
        if digest(path)!=expected:raise RuntimeError('충돌 이전 Clamp 전체보고서 출처 오류 '+name)
    # 현재 native의 정상2274 출력/원본도구는 두 번째 검사 교정으로 변경하지 않는다.
    current_native=json.loads(NATIVE.read_text(encoding='utf-8'));current_native.pop('cpp_input_correction',None)
    current_native['files']['cpppj/tests/GunImpactTests.cpp']=checkpoint['files']['cpppj/tests/GunImpactTests.cpp']
    if current_native!=checkpoint:raise RuntimeError('충돌 C++ 교정 원본2274 결과/출처 불변 오류')
    from decomp_gunimpact_oracle import ImpactOracle
    result={}
    for edition,address in (('originals',0x5012d0),('originalCD',0x501764)):
        oracle=ImpactOracle(edition);raw=bytes(oracle.mu.mem_read(address,4));value=struct.unpack('<f',raw)[0]
        if value!=1.0:raise RuntimeError('충돌 실제 하한float 상수 불일치')
        result[edition]=dict(address=f'{address:08x}',bytes=raw.hex(),value=value,nan='보존' if edition=='originals' else '255')
    return result


def generate():
    """동일 입력 재방문을 신규 고유0으로 기록하고 현재 source/원본보고서 SHA를 연결한다."""
    native=audit();current=json.loads(NATIVE.read_text(encoding='utf-8'));current['files']['cpppj/tests/GunImpactTests.cpp']=digest(ROOT/'cpppj/tests/GunImpactTests.cpp');current['cpp_input_correction']=dict(previous_full_report_sha256=digest(CPP_BEFORE/NATIVE.name),previous_clamp_report_sha256=digest(CPP_BEFORE/REPORT.name),new_native=0,new_unique=0,reason='Python True/False suppressed 입력만 숫자로 해석하고 실제50.5좌표가 보이는 원본1024x768 viewport를 공급. 최종 단언 분리. production/2274fixture/원본tool불변.')
    NATIVE.write_text(json.dumps(current,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    paths=[Path(__file__),NATIVE,*[ROOT/name for name in CHANGES],ROOT/'cpppj/tests/fixtures/gunimpact-x86.tsv',*[p for p in BEFORE.iterdir() if p.is_file()],*[p for p in CPP_BEFORE.iterdir() if p.is_file()]]
    REPORT.write_text(json.dumps(dict(schema=2,host='HJOW-Athlon',native_rows=2274,new_unique_rows=0,new_native_rows=0,initial_full_snapshot=True,cpp_full_snapshot=True,change='최초Clamp 실제하한1/NaN 교정과 첫C++ 검사 입력문자열/표시viewport 교정. 전체두단계자료보존.',pe_clamp=native,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');verify()


def verify():
    """두 실제 PE의 상수/fixture와 이전·현재 전체 source lineage를 다시 검사한다."""
    native=audit();report=json.loads(REPORT.read_text(encoding='utf-8'))
    if report['pe_clamp']!=native:raise RuntimeError('충돌 실제 좌표 비교 근거 불일치')
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('충돌 교정 SHA 불일치 '+name)
    print(json.dumps(dict(verified=True,native_rows=2274,new_unique_rows=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
