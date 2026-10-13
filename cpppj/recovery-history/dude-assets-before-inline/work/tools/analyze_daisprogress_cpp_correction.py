#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다이스 상위 검사의 중복 선언 하나만 교정하고 원본2964/이전 연결 근거를 보존한다."""
import argparse,json
from pathlib import Path
from decomp_daisprogress_oracle import ROOT,FIXTURE,REPORT,sha,verify as native_verify

HISTORY=ROOT/'cpppj/recovery-history/daisprogress-before-cpp-declaration' # 수정 전 검사/연결 도구·보고서/원본 자료 전체다.
SUCCESSOR=ROOT/'cpppj/recovery-daisprogress-cpp-correction-evidence.json' # 원본 입력을 추가하지 않는 별도 현재 검사 연결 근거다.
TEST='cpppj/tests/DaisProgressTests.cpp' # 중복 std::string results 선언 하나만 지우는 파일이다.

def expected():
    """전체 이전 자료와 정확 역변환/생산·원본 fixture 불변을 확인한다."""
    native_verify();data=json.loads((HISTORY/'cpppj/recovery-daisprogress-cpp-binding-evidence.json').read_text(encoding='utf-8'));files={};dependencies={}
    old=(HISTORY/TEST).read_text(encoding='utf-8');current=(ROOT/TEST).read_text(encoding='utf-8');needle='std::string results;std::string results;';replacement='CHECK(random.State()==Number(row[6]));std::string results;'
    assert old.count(needle)==1 and current.count(replacement)==1
    restored=current.replace(replacement,replacement+'std::string results;');assert restored==old
    # 이 연결 도구/보고서와 원본2964은 그 당시 SHA를 보존한다.
    for name,value in data['files'].items():
        path=HISTORY/name if (HISTORY/name).exists() else ROOT/name;assert sha(path)==value,name
        dependencies[name]=dict(sha256=value,preserved_or_current=path.relative_to(ROOT).as_posix())
    for name in ('cpppj/recovery-daisprogress-evidence.json','cpppj/tests/fixtures/daisprogress-x86.tsv','tools/decomp_daisprogress_oracle.py'):
        assert (ROOT/name).read_bytes()==(HISTORY/name).read_bytes(),name
    # 이전 자료는 다른 PC에서도 원문으로 재검사할 수 있도록 파일 전체를 묶는다.
    for path in sorted(HISTORY.rglob('*')):
        if path.is_file():files[path.relative_to(ROOT).as_posix()]=sha(path)
    files[TEST]=sha(ROOT/TEST);files[Path(__file__).relative_to(ROOT).as_posix()]=sha(Path(__file__))
    return dict(host='HJOW-Athlon',decompile_date='2026-10-14',normal_inputs=2964,new_native_inputs=0,production_unchanged=True,entire_test_restored_exactly=True,prior_binding=dependencies,files=files,scope='현재 전용 검사에서 중복 results 선언 하나만 삭제. 생산/독립원본2964/tool/fixture/이전 연결 보고서 전체 원문은 불변이다. 공유 C++ 대조는 별도 실행한다.')

def main(check):
    """새 successor 작성 또는 현재 검사·전체 이전 근거의 불변을 확인한다."""
    data=expected()
    if check:assert json.loads(SUCCESSOR.read_text(encoding='utf-8'))==data
    else:SUCCESSOR.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('다이스 상위 검사 중복 선언 교정: 전체 정확 역변환, 원본2964 불변, 추가0')

# 명시 CLI 요청에서만 별도 교정 근거를 작성하거나 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();main(args.verify)
