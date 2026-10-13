#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 같은 Host 소비자의 실제 최초 Factory 입력을 이전 전체 문법/원본 근거와 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 workspace의 전체 파일을 읽는 기준이다.
HISTORY=ROOT/'cpppj/hist/priest-arriv-dbe45' # 최초 CPP 실행 실패 전후의 전체 의존 자료를 보존한다.
TEST='cpppj/tests/PriestPathArrivalHostTests.cpp' # Native 입력의 합성 슬롯을 실제 생성 상태로 바꾸는 소비자다.
PRIOR='cpppj/priest-arrival-test-syntax-lineage.json' # 이전 문법 successor 원문은 덮지 않는다.
REPORT=ROOT/'cpppj/priest-arrival-factory-input-lineage.json' # 동일 원본1270/생산 불변의 새 소비자 successor다.
OLD='CHECK(factory.Construct(158,Sid{50})==Sid{50});' # Construct만 쓰면 Native 입력이 지운 Allocate void4가 복구되지 않는다.
NEW='scene.pool.Reset();for (std::uint16_t sid=5;sid<50;++sid) CHECK(scene.pool.Allocate(2)==Sid{sid});CHECK(factory.Create(158,2)==Sid{50});' # 실제 Allocate→constructor→type→PostCreate의 같은 부모50을 만든다.
COMMENT='        // Native용 슬롯은 Allocate의 void4를 지웠으므로 같은 풀을 재설정하고 실제5..49 할당 뒤 Factory.Create로 부모50을 만든다.\n' # 새 초기화/반복문의 용도와 입력 보존을 설명한다.

def sha(path):
    """파일 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def capture(logs):
    """현재 전체 생산/Native/fixture/name/syntax/전수/Host/지원헤더/종료 실패로그를 보존한다."""
    assert not HISTORY.exists();old=importlib.import_module('analyze_priest_arrival_test_syntax_lineage')
    data=json.loads((ROOT/PRIOR).read_text(encoding='utf-8'));assert data==old.expected()
    names={PRIOR,'tools/analyze_priest_arrival_test_syntax_lineage.py',TEST,*data['files'],
           'docs/exe/cpp-priest-halt-20261014.md','docs/exe/cpp-carrier-arrival-audit-20261014.md'}
    # 최초 실제 실행 오류와 완료된 build 로그를 전체 바이트로 저장한다.
    for name in logs:
        resolved=(ROOT/name).resolve();relative=resolved.relative_to(ROOT).as_posix();assert relative.startswith('extracted/') and resolved.is_file();names.add(relative)
    saved={}
    # 전체 파일 저장과 전체 SHA가 같은 다음 입력 교정의 기준이다.
    for name in sorted(names):
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/name).read_bytes());saved[name]=sha(target)
    (HISTORY/'baseline.json').write_text(json.dumps(dict(files=saved,logs=list(logs)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Factory 입력 교정 전 전체 보존:',len(saved))

def inverse():
    """실제 Factory 준비 한 조각과 설명만 되돌려 최초 실행 소비자 전체를 복원한다."""
    value=(ROOT/TEST).read_bytes()
    # 입력셋업 외 명령/좌표/프레임/경로/Host/효과/CHECK는 그대로 유지해야 한다.
    for after,before in ((COMMENT,''),(NEW,OLD)):
        token=after.encode('utf-8');assert value.count(token)==1,after;value=value.replace(token,before.encode('utf-8'),1)
    assert value==(HISTORY/TEST).read_bytes();return value

def expected():
    """같은 입력 소비자의 정확 역변환과 이전 syntax/name/Carrier 출처를 재현한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'));restored=inverse()
    for name,digest in baseline['files'].items():
        assert sha(HISTORY/name)==digest,name
        if name==TEST:assert hashlib.sha256(restored).hexdigest()==digest
        elif not name.startswith(('docs/','extracted/run14-')):assert sha(ROOT/name)==digest,name
    old=importlib.import_module('analyze_priest_arrival_test_syntax_lineage');original=Path.read_bytes
    def historical(path):
        """이전 문법 감사가 최초 CPP 실행의 전체 검사 바이트를 읽도록 한다."""
        return original(HISTORY/TEST if path.resolve()==(ROOT/TEST).resolve() else path)
    Path.read_bytes=historical
    try:assert json.loads(old.REPORT.read_text(encoding='utf-8'))==old.expected()
    finally:Path.read_bytes=original
    # 실제 Factory가 요구하는 Allocate void4와 Construct의 몸체 한계를 기존 원문에서 확인한다.
    pool=(ROOT/'cpppj/src/o/SidPool.cpp').read_text(encoding='utf-8');factory=(ROOT/'cpppj/src/o/SquidFactory.cpp').read_text(encoding='utf-8')
    assert 'bytes_[offset+kState]=kVoid;' in pool and 'ApplyConstructor(bytes,record); return sid;' in factory
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,
                preserved_halt_normal_inputs=864,preserved_carrier_normal_inputs=406,original_normal_recount=0,
                whole_consumer_exact_inverse=True,all_production_native_fixture_input_bytes_unchanged=True,
                prior_syntax_name_carrier_binding_reproduced=True,prior_syntax_sha256=sha(ROOT/PRIOR),
                old_test_sha256=sha(HISTORY/TEST),current_test_sha256=sha(ROOT/TEST),tool_sha256=sha(Path(__file__)),
                files={name:sha(ROOT/name) for name in baseline['files'] if not name.startswith(('docs/','extracted/run14-'))},
                history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()},
                scope='같은Host의scene0은다른후보가없다. Native용 raw입력의 state0/해시삭제 후 Construct만 실행해 Pop이void4를못찾은 입력구성을 실제같은Pool Reset→Allocate5..49→Factory.Create(158,2) 부모50으로 교정한다. HP200/owner1/좌표/프레임/수동경로/명령/후처리/CHECK는불변이고Native864·406/전수검사/Support/생산body변경0이다. 기존syntax/name/Carrier보고서원문은당시Test view로재현하며새원본관찰0이다.')

def main(arguments):
    """교정 전 전체 보존 또는 현재 최소 Factory 입력 교정 successor 감사를 수행한다."""
    if arguments.capture:capture(arguments.log);return
    data=expected()
    if arguments.verify:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 Factory 입력: 실제Allocate/Create/같은부모50/전체검사정확역변환/생산·Native1270불변/기존근거재현')

# 명시 CLI에서만 종료 원문 보존/현재 successor 작성/감사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',action='store_true')
    parser.add_argument('--log',action='append',default=[]);parser.add_argument('--verify',action='store_true');main(parser.parse_args())
