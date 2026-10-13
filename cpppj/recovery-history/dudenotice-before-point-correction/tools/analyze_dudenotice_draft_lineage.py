#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DudeNotice 최초 414행 전체자료를 보존하고 type14의 Burning 명칭 교정을 감사한다."""
import argparse
import copy
import json
from pathlib import Path
from decomp_owner_oracle import ROOT,digest

# 처음 정상반환한 자료와 현재 source/도구/fixture의 전체 바이트를 연결한다.
HISTORY=ROOT/'cpppj/recovery-history/dudenotice-initial'
REPORT=ROOT/'cpppj/recovery-dudenotice-evidence.json'
LINEAGE=ROOT/'cpppj/recovery-dudenotice-draft-lineage-evidence.json'
PATHS=('cpppj/src/o/RawDudeNotice.cpp','cpppj/src/o/RawDudeNotice.h','cpppj/tests/DudeNoticeTests.cpp',
       'tools/decomp_dudenotice_oracle.py','cpppj/tests/fixtures/dudenotice-x86.tsv','cpppj/recovery-dudenotice-evidence.json')
# 실제 호출 주소/숫자/행 순서는 그대로 두고 잘못 붙인 작업자 이름만 바꾼다.
CHANGES={
 PATHS[0]:(('dudeProcessType','burningProcessType'),('createDudeProcess','createBurning'),('DudeProcess','Burning')),
 PATHS[1]:(('dudeProcessType','burningProcessType'),('createDudeProcess','createBurning'),('DudeProcess14','Burning14 모드0'),('실제Regular/DudeProcess/DudePath','실제Regular/Burning/DudePath'),('미복원Dude/Path','미복원Burning/Path')),
 PATHS[2]:(('createDudeProcess','createBurning'),('worker','burning'),('MissingWorker','MissingBurning')),
 PATHS[3]:(('dudeCtor','burningCtor'),('dudeType','burningType'),("'dude'","'burning'"),('dude=','burning='),('regular,dude,target','regular,burning,target'),('burning=dude','burning=burning'),('DudeProcess14','Burning14(mode0)'),('worker','burning'),('Dude ctor','Burning 모드0 ctor'),('burning 경제','Burning 효과'),('기존Regular/Dude','기존Regular/Burning'),('새Dude/Path','새Burning/Path')),
 PATHS[4]:(('DudeProcess14/Path24','Burning14(mode0)/Path24'),),
}


def expected_sources():
    """최초 전체 자료에 명시한 이름 교정만 적용해 현재 바이트를 독립 계산한다."""
    old=json.loads((HISTORY/REPORT.name).read_text(encoding='utf-8'));result={}
    # 각 최초 파일은 당시 원본 실행 보고서가 기록한 전체 SHA와 같아야 한다.
    for relative in PATHS[:-1]:
        path=HISTORY/Path(relative).name
        if digest(path)!=old['files'][relative]:raise RuntimeError('Dude 초기 전체자료 SHA 오류 '+relative)
        value=path.read_bytes()
        # 인자/원본주소/수치/출력행은 이름 교정에 포함되지 않는다.
        for before,after in CHANGES[relative]:value=value.replace(before.encode('utf-8'),after.encode('utf-8'))
        result[relative]=value
    return old,result


def corrected_report(old):
    """정상 관찰은 그대로 보존하고 현재 파일 SHA와 명칭의 정확한 출처만 연결한다."""
    report=copy.deepcopy(old)
    report['new_burning_constructors_completed']=report.pop('new_worker_constructors_completed')
    report['limitations']=[text.replace('DudeProcess14','Burning14(mode0)').replace('worker 경제','Burning 효과') for text in report['limitations']]
    # 같은 414개 자료의 주석과 공개 이름만 달라진 현재 전체 SHA다.
    for relative in PATHS[:-1]:report['files'][relative]=digest(ROOT/relative)
    report['draft_lineage']=dict(prior_full_report_sha256=digest(HISTORY/REPORT.name),normal_preserved=414,new_native=0,new_unique=0,
        reason='00423b20/CD004a5a50과type14는Burning이다. Notice wrapper모드0와Damageable wrapper모드1을구분하며전체최초6자료와414개관찰행을보존한다.')
    return report


def generate():
    """주석을 제외한 fixture 출력은 전부 유지하며 전체 변경분의 역변환 근거를 만든다."""
    old,expected=expected_sources()
    # 아직 등록 전 초안의 명시 이름 교정만 현재 자료에 적용한다.
    for relative,value in expected.items():(ROOT/relative).write_bytes(value)
    REPORT.write_text(json.dumps(corrected_report(old),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    LINEAGE.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',normal_preserved=414,new_native=0,new_unique=0,
        history_files={str((HISTORY/Path(relative).name).relative_to(ROOT).as_posix()):digest(HISTORY/Path(relative).name) for relative in PATHS},
        current_files={relative:digest(ROOT/relative) for relative in PATHS},tool_sha256=digest(__file__)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8');verify()


def verify():
    """이름의 정확한 변경과 전체 source/도구/보고서·414개 원본 출력 불변을 감사한다."""
    old,expected=expected_sources();lineage=json.loads(LINEAGE.read_text(encoding='utf-8'))
    # 최초 전체 자료의 명시 교정 밖 쓰기는 허용하지 않는다.
    for relative,value in expected.items():
        if (ROOT/relative).read_bytes()!=value:raise RuntimeError('Dude 초안 전체 이전몸체 연결 오류 '+relative)
    if json.loads(REPORT.read_text(encoding='utf-8'))!=corrected_report(old):raise RuntimeError('Dude 초안 전체보고서/관찰 오류')
    before=(HISTORY/Path(PATHS[4]).name).read_bytes().splitlines();current=(ROOT/PATHS[4]).read_bytes().splitlines()
    if [row for row in before if row and not row.startswith(b'#')]!=[row for row in current if row and not row.startswith(b'#')]:raise RuntimeError('Dude 414개 원본입력/출력행 변경')
    # 과거와 현재의 실제 전체 원문 SHA를 각각 검사한다.
    for group in ('history_files','current_files'):
        for relative,value in lineage[group].items():
            if digest(ROOT/relative)!=value:raise RuntimeError('Dude 초안 SHA 오류 '+relative)
    if lineage['tool_sha256']!=digest(__file__) or lineage['new_native'] or lineage['new_unique'] or old['total']!=414:raise RuntimeError('Dude 초안 도구/횟수 오류')
    print(json.dumps(dict(verified=True,normal_preserved=414,new_native=0,new_unique=0),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
