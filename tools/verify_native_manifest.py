#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보존된 원문 관측 보고서의 SHA만 검증하며 게임이나 관측 도구를 실행하지 않는다."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 보고서의 원래 저장소 상대경로를 해석하는 기준이다.


def main():
    """복사한 최초 보고서도 내용이나 원래 경로를 바꾸지 않고 모든 입력 SHA를 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path, nargs='+', help='sha256 경로 표를 가진 보존 보고서')
    arguments = parser.parse_args()
    checked = []  # 각 보고서의 검증 수이며 원문 재실행 결과와 구별한다.
    for path in arguments.manifest:  # 지정한 보고서마다 빠진 경로나 해시를 성공으로 넘기지 않는다.
        report = json.loads(path.read_text(encoding='utf-8'))
        entries = report['sha256']
        if not entries:
            raise ValueError(f'원문 SHA 표가 비었습니다: {path}')
        for relative, expected in entries.items():  # 현재 실제 파일 바이트를 최초 보고서의 모든 SHA와 비교한다.
            actual = hashlib.sha256((ROOT / relative).read_bytes()).hexdigest()
            if actual != expected:
                raise ValueError(f'원문 SHA 불일치: {relative}')
        checked.append({'manifest': str(path), 'paths': len(entries), 'observations': report.get('observations')})
    print(json.dumps({'verified': True, 'reports': checked}, ensure_ascii=True))


if __name__ == '__main__':
    main()
