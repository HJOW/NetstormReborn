#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""단위12 바이트 속성 설명의 인코딩 교정을 이전 전체 최종 근거와 연결한다."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 저장소 전체 자료의 절대 기준이다.
HISTORY=ROOT/'cpppj/hist/campaign-run-6e53a'  # 첫 성공 근거의 전체 원문이다.
REPORT=ROOT/'cpppj/run12-attribute-lineage.json'  # 설명만 바뀐 현재 속성의 별도 출처다.
SAVED=('.gitattributes','tools/analyze_campaign_runtime12.py','cpppj/run12.json',
       'extracted/run12-final-evidence-generation.log')  # 교정 직전 속성/도구/성공 근거/로그를 모두 보존한다.
COMMENT='# 단위12의 전체 소스 및 원본 대조 자료 바이트를 Git에서도 그대로 보존한다.'  # 속성 동작을 한국어로 설명한다.


def sha(path):
    """파일 전체 바이트의 SHA256을 반환한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """설명 교정 전에 첫 최종 성공 자료와 정확한 설명 한 줄을 저장한다."""
    assert not HISTORY.exists()
    values={}
    # 전체 파일을 복사하며 기존 원본 관찰의 수나 출처를 바꾸지 않는다.
    for name in SAVED:
        target=HISTORY/name
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes((ROOT/name).read_bytes())
        values[name]=sha(target)
    lines=(ROOT/'.gitattributes').read_bytes().splitlines(keepends=True)
    candidates=[line for line in lines if line.startswith(b'# ??12')]
    assert len(candidates)==1
    data=dict(files=values,previous_comment_hex=candidates[0].hex())
    (HISTORY/'baseline.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위12 최초 성공 전체 근거4파일 보존')


def expected():
    """설명 한 줄 정확 역변환과 첫 성공 관찰/현재 성공 근거의 동일성을 확인한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'))
    # 저장된 전체 당시 자료의 바이트를 각각 확인한다.
    for name,value in baseline['files'].items():
        assert sha(HISTORY/name)==value,name
    previous=(HISTORY/'.gitattributes').read_bytes()
    current=(ROOT/'.gitattributes').read_bytes()
    old=bytes.fromhex(baseline['previous_comment_hex'])
    new=(COMMENT+'\n').encode('utf-8')
    assert current.count(new)==1 and current.replace(new,old,1)==previous
    prior=json.loads((HISTORY/SAVED[2]).read_text(encoding='utf-8'))
    latest=json.loads((ROOT/SAVED[2]).read_text(encoding='utf-8'))
    # 첫 전체 성공의 SHA는 당시 속성만 보존 원문에서 읽고 다른 파일은 현재 원문과 비교한다.
    for name,value in prior['files'].items():
        source=HISTORY/name if name=='.gitattributes' else ROOT/name
        assert sha(source)==value,name
    adjusted=json.loads(json.dumps(prior))
    adjusted['files']['.gitattributes']=sha(ROOT/'.gitattributes')
    assert latest==adjusted
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,
                normal_cases=12646,assertion_cases=652,all_tests=1056,exact_comment_inverse=True,
                production_tests_fixtures_and_counts_unchanged=True,tool_sha256=sha(Path(__file__)),
                current_attribute_sha256=sha(ROOT/'.gitattributes'),current_final_report_sha256=sha(ROOT/SAVED[2]),
                history={p.relative_to(ROOT).as_posix():sha(p) for p in HISTORY.rglob('*') if p.is_file()})


def main(arguments):
    """기준 전체 저장 또는 현재 설명 교정의 생성/검증을 수행한다."""
    if arguments.capture:
        capture()
        return
    data=expected()
    if arguments.verify:
        assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:
        REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('단위12 속성 설명 정확 역변환/최초 전체 성공 근거 보존/새관찰0')


# 최초 성공 전체 보존 뒤 설명만 교정하고 같은 성공 근거를 재생성한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',action='store_true')
    parser.add_argument('--verify',action='store_true')
    main(parser.parse_args())
