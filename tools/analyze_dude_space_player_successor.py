#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dude 공간 고유36 입력을 새 Player 분배와 변경 전 전체 소스에 연결한다."""
import argparse
import hashlib
import json
from pathlib import Path
import analyze_dude_space_predicate_scope as previous
import analyze_player_postpop_binding_lineage as player

ROOT = Path(__file__).resolve().parent.parent  # 현재 공유 저장소의 읽기 기준이다.
REPORT = ROOT / 'cpppj/recovery-dude-space-player-successor-evidence.json'  # Player 분배 후 현재 Dude 소비자의 별도 출처다.
HISTORY = ROOT / 'cpppj/recovery-history/dude-space-before-player-binding'  # 이전 감사/전체 fixture/실패 로그를 보관한다.
SHARED = ('cpppj/src/o/SquidPostPop.h', 'cpppj/src/o/SquidPostPop.cpp')  # 전체 역변환이 필요한 두 공유 파일이다.


def sha(body):
    """전체 파일 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


def expected():
    """Player 등록만 정확히 역변환한 당시 view로 Dude의 이전 감사 계약을 유지한다."""
    manifest = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))
    saved_hashes, shared_hashes = {}, {}
    # 새 분배 후 첫 실패와 이전 도구/보고서/48행을 변경 전 전체 바이트로 보존한다.
    for name, digest in manifest['files'].items():
        saved = HISTORY / name
        assert sha(saved.read_bytes()) == digest, name
        if not name.startswith('extracted/'):
            assert (ROOT / name).read_bytes() == saved.read_bytes(), name
        saved_hashes[saved.relative_to(ROOT).as_posix()] = digest
    # 새 Player 조각만 제거하면 기준13의 당시 공유 두 원문으로 정확히 돌아가야 한다.
    for name in SHARED:
        old = (player.HISTORY / 'work' / name).read_bytes()
        restored = player.restore(name, (ROOT / name).read_text(encoding='utf-8'))
        inverse = restored.replace('\n', '\r\n').encode('utf-8') if b'\r\n' in old else restored.encode('utf-8')
        assert inverse == old, name
        shared_hashes[name] = dict(current_sha256=sha((ROOT / name).read_bytes()), previous_sha256=sha(old),
                                   whole_previous_source_inverse=True)
    original = Path.read_bytes

    def historical(path):
        """정확 역변환이 확인된 두 공유 파일만 당시 바이트를 읽으며 실제 파일은 쓰지 않는다."""
        resolved = path.resolve()
        for name in SHARED:
            if resolved == (ROOT / name).resolve():
                return original(player.HISTORY / 'work' / name)
        return original(path)

    Path.read_bytes = historical
    try:
        previous.main(True)
    finally:
        Path.read_bytes = original
    return dict(schema=1, host='HJOW-Athlon', date='2026-10-14', unique_native_inputs=36,
                duplicate_executions=12, new_native_inputs=0, requires_shared_unpop_change=False,
                shared=shared_hashes, history=saved_hashes, tool_sha256=sha(Path(__file__).read_bytes()),
                prior_report_sha256=sha(previous.REPORT.read_bytes()),
                scope='Player 전체 PostPop의 새 분배만 제거한 같은 기존 Dude 소비자다. 이전48행/고유36/반복12와 실제 writesSpots=false를 유지하며 기존 감사 보고서를 덮어쓰지 않는다.')


def main(verify):
    """새 현재 연결 근거를 작성하거나 이미 고정한 전체 바이트를 감사한다."""
    data = expected()
    if verify:
        assert json.loads(REPORT.read_text(encoding='utf-8')) == data
    else:
        REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('Dude 공간 Player 분배 연결: 기존고유36/반복12·전체역변환·추가0 감사 통과')


# 현재 연결은 별도 보고서에만 기록하며 이전 Native/분기 감사 파일은 수정하지 않는다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    main(parser.parse_args().verify)
