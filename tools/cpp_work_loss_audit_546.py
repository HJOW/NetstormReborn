#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""완료545와 진행546의 저장 상태를 대조하고 이 세션 파일만 복구 자료로 보존한다."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path

# 저장소·완료 제품·진행 중 최종 빌드·원본 보호 기준의 위치다.
ROOT = Path(__file__).resolve().parents[1]
WORK_ROOT = ROOT / 'extracted/cpp-banner-option-546'
BASELINE = ROOT / 'extracted/cpp-f8-input-545/freeze-final.json'
PROTECTED = ROOT / 'extracted/cpp-object-hit-524/protected-before.json'
REPORTS = ('cpppj/priest-display-20261028.json', 'cpppj/construction-mana-20261028.json',
           'cpppj/chat-divider-runtime-20261028.json', 'cpppj/view-key-runtime-20261028.json',
           'cpppj/f8-key-runtime-20261028.json', 'cpppj/work-loss-audit-20261028.json',
           'cpppj/work-loss-audit-545-20261028.json')


def sha(path):
    """실제 파일 바이트를 해시하여 개행이나 인코딩 변경까지 검출한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    """저장된 UTF-8 JSON 기준을 읽고 원본에는 쓰지 않는다."""
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write(path, value):
    """점검 전용 JSON을 UTF-8과 LF로 저장한다."""
    path.write_bytes((json.dumps(value, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))


def git(*args, env=None, check=True):
    """셸 문자열 보간 없이 Git을 실행하며 별도 인덱스를 지정할 수 있다."""
    return subprocess.run(['git', '-c', 'core.autocrlf=false', *args], cwd=ROOT,
                          capture_output=True, env=env, check=check)


def compare(manifest, base=ROOT):
    """각 기준 파일의 누락·비정상 빈 파일·내용 변경을 구분한다."""
    missing, empty, changed = [], [], []
    # 파일별로 기준 해시와 실제 바이트를 대조한다.
    for name, digest in manifest.items():
        path = base / name
        if not path.is_file():
            missing.append(name)
        elif sha(path) != digest:
            changed.append(name)
            if not path.stat().st_size:
                empty.append(name)
    return {'count': len(manifest), 'missing': missing, 'unexpectedEmpty': empty, 'changed': changed}


def main():
    """게임·빌드·제품 수정 없이 저장된 작업·근거·Git 객체와 복구 패치를 검증한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--recovery-patch', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    patch = args.recovery_patch.resolve()
    if output.exists() or patch.exists():
        raise RuntimeError('기존 점검 자료를 덮어쓰지 않습니다')
    output.mkdir(parents=True)
    head = git('rev-parse', 'HEAD').stdout.decode().strip()
    initial_status = git('status', '--short').stdout.decode('utf-8')
    work = read(WORK_ROOT / 'current-final.json')
    initial_hashes = {name: sha(ROOT / name) for name in work}
    # 진행 중 소스·검사·fixture·도구·인계 문서 37개를 정확한 바이트로 복사한다.
    for name in initial_hashes:
        dest = output / 'current-work' / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, dest)
    write(output / 'current-work.json', initial_hashes)
    missing, empty, tracked_count = [], [], 0
    # HEAD의 blob만 검사하며 서브모듈은 일반 파일 누락으로 분류하지 않는다.
    for entry in git('ls-tree', '-r', '-l', '-z', head).stdout.split(b'\0'):
        if not entry:
            continue
        metadata, raw_name = entry.split(b'\t', 1)
        mode, kind, oid, size = metadata.split()
        if kind != b'blob':
            continue
        tracked_count += 1
        name = raw_name.decode('utf-8')
        path = ROOT / name
        if not path.is_file():
            missing.append(name)
        elif int(size) and not path.stat().st_size:
            empty.append(name)

    baseline = read(BASELINE)
    source = compare(baseline['source'])
    expected = read(WORK_ROOT / 'source-check.json')['changedExisting']
    source['expectedChanges'] = expected
    source['unexpectedChanges'] = sorted(set(source['changed']) - set(expected))
    frozen = read(WORK_ROOT / 'freeze-final.json')
    final_source = compare(frozen['source'])
    before_build = read(WORK_ROOT / 'before-build2.json')
    production = {name: digest for name, digest in before_build.items() if name.startswith('cpppj/src/')}
    previous_work = compare(work)
    # 마지막 사본 뒤 UI 검사 도구에 적용한 메뉴·개인 사본 경로 수정은 별도로 기록한다.
    previous_work['expectedLaterChanges'] = ['tools/cpp_banner_smoke.py', 'tools/decomp_banner_key.py']
    previous_work['unexpectedChanges'] = sorted(set(previous_work['changed']) - set(previous_work['expectedLaterChanges']))
    protected = read(PROTECTED)
    protected_result = compare(protected['files'])
    protected_result['agentsUnchanged'] = sha(ROOT / 'AGENTS.md') == protected['agents']
    evidence = []
    # 완료된 단위와 이전 소실 점검이 명시한 원시 자료를 실제 파일과 대조한다.
    for name in REPORTS:
        rows = read(ROOT / name)['rawEvidence']
        evidence.append({'report': name, **compare({row['path']: row['sha256'] for row in rows})})
    raw = {}
    # 복사한 게임 자산·개인 설정을 제외하고 진행 중 원시 로그와 실패 자료도 고정한다.
    for path in sorted(WORK_ROOT.rglob('*')):
        parts = set(path.relative_to(WORK_ROOT).parts)
        if path.is_file() and not ({'data', 'settings-backup'} & parts):
            raw[path.relative_to(ROOT).as_posix()] = sha(path)
    write(output / 'work-evidence.json', raw)

    binaries = {name: {'sha256': sha(ROOT / 'cpppj/build/bin/Release' / name), 'recorded': digest}
                for name, digest in frozen['exes'].items()}
    # 실제 인덱스를 건드리지 않고 이 세션 36개 파일만 포함하는 복구 패치를 만든다.
    patch_files = sorted(set(work) - {'LEFT_JOBS.md'})
    env = os.environ.copy()
    env['GIT_INDEX_FILE'] = str(output / 'recovery.index')
    git('read-tree', head, env=env)
    git('add', '--', *patch_files, env=env)
    patch.parent.mkdir(parents=True, exist_ok=True)
    patch.write_bytes(git('diff', '--cached', '--binary', head, '--', *patch_files, env=env).stdout)
    reverse = git('apply', '--check', '--reverse', '--whitespace=nowarn', str(patch), check=False)
    (output / 'patch-check.log').write_bytes(reverse.stdout + reverse.stderr)
    fsck = git('fsck', '--connectivity-only', '--no-dangling', check=False)
    (output / 'git-connectivity.log').write_bytes(fsck.stdout + fsck.stderr)
    ancestry = {commit: git('merge-base', '--is-ancestor', commit, head, check=False).returncode
                for commit in ('1ddc4ba0', '55520f78', '2efb39ae', '485ec285', 'e114d77b', '73506bcb')}
    checks = {'finalSource2941': final_source,
              'productionMatchesSuccessfulBuild': compare(production),
              'successfulBuildCopies': compare(before_build, WORK_ROOT / 'before-build2'),
              'previousSnapshotCopies': compare(work, WORK_ROOT / 'current-final'),
              'protected': protected_result, 'currentCopies': compare(initial_hashes, output / 'current-work'),
              'stableDuringAudit': compare(initial_hashes)}
    failures = missing + empty + source['missing'] + source['unexpectedChanges'] + previous_work['missing'] + previous_work['unexpectedChanges']
    # 모든 보존 검사와 Git 검사를 판정하며 예상한 진행 중 변경을 소실로 세지 않는다.
    for label, item in [*checks.items(), *[(row['report'], row) for row in evidence]]:
        if item['missing'] or item['changed']:
            failures.append(label)
    if not protected_result['agentsUnchanged'] or reverse.returncode or fsck.returncode or any(ancestry.values()):
        failures.append('보호 문서·패치·Git 연결 검사')
    if any(row['sha256'] != row['recorded'] for row in binaries.values()):
        failures.append('최종 실행파일 해시')
    result = {'scope': '사용자 우선 요청: 완료545 및 진행546 소실 점검', 'head': head,
              'host': os.environ.get('COMPUTERNAME'), 'statusAtStart': initial_status,
              'tracked': {'count': tracked_count, 'missing': missing, 'unexpectedEmpty': empty},
              'source545': source, 'checks': checks, 'previousSnapshotCurrent': previous_work,
              'completedEvidence': evidence, 'work546RawEvidence': {'count': len(raw),
                  'manifest': (output / 'work-evidence.json').relative_to(ROOT).as_posix()},
              'binaries': binaries, 'completedCommitAncestry': ancestry, 'gitConnectivityExitCode': fsck.returncode,
              'recoveryPatch': {'path': patch.relative_to(ROOT).as_posix(), 'sha256': sha(patch),
                  'reverseCheckExitCode': reverse.returncode, 'files': patch_files},
              'existingValidation546': {'build': read(WORK_ROOT / 'build2.json'),
                  'tests': read(WORK_ROOT / 'tests-final.json')},
              'sourceModified': False, 'dotnetModified': False, 'gameBuildTestLaunched': False,
              'capacityErrorDirectCause': '미확정: 서비스 용량 오류와 파일 원복의 인과를 파일 상태만으로 판정할 수 없음',
              'limits': '저장되거나 기록된 기준이 전혀 없는 변경의 과거 존재 여부는 판단할 수 없음',
              'failures': failures}
    write(output / 'audit.json', result)
    print(json.dumps({'tracked': tracked_count, 'source545': source['count'], 'source546': final_source['count'],
                      'expectedChanged': len(source['changed']), 'protected': protected_result['count'],
                      'evidence': sum(row['count'] for row in evidence), 'currentCopies': len(initial_hashes),
                      'raw546': len(raw), 'patchCheck': reverse.returncode, 'fsck': fsck.returncode,
                      'failures': failures}, ensure_ascii=False))
    if failures:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
