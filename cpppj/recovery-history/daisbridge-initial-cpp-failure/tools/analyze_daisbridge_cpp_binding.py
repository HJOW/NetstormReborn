#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새 Bridge640 원본과 같은 풀 C++ 소비자의 전체 현재 출처·초기 smoke 보존을 연결한다."""
import argparse
import json
from pathlib import Path
from decomp_daisbridge_oracle import ROOT,REPORT as NATIVE,FIXTURE,sha,verify as native_verify

TEST=ROOT/'cpppj/tests/DaisBridgeTests.cpp' # 현재640 전수의 실제 Path/Host/packet/Frame/cleanup 소비자다.
HISTORY=ROOT/'cpppj/recovery-history/daisbridge-initial-smoke' # hash1/spot 초기 충돌의 전체14smoke를 보존한다.
REPORT=ROOT/'cpppj/recovery-daisbridge-cpp-binding-evidence.json' # 새관찰을 합산하지 않는 현재 C++ 연결 근거다.

def expected():
    """현재640 출처와 초기 전체 원문/두 정밀도 차이·실제 C++ 연결을 감사한다."""
    native_verify()
    native=json.loads(NATIVE.read_text(encoding='utf-8'))
    assert native['total']==640 and not native['smoke'] and native['game_runs']==0
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts={edition:dict(normal_inputs=sum(row[0]==edition for row in rows),path_created=sum(row[0]==edition and int(row[3])>0 for row in rows)) for edition in native['editions']}
    time_changes={}
    # 정밀도 차이는 효과/raw/좌표/방향이 같고 double 시각 단어만 ±1ULP임을 확인한다.
    for item in native['precision_differences']:
        first=item['result53']
        second=item['result64']
        assert first[:3]==second[:3] and first[4:]==second[4:]
        left=first[3].split(';')
        right=second[3].split(';')
        assert len(left)==len(right)
        for a,b in zip(left,right):
            one=a.split(',')
            two=b.split(',')
            assert one[:3]==two[:3]
            difference=int(two[3])-int(one[3])
            if difference:
                assert difference in (-1,1)
                key=f'{item["edition"]}:{difference}'
                time_changes[key]=time_changes.get(key,0)+1
    assert len(native['precision_differences'])==18 and time_changes=={'originals:-1':14,'originals:1':12}
    old=json.loads((HISTORY/'cpppj/recovery-daisbridge-evidence.json').read_text(encoding='utf-8'))
    assert old['total']==14 and old['smoke'] and old['game_runs']==0
    assert sha(HISTORY/'tools/decomp_daisbridge_oracle.py')==old['files']['tools/decomp_daisbridge_oracle.py']
    assert sha(HISTORY/'cpppj/tests/fixtures/daisbridge-x86.tsv')==old['files']['cpppj/tests/fixtures/daisbridge-x86.tsv']
    history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()}
    assert len(history)==5
    files={Path(__file__),TEST,NATIVE,FIXTURE,ROOT/'tools/decomp_daisbridge_oracle.py'}
    # 같은 풀에 직접 연결한 생산 몸체 전체를 C++ 빌드 스냅샷과 연결한다.
    for name in ('RawDaisRegular','RawPathStart','RawPathRuntime','RawPathProcess','RawPathSearch','RawPathData','RawPathCleanup','RawTempleRitual','RawPriestRitualActions','RawRunePostPop','RawCarrierPreDestroy','RawContainedLookup','RawSquidNeighbors','SquidProcess','SquidPop','SquidUnpop','SquidFrame','SquidReward'):
        for suffix in ('.h','.cpp'):files.add(ROOT/f'cpppj/src/o/{name}{suffix}')
    files.update(ROOT/name for name in ('cpppj/src/client/RawPriestWorldBindings.h','cpppj/src/client/RawPriestWorldBindings.cpp'))
    return dict(host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=640,assert_inputs=0,new_native_inputs=0,initial_smoke_inputs_not_added=14,
                editions=counts,x87_different_inputs=18,x87_different_time_words=26,x87_time_word_deltas=time_changes,
                test_count=1,test_prefix='rawDaisBridge_',fixture_macro='NETSTORM_DAISBRIDGE_FIXTURE',history=history,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},
                scope='새 상위640은 원본보고서에서 한 번만 합산한다. C++ 현재 검사 SHA를 같은 실제 PathStart/BFS/Frame/Host/서버 PathData/구 경로 Kill/공간 접두/RawPathCleanup/Temple.Ritual에 연결한다. generic사제 VT·1×1 표시 합성 입력이며 전체걷기·가상도착·의식307/룬304·실제캠페인 metadata·UI/음향은 후속이다. 최초14smoke/기존512/668/2964는 새원본에 합산하지 않는다.')

def main(check):
    """명시 CLI에서 현재 C++ 연결 자료를 작성하거나 전체 SHA를 감사한다."""
    data=expected()
    if check:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('Dais Bridge C++ 연결: 정상640/assert0·x87차이18입력/26단어·현재전체SHA·초기14제외·추가0')

# 명시 호출에서만 현재 연결 자료를 작성하거나 검증한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    main(args.verify)
