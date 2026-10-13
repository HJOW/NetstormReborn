#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Player152 최초 Pop 분배를 기준13 전체 소스·당시 근거와 현재 소비자에 연결한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent  # 현재 공유 저장소의 파일 읽기 기준이다.
HISTORY = ROOT / 'cpppj/recovery-history/player-postpop-baseline'  # 변경 전 전체 Git/작업 바이트를 보관한다.
REPORT = ROOT / 'cpppj/recovery-player-postpop-binding-lineage-evidence.json'  # 새 분배의 별도 현재 근거다.
COMMIT = '860030e73e9e11599c89334b2e6ab4b0c6e1702d'  # 단위13 완료 기준이며 기존 근거를 덮어쓰지 않는다.
HEADER_LINES = (
    'class RawPlayerWorld; // Player152의 실제 최초 기록·예약·공통후처리를 연결한다.\n',
    '    // Player152의 실제 전체 후처리를 같은 풀에 등록하며 nullptr은 연결을 해제한다.\n',
    '    void SetPlayerPostPop(const RawPlayerWorld* postPop);\n',
    '    // 현재 Player 생성자 가상 표와 등록된 전체 몸체가 일치하는가.\n',
    '    bool HandlesPlayer(Sid sid) const;\n',
    '    const RawPlayerWorld* playerPostPop_{}; // 같은9기록/SID·최초예약 뒤 공통 깊이를 한 번 줄이는 전체 몸체다.\n',
)  # 추가 선언과 사용 설명만 고유한 전체 줄로 제거한다.
REGISTRATION = '''// 실제 Player 최초 기록/예약 몸체를 같은 풀에만 명시 등록한다.
void SquidPostPop::SetPlayerPostPop(const RawPlayerWorld* postPop) {
    if (postPop && &postPop->Pool()!=&pool_) throw std::invalid_argument("Player Pop/후처리 풀 불일치");
    playerPostPop_=postPop;
}
// 실제 생성자의 가상 표 판단은 등록된 Player 몸체를 재사용한다.
bool SquidPostPop::HandlesPlayer(Sid sid) const { return playerPostPop_ && playerPostPop_->Handles(sid); }
'''  # 현재 등록/동일 풀 보호의 전체 고유 조각이다.
ACTIVATION = '''    // Player는 현재9기록/SID와 조건부예약 뒤 자기 몸체에서 공통 장부를 한 번 호출한다.
    if (HandlesPlayer(sid)) {
        playerPostPop_->PostPop(sid,flags);
        if (state_.depth!=before) throw std::logic_error("Player Activate 공통 깊이 불일치");
        return;
    }
'''  # 새 실제 분배만 제거하고 기존 모든 파생 흐름을 유지한다.


def sha(body):
    """전체 원문 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


def once(body, fragment, previous=''):
    """새 고유 조각 하나만 이전 값으로 역변환한다."""
    if body.count(fragment) != 1:
        raise RuntimeError('Player 분배 역변환 위치 오류: ' + fragment[:70])
    return body.replace(fragment, previous, 1)


def restore(name, body):
    """추가 등록/판단/검사/실행만 제거해 기준13 전체 본문을 복원한다."""
    if name.endswith('SquidPostPop.h'):
        # 새 선언/설명/멤버 외에 기존 헤더의 모든 바이트를 유지한다.
        for fragment in HEADER_LINES:
            body = once(body, fragment)
    elif name.endswith('SquidPostPop.cpp'):
        # 실제 새 분배 조각은 각각 정확히 한 곳에만 존재해야 한다.
        for fragment in ('#include "o/RawPlayerWorld.h"\n', ' || HandlesPlayer(sid)', REGISTRATION,
                         '    if (HandlesPlayer(sid)) playerPostPop_->Validate(sid,flags,pop);\n', ACTIVATION):
            body = once(body, fragment)
    else:
        raise RuntimeError('Player 공유 분배 범위 밖 소스: ' + name)
    return body


def observe():
    """변경 전 전체 바이트와 이전 보고서, 현재 Player 입력을 각각 확인한다."""
    manifest = json.loads((HISTORY / 'capture.json').read_text(encoding='utf-8'))
    assert manifest['baseline_commit'] == COMMIT
    history, sources, reports, dependencies = {}, {}, {}, {}
    # 전체 보존 자료의 SHA를 확인하고 Git/작업 원문을 서로 구분한다.
    for name, item in manifest['captured'].items():
        saved = HISTORY / 'work' / name
        assert sha(saved.read_bytes()) == item['work_sha256'], name
        history[saved.relative_to(ROOT).as_posix()] = item['work_sha256']
        if 'git_sha256' in item:
            previous = HISTORY / 'git' / name
            assert sha(previous.read_bytes()) == item['git_sha256'], name
            history[previous.relative_to(ROOT).as_posix()] = item['git_sha256']
    history[(HISTORY / 'capture.json').relative_to(ROOT).as_posix()] = sha((HISTORY / 'capture.json').read_bytes())
    # 두 공유 소스는 실제 Git 본문과 변경 전 작업의 개행까지 정확히 역복원한다.
    for name in manifest['shared']:
        saved = (HISTORY / 'work' / name).read_bytes()
        git_body = (HISTORY / 'git' / name).read_bytes()
        actual_git = subprocess.run(['git', 'show', COMMIT + ':' + name], cwd=ROOT, capture_output=True, check=True).stdout
        assert actual_git == git_body, name
        current = (ROOT / name).read_text(encoding='utf-8')
        restored = restore(name, current)
        assert restored == saved.decode('utf-8').replace('\r\n', '\n'), name
        assert restored == git_body.decode('utf-8').replace('\r\n', '\n'), name
        inverse_bytes = restored.replace('\n', '\r\n').encode('utf-8') if b'\r\n' in saved else restored.encode('utf-8')
        assert inverse_bytes == saved, name
        sources[name] = dict(baseline_work_sha256=sha(saved), baseline_git_sha256=sha(git_body),
                            current_sha256=sha((ROOT / name).read_bytes()), whole_previous_source_inverse=True,
                            whole_previous_work_bytes_inverse=True)
    # 기존25보고서 자체의 당시 SHA는 수정하지 않고 그대로 확인한다.
    for name in manifest['prior_reports']:
        previous = (HISTORY / 'work' / name).read_bytes()
        assert (ROOT / name).read_bytes() == previous, name
        reports[name] = dict(sha256=sha(previous), new_native_inputs=0)
    # 단위13 완료 보고서의 전체 파일을 당시 저장한 view로 확인한다. 현재 파일은 쓰지 않는다.
    prior = json.loads((HISTORY / 'work/cpppj/recovery-campaign-runtime13-evidence.json').read_text(encoding='utf-8'))
    for name, expected in prior['files'].items():
        saved = HISTORY / 'work' / name
        target = saved if saved.is_file() else ROOT / name
        assert sha(target.read_bytes()) == expected, name
        dependencies[name] = dict(reported_sha256=expected, verified_path=target.relative_to(ROOT).as_posix())
    names = (*manifest['shared'], 'cpppj/src/o/RawPlayerWorld.h', 'cpppj/src/o/RawPlayerWorld.cpp',
             'cpppj/tests/PlayerWorldTests.cpp', 'tools/decomp_player_world_oracle.py',
             'cpppj/recovery-player-world-evidence.json', 'cpppj/tests/fixtures/player-world-x86.tsv')
    current_files = {name: sha((ROOT / name).read_bytes()) for name in names}
    return dict(sources=sources, history=history, prior_reports=reports,
                previous_unit13=dict(all_tests=prior['all_tests'], new_native_inputs=0, dependencies=dependencies), files=current_files)


def main(verify):
    """좁은 공유 연결을 새 successor로 기록하거나 고정한 현재 바이트를 확인한다."""
    data = dict(schema=1, host='HJOW-Athlon', date='2026-10-14', baseline_commit=COMMIT,
                new_native_inputs=0, lineage=observe(), tool_sha256=sha(Path(__file__).read_bytes()),
                scope='같은 풀의 실제 Player152 전체 PostPop 등록/판단/사전검사/Activate만 추가한다. 이전 모든 본문과 단위13 전체 결과는 당시 바이트로 검증하며 기존 보고서의 SHA를 고치지 않는다.')
    if verify:
        assert json.loads(REPORT.read_text(encoding='utf-8')) == data
    else:
        REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(verified=True, previous_all_tests=1066, preserved_files=len(data['lineage']['history']),
                          prior_reports=len(data['lineage']['prior_reports']), new_native_inputs=0), ensure_ascii=False))


# 명시 CLI에서만 새 출처를 만들거나 현재 고정 자료를 감사한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    main(parser.parse_args().verify)
