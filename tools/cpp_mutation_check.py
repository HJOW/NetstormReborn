#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""cpppj 소스를 일부러 한 군데씩 틀리게 바꿔, 기계어 대조 검사가 그 차이를 실제로 잡아내는지 확인한다.

변이마다 소스 한 파일을 바꿔 Release 빌드와 콘솔 검사를 실행하고, 끝나면(실패/중단 포함) 원래 바이트로 되돌린 뒤 다시 빌드한다.
게임·클론 창·원본 실행 파일은 실행하지 않는다. 빌드 폴더(cpppj/build)가 이미 구성돼 있어야 한다.

    python -X utf8 tools/cpp_mutation_check.py --list           # 변이 목록과 적용 가능 여부만 확인(빌드 없음)
    python -X utf8 tools/cpp_mutation_check.py                  # 모든 변이 실행
    python -X utf8 tools/cpp_mutation_check.py --only process-zero-keeps-running
    python -X utf8 tools/cpp_mutation_check.py --cmake "C:/.../cmake.exe"

각 변이는 기대하는 실패 검사 이름의 접두사를 갖는다. 그 접두사의 검사가 하나도 실패하지 않으면 "미검출"로 보고한다.
"""
import argparse
import subprocess
import sys
from pathlib import Path

# 저장소 루트와 빌드/검사 실행 파일 위치.
ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / 'cpppj/build'
TESTS = BUILD / 'bin/Release/netstorm_tests.exe'
# 변이 목록: 이름, 대상 파일, 바꿀 원문(파일에 정확히 한 번 있어야 한다), 바꾼 문장, 실패해야 하는 검사 이름 접두사.
MUTATIONS = [
    dict(name='process-zero-keeps-running', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='if (result>0.0f || (patch && std::isnan(result))) {',
         after='if (result>=0.0f || (patch && std::isnan(result))) {',
         note='처리기 반환 0을 종료가 아니라 재예약으로 취급'),
    dict(name='process-unlink-under-dead-parent', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='    if (parentState&kDead) return;\n',
         after='    // 변이: dead 부모에서도 종속 체인을 고친다.\n',
         note='부모가 지워지는 중에도 form을 체인에서 뺌'),
    dict(name='process-skip-previous-link', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='    if (head) SetPrevious(Sid{head},form.value);\n',
         after='    // 변이: 기존 머리의 이전 항목 연결 생략.\n',
         note='부착 때 기존 머리의 이전 항목을 쓰지 않음'),
    dict(name='process-run-abstract-parent', file='cpppj/src/o/SquidProcess.cpp', expect='process_patch',
         before='    if (patch && (raw[40]&1) && !(types_.at(raw[kType]).flags2&kAbstractRunGenus)) return;\n',
         after='    // 변이: 패치판 abstract 부모 건너뛰기 생략.\n',
         note='패치판에서 abstract 부모의 Regular도 실행'),
    dict(name='process-no-geyser-poll', file='cpppj/src/o/SquidProcess.cpp', expect='process_',
         before='    if (raw[kType]==state_.pollType && process.time_-state_.now>kPollLimit) process.time_=state_.now;\n',
         after='    // 변이: geyser의 먼 예약 당김 생략.\n',
         note='geyser에 붙은 Regular의 0.5초 당김 생략'),
    dict(name='reward-percent-rounding', file='cpppj/src/o/SquidReward.cpp', expect='reward_',
         before='static_cast<std::uint32_t>(state_.percent)*static_cast<std::uint32_t>(cost)))/100);',
         after='static_cast<std::uint32_t>(state_.percent)*static_cast<std::uint32_t>(cost)))/100+1);',
         note='삭제 보상 지급액을 1 크게 계산'),
    dict(name='reward-skip-noise', file='cpppj/src/o/SpStore.cpp', expect='reward_patch',
         before='        for (int n=0;n<kNoisePerBit;++n) word|=(std::uint32_t{1}<<rng_.Next(kBitLimit))&~mask;\n',
         after='        // 변이: SP 저장소의 난수 잡음 생략.\n',
         note='패치판 SP 저장소가 난수를 소비하지 않음'),
]


def run(command):
    """명령을 실행하고 표준 출력/오류를 합쳐 돌려준다. 실패 코드는 호출자가 판단한다."""
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, encoding='utf-8', errors='replace')
    return result.returncode, result.stdout + result.stderr


def build(cmake):
    """Release 빌드를 실행한다. 빌드 오류는 변이가 컴파일되지 않았다는 뜻이므로 그대로 보고한다."""
    return run([cmake, '--build', str(BUILD), '--config', 'Release'])


def check(mutation):
    """대상 파일에 원문이 정확히 한 번 있는지 확인하고 원래 바이트를 돌려준다."""
    path = ROOT / mutation['file']
    original = path.read_bytes()
    count = original.decode('utf-8').count(mutation['before'])
    return path, original, count


def main():
    """변이를 차례로 적용/빌드/검사하고, 어떤 경우에도 원래 소스로 되돌린 뒤 다시 빌드한다."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--list', action='store_true', help='변이 목록과 적용 가능 여부만 출력한다')
    parser.add_argument('--only', action='append', help='이 이름의 변이만 실행한다(여러 번 줄 수 있다)')
    parser.add_argument('--cmake', default='cmake', help='cmake 실행 파일 경로')
    options = parser.parse_args()
    selected = [m for m in MUTATIONS if not options.only or m['name'] in options.only]
    if options.list:
        # 빌드 없이 원문 위치만 확인한다.
        for mutation in selected:
            _, _, count = check(mutation)
            print(f"{mutation['name']}\t{mutation['file']}\t원문 {count}곳\t{mutation['note']}")
        return 0 if all(check(m)[2] == 1 for m in selected) else 1
    if not TESTS.exists():
        print('검사 실행 파일이 없습니다. 먼저 cpppj를 Release로 빌드하세요.')
        return 2
    missed = 0
    # 변이는 한 번에 하나만 적용한다. 원래 바이트는 메모리에 들고 있다가 finally에서 되돌린다.
    for mutation in selected:
        path, original, count = check(mutation)
        if count != 1:
            print(f"{mutation['name']}: 원문이 {count}곳이라 건너뜀(소스가 바뀌었으면 변이 정의를 고쳐야 함)")
            missed += 1
            continue
        try:
            path.write_bytes(original.decode('utf-8').replace(mutation['before'], mutation['after']).encode('utf-8'))
            code, output = build(options.cmake)
            if code != 0:
                print(f"{mutation['name']}: 빌드 실패(변이가 컴파일되지 않음)")
                missed += 1
                continue
            _, output = run([str(TESTS)])
            failed = [line.split('] ', 1)[1].strip() for line in output.splitlines() if line.startswith('[FAIL]')]
            checks = sum('CHECK failed' in line for line in output.splitlines())
            hit = [name for name in failed if name.startswith(mutation['expect'])]
            status = '검출' if hit else '미검출'
            if not hit: missed += 1
            print(f"{mutation['name']}: {status} — 실패 검사 {len(failed)}개({', '.join(failed) or '없음'}), 실패 CHECK {checks}개")
        finally:
            path.write_bytes(original)
    # 마지막 변이의 목적 파일이 남지 않도록 원래 소스로 다시 빌드한다.
    code, _ = build(options.cmake)
    print('원래 소스로 재빌드:', '성공' if code == 0 else '실패')
    print(f'변이 {len(selected)}개 중 미검출/건너뜀 {missed}개')
    return 1 if missed or code else 0


if __name__ == '__main__':
    sys.exit(main())
