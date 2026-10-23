#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""최초166관측의 원래 SHA를 보존하고 역사적 Screen 입력만 저장된 바이트에 대응한다."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 현재 저장소의 보호 PE/정밀 C와 기존 helper를 읽는 기준이다.
ARCHIVE = ROOT / 'docs/exe/evidence/screen-palette-clear-20261024'  # 제품 적용 전 도구/보고서/Screen 원문 바이트다.
MAPPED = {  # 최초 보고서의 원래 경로 문자열을 바꾸지 않고 현재 보존 위치만 선택한다.
    'extracted/cpp-palette-device-20261024-proposed/oracle_screen_palette_clear.py': ARCHIVE / 'oracle_screen_palette_clear.py',
    'extracted/cpp-palette-device-20261024-proposed/screen-palette-clear-first.tsv': ROOT / 'cpppj/tests/fixtures/screen-palette-clear-x86.tsv',
    'cpppj/src/client/Screen.h': ARCHIVE / 'Screen.h.before',
    'cpppj/src/client/Screen.cpp': ARCHIVE / 'Screen.cpp.before',
}


def main():
    """최초 관측의 각 입력·도구·fixture를 재확인하며 현재 제품과 역사적 기준을 혼동하지 않는다."""
    report = json.loads((ARCHIVE / 'screen-palette-clear-first.json').read_text(encoding='utf-8'))
    for logical, expected in report['sha256'].items():  # 원래 보고서의 모든 SHA를 생략 없이 대조한다.
        physical = MAPPED.get(logical, ROOT / logical)
        actual = hashlib.sha256(physical.read_bytes()).hexdigest()
        if actual != expected:
            raise RuntimeError(f'최초 팔레트 clear SHA 불일치: {logical}')
    if report['observations'] != 166:
        raise RuntimeError('최초 팔레트 clear 관측 수 불일치')
    print(json.dumps({'verified': True, 'observations': report['observations'], 'sha256_paths': len(report['sha256'])}))


if __name__ == '__main__':
    main()
