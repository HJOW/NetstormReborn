#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""내용물 삭제 앞의 현재 커서 DWORD 비교 leaf를 두 원본 PE에서 먼저 고정한다."""
import argparse
import csv
import json
import socket
import struct
from decomp_cursordrop_oracle import DropOracle, ROOT, digest
from decomp_constructionplace_oracle import VTABLE

FIXTURE = ROOT / 'cpppj/tests/fixtures/content-cursor-width-x86.tsv'  # 실제 현재번호·자기부모 비교 결과다.
REPORT = ROOT / 'cpppj/content-cursor-width-native-20261026.json'  # 명시 Cancel 경계와 원문 leaf 범위를 구별한다.


def main():
    """제품이나기존fixture를읽지않고추가CPU관측또는최초SHA확인을수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        result = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, wanted in result['sha256'].items():  # 원문·도구·새기대의최초바이트를확인한다.
            if digest(ROOT / name) != wanted:
                raise RuntimeError('내용물커서최초SHA불일치: ' + name)
        print(json.dumps({'verified': True, 'observations': result['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('내용물커서최초관측덮어쓰기금지')
    rows = ['# edition / control / cursorDword / selfParentWord / actualCancelCalls / currentCursorAfter / trace']
    paths = {'originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_content_cursor_width.py', 'tools/decomp_cursordrop_oracle.py', 'tools/decomp_constructionplace_oracle.py', 'tools/decomp_owner_oracle.py', 'tools/decomp_priestgeometry_oracle.py', 'tools/decomp_damageablepredestroy_oracle.py', 'cpppj/tests/fixtures/content-cursor-width-x86.tsv'}
    for edition, entry in (('originals', 0x447390), ('originalCD', 0x43d4c0)):  # 같은논리leaf의서로다른원문을실행한다.
        oracle = DropOracle(edition)
        source = ROOT / 'extracted/refined' / edition / 'functions.tsv'
        facts = {int(row['entry'], 16): row for row in csv.DictReader(source.open(encoding='utf-8'), delimiter='\t')}
        for part in facts[entry]['ranges'].split(';'):  # 현재호스트정밀본의실제몸체만추가한다.
            low, high = (int(value, 16) for value in part.split('-'))
            oracle.allowed.append((low, high + 1))
        paths.add(source.relative_to(ROOT).as_posix())
        paths.update(path.relative_to(ROOT).as_posix() for path in oracle.exports)
        for cursor in (0, 1, 50, 65535, 0x10032, 0x80000032, 0xffffffff):
            for parent in (0, 50):  # 자기부모분기는현재커서와별도로선택된다.
                observations = []
                for control in (0x27f, 0x37f):
                    oracle.case = {'first': parent, 'second': parent, 'sourceChange': 0}
                    oracle.events, oracle.messages, oracle.write_ranges = [], {}, []
                    oracle.sources = 0
                    oracle.put('preview', cursor)
                    oracle.mu.mem_write(oracle.slot(50), bytes(oracle.stride))
                    oracle.mu.mem_write(oracle.slot(50), struct.pack('<I', VTABLE))  #실제가상+0의부모번호반환만명시입력이다.
                    oracle.invoke(entry, [50], control)
                    selected = sum(value == 'I:0' for value in oracle.events)
                    output = [str(selected), str(oracle.get('preview')), ';'.join(oracle.events) or '-']
                    observations.append(output)
                    rows.append('\t'.join([edition, str(control), str(cursor), str(parent), *output]))
                if observations[0] != observations[1]:
                    raise RuntimeError('내용물커서정밀도차이')
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    result = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'observations': len(rows) - 1, 'osExecuted': False, 'originalGameExecuted': False, 'scope': '실제447390/CD43d4c0의DWORD동등비교와자기부모WORD분기를실행했다. 선택된Cancel은명시호출관측이다. 전체ContentPreDestroy나실제Cancel본문실행으로설명하지않는다.', 'sha256': {name: digest(ROOT / name) for name in sorted(paths)}}
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'observations': result['observations']}))


if __name__ == '__main__':
    main()
