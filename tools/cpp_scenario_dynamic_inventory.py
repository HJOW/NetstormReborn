#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 영문 대본의 동적 HTML 명령 템플릿과 치환 인자를 실행 없이 조사한다."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import socket

from taff import TaffArchive

ROOT = Path(__file__).resolve().parents[1]  # 보호 자산은 읽기만 하고 보고서는 지정한 새 경로에 쓴다.
META = re.compile(r'\$(ButtonInstant|Button|Menu|Checked|Timeout|OnExit|Key)[ \t]*=[ \t]*([^,\r\n]+),([^,\r\n]+),([^\r\n]*)')  # 섹션 경계를 넘어가는 오자 문장을 명령 이름으로 합치지 않는 정적 조사 문법이다.
INLINE = re.compile(r'<\$([^,;>#\r\n]+)([^>\r\n]*)>')  # 치환 전의 명시 태그이며 실제 HTML 실행 횟수가 아니다.


def main():
    """TARC/느슨한 파일의 출처를 유지해 동적 명령·인자 목록과 원시 해시를 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise FileExistsError('기존 조사 보고서를 덮어쓰지 않는다')
    archive = TaffArchive(str(ROOT / 'originals/netstorm.tarc'))
    documents = [('tarc:' + entry.name, archive.read(entry)) for entry in archive.entries if entry.name.lower().endswith('.english')]
    documents += [(path.relative_to(ROOT).as_posix(), path.read_bytes()) for path in sorted((ROOT / 'originals').rglob('*.english'))]
    dynamic_names = defaultdict(list)  # 명령 이름 자체에 치환식이 있는 원시 위치다.
    dynamic_arguments = defaultdict(list)  # 명령 이름은 고정이고 표시/인자가 바뀌는 위치다.
    manifest = []  # 첫 관측 자료의 출처와 내용 해시를 유지한다.
    for name, raw in documents:  # 보호 파일과 아카이브 payload를 쓰거나 게임을 구동하지 않는다.
        text = raw.decode('latin-1')
        manifest.append((name, hashlib.sha256(raw).hexdigest()))
        candidates = [(match.start(), match[1], match[3], match[2] + ',' + match[4]) for match in META.finditer(text)]
        candidates += [(match.start(), 'inline', match[1], match[2]) for match in INLINE.finditer(text)]
        for offset, kind, command, argument in candidates:  # 두 경계의 템플릿을 구별하며 치환 결과를 추측하지 않는다.
            row = dict(source=name, line=text.count('\n', 0, offset) + 1, kind=kind, command=command, argument=argument)
            if '{' in command:
                dynamic_names[command].append(row)
            elif '{' in argument:
                dynamic_arguments[command].append(row)
    names = {key: dict(count=len(rows), examples=rows[:8]) for key, rows in sorted(dynamic_names.items())}  # 보고서에는 대표 위치만 싣고 전체 위치는 별도 해시로 고정한다.
    arguments = {key: dict(count=len(rows), examples=rows[:8]) for key, rows in sorted(dynamic_arguments.items())}  # 대소문자 차이를 원시 표에 남긴다.
    record = dict(unit=940, date='2026-11-06', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-11-03'),
                  documents=len(documents), dynamicCommandNames=names, dynamicArgumentCommands=arguments,
                  allOccurrencesSha256=hashlib.sha256(json.dumps([dict(dynamic_names), dict(dynamic_arguments)], ensure_ascii=False, sort_keys=True).encode('utf-8')).hexdigest(),
                  sourceManifestSha256=hashlib.sha256(json.dumps(manifest, separators=(',', ':')).encode('utf-8')).hexdigest(),
                  gameExecutions=0, multiplayerTests=0,
                  limits=['보호 영문 문서의 정적 템플릿 조사이며 실제 Config 확장·가림·조건·동적 실행 전체를 증명하지 않는다.',
                          '임의 사용자 파일과 설정에서 새로 만드는 명령 이름은 이 목록으로 제한하지 않는다.',
                          '기존687/715의 전체 리터럴 표를 대체하지 않으며 원본 오자를 자동 수정하지 않는다.'])
    args.output.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(documents=len(documents), dynamicNames=len(dynamic_names), dynamicArgumentNames=len(dynamic_arguments)), ensure_ascii=False))


if __name__ == '__main__':
    main()
