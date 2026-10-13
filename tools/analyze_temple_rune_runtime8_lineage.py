#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""08 전수 대조의 지면 입력 충돌/시간 this 교정과 수정 전 원본 근거 보존을 감사한다."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent # 같은 공유 저장소의 절대 기준이다.
SNAPSHOT=ROOT/'cpppj/hist/temple-rune-afcc3' # 수정 전 전용 소스/검사/도구/fixture/보고서를 커밋 가능한 원경로로 보존한다.
REPORT=ROOT/'cpppj/temple-rune-runtime8-lineage.json' # 기존 등록/HP 이동 근거를 보존하는 별도 교정 보고서다.
RUNE_COMMENT='// 발견한 Dais가 시간 helper의 this다. Rune의 위치로 대신 계산하지 않는다(두 판본의 실제 ECX 추적 확인).\n' # 교정 설명만 역제거할 때 쓰는 정확한 한줄이다.
MODULES=('templeritual','runepostpop') # 새 고유 입력 수를 늘리지 않고 현재 결과로 대체하는 두 모듈이다.


def sha(data):
    """파일 바이트와 기존 토큰의 정확한 SHA256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def tokens(text):
    """문자열 리터럴을 보존하고 주석/공백만 제외한다."""
    parts=re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]',text)
    return '\0'.join(part for part in parts if not part.startswith(('//','/*'))).encode('utf-8')


def restore(name,text):
    """명시된 두 교정 외 기존 전체 본문은 되돌리거나 변경하지 않는다."""
    if name=='cpppj/src/o/RawRunePostPop.cpp':
        assert text.count(RUNE_COMMENT)==1 and text.count('ConstructionSeconds(dais)')==1
        return text.replace(RUNE_COMMENT,'').replace('ConstructionSeconds(dais)','ConstructionSeconds(rune)')
    if name=='tools/decomp_templeritual_oracle.py':
        prefix='RITUAL_SPOTS=0x11600000 # '
        lines=text.splitlines(keepends=True);selected=[line for line in lines if line.startswith(prefix)];assert len(selected)==1
        text=text.replace(selected[0],'')
        allocation=';self.mu.mem_map(RITUAL_SPOTS,0x10000);self.put(self.g[\'spots\'],RITUAL_SPOTS)'
        assert text.count(allocation)==1 and text.count('self.mu.mem_write(RITUAL_SPOTS,')==1
        return text.replace(allocation,'').replace('self.mu.mem_write(RITUAL_SPOTS,','self.mu.mem_write(SPOTS,')
    return text


def rows(path):
    """독립 fixture의 입력 식별자와 현재 결과를 그대로 읽는다."""
    return [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]


def observe():
    """수정 전 관찰 출처, 전체 기존 소스와 동일 입력 재관찰을 함께 증명한다."""
    snapshots={};current={};source={};modules={}
    # 모든 수정 전 보존 파일을 현재 파일과 혼동하지 않고 개별 바이트 SHA로 기록한다.
    for path in sorted(SNAPSHOT.rglob('*')):
        if path.is_file():snapshots[path.relative_to(ROOT).as_posix()]=sha(path.read_bytes())
    assert snapshots
    # 원본 관찰 출처는 당시 보고서가 가진 SHA로 계속 확인한다.
    for module,expected in zip(MODULES,(6720,1920)):
        name=f'cpppj/{module}.json';before=json.loads((SNAPSHOT/name).read_text(encoding='utf-8'));after=json.loads((ROOT/name).read_text(encoding='utf-8'))
        assert before['total']==after['total']==expected and before['game_runs']==after['game_runs']==0
        for file,value in before['files'].items():
            preserved=SNAPSHOT/file
            assert sha((preserved if preserved.exists() else ROOT/file).read_bytes())==value,file
        # 재관찰 보고서가 끝나 현재 production/test/tool/원본 출처 SHA까지 일치해야 최종 lineage를 만든다.
        for file,value in after['files'].items():assert sha((ROOT/file).read_bytes())==value,file
        fixture=f'cpppj/tests/fixtures/{module}-x86.tsv';old_rows=rows(SNAPSHOT/fixture);new_rows=rows(ROOT/fixture)
        old_ids=[tuple(row[:2]) for row in old_rows];new_ids=[tuple(row[:2]) for row in new_rows]
        assert old_ids==new_ids and len(set(old_ids))==expected
        if module=='runepostpop':assert (SNAPSHOT/fixture).read_bytes()==(ROOT/fixture).read_bytes()
        modules[module]=dict(unique_cases=expected,additional_unique_cases=0,case_identifiers_unchanged=True,prior_report_sha256=sha((SNAPSHOT/name).read_bytes()),current_report_sha256=sha((ROOT/name).read_bytes()),fixture_changed_rows=sum(old!=new for old,new in zip(old_rows,new_rows)))
        current[name]=sha((ROOT/name).read_bytes());current[fixture]=sha((ROOT/fixture).read_bytes())
    # fixture/보고서를 제외한 전체 파일에 허용된 교정만 역적용하면 이전 문자/토큰과 같다.
    for preserved in sorted(SNAPSHOT.rglob('*')):
        if not preserved.is_file():continue
        name=preserved.relative_to(SNAPSHOT).as_posix()
        if name.endswith(('.h','.cpp','.py')):
            old=preserved.read_text(encoding='utf-8');now=(ROOT/name).read_text(encoding='utf-8');restored=restore(name,now);assert old==restored and tokens(old)==tokens(restored),name
            source[name]=dict(current_sha256=sha((ROOT/name).read_bytes()),prior_sha256=sha(preserved.read_bytes()),prior_token_sha256=sha(tokens(old)),restored_entire_source_unchanged=True)
            current[name]=sha((ROOT/name).read_bytes())
        elif name=='cpppj/runepostpop-lineage.json':
            assert preserved.read_bytes()==(ROOT/name).read_bytes();current[name]=sha((ROOT/name).read_bytes())
    return dict(snapshot_files=snapshots,current_files=current,source_lineage=source,modules=modules)


def generate():
    """원본 재관찰의 현재 SHA와 이전 출처 보존을 별도 JSON으로 연결한다."""
    data=dict(host='HJOW-Athlon',lineage=observe(),tool_sha256=sha(Path(__file__).read_bytes()),additional_unique_cases=0,initial_cpp_failed_checks=dict(templeritual=1330,runepostpop=26),corrections=['Temple oracle: 네 해시 배열과 지면 주소의 겹침을 전용 지면 주소로 교정. 같은 입력 식별자6720개를 전량 재관찰하며 이전 결과를 보존한다.','Rune production: 시간 helper의 this를 발견 Dais로 교정. 실제 원본 ECX 추적은 standalone Rune8, PostPop 내부 Dais6이며 fixture1920개는 변하지 않는다.','두 production/test의 나머지 전체 문자/토큰과 기존 SquidPostPop/HP/등록 근거는 보존한다. 임시 진단출력은 최종 검사에서 제거했다.'])
    REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


def verify():
    """현재 소스·동일 고유 입력·보존 출처가 보고서와 정확히 같은지 확인한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));assert data['additional_unique_cases']==0 and data['lineage']==observe() and data['tool_sha256']==sha(Path(__file__).read_bytes())
    print('08 Temple/Rune 교정 lineage: 전체 기존 소스/검사·이전 출처 보존, 추가 고유 입력0')


# 명시 호출에서만 새 교정 보고서를 만들거나 현재 SHA를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
