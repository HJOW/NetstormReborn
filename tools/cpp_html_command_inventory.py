#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 loose/TARC 영문 HTML의 명시 메뉴·버튼/태그 명령을 읽어 실제 연결 대조 후보를 찾는다."""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 현재 보호 원문과 제품 소스가 있는 같은 저장소다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from taff import TaffArchive


def digest(data):
    """제품 파서 대신 원문 전체 바이트의 물리 SHA를 기록한다."""
    return hashlib.sha256(data).hexdigest()


def main():
    """프로세스나 보호 자료를 쓰지 않고 지정한 새 보고서만 생성한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--output', type=Path, required=True);args = parser.parse_args();assert not args.output.exists()
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'));names = {}
    for entry in range(0x542b80, 0x543500, 8):  # 확인된 원본 이름표의 문자열/현재 명령 DWORD 쌍이다.
        pointer, address = struct.unpack('<II', pe.get_data(entry - 0x400000, 8))
        if 0x400000 <= pointer < 0x540000 and 0x542700 <= address < 0x542b80:
            name = pe.get_data(pointer - 0x400000, 100).split(b'\0')[0].decode('ascii');names[name.lower()] = dict(name=name, entry=f'{entry:08x}', globalAddress=f'{address:08x}', command=f'{struct.unpack("<I", pe.get_data(address - 0x400000, 4))[0]:08x}')
    documents = [];archive_path = ROOT / 'originals/netstorm.tarc';archive = TaffArchive(str(archive_path))
    for entry in archive.entries:  # TARC 원문도 loose 파일과 별도로 남겨 출처/가림 관계를 감추지 않는다.
        if entry.name.lower().endswith('.english'):
            documents.append(('tarc:' + entry.name, archive.read(entry)))
    for path in sorted((ROOT / 'originals').rglob('*')):  # 느슨한 영문 파일은 실제 보관 경로를 모두 읽는다.
        if path.is_file() and path.suffix.lower() == '.english':
            documents.append((path.relative_to(ROOT).as_posix(), path.read_bytes()))
    uses = defaultdict(list);kinds = Counter();source_manifest = []
    pattern = re.compile(r'\$(ButtonInstant|Button|Menu|Checked|Timeout|OnExit|Key|Input|Zsend)[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+)')  # 메타의 명시 첫세 인자이며 제품의 원문 regex와 같은 경계다.
    for path, raw in documents:  # Config 치환 전 리터럴과 메뉴 둘째 인자를 추출한다.
        source_manifest.append((path, digest(raw)));text = raw.decode('latin-1')
        for match in pattern.finditer(text):
            kind, label, command, argument = match.groups();kinds[kind] += 1
            if kind not in ('Input', 'Zsend'):
                uses[command.lower()].append(dict(source=path, kind=kind, label=label[:100], argument=argument[:180]))
        for match in re.finditer(r'<\s*\$\s*([A-Za-z][A-Za-z0-9_]*)', text):  # HTML1차의 명시 명령 태그도 별도 종류로 센다.
            uses[match[1].lower()].append(dict(source=path, kind='tag', label='', argument=''));kinds['tag'] += 1
    sources = [(path.relative_to(ROOT).as_posix(), path.read_text('utf-8')) for path in (ROOT / 'cpppj/src').rglob('*') if path.suffix in ('.cpp', '.h', '.inc')]
    rows = []
    for key, values in sorted(uses.items()):  # 문자열 존재는 소스 대조의 출발점이며 구현 완료 판정이 아니다.
        hits = [path for path, source in sources if re.search('"' + re.escape(key) + '"', source, re.I)]
        rows.append(dict(key=key, native=names.get(key), occurrences=len(values), sourceFiles=len({row['source'] for row in values}), cppStringHits=hits[:10], examples=values[:8]))
    record = dict(unit=715, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), kind='read-only literal HTML inventory', gameExecutions=0, nativeExecutions=0, multiplayerTests=0, scannedNameEntries=len(names), documents=len(documents), sourceKinds=dict(kinds), commands=rows, sourceManifestSha256=digest(json.dumps(source_manifest, ensure_ascii=False, separators=(',', ':')).encode('utf-8')), sha256={name: digest((ROOT / name).read_bytes()) for name in ['AGENTS.md', 'originals/Netstorm.exe', 'originals/netstorm.tarc', 'tools/cpp_html_command_inventory.py']}, limits=['Config/TARC의 실제 가림·동적 치환·전체 명령 실행 완료 판정이 아니다', 'CPP 문자열 존재는 주석/무관 분기를 포함할 수 있으며 이후 실제 이름 조회/입력 부모를 읽어 확인한다', '메타의 줄바꿈 캡처/오자/동적 이름을 임의 trim/별칭으로 바꾸지 않는다'])
    args.output.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(documents=len(documents), commands=len(rows), kinds=dict(kinds), noCppLiteral=[dict(key=row['key'], native=row['native'], count=row['occurrences'], examples=row['examples'][:2]) for row in rows if not row['cppStringHits']]), ensure_ascii=False))


if __name__ == '__main__':
    main()
