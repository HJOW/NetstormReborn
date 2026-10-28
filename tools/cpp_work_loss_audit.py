#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""중단 뒤 추적 파일·완료 제품·원시 근거를 읽기 대조하고 cpppj 작업만 복구 패치로 보존한다."""
import argparse
import difflib
import hashlib
import json
import subprocess
from pathlib import Path

# 이 점검의 저장소와 완료542 기준이다. 원본/소스 파일에는 쓰지 않는다.
ROOT=Path(__file__).resolve().parents[1]
BASELINE=ROOT/'extracted/cpp-construction-mana-542/freeze-final.json'
PROTECTED=ROOT/'extracted/cpp-object-hit-524/protected-before.json'
WORK=('cpppj/src/client/GameMenuCommands.cpp','cpppj/src/client/UberGump.cpp',
      'cpppj/tests/ProductionInitialContextTests.cpp','tools/cpp_chat_divider_smoke.py')


def sha(path):
    """파일 전체 바이트를 읽으며 없어진 파일은 호출자가 별도로 분류한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    """해당 작업 PC가 보존한 UTF-8 JSON 근거를 읽는다."""
    return json.loads(path.read_text(encoding='utf8'))


def git(*args):
    """경로를 셸 문자열에 끼우지 않고 같은 저장소의 읽기 명령을 실행한다."""
    return subprocess.check_output(['git','-c','core.autocrlf=false',*args],cwd=ROOT)


def compare(manifest):
    """삭제와 내용 변경을 구별하며 다른 세션의 파일을 복원하거나 덮어쓰지 않는다."""
    missing=[]
    changed=[]
    for name,digest in manifest.items():
        path=ROOT/name
        if not path.is_file():missing.append(name)
        elif sha(path)!=digest:changed.append(name)
    return {'count':len(manifest),'missing':missing,'changed':changed}


def main():
    """Git 전체 파일 존재·완료542·보호1819·완료 근거와 현재4파일 보존을 한 보고서로 고정한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--recovery-patch',type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=True)
    initial=read(output/'current-work.json')
    head=git('rev-parse','HEAD').decode().strip()
    status=git('status','--short').decode('utf8')
    missing=[]
    emptied=[]
    count=0
    for entry in git('ls-tree','-r','-l','-z','HEAD').split(b'\0'):
        # Git blob만 검사하며 서브모듈 포인터를 일반 파일로 오인하지 않는다.
        if not entry:continue
        metadata,name=entry.split(b'\t',1)
        mode,kind,oid,size=metadata.split()
        if kind!=b'blob':continue
        count+=1
        relative=name.decode('utf8')
        path=ROOT/relative
        if not path.is_file():missing.append(relative)
        elif int(size)>0 and path.stat().st_size==0:emptied.append(relative)
    baseline=read(BASELINE)
    source=compare(baseline['source'])
    source['expectedWorkChanges']=list(WORK[:3])
    source['unexpectedChanges']=[name for name in source['changed'] if name not in WORK[:3]]
    protected=read(PROTECTED)
    protectedResult=compare(protected['files'])
    protectedResult['agentsUnchanged']=sha(ROOT/'AGENTS.md')==protected['agents']
    evidence=[]
    for name in ('cpppj/priest-display-20261028.json','cpppj/construction-mana-20261028.json'):
        # 완료541/542가 참조하는 원시 자료62개를 실제 파일과 다시 대조한다.
        report=read(ROOT/name)
        result=compare({row['path']:row['sha256'] for row in report['rawEvidence']})
        evidence.append({'report':name,**result})
    preserved=[]
    for row in initial['files']:
        # 점검 시작 시 복사한4파일의 현재 내용과 사본을 각각 검증한다.
        name=row['path']
        current=ROOT/name
        copy=output/'current-work'/name
        preserved.append({**row,'currentMatches':current.is_file() and sha(current)==row['sha256'],
                          'copyMatches':copy.is_file() and sha(copy)==row['sha256']})
    patch=git('diff','--binary','HEAD','--',*WORK[:3])
    tool=(ROOT/WORK[3]).read_text(encoding='utf8').splitlines(keepends=True)
    patch+=('diff --git a/'+WORK[3]+' b/'+WORK[3]+'\nnew file mode 100644\n'+
            ''.join(difflib.unified_diff([],tool,fromfile='/dev/null',tofile='b/'+WORK[3]))).encode('utf8')
    recovery=args.recovery_patch.resolve()
    recovery.parent.mkdir(parents=True,exist_ok=True)
    if recovery.exists() and recovery.read_bytes()!=patch:raise RuntimeError('기존 복구 패치를 덮어쓰지 않습니다')
    recovery.write_bytes(patch)
    checked=subprocess.run(['git','apply','--check','--reverse',str(recovery)],cwd=ROOT,capture_output=True)
    executables={name:{'current':sha(ROOT/'cpppj/build/bin/Release'/name),'completed542':digest}
                 for name,digest in baseline['exes'].items()}
    result={'scope':'cpppj 완료541/542와 진행543 소실 점검','head':head,'statusAtStart':status,
            'tracked':{'count':count,'missing':missing,'unexpectedEmpty':emptied},'source542':source,
            'protected':protectedResult,'completedEvidence':evidence,'preservedWork':preserved,
            'recoveryPatch':{'path':recovery.relative_to(ROOT).as_posix(),'sha256':sha(recovery),
                             'reverseCheckExitCode':checked.returncode,'containsOnlyThisSessionFiles':list(WORK)},
            'executables':executables,'binaryStatus':'현재 실행파일은 진행543 빌드이며 완료542 해시와 다름. 파일 삭제와 구별',
            'sourceMutationsDuringAudit':False,'dotnetFilesModifiedByAudit':False,
            'previousReversion':'이전 미커밋2파일 변경의 HEAD 원복은 관측됐으며 현재 복구됨. 모델 용량 오류가 직접 원인인지는 미확정'}
    failures=missing+emptied+source['missing']+source['unexpectedChanges']+protectedResult['missing']+protectedResult['changed']
    failures += [row['report'] for row in evidence if row['missing'] or row['changed']]
    failures += [row['path'] for row in preserved if not row['currentMatches'] or not row['copyMatches']]
    if not protectedResult['agentsUnchanged'] or checked.returncode:failures.append('AGENTS 또는 복구 패치 검사')
    result['failures']=failures
    (output/'audit.json').write_bytes((json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode('utf8'))
    print(json.dumps({'tracked':count,'missing':len(missing),'unexpectedEmpty':len(emptied),
                     'sourceBaseline':source['count'],'expectedChanged':source['changed'],
                     'protected':protectedResult['count'],'evidence':sum(row['count'] for row in evidence),
                     'workCopies':len(preserved),'reversePatchCheck':checked.returncode,'failures':failures},ensure_ascii=False))
    if failures:raise SystemExit(1)


if __name__=='__main__':
    main()
