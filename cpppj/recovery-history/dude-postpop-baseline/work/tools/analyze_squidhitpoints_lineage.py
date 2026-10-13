#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""b76adf9 건설 공통HP쓰기 몸체 이동과 기존 전체소스/원본출처 보존을 감사한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent # 동일 공유저장소의 고정된경로다.
REPORT=ROOT/'cpppj/recovery-squidhitpoints-lineage-evidence.json' # 이전보고서를바꾸지않는새몸체이동근거다.
OLD_HEADER='cpppj/src/o/RawConstructionProcess.h' # 선언을그대로보존하는기존건설클래스다.
OLD_SOURCE='cpppj/src/o/RawConstructionProcess.cpp' # private본문과새include외에는전체소스를보존한다.
NEW_HEADER='cpppj/src/o/RawSquidHitPoints.h' # 공통직접쓰기의새공유선언이다.
NEW_SOURCE='cpppj/src/o/RawSquidHitPoints.cpp' # 기존본문을pool매개변수로옮긴새공유구현이다.
NATIVE='cpppj/recovery-constructionprocess-evidence.json' # 앞서검증한2868개원본관찰출처다.
INTEGRATED='cpppj/recovery-campaign-runtime7-evidence.json' # 기준07의공유Release/983개최종검사근거를보존한다.


def sha(data):
    """정확한파일/정규화토큰SHA256을계산한다."""
    return hashlib.sha256(data).hexdigest()


def git(*args):
    """고정git객체만읽고수정/commit/push하지않는다."""
    return subprocess.run(['git',*args],cwd=ROOT,capture_output=True,check=True).stdout


def tokens(text):
    """문자열리터럴을보존하며주석/공백만제외해모든기존토큰을비교한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',text)
    return '\0'.join(part for part in parts if not part.startswith(('//','/*'))).encode('utf-8')


def body(text,signature):
    """명시서명의중괄호본문전체를읽으며이몸체내문자열/중첩중괄호도보존한다."""
    start=text.index(signature);opening=text.index('{',start);depth=0
    # 새/기존setter는문자열리터럴이없고단일loop본문만있다.
    for index in range(opening,len(text)):
        depth+=text[index]=='{';depth-=text[index]=='}'
        if depth==0:return text[opening:index+1]
    raise AssertionError('HP setter 중괄호 없음')


def observe(commit):
    """새include/위임만되돌린전체소스와옮긴본문토큰이기준07과같음을증명한다."""
    baseline=git('show',commit+':'+OLD_SOURCE).decode('utf-8').replace('\r\n','\n');current=(ROOT/OLD_SOURCE).read_text(encoding='utf-8');header=git('show',commit+':'+OLD_HEADER).decode('utf-8').replace('\r\n','\n');assert (ROOT/OLD_HEADER).read_text(encoding='utf-8')==header
    signature='void RawConstructionProcess::SetHitPoints(Sid sid,std::int32_t value) const';old_body=body(baseline,signature);current_body=body(current,signature);assert tokens(current_body)==tokens('{ SetRawSquidHitPoints(pool_,sid,value); }')
    restored=current.replace('#include "o/RawSquidHitPoints.h"\n','').replace(current_body,old_body);assert restored==baseline and tokens(restored)==tokens(baseline)
    moved=body((ROOT/NEW_SOURCE).read_text(encoding='utf-8'),'void SetRawSquidHitPoints(SidPool& pool,Sid sid,std::int32_t value)');assert moved.replace('pool.','pool_.')==old_body and tokens(moved.replace('pool.','pool_.'))==tokens(old_body)
    native=json.loads((ROOT/NATIVE).read_text(encoding='utf-8'));assert native['total']==2868 and (ROOT/NATIVE).read_bytes()==git('show',commit+':'+NATIVE);assert (ROOT/INTEGRATED).read_bytes()==git('show',commit+':'+INTEGRATED)
    # 이전보호PE/원본몸체/fixture/도구의정확한SHA는변경없이그대로검증한다.
    for name,value in native['files'].items():assert sha((ROOT/name).read_bytes())==value,name
    preserved={name:sha((ROOT/name).read_bytes()) for name in (NATIVE,INTEGRATED,'cpppj/src/o/RawFortObjectLoad.h','cpppj/src/o/RawFortObjectLoad.cpp')}
    for name in preserved:assert (ROOT/name).read_bytes().replace(b'\r\n',b'\n')==git('show',commit+':'+name).replace(b'\r\n',b'\n'),name
    return dict(baseline_source_sha256=sha(baseline.encode('utf-8')),restored_existing_source_sha256=sha(restored.encode('utf-8')),existing_tokens_sha256=sha(tokens(baseline)),moved_body_tokens_sha256=sha(tokens(old_body)),all_other_existing_source_unchanged=True,moved_body_only_identifier_change='pool_ → pool 매개변수',preserved_files=preserved,old_native_cases_preserved_not_recounted=2868)


def generate():
    """기준07을고정하고현재새공유API SHA와본문이동lineage만새로기록한다."""
    commit=git('rev-parse','b76adf9').decode('ascii').strip();result=observe(commit);files={name:sha((ROOT/name).read_bytes()) for name in (OLD_HEADER,OLD_SOURCE,NEW_HEADER,NEW_SOURCE)}
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',baseline_commit=commit,files=files,lineage=result,tool_sha256=sha(Path(__file__).read_bytes()),new_unique_cases=0,source_addresses=dict(originals='004adca0 DWORD@26',originalCD='004aed20 lowWORD@26'),scope='공통직접쓰기만이동한다. 사제가상HP전환/건설완료/표시/notify/회복효과를대체하지않으며RawFortObjectLoad는그대로다.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """기존전체소스/이전관찰보존과현재공유API파일SHA를다시감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));assert report['new_unique_cases']==0 and report['lineage']==observe(report['baseline_commit']);assert report['files']=={name:sha((ROOT/name).read_bytes()) for name in (OLD_HEADER,OLD_SOURCE,NEW_HEADER,NEW_SOURCE)};assert report['tool_sha256']==sha(Path(__file__).read_bytes());print('공통HP쓰기 body 이동·기준07 전체기존소스/토큰·기존2868 관찰 보존')


# 명시호출에서만새이동근거를만들거나현재SHA를감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
