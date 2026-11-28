#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새 관측 JSON을 별도984행 TSV로 옮긴다. bool기대값은 계산하지 않고 실제 EAX를 그대로 저장한다."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    """새 ignored 출력에만 기록하며 입력관측·경계·현재CS주소 정규화 계약을 머리에 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('input', type=Path); parser.add_argument('output', type=Path); args = parser.parse_args()
    folder = Path(__file__).resolve().parents[1] / 'extracted/dotnet-scene-world-draft'; source = args.input.resolve(); output = args.output.resolve()
    if source.parent != folder or output.parent != folder or output.exists():
        raise RuntimeError('같은 ignored 폴더의 입력과 아직 없는 새 출력만 허용한다')
    data = json.loads(source.read_text(encoding='utf-8')); rows = data['rows']
    if len(rows) != 984 or data['binary_sha256'] != 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('승인한 두 함수 원본/984입력 관측과 다르다')
    lines = ['# PATCH 00449220/0042cbd0의 새 독립 관측984행. 게임/OS/채널은 실행하지 않았다.',
        '# 원문기록경계=priest,altar,count,local. 하위본체/실제Game결선 검증으로 세지 않는다.',
        '# 입력SHA256=' + hashlib.sha256(source.read_bytes()).hexdigest(),
        '# 입력=owner,priest,altar,frame_signed,count,mutate,challenge,known,flag',
        '# 열=edition operation input eax events frame_after current_cs(A/B:실제관측0x16100000/0x16110000정규화)']
    # 입력은 원시 필드 순서로 옮기고 출력은 원본 관측에서만 가져온다.
    for row in rows:
        case = row['input']; values = [case.get(key, 0) for key in ('owner', 'priest', 'altar', 'frame', 'count', 'mutate', 'challenge', 'known', 'flag')]
        events = []
        # 각 경계 인자를 기록된 순서대로 유지한다. Count의 주소는 원문 격리 주소 그대로다.
        for event in row['events']:
            events.append(event[0] + (':' + ','.join(map(str, event[1:])) if len(event) > 1 else ''))
        cs = {0x16100000: 'A', 0x16110000: 'B'}.get(row['current_cs_after'])
        if cs is None or row['eax'] != row['bool_byte'] or row['eax'] not in (0, 1):
            raise RuntimeError('예상 밖 실제CS주소/반환폭 관측')
        lines.append('\t'.join(('originals', case['operation'], ','.join(map(str, values)), str(row['eax']),
            '|'.join(events) or '-', str(row['frame_after']), cs)))
    output.write_text('\n'.join(lines) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
