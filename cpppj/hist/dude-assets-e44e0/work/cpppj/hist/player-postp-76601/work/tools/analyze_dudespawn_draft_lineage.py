#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DudeSpawn 최초288원본과 C++ 초안의 archive 경로 교정을 전체 원문으로 연결한다."""
import argparse
import json
from decomp_owner_oracle import ROOT,digest

# 최초 원본 관찰을 만든 전체6파일과 현재 successor 보고서다.
HISTORY=ROOT/'cpppj/hist/dudespawn-initial'
NATIVE=ROOT/'cpppj/dudespawn.json'
REPORT=ROOT/'cpppj/dudespawn-draft-lineage.json'
TEST='cpppj/tests/DudeSpawnTests.cpp'
# 컴파일 전 초안에서 존재하지 않는 Root() 접근을 명시 판본 root 입력으로 옮긴 정확 세 변화다.
CHANGES=(
 ('struct DudeAssets {\n','struct DudeAssets {\n    std::filesystem::path root; // 판본의 보호 archive 경로이며 BaseFileSystem과 같은 입력이다.\n'),
 ('explicit DudeAssets(OriginalEdition edition):files(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")),game(Register(),edition)',
  'explicit DudeAssets(OriginalEdition edition):root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")),files(root),game(Register(),edition)'),
 ('files.RegisterArchive(files.Root()/"netstorm.tarc")','files.RegisterArchive(root/"netstorm.tarc")'),
)


def check():
    """전체 초안을 역변환하고 원본 fixture/도구/production의 전체 바이트 보존을 확인한다."""
    old=json.loads((HISTORY/'cpppj/dudespawn.json').read_text(encoding='utf-8'));current=(ROOT/TEST).read_text(encoding='utf-8')
    # 변화마다 정확 한 번만 제거하며 나머지 test 전체를 보존한다.
    for before,after in CHANGES:
        if current.count(after)!=1:raise RuntimeError('DudeSpawn 초안 교정 위치 오류')
        current=current.replace(after,before,1)
    if current!=(HISTORY/TEST).read_text(encoding='utf-8'):raise RuntimeError('DudeSpawn test 전체 역변환 불일치')
    for name in ('cpppj/src/o/RawDudeSpawn.h','cpppj/src/o/RawDudeSpawn.cpp','tools/decomp_dudespawn_oracle.py','cpppj/tests/fixtures/dudespawn-x86.tsv'):
        if (ROOT/name).read_bytes()!=(HISTORY/name).read_bytes() or digest(ROOT/name)!=old['files'][name]:raise RuntimeError('DudeSpawn 원본/source 보존 불일치 '+name)
    return old


def generate():
    """최초 보고서를 보존한 채 같은288입력/newnative0의 현재 test SHA를 연결한다."""
    old=check();history={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()};paths=[ROOT/TEST,ROOT/'cpppj/src/o/RawDudeSpawn.h',ROOT/'cpppj/src/o/RawDudeSpawn.cpp',ROOT/'tools/decomp_dudespawn_oracle.py',ROOT/'cpppj/tests/fixtures/dudespawn-x86.tsv',ROOT/'tools/analyze_dudespawn_draft_lineage.py'];report=dict(schema=1,host='HJOW-Athlon',total=old['total'],new_native=0,cpp_build_attempts=0,history=history,files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},reason='미공유 컴파일 초안의 BaseFileSystem에 없는 Root() 접근을 이미 같은 입력인 판본 root 경로 멤버로 옮긴다. 최초 전체6자료/288원본과 미완료 shared Pop 분배 경계를 보존한다.')
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');native=json.loads(NATIVE.read_text(encoding='utf-8'));native['files'][TEST]=digest(ROOT/TEST);native['files']['tools/analyze_dudespawn_draft_lineage.py']=digest(ROOT/'tools/analyze_dudespawn_draft_lineage.py');native['files'][REPORT.relative_to(ROOT).as_posix()]=digest(REPORT);native['draft_lineage']=dict(previous_report_sha256=digest(HISTORY/'cpppj/dudespawn.json'),previous_fixture_sha256=digest(HISTORY/'cpppj/tests/fixtures/dudespawn-x86.tsv'),new_native=0,cpp_build_attempts=0);NATIVE.write_text(json.dumps(native,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');verify()


def verify():
    """현재/과거 원문의 SHA와 같은 원본 격자288의 불변을 감사한다."""
    old=check();report=json.loads(REPORT.read_text(encoding='utf-8'));native=json.loads(NATIVE.read_text(encoding='utf-8'))
    for values in (report['history'],report['files']):
        for name,expected in values.items():
            if digest(ROOT/name)!=expected:raise RuntimeError('DudeSpawn 초안 lineage SHA 오류 '+name)
    if native['total']!=old['total'] or native['editions']!=old['editions'] or report['new_native'] or native['draft_lineage']['new_native']:raise RuntimeError('DudeSpawn 이전 관찰 불변 오류')
    print(json.dumps(dict(verified=True,total=old['total'],new_native=0,history_files=len(report['history']),cpp_completed=False),ensure_ascii=False))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
