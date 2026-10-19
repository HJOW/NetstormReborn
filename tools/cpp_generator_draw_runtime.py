#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제네레이터 원본 표시 복원과 실제 건설 화면의 근거를 감사한다."""
import argparse
import hashlib
import json
from cpp_campaign_entry_smoke import ROOT
from cpp_early5_spells_runtime import inspect_combat

REPORT=ROOT/'cpppj/generator-draw-runtime.json'  # 원본 관측과 현재 연결을 구분한 기록이다.


def sha(path):
    """현재 파일 전체 바이트를 고정하며 과거 실행 파일 해시를 바꾸지 않는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """원본 관측·최초 실패·최종 검사·실제 UI 원문의 현재 출처를 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options=parser.parse_args()
    if not options.verify:
        parser.error('--verify가 필요합니다')
    record=json.loads(REPORT.read_text('utf-8'))
    # 현재 소스와 보존된 모든 증거를 각각 확인한다.
    for group in ('sources','evidence'):
        # 실행 결과는 새 결과로 해석하지 않고 정확한 파일 바이트만 비교한다.
        for name,value in record[group].items():
            assert sha(ROOT/name)==value,name
    native=json.loads((ROOT/'cpppj/generator-draw-native.json').read_text('utf-8'))
    assert (native['inputs'],native['normalReturns'])==(24,48)
    assert native['virtuals']=={'0x78':'0x4acfe0','0xa4':'0x41c100'}
    before=json.loads((ROOT/record['validation']['beforeTests']).read_text('utf-8'))
    assert before['tests']==1 and before['failed']==6 and before['returnCode']==1
    full=json.loads((ROOT/record['validation']['fullTests']).read_text('utf-8'))
    focused=json.loads((ROOT/record['validation']['focusedTests']).read_text('utf-8'))
    assert full['tests']==1747 and full['failed']==full['returnCode']==0
    assert focused['tests']==1 and focused['failed']==focused['returnCode']==0 and full['binarySha256']==focused['binarySha256']
    # 실제 생산/건설 결과를 같은 상태 원문에서 다시 읽는다.
    for name,value in record['observations'].items():
        assert inspect_combat(ROOT/name)==value,name
        assert value['returnCode']==0 and value['originalsUnchanged'] and value['nextFailure'] is None
        assert len(value['forwardGeneratorsAfterWait'])==2
    assert not record['goalComplete'] and not record['fullFidelityVerified']
    print('제네레이터24원문/48정상 반환·전체1747검사·실제 건설 화면 출처 통과')


if __name__=='__main__':
    main()
