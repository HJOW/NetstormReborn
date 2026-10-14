#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보존 이력의 중첩 Git 속성보다 우선하는 로컬 원문 보존 규칙을 설치한다."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 이 저장소의 이력과 로컬 Git 설정만 처리한다.
RULE = b'cpppj/hist/** -text whitespace=cr-at-eol,-blank-at-eol,-blank-at-eof,-space-before-tab'  # 과거 .gitattributes가 보존 파일의 줄바꿈을 변환하지 못하게 한다.


def main(restore=False):
    """기존 로컬 속성을 유지해 규칙을 추가하고 선택적으로 체크아웃의 줄바꿈 변환만 복원한다."""
    location = subprocess.check_output(['git', 'rev-parse', '--git-path', 'info/attributes'], cwd=ROOT).decode().strip()
    attributes = (ROOT / location).resolve()
    previous = attributes.read_bytes() if attributes.exists() else b''
    if RULE not in previous.splitlines():
        attributes.parent.mkdir(parents=True, exist_ok=True)
        attributes.write_bytes(previous + (b'\n' if previous and not previous.endswith(b'\n') else b'') + RULE + b'\n')
    print('이력 원문 보존 속성 설치:', attributes)
    if not restore:
        return
    record = json.loads((ROOT / 'docs/path-map.json').read_text(encoding='utf-8'))
    repairs = []
    # 현재 이름에 해당하는 HEAD 원문만 읽고 실제 내용 변경은 전체 계획 단계에서 거부한다.
    for entry in record['files'].values():
        name = entry['target']
        if not name.startswith('cpppj/hist/'):
            continue
        path = ROOT / name
        current = path.read_bytes()
        if hashlib.sha256(current).hexdigest() == entry['after']:
            continue
        body = subprocess.check_output(['git', 'show', 'HEAD:' + name], cwd=ROOT)
        assert hashlib.sha256(body).hexdigest() == entry['after'], 'HEAD와 대응표가 다릅니다: ' + name
        assert current.replace(b'\r\n', b'\n') == body.replace(b'\r\n', b'\n'), '줄바꿈 이외의 편집을 보존합니다: ' + name
        assert path.resolve().is_relative_to(ROOT / 'cpppj/hist')
        repairs.append((path, body))
    # 사전 대조가 모두 성공한 경우에만 체크아웃 과정에서 바뀐 줄바꿈을 되돌린다.
    for path, body in repairs:
        path.write_bytes(body)
    print('체크아웃 줄바꿈 복원:', len(repairs), '파일')


# 기본은 로컬 속성 설치이며 --restore는 커밋된 이력의 줄바꿈만 복원한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--restore', action='store_true')
    arguments = parser.parse_args()
    main(restore=arguments.restore)
