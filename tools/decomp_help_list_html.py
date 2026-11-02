#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 정확한 QuickHelp HTML 원문 실행기로 새 통합용 stream 바이트만 work에 추가한다."""
from pathlib import Path
import argparse
import sys
import json
import hashlib

ROOT = Path(__file__).resolve().parents[1]  # 같은 보호 PE와 정밀 디컴파일을 읽기만 한다.
WORK = None  # 새 --output 디렉터리를 지정하기 전에는 파일을 작성하지 않는다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_quick_help_html import Oracle, TYPES


def main():
    """기대 바이트를 실제 원문에서 읽으며 기존 fixture는 수정하지 않는다."""
    global WORK
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True,help="기존 파일이 없는 새 원시 디렉터리")
    WORK=parser.parse_args().output.resolve()
    if WORK.exists(): raise FileExistsError(WORK)
    (WORK/"modified/cpppj/tests/fixtures").mkdir(parents=True)
    oracle = Oracle()
    rows = []
    inputs = ['<a name="AlphaType">First <b>bold</b><br>Line</a>\nSTOP', '<a name="AlphaType"><info><p>After</a>\nSTOP', '<a name="AlphaType">A\r\nB<a href="#other">Link</a></a>\nSTOP']
    for source in inputs:  # profile1의 현재 TYPE/번역/Config 입력은 기존 검토한 도구와 같다.
        row = (source, 'AlphaType', 1, 1, 1, 8)
        result = oracle.sample('D', row)
        assert result == oracle.sample('D', row)
        hp = oracle.call(0x49fce0, [], TYPES + 500)
        rows.append((source.encode().hex(), result.split('|')[3], str(hp)))
    fixture = WORK / 'modified/cpppj/tests/fixtures/quick-help-list-html-x86.tsv'
    report = WORK / 'native-html.json'
    assert not fixture.exists() and not report.exists()
    fixture.write_text('# input source hex; actual QuickHelpHtml stream hex; native current hitpoints\n' + '\n'.join('\t'.join(row) for row in rows) + '\n', 'utf-8', newline='\n')
    record = dict(inputs=len(rows), observations=len(rows) * 2, hitPointsGetterObservations=len(rows), actual='existing exact 4c4c00/4c4cc0/4c4c60 + base HTML/links +49fce0', boundary='synthetic TYPE profile1 from reviewed native tool, same independent values for adapter integration; pane/stream/translate/config are explicit boundaries; no original UI/OS/game', gameExecutions=0, multiplayerTests=0, fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest())
    report.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print(json.dumps(dict(inputs=len(rows), observations=len(rows) * 2)))


if __name__ == '__main__':
    main()
