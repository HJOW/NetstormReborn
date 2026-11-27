# 같은 고정 입력으로 Node22와 Node26 전체를 순차 실행하고 각 종료 때 원시 해시를 다시 검사한다.
from pathlib import Path
import datetime
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time

# 작업 경로와 기존 회귀의 입력 집합이다. 새 추적 소스만 더하고 Git 제외 실험은 넣지 않는다.
sys.stdout.reconfigure(encoding='utf-8')
web = Path.cwd()
root = web.parent
output = web / 'extracted'
old = json.loads((output / 'webpj-e1420-freeze.json').read_text(encoding='utf-8'))
names = set(old['sourceFiles'])
names.update(subprocess.check_output(['git', 'ls-files', '--', 'webpj/www', 'webpj/tests', 'webpj/e2e', 'webpj/tools', 'webpj/translations'], cwd=root, text=True, encoding='utf-8').splitlines())
state_path = output / 'e1441-1442-run-state.json'
state = {'phase': 'freezing', 'createdAt': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'runnerPid': os.getpid(), 'sourceMutationForbiddenUntilBothDone': True, 'completed': []}

# 현재 진행 정보를 독립 JSON에 남겨 모델 문맥이 바뀌어도 실행을 중복하지 않게 한다.
def write_state():
    state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

# 입력 파일은 원시 바이트와 CRLF만 정규화한 바이트의 두 해시로 기록한다.
def hashes(name):
    data = (root / name).read_bytes()
    return {'raw': hashlib.sha256(data).hexdigest(), 'lf': hashlib.sha256(data.replace(b'\r\n', b'\n')).hexdigest()}

write_state()
source_files = {name: hashes(name) for name in sorted(names)}
source_head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
# 실제 실행 파일을 명시하고 이번 버전의 전체 검사는 같은 시점에 실행하지 않는다.
versions = [('E1441', Path(shutil.which('node')).resolve(), 'v26.13.0'), ('E1442', root / 'extracted/webpj-node22/node_modules/node-win-x64/bin/node.exe', 'v22.23.5')]

# 한 버전이 끝나고 통과/입력 고정을 확인한 뒤 다음 버전을 시작한다.
for unit, executable, expected_version in versions:
    version = subprocess.check_output([str(executable), '--version'], text=True).strip()
    if version != expected_version:
        raise RuntimeError(f'예상 Node 버전 불일치: {version} / {expected_version}')
    tag = unit.lower()
    log_path = output / f'webpj-{tag}-node{version.split(".")[0][1:]}.log'
    freeze = {'unit': f'WB04-{unit}', 'recordedAt': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'head': source_head, 'node': version, 'nodeExecutable': str(executable), 'nodeExecutableSha256': hashlib.sha256(executable.read_bytes()).hexdigest(), 'command': 'node --test --test-concurrency=2 tests/*.test.mjs', 'periodAtBatchStart': {'v26': 4, 'v22': 8}, 'sourceFiles': source_files, 'sourceNote': 'E1436실제간헐천집계/건설중생산상한4검사·E1434 Outpost삭제3검사포함. Node26뒤Node22순차정기전체. Git제외AI/씨앗입력오라클만별도진행하며작성WB소스/설정/대응표/공개자산/원본복사본은고정.', 'expectedTests': 6919}
    (output / f'webpj-{tag}-freeze.json').write_text(json.dumps(freeze, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    env = dict(os.environ)
    env['PATH'] = str(executable.parent) + os.pathsep + env.get('PATH', '')
    start = time.monotonic()
    # 원문을 바이너리 스트림으로 보존하고 현재 테스트 프로세스 식별자를 기록한다.
    with log_path.open('wb') as stream:
        child = subprocess.Popen([str(executable), '--test', '--test-concurrency=2', 'tests/*.test.mjs'], cwd=web, env=env, stdout=stream, stderr=subprocess.STDOUT)
        state.update({'phase': 'running', 'unit': unit, 'version': version, 'childPid': child.pid, 'log': str(log_path), 'files': len(source_files)})
        write_state()
        print(json.dumps({'unit': unit, 'phase': 'running', 'node': version, 'files': len(source_files), 'pid': child.pid}), flush=True)
        code = child.wait()
    # 원문을 수정하지 않고 TAP/spec의 마지막 요약만 해석한다.
    plain = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', log_path.read_text(encoding='utf-8', errors='replace'))
    counts = {}
    for key in ['tests', 'pass', 'fail', 'cancelled', 'skipped', 'todo']:
        matches = re.findall(rf'^(?:#|ℹ)\s+{key}\s+(\d+)\s*$', plain, re.M)
        counts[key] = int(matches[-1]) if matches else None
    durations = re.findall(r'^(?:#|ℹ)\s+duration_ms\s+([\d.]+)', plain, re.M)
    counts['durationMs'] = float(durations[-1]) if durations else None
    changed = []
    newline_only = []
    # 작성 입력의 변경을 실패 원인과 독립적으로 검사한다.
    for name, expected in source_files.items():
        current = hashes(name)
        if current['raw'] != expected['raw']:
            changed.append(name)
            if current['lf'] == expected['lf']:
                newline_only.append(name)
    check = {'unit': f'WB04-{unit}', 'recordedAt': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'nodeExit': code, 'files': len(source_files), 'changed': changed, 'newlinesOnly': newline_only, 'results': counts, 'runnerElapsedSeconds': time.monotonic() - start}
    (output / f'webpj-{tag}-freeze-check.json').write_text(json.dumps(check, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    passed = code == 0 and counts['tests'] == counts['pass'] == 6919 and all(counts[key] == 0 for key in ['fail', 'cancelled', 'skipped', 'todo']) and not changed
    state['completed'].append(check)
    state['phase'] = 'checked' if passed else 'failed'
    write_state()
    print(json.dumps({'unit': unit, 'phase': state['phase'], 'nodeExit': code, 'results': counts, 'changed': changed}, ensure_ascii=False), flush=True)
    if not passed:
        state['sourceMutationForbiddenUntilBothDone'] = False
        write_state()
        sys.exit(code or 8)

# 전체와 종료 해시가 모두 확인돼야 작성 입력의 고정을 해제한다.
state['phase'] = 'complete'
state['sourceMutationForbiddenUntilBothDone'] = False
write_state()
