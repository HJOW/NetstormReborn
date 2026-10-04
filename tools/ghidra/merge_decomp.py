#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""전체 디컴파일 결과(Netstorm.c)와 누락 함수 복구 결과(*-missing.c)를 주소 순으로 합친 파일을 만든다.

기존 Netstorm.c 는 문서의 줄 번호 인용 때문에 건드리지 않고, 합친 결과는 별도 파일(<이름>.all.c)로 쓴다.
같은 주소가 양쪽에 있으면 전체 디컴파일(Netstorm.c) 쪽을 우선한다.
복구 함수 중 '--skip-range 시작-끝'(16진수)에 들어가는 것은 합치지 않는다. 패치판은 메인 프레임 함수 FUN_004d62b0
(0x4d62b0~0x4da64f) 안의 switch 분기 블록이 각각 1,650줄짜리 가짜 함수로 복구되어 기본으로 제외한다.

사용 예 (저장소 루트에서):
    python tools/ghidra/merge_decomp.py                       # 패치판 (originals)
    python tools/ghidra/merge_decomp.py --edition originalCD  # 이전 CD판
"""
import argparse
import re
import sys
from pathlib import Path

# 함수 블록 머리줄 형식: "// ==== 함수이름 @ 주소" 로 시작한다
HEADER_RE = re.compile(r'^// ==== (\S+) @ ([0-9a-fA-F]+)', re.M)
# 판본별 기본 제외 범위 (복구 함수 중 이 범위 안에서 시작하는 것은 가짜 함수로 보고 합치지 않는다)
DEFAULT_SKIP = {
    'originals': [(0x4d62b0, 0x4da64f)],
    'originalCD': [],
}

# 함수 이름 대신 주소만 적힌 실패 줄: "// ==== 주소 함수를 만들 수 없음"
FAIL_RE = re.compile(r'^// ==== ([0-9a-fA-F]+) 함수를 만들 수 없음', re.M)


def split_blocks(text):
    """텍스트를 (주소, 블록 문자열) 목록으로 나눈다. 머리줄 앞의 내용은 버린다."""
    marks = [m.start() for m in re.finditer(r'^// ==== ', text, re.M)]
    marks.append(len(text))
    blocks = []
    for begin, end in zip(marks, marks[1:]):
        block = text[begin:end]
        head = HEADER_RE.match(block)
        if head:
            blocks.append((int(head.group(2), 16), block))
            continue
        fail = FAIL_RE.match(block)
        if fail:
            blocks.append((int(fail.group(1), 16), block))
    return blocks


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--edition', choices=['originals', 'originalCD'], default='originals')
    parser.add_argument('--skip-range', action='append', default=[], metavar='시작-끝',
                        help='합치지 않을 복구 함수 주소 범위(16진수, 예: 4d62b0-4da64f). 여러 번 지정 가능')
    args = parser.parse_args()
    # 기본 제외 범위에 명령줄 범위를 더한다
    skip = list(DEFAULT_SKIP[args.edition])
    for spec in args.skip_range:
        lo, hi = spec.split('-')
        skip.append((int(lo, 16), int(hi, 16)))

    # 저장소 루트 (이 스크립트 기준 두 단계 위)와 판본별 입출력 경로
    root = Path(__file__).resolve().parents[2]
    if args.edition == 'originalCD':
        base = root / 'extracted' / 'originalCD' / 'decomp' / 'NETSTORM.c'
    else:
        base = root / 'extracted' / 'decomp' / 'Netstorm.c'
    missing = root / 'extracted' / 'decomp-at' / f'{args.edition}-missing.c'
    out = base.with_suffix('.all.c')
    for path in (base, missing):
        if not path.exists():
            sys.exit(f'입력 파일이 없습니다: {path}')

    # 전체 디컴파일 블록을 먼저 넣고, 없는 주소만 누락 함수 블록으로 채운다
    merged = {}
    for addr, block in split_blocks(base.read_text(encoding='utf-8')):
        merged[addr] = block
    added = 0
    skipped = 0
    for addr, block in split_blocks(missing.read_text(encoding='utf-8')):
        # 제외 범위 안의 복구 함수는 건너뛴다
        if any(lo <= addr <= hi for lo, hi in skip):
            skipped += 1
            continue
        if addr not in merged:
            merged[addr] = block
            added += 1

    # 주소 순으로 기록
    with open(out, 'w', encoding='utf-8', newline='') as fp:
        for addr in sorted(merged):
            block = merged[addr]
            fp.write(block if block.endswith('\n') else block + '\n')
    print(f'합친 함수 {len(merged)}개 (누락 복구분 {added}개 추가, 제외 범위 {skipped}개 건너뜀) → {out}')


if __name__ == '__main__':
    main()
