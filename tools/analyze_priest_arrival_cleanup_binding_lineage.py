#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 Host cleanup의 실제 삭제flags/필수contained 조회 연결을 이전 전체 근거와 연결한다."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1] # 현재 저장소의 전체 바이트를 읽는다.
HISTORY=ROOT/'cpppj/recovery-history/priest-arrival-before-cleanup-binding' # 두 번째 Host 실패의 전체 현재 자료를 보존한다.
TEST='cpppj/tests/PriestPathArrivalHostTests.cpp' # 현재 같은 Pool/Host의 도착/삭제 소비자다.
PRIOR='cpppj/recovery-priest-arrival-factory-input-lineage-evidence.json' # 이전 Factory 입력 successor 원문은 그대로 둔다.
REPORT=ROOT/'cpppj/recovery-priest-arrival-cleanup-binding-lineage-evidence.json' # 원본1270 불변의 현재 소비자 연결 successor다.
OLD='CHECK(parent==Sid{50} && flags==0);++cleanups;scene.cleanup->AfterSpatial(current,parent);' # 잘못된 client Form flags와 미연결 조회의 이전 문장이다.
NEW='CHECK(parent==Sid{50} && flags==0x10);++cleanups;pathCleanup.AfterSpatial(current,parent);' # 서버의 client Form 삭제flags와 실제 새consumer cleanup을 연결한다.
BLOCK='''        // 패치 cleanup은귀환장소0이어도contained를조회하므로같은Pool의기존실제FindContained를연결한다.
        PathCleanupHooks finalCleanupHooks;finalCleanupHooks.reserveShared=ArrivalForbidden<bool>();finalCleanupHooks.addShared=ArrivalForbidden<void,Sid,std::uint32_t,float>();
        finalCleanupHooks.contained=[&](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { return scene.carrier->FindContained(sid,mask,kind,index); };
        finalCleanupHooks.graphFor=ArrivalForbidden<std::uint32_t,Sid>();finalCleanupHooks.ritual=ArrivalForbidden<void,Sid,Sid,std::uint32_t>();finalCleanupHooks.refund=ArrivalForbidden<void,std::uint32_t,std::uint32_t>();finalCleanupHooks.destroy=ArrivalForbidden<void,Sid,std::uint32_t>();RawPathCleanup pathCleanup(scene.pool,scene.hash,scene.types,scene.cleanupState,std::move(finalCleanupHooks));
''' # 새 local의 필수 getter 하나만 실제 연결하고 미선택 큰 효과의 strict 거부를 유지한다.

def sha(path):
    """파일 전체 바이트 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def capture(logs):
    """현재 전체Factory/syntax/name/생산/Native/fixture/검사/Support/완성로그를 먼저 저장한다."""
    assert not HISTORY.exists();prior=importlib.import_module('analyze_priest_arrival_factory_input_lineage')
    data=json.loads((ROOT/PRIOR).read_text(encoding='utf-8'));assert data==prior.expected()
    names={PRIOR,'tools/analyze_priest_arrival_factory_input_lineage.py',TEST,*data['files'],
           'docs/exe/cpp-priest-halt-20261014.md','docs/exe/cpp-carrier-arrival-audit-20261014.md'}
    # 이미 종료된 두 번째 Host와 다섯 번째 빌드 로그만 전체 바이트로 저장한다.
    for name in logs:
        resolved=(ROOT/name).resolve();relative=resolved.relative_to(ROOT).as_posix();assert relative.startswith('extracted/') and resolved.is_file();names.add(relative)
    saved={}
    # 현재 의존 파일의 전체 원문과 전체 SHA를 같은 역사 경로에 보존한다.
    for name in sorted(names):
        target=HISTORY/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/name).read_bytes());saved[name]=sha(target)
    (HISTORY/'baseline.json').write_text(json.dumps(dict(files=saved,logs=list(logs)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 cleanup 연결 전 전체 보존:',len(saved))

def inverse():
    """새local getter 연결과client 삭제flags 기대만 되돌려 이전 검사 전체를 복원한다."""
    value=(ROOT/TEST).read_bytes()
    # 두 고유 변경 조각 밖의 입력/HP/owner/경로/명령/공간/기존 CHECK는 그대로 유지한다.
    for after,before in ((BLOCK,''),(NEW,OLD)):
        token=after.encode('utf-8');assert value.count(token)==1,after;value=value.replace(token,before.encode('utf-8'),1)
    assert value==(HISTORY/TEST).read_bytes();return value

def expected():
    """생산/원본 불변과 이전Factory→syntax→name→Carrier의 당시Test view를 감사한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'));restored=inverse()
    for name,digest in baseline['files'].items():
        assert sha(HISTORY/name)==digest,name
        if name==TEST:assert hashlib.sha256(restored).hexdigest()==digest
        elif not name.startswith(('docs/','extracted/campaign-runtime14-')):assert sha(ROOT/name)==digest,name
    prior=importlib.import_module('analyze_priest_arrival_factory_input_lineage');original=Path.read_bytes
    def historical(path):
        """직전Factory 감사가 저장한 전체 이전 Host 검사를 읽도록 한다."""
        return original(HISTORY/TEST if path.resolve()==(ROOT/TEST).resolve() else path)
    Path.read_bytes=historical
    try:assert json.loads(prior.REPORT.read_text(encoding='utf-8'))==prior.expected()
    finally:Path.read_bytes=original
    destroy=(ROOT/'cpppj/src/o/RawSquidDestroy.cpp').read_text(encoding='utf-8');cleanup=(ROOT/'cpppj/src/o/RawPathCleanup.cpp').read_text(encoding='utf-8')
    assert 'if (pool_.IsServer() && client) flags|=kClient;' in destroy
    assert 'const auto nugget=hooks_.contained(parent,kNuggetMask,state_.nuggetType,0);' in cleanup
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',new_native_inputs=0,preserved_normal_inputs=1270,
                original_halt_and_carrier_recount=0,whole_test_exact_inverse=True,production_native_fixture_support_unchanged=True,
                prior_factory_syntax_name_carrier_reproduced=True,prior_factory_sha256=sha(ROOT/PRIOR),tool_sha256=sha(Path(__file__)),
                current_test_sha256=sha(ROOT/TEST),previous_test_sha256=sha(HISTORY/TEST),
                files={name:sha(ROOT/name) for name in baseline['files'] if not name.startswith(('docs/','extracted/campaign-runtime14-'))},
                history={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(HISTORY.rglob('*')) if path.is_file()},
                scope='서버가client Form을Kill0하면실제Destroy가flags0x10을추가하므로cleanup 기대만0→0x10으로바꾼다. 패치cleanup은부모가live면ReturnPlace0이어도contained를조회하므로consumer local cleanup에같은기존Carrier.FindContained만직접연결한다. ContentForm6이없는scene0에서실제Form54는자연제외되며빈getter/noop을공급하지않는다. helper0/다이스없음등미선택큰효과strict예외는유지한다. 입력/생산/Native864·406/Support/전수검사/이전보고서변경0,새원본입력0이다.')

def main(arguments):
    """교정 전 전체 저장 또는 현재 최소cleanup 소비자 연결 successor 감사를 수행한다."""
    if arguments.capture:capture(arguments.log);return
    data=expected()
    if arguments.verify:assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('사제 cleanup: clientflags0x10/실제FindContained/전체검사역변환/생산·Native1270불변/이전전체근거재현')

# 명시 CLI에서만 전체 보존/현재 successor 생성/감사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',action='store_true')
    parser.add_argument('--log',action='append',default=[]);parser.add_argument('--verify',action='store_true');main(parser.parse_args())
