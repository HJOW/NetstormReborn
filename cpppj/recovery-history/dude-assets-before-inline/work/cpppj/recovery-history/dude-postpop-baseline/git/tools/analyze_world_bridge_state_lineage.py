#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 표면 전역 참조의 공개 추가와 기존 RawSurfaceWorld 전체 소스 보존을 연결한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

# 직전 전체983개 성공 단위의 git blob을 비교 기준으로 고정한다.
ROOT = Path(__file__).resolve().parent.parent
BASELINE = 'b76adf9'
REPORT = ROOT / 'cpppj/recovery-world-bridge-state-lineage-evidence.json'
# 이 참조 선언과 Core 반환 인자 한 개만 이전 파일에서 달라져야 한다.
ADDITION = '    // bridgeState: 같은 월드의 실제 표면 소유자 분배·연결이 읽는 현재 mutable 전역이다.\n    o::BridgeConnectState& bridgeState;\n'


def digest(data):
    """원본 blob/파일 바이트의 SHA-256을 반환한다."""
    return hashlib.sha256(data).hexdigest()


def audit():
    """추가한 참조/인자를 제거하면 이전 헤더/구현 전체와 같음을 확인한다."""
    before = {}
    current = {}
    # 모든 기존 메서드와 상태의 나머지 소스를 주석까지 비교한다.
    for filename in ('cpppj/src/client/RawSurfaceWorld.h', 'cpppj/src/client/RawSurfaceWorld.cpp'):
        old = subprocess.check_output(['git', 'show', f'{BASELINE}:{filename}'], cwd=ROOT)
        new = (ROOT / filename).read_bytes()
        old_text = old.decode('utf-8').replace('\r\n', '\n')
        new_text = new.decode('utf-8').replace('\r\n', '\n')
        if filename.endswith('.h'):
            if new_text.count(ADDITION) != 1:
                raise RuntimeError('월드 전역 참조 선언 개수 오류')
            restored = new_text.replace(ADDITION, '')
        else:
            added = 's.processState,s.kernel,s.links};'
            if new_text.count(added) != 1:
                raise RuntimeError('Core 현재 전역 반환 인자 개수 오류')
            restored = new_text.replace(added, 's.processState,s.kernel};')
        if restored != old_text:
            raise RuntimeError(f'기존 월드 전체 소스 차이: {filename}')
        before[filename] = digest(old)
        current[filename] = digest(new)
    # 직전 검증 보고서/로그는 수정하지 않고 현재 파일 해시로 연결한다.
    paths = ('tools/analyze_world_bridge_state_lineage.py', 'cpppj/recovery-campaign-runtime7-evidence.json',
             'extracted/campaign-runtime7-build-final.log', 'extracted/campaign-runtime7-all-final-tests.log')
    return {'host': 'HJOW-Athlon',
            'baseline_commit': subprocess.check_output(['git', 'rev-parse', BASELINE], cwd=ROOT, text=True).strip(),
            'baseline_source_blob_sha256': before, 'current_source_sha256': current,
            'all_remaining_header_and_implementation_identical': True,
            'new_native_cases': 0, 'prior_all_tests_not_added': 983,
            'files': {name: digest((ROOT / name).read_bytes()) for name in paths}}


def main():
    """기본 실행은 참조 추가 연결 근거 생성이며 --verify는 저장된 기록과 다시 비교한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    data = audit()
    if args.verify:
        if json.loads(REPORT.read_text(encoding='utf-8')) != data:
            raise RuntimeError('월드 현재 전역 참조 lineage 차이')
    else:
        REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('월드 현재 전역 참조1개/반환 인자1개 추가·기존 전체 소스/983개 완료 출처 보존 확인')


if __name__ == '__main__':
    main()
