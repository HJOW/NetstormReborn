#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 그래프 순회의 내부 Form 생략을 기준12 전체 몸체와 이전 관찰에 연결한다."""
import argparse
import hashlib
import importlib
import json
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 같은 저장소의 전체 원문을 읽는 기준이다.
HISTORY=ROOT/'cpppj/hist/graph-forms-24ec7'  # 기준12 Git와 당시 작업 원문 및 부모 준비 자료다.
REPORT=ROOT/'cpppj/graph-forms-lineage.json'  # 좁은 전체순회 확장의 현재 출처다.
SOURCE='cpppj/src/o/RawGraph.cpp'  # 전체 계산/부분 검증 몸체를 보존할 공유 파일이다.
BEFORE='''        if (bytes[10]<kFirstAssetTypeNumber || bytes[10]>=types_.size()) {
            if (required[id] || fullPool) throw std::logic_error("raw 그래프 후보 타입 오류");
            continue;
        }
'''  # 이전의 내부 Form 전체풀 거부 조건 원문이다.
AFTER='''        // 내부 Form은 원본 flags getter의 비표면 타입이며 공간 머리에 없으면 전체순회에서 생략한다.
        if (bytes[10]<kFirstAssetTypeNumber) {
            if (required[id]) throw std::logic_error("raw 그래프 후보 타입 오류");
            continue;
        }
        if (bytes[10]>=types_.size()) {
            if (required[id] || fullPool) throw std::logic_error("raw 그래프 후보 타입 오류");
            continue;
        }
'''  # 해시의 내부 Form과 범위 밖 타입은 계속 거부하며 기존 자산 계산은 보존한다.


def sha(path):
    """전체 파일 바이트를 SHA256으로 확인한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture():
    """공유 변경 직전에 기준12 자료와 준비된 부모 후속의 전체 바이트를 보존한다."""
    assert not HISTORY.exists()
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    assert commit.startswith('db9b626')
    tracked=set(subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode('utf-8').split('\0'))
    names={SOURCE}
    # 공유 그래프를 참조한 모든 기존 근거 및 그래프 입력/검사를 전체 보존한다.
    for name in tracked:
        if name.startswith('cpppj/recovery-') and '/' not in name[len('cpppj/'):]:
            if SOURCE in (ROOT/name).read_text(encoding='utf-8'):names.add(name)
        if name.startswith('cpppj/tests/') and 'graph' in name.lower() and name.endswith(('.cpp','.tsv')):
            names.add(name)
    # 아직 미등록인 부모 준비 자료도 이전 SHA를 덮어쓰지 않고 별도 전체 스냅샷으로 저장한다.
    for pattern in ('cpppj/fortworldload*.json','tools/*fortworld*.py',
                    'cpppj/tests/FortWorldLoad*.cpp','cpppj/tests/fixtures/fortworldload*.tsv'):
        names.update(path.relative_to(ROOT).as_posix() for path in ROOT.glob(pattern))
    values={}
    # Git에 있는 파일은 Git와 작업 원문 두 종류를 구분해 보존한다.
    for name in sorted(names):
        work=HISTORY/'work'/name
        work.parent.mkdir(parents=True,exist_ok=True)
        work.write_bytes((ROOT/name).read_bytes())
        values[name]={'work_sha256':sha(work)}
        if name in tracked:
            git=HISTORY/'git'/name
            git.parent.mkdir(parents=True,exist_ok=True)
            git.write_bytes(subprocess.check_output(['git','show',commit+':'+name],cwd=ROOT))
            assert git.read_bytes().replace(b'\r\n',b'\n')==work.read_bytes().replace(b'\r\n',b'\n'),name
            values[name]['git_sha256']=sha(git)
    (HISTORY/'baseline.json').write_text(json.dumps(dict(commit=commit,files=values),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('내부 Form 그래프 기준 전체 보존',len(values),'파일')


def audit():
    """내부 Form 생략 조건만 정확 역변환하고 이전 전체 근거의 원문 불변을 감사한다."""
    baseline=json.loads((HISTORY/'baseline.json').read_text(encoding='utf-8'))
    # 기준의 각 전체 바이트와 Git 원문을 확인하고 현재 소스만 역변환한다.
    for name,values in baseline['files'].items():
        work=HISTORY/'work'/name
        assert sha(work)==values['work_sha256'],name
        if 'git_sha256' in values:
            git=HISTORY/'git'/name
            assert sha(git)==values['git_sha256'],name
            assert git.read_bytes()==subprocess.check_output(['git','show',baseline['commit']+':'+name],cwd=ROOT),name
        current=(ROOT/name).read_bytes()
        if name==SOURCE:
            newline=b'\r\n' if b'\r\n' in current else b'\n'
            before=BEFORE.encode('utf-8').replace(b'\n',newline)
            after=AFTER.encode('utf-8').replace(b'\n',newline)
            assert current.count(after)==1
            current=current.replace(after,before,1)
        assert current==work.read_bytes(),name
    return baseline


def previous_consumer():
    """부모 후속의 옛 전체 의존성은 보존한 그래프 source view로 그대로 검증한다."""
    consumer=importlib.import_module('analyze_fortworldload_consumer_lineage')
    loader=consumer.module
    def load_previous(name,path):
        """옛 binding의 그래프 SHA만 당시 전체 원문을 읽고 나머지 검사는 유지한다."""
        module=loader(name,path)
        if path=='tools/analyze_fortworldload_binding.py':
            original=module.pin
            def pin(relative):
                """원래 path는 유지하고 이미 역변환한 그래프의 당시 바이트를 제공한다."""
                selected=(HISTORY/'work'/SOURCE).relative_to(ROOT).as_posix() if relative==SOURCE else relative
                value=original(selected)
                value['path']=relative
                return value
            module.pin=pin
        return module
    consumer.module=load_previous
    try:
        consumer.verify()
    finally:
        consumer.module=loader


def expected():
    """새 원본 수와 구분한 공유 확장 출처를 전체 이전 파일에 연결한다."""
    baseline=audit()
    previous_consumer()
    return dict(schema=1,host='HJOW-Athlon',decompile_date='2026-10-14',baseline_commit=baseline['commit'],
                new_shared_api_native_cases=0,old_fixture_report_bytes_unchanged=True,entire_source_inverse=True,
                current_source_sha256=sha(ROOT/SOURCE),tool_sha256=sha(Path(__file__)),
                history={p.relative_to(ROOT).as_posix():sha(p) for p in HISTORY.rglob('*') if p.is_file()},
                change='내부 Form은 해시 머리에 없는 비표면 입력일 때 전체풀에서 생략한다. 해시의 내부Form/범위밖 타입 거부와 기존 자산 계산 전체는 보존한다.')


def main(arguments):
    """변경 전 전체 보존 또는 현재 최소 확장의 생성/검증을 실행한다."""
    if arguments.capture:
        capture()
        return
    data=expected()
    if arguments.verify:
        assert json.loads(REPORT.read_text(encoding='utf-8'))==data
    else:
        REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('내부 Form 그래프 전체 본문 역변환/이전 근거/부모 소비자 출처 감사 통과')


# --capture로 전체 기준을 보존한 뒤 --verify로 현재 좁은 확장의 원문을 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',action='store_true')
    parser.add_argument('--verify',action='store_true')
    main(parser.parse_args())
