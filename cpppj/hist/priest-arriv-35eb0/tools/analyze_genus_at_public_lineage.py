#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 GenusAt 선언 공개 이동을 기준13 전체 Git/작업 원문과 기존4412 근거에 연결한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 공유 workspace와 Git 원문을 읽는 기준이다.
BASELINE='860030e' # 단위13 최종1066 성공 뒤 커밋된 기준이다.
HISTORY=ROOT/'cpppj/hist/genus-at-pub-4fe2a' # 기존 전체 Git/LF와 작업 바이트를 별도 보존한다.
HEADER='cpppj/src/o/RawDamageablePostDestroy.h' # private→public 선언 위치만 바뀌는 기존 헤더다.
CPP='cpppj/src/o/RawDamageablePostDestroy.cpp' # 실제 getter/드롭/HP/다리/공통post 몸체는 변경하지 않는다.
REPORT=ROOT/'cpppj/genus-at-public-lineage.json' # 새관찰0과 동일 기존4412를 current header와 연결한다.
DECLARATION='    Sid GenusAt(float x,float y,std::uint32_t genus) const; // plain 절삭한 한 칸의 첫 일반 finder 후보를 조회한다.\n' # 기존 선언/설명을 전체 그대로 이동한다.

def sha(path):
    """파일 전체 바이트 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def capture():
    """수정 전 기존 두 생산 파일/4412보고서/Host/관련 관찰/fixture의 Git·작업 전체를 보존한다."""
    assert not HISTORY.exists()
    names={HEADER,CPP,'cpppj/tests/DamageablePostDestroyTests.cpp','cpppj/tests/DamageablePostDestroyHostTests.cpp','cpppj/tests/fixtures/damageablepostdestroy-x86.tsv',
           'tools/decomp_damageablepostdestroy_oracle.py','tools/analyze_damageablepostdestroy.py','tools/analyze_damageablepost_host_binding.py','tools/analyze_damageablepost_host_group_lineage.py'}
    # 기존 현재 보고서의 shared 경로 참조를 직접 찾아 당시 관찰/소스 관계 전체를 보존한다.
    for path in (ROOT/'cpppj').glob('recovery-*.json'):
        if HEADER in path.read_text(encoding='utf-8') or CPP in path.read_text(encoding='utf-8'):names.add(path.relative_to(ROOT).as_posix())
    saved={}
    # 기준 Git 원문과 현재 작업 원문은 별도 저장하며 줄끝 차이도 감추지 않는다.
    for name in sorted(names):
        work=HISTORY/'work'/name;git=HISTORY/'git'/name;work.parent.mkdir(parents=True,exist_ok=True);git.parent.mkdir(parents=True,exist_ok=True)
        work.write_bytes((ROOT/name).read_bytes());git.write_bytes(subprocess.check_output(['git','show',BASELINE+':'+name],cwd=ROOT))
        assert work.read_bytes().replace(b'\r\n',b'\n')==git.read_bytes().replace(b'\r\n',b'\n'),name
        saved[name]=dict(work_sha256=sha(work),git_sha256=sha(git))
    (HISTORY/'baseline.json').write_text(json.dumps(dict(commit=BASELINE,files=saved),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('GenusAt 기준 전체 Git/작업 보존:',len(saved))

def expected():
    """선언의 정확 역이동과 현재 전체 나머지 몸체/기존4412 근거 불변을 감사한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'));assert baseline['commit']==BASELINE
    inverse=(ROOT/HEADER).read_bytes();newline=b'\r\n' if b'\r\n' in inverse else b'\n';declaration=DECLARATION.encode('utf-8').replace(b'\n',newline)
    assert inverse.count(declaration)==1
    insertion=('    const SidPool& Pool() const;\n'+DECLARATION).encode('utf-8').replace(b'\n',newline)
    assert inverse.count(insertion)==1
    inverse=inverse.replace(insertion,b'    const SidPool& Pool() const;'+newline,1)
    prior='    std::array<float,2> Position(Sid sid) const; // 현재 raw float 좌표의 비트다.\n'.encode('utf-8').replace(b'\n',newline)
    assert inverse.count(prior)==1;inverse=inverse.replace(prior,prior+declaration,1)
    # 정확 역이동한 헤더와 모든 기존 보고서/fixture/검사/도구/Cpp 몸체를 전체 비교한다.
    for name,item in baseline['files'].items():
        work=HISTORY/'work'/name;git=HISTORY/'git'/name
        assert sha(work)==item['work_sha256'] and sha(git)==item['git_sha256'],name
        assert git.read_bytes()==subprocess.check_output(['git','show',BASELINE+':'+name],cwd=ROOT),name
        assert (inverse if name==HEADER else (ROOT/name).read_bytes())==work.read_bytes(),name
    native=json.loads((ROOT/'cpppj/damageablepostdestroy.json').read_text(encoding='utf-8'));assert native['total']==4412 and native['normal_total']==4412 and native['assertion_total']==0
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',baseline_commit=BASELINE,new_native_inputs=0,preserved_normal_inputs=4412,preserved_assert_inputs=0,
                declaration_exact_inverse=True,all_cpp_bodies_unchanged=True,existing_reports_and_fixtures_unchanged=True,baseline_files=baseline['files'],
                current_sources={name:sha(ROOT/name) for name in (HEADER,CPP)},tool_sha256=sha(Path(__file__)),history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()},
                scope='기존 GenusAt 선언/설명 한 줄을 private에서 public으로 이동한다. 기존 전체 getter/드롭/섬정리/주변HP/다리/공통post 계산/포인터/필드/조건은 불변이며4412/Host/관련관찰을 재합산하지 않는다. 새 CarrierArrival의 같은 getter 소비자 연결은 별도 Native406/CPP 근거로 검증한다.')

def main(arguments):
    """수정 전 저장 또는 최소 선언 이동의 current lineage 생성/검증을 수행한다."""
    if arguments.capture:capture();return
    data=expected()
    if arguments.verify:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('GenusAt 공개 이동: 기준13 전체 Git/작업/선언정확역이동/Cpp·기존4412불변/새관찰0')

# 명시 호출에서만 기준 전체 자료를 보존하거나 current lineage를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',action='store_true');parser.add_argument('--verify',action='store_true');main(parser.parse_args())
