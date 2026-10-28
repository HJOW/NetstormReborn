#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""중단 의심 시 완료544와 미커밋545의 사본·해시·실행 근거를 대조하고 복구 자료를 만든다."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path

# 저장소와 이번 점검에서 읽을 완료 기준 및 보호 기준이다.
ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / 'extracted/cpp-view-keys-544/freeze-final.json'
WORK_ROOT = ROOT / 'extracted/cpp-f8-input-545'
PROTECTED = ROOT / 'extracted/cpp-object-hit-524/protected-before.json'
EXPECTED = ('cpppj/src/client/UberGump.cpp', 'cpppj/src/client/ViewKeyInput.cpp',
            'cpppj/src/client/ViewKeyInput.h', 'cpppj/tests/CMakeLists.txt')
REPORTS = ('cpppj/priest-display-20261028.json', 'cpppj/construction-mana-20261028.json',
           'cpppj/chat-divider-runtime-20261028.json', 'cpppj/view-key-runtime-20261028.json',
           'cpppj/work-loss-audit-20261028.json')


def sha(path):
    """파일의 실제 바이트 해시를 구해 인코딩·개행의 우발 변경도 검출한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    """기존 UTF-8 JSON을 수정하지 않고 읽는다."""
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write(path, value):
    """이번 점검 전용 출력에만 UTF-8 JSON을 생성한다."""
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def git(*args, env=None, check=True):
    """셸 보간 없이 Git을 호출하며 임시 인덱스는 호출자가 명시한 경우만 사용한다."""
    return subprocess.run(['git', '-c', 'core.autocrlf=false', *args], cwd=ROOT,
                          env=env, capture_output=True, check=check)


def compare(manifest, base=ROOT):
    """기준 파일의 누락·예상 밖 빈 파일·내용 변경을 각각 분류한다."""
    missing, empty, changed = [], [], []
    # 각 파일은 현재 트리 또는 보존 사본에서 한 번씩 읽어 기준 해시와 비교한다.
    for name, digest in manifest.items():
        path = base / name
        if not path.is_file():
            missing.append(name)
        elif sha(path) != digest:
            changed.append(name)
            if path.stat().st_size == 0:
                empty.append(name)
    return {'count': len(manifest), 'missing': missing, 'unexpectedEmpty': empty,
            'changed': changed}


def main():
    """게임·빌드·제품 수정 없이 보존 상태와 복구 패치의 현재 트리 역검사를 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--recovery-patch', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        raise RuntimeError('기존 점검 원시를 덮어쓰지 않습니다')
    output.mkdir(parents=True)
    head = git('rev-parse', 'HEAD').stdout.decode().strip()
    initial_status = git('status', '--short').stdout.decode('utf-8')
    work = read(WORK_ROOT / 'before-final-build.json')
    initial_hashes = {name: sha(ROOT / name) for name in (*work, 'LEFT_JOBS.md')}
    # 새 파일과 인계 문서까지 복사하고, 제품 코드는 원래 위치에서 건드리지 않는다.
    for name in initial_hashes:
        dest = output / 'current-work' / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, dest)
    write(output / 'current-work.json', initial_hashes)

    tracked_missing, tracked_empty, tracked_count = [], [], 0
    # HEAD의 blob만 확인하여 서브모듈 포인터를 삭제된 일반 파일로 오인하지 않는다.
    for entry in git('ls-tree', '-r', '-l', '-z', 'HEAD').stdout.split(b'\0'):
        if not entry:
            continue
        metadata, name = entry.split(b'\t', 1)
        mode, kind, oid, size = metadata.split()
        if kind != b'blob':
            continue
        tracked_count += 1
        relative = name.decode('utf-8')
        path = ROOT / relative
        if not path.is_file():
            tracked_missing.append(relative)
        elif int(size) > 0 and path.stat().st_size == 0:
            tracked_empty.append(relative)

    source = compare(read(BASELINE)['source'])
    source['expectedChanges'] = list(EXPECTED)
    source['unexpectedChanges'] = sorted(set(source['changed']) - set(EXPECTED))
    final_work = compare(work)
    final_copies = compare(work, WORK_ROOT / 'before-final-build')
    initial_work = read(WORK_ROOT / 'before-build.json')
    initial_copies = compare(initial_work, WORK_ROOT / 'before-build')
    protected = read(PROTECTED)
    protected_result = compare(protected['files'])
    protected_result['agentsUnchanged'] = sha(ROOT / 'AGENTS.md') == protected['agents']
    evidence = []
    # 완료 보고서가 명시한 원시만 읽고 사본 데이터 폴더 전체의 불필요한 중복 해시는 피한다.
    for name in REPORTS:
        rows = read(ROOT / name)['rawEvidence']
        evidence.append({'report': name, **compare({row['path']: row['sha256'] for row in rows})})

    raw = {}
    # 미완료545도 현재 근거를 해시로 고정하되 개인 설정 백업과 복사한 게임 자산은 제외한다.
    for path in sorted(WORK_ROOT.rglob('*')):
        if path.is_file() and not ({'data', 'settings-backup'} & set(path.relative_to(WORK_ROOT).parts)):
            raw[path.relative_to(ROOT).as_posix()] = sha(path)
    write(output / 'work-evidence.json', raw)
    final_summary = read(WORK_ROOT / 'final/summary.json')
    final_tests = read(WORK_ROOT / 'tests-final.json')
    game = ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe'
    tests = ROOT / 'cpppj/build/bin/Release/netstorm_tests.exe'
    binaries = {'game': {'sha256': sha(game), 'recorded': final_summary['binarySha256']},
                'tests': {'sha256': sha(tests), 'recorded': final_tests['binarySha256']}}
    # 별도 인덱스로 제품8파일만 패치화하여 다른 세션의 실제 인덱스를 변경하지 않는다.
    patch = args.recovery_patch.resolve()
    if patch.exists():
        raise RuntimeError('기존 복구 패치를 덮어쓰지 않습니다')
    env = os.environ.copy()
    env['GIT_INDEX_FILE'] = str(output / 'recovery.index')
    git('read-tree', 'HEAD', env=env)
    git('add', '--', *work, env=env)
    patch.parent.mkdir(parents=True, exist_ok=True)
    patch.write_bytes(git('diff', '--cached', '--binary', 'HEAD', '--', *work, env=env).stdout)
    reverse = git('apply', '--check', '--reverse', '--whitespace=nowarn', str(patch), check=False)
    (output / 'patch-check.log').write_bytes(reverse.stdout + reverse.stderr)
    fsck = git('fsck', '--connectivity-only', '--no-dangling', check=False)
    (output / 'git-connectivity.log').write_bytes(fsck.stdout + fsck.stderr)
    ancestry = {}
    # 완료541~544와 이전 점검 커밋에 현재 HEAD에서 도달할 수 있는지 각각 확인한다.
    for commit in ('1ddc4ba0', '55520f78', '37dbf68b', '2efb39ae', '485ec285'):
        ancestry[commit] = git('merge-base', '--is-ancestor', commit, head, check=False).returncode
    copied = compare(initial_hashes, output / 'current-work')
    stable = compare(initial_hashes)
    failures = tracked_missing + tracked_empty + source['missing'] + source['unexpectedChanges']
    # 보존 대상별 실패를 모으며 예상한 소스4개 변경을 소실로 세지 않는다.
    for label, item in [('finalWork', final_work), ('finalCopies', final_copies),
                        ('initialCopies', initial_copies), ('protected', protected_result),
                        ('copied', copied), ('stable', stable), *[(row['report'], row) for row in evidence]]:
        if item['missing'] or item['changed']:
            failures.append(label)
    if not protected_result['agentsUnchanged'] or reverse.returncode or fsck.returncode or any(ancestry.values()):
        failures.append('보호 문서·복구 패치·Git 연결 검사')
    if any(row['sha256'] != row['recorded'] for row in binaries.values()):
        failures.append('최종 실행파일 해시')
    result = {'scope': '사용자 우선 요청에 따른 완료544 및 미커밋545 소실 점검',
              'head': head, 'statusAtStart': initial_status,
              'tracked': {'count': tracked_count, 'missing': tracked_missing, 'unexpectedEmpty': tracked_empty},
              'source544': source, 'work545MatchesFinalBuild': final_work,
              'work545FinalBuildCopies': final_copies, 'work545InitialBuildCopies': initial_copies,
              'protected': protected_result, 'completedEvidence': evidence,
              'currentCopies': copied, 'stableDuringAudit': stable,
              'work545RawEvidence': {'count': len(raw), 'manifest': (output / 'work-evidence.json').relative_to(ROOT).as_posix()},
              'binaries': binaries, 'completedCommitAncestry': ancestry,
              'gitConnectivityExitCode': fsck.returncode,
              'recoveryPatch': {'path': patch.relative_to(ROOT).as_posix(), 'sha256': sha(patch),
                                'reverseCheckExitCode': reverse.returncode, 'files': list(work)},
              'existingValidation545': {'build': read(WORK_ROOT / 'build-final.json'),
                                       'registeredTests': final_tests['registered'],
                                       'focusedTests': final_tests['uniqueExecuted'],
                                       'finalGameRuns': len(final_summary['results']) + len(final_summary['editors']),
                                       'finalGameExitCodes': [row['run']['returnCode'] for row in final_summary['results'] + final_summary['editors']]},
              'sourceModified': False, 'dotnetModified': False, 'gameBuildTestLaunched': False,
              'capacityErrorDirectCause': '미확정: 파일 상태 검사만으로 서비스 오류와 원복의 인과를 판단할 수 없음',
              'limits': '저장되거나 기록된 기준이 전혀 없는 변경의 과거 존재 여부는 이 검사로 판단할 수 없음',
              'failures': failures}
    write(output / 'audit.json', result)
    print(json.dumps({'tracked': tracked_count, 'source544': source['count'], 'expectedChanges': source['changed'],
                      'work545': final_work, 'protected': protected_result['count'],
                      'completedEvidence': sum(row['count'] for row in evidence), 'currentCopies': copied['count'],
                      'raw545': len(raw), 'patchCheck': reverse.returncode, 'fsck': fsck.returncode,
                      'failures': failures}, ensure_ascii=False))
    if failures:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
