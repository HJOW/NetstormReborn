#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 지원 생성자의 실제 VT+24를 두 보호 PE에서 읽어 내용물 통지 표를 만든다."""
import collections
import hashlib
import json
from pathlib import Path
import re
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 입력과 생성 표를 찾는 저장소 루트다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile

CATALOG = ROOT / 'cpppj/src/o/DerivedConstructors.inc'  # 실제 지원 파생 생성자의 최종 VT 목록이다.
OUTPUT = ROOT / 'cpppj/src/o/ContentNoticeVtables.inc'  # 런타임이 사용하는 원본 VT/진입점 대응표다.
REPORT = ROOT / 'cpppj/content-notice-table.json'  # 입력·출력 해시와 판본별 집계다.
SPECS = (  # 기본 Squid와 실제 내용물 통지 구현들의 판본별 주소다.
    ('originals', 'Netstorm.exe', 'kPatchContentNotices', 0x501dd0,
     (0x41c100, 0x429560, 0x449660, 0x453b50, 0x490750)),
    ('originalCD', 'NETSTORM.EXE', 'kCdContentNotices', 0x5058d8,
     (0x401c20, 0x4e6390, 0x4816c0, 0x47a6c0, 0x407e30)),
)


def digest(path):
    """보호 파일과 생성 표의 SHA-256을 읽기 전용으로 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """파생 생성자 표에서 실제 최종 VT만 수집하고 각 가상 +24 DWORD를 그대로 옮긴다."""
    parts = CATALOG.read_text(encoding='utf-8').split('// originalCD', 1)
    lines = ['// 실제 지원 생성자의 최종 VT+0x24. decomp_content_notice_table.py로 두 PE에서 읽는다.']
    report = {'host': socket.gethostname(), 'date': '2026-10-15', 'tool_sha256': digest(Path(__file__)),
              'catalog_sha256': digest(CATALOG), 'editions': {}}
    # 판본마다 별도 PE와 생성자 집합을 읽는다. 자동 판본 함수 대응표는 사용하지 않는다.
    for part, (edition, filename, name, base, supported) in zip(parts, SPECS):
        binary = ROOT / edition / filename
        pe = pefile.PE(str(binary))
        tables = {base}
        # 각 레시피의 두 번째 값만 최종 가상 표다. 중간 쓰기 값은 수집하지 않는다.
        for match in re.finditer(r'^\s*\{0x[0-9a-fA-F]+U,0x([0-9a-fA-F]+)U,', part, re.M):
            tables.add(int(match.group(1), 16))
        counts = collections.Counter()
        lines += [f'// {edition}의 현재 지원 가상 표와 실제 내용물 부모 통지 진입점이다.',
                  f'constexpr std::array<ContentNoticeEntry,{len(tables)}> {name}{{{{']
        # 같은 VT가 여러 생성자에서 나와도 실행 시 분배 키는 한 번만 둔다.
        for table in sorted(tables):
            entry, = struct.unpack('<I', pe.get_data(table + 0x24 - pe.OPTIONAL_HEADER.ImageBase, 4))
            if entry not in supported:
                raise ValueError(f'{edition}: unknown content notice {table:08x} -> {entry:08x}')
            counts[f'{entry:08x}'] += 1
            lines.append(f'    {{0x{table:08x}U,0x{entry:08x}U}},')
        lines.append('}};')
        report['editions'][edition] = {'binary_sha256': digest(binary), 'tables': len(tables), 'entries': dict(counts)}
    OUTPUT.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    report['output_sha256'] = digest(OUTPUT)
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
