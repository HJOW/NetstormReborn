#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 영문 HTML에서 쓰이는 이름 중 보수적으로 비멀티43개만 보호 PE에서 고정한다."""
import hashlib
import json
from pathlib import Path
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 현재 호스트의 원문 이름표와 최초 리터럴 목록을 사용한다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile

EXCLUDED = {'accountsetup', 'chalforcejoinbattle', 'chaljoinbattlenowarning', 'chalsailjoinbattle', 'chalsailpos', 'confirmalliance', 'decendtochallevel', 'gomultiplayer', 'launchrootserver', 'leavebattle', 'leavecheater', 'refreshplayers', 'requestmenu', 'requeststormpower'}  # MP 전용/공유 경계14개는 새 이름 조회 검사에서도 제외한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/html-solo-command-names-pe.tsv'  # 원문 주소/이름/코드의 최초 물리 출력이다.
REPORT = ROOT / 'cpppj/html-solo-command-names-native-20261102.json'  # 이름표 읽기와 실제 기계어 실행을 구별한다.


def digest(path):
    """현재 근거의 전체 바이트 SHA를 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """제품 이름 조회기를 기대값으로 사용하지 않고 같은 PE의 포인터/문자열/DWORD를 읽는다."""
    if '--verify' in sys.argv:
        report = json.loads(REPORT.read_text('utf-8'))
        for name, sha in report['sha256'].items():  # 최초 근거와 출력 파일을 읽기 검증한다.
            assert digest(ROOT / name) == sha, name
        print('HTML 비멀티 최초 이름표43개 보존 확인');return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 최초 이름표 덮어쓰기 금지'
    inventory = ROOT / 'cpppj/html-command-inventory-20261102.json';facts = json.loads(inventory.read_text('utf-8'));selected = [row for row in facts['commands'] if row['native'] and row['key'] not in EXCLUDED]
    assert len(selected) == 43 and len(EXCLUDED) == 14
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'));rows = []
    for row in selected:  # 목록의 주소도 실제 PE에서 다시 읽고 원문 이름/코드와 일치해야 채택한다.
        entry = int(row['native']['entry'], 16);pointer, address = struct.unpack('<II', pe.get_data(entry - 0x400000, 8));name = pe.get_data(pointer - 0x400000, 100).split(b'\0')[0].decode('ascii');code = struct.unpack('<I', pe.get_data(address - 0x400000, 4))[0]
        assert name == row['native']['name'] and f'{address:08x}' == row['native']['globalAddress'] and f'{code:08x}' == row['native']['command']
        rows.append((name, f'{entry:08x}', f'{address:08x}', f'{code:08x}'))
    FIXTURE.write_text('name\tentry\tglobalAddress\tcommand\n' + ''.join('\t'.join(row) + '\n' for row in rows), 'utf-8', newline='\n')
    paths = ['AGENTS.md', 'originals/Netstorm.exe', 'cpppj/html-command-inventory-20261102.json', 'tools/decomp_html_solo_command_names.py', FIXTURE.relative_to(ROOT).as_posix()]
    report = dict(unit=715, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), names=len(rows), excluded=sorted(EXCLUDED), nativeExecutions=0, gameExecutions=0, multiplayerTests=0, fixtureSha256=digest(FIXTURE), limits=['PE의 정적 이름표43개이며 명령 몸체/전체 동적 Config/전체 MP 이름표를 실행하지 않는다', '대소문자 변형/실제 HTML 조회기와의 비교는 후속 CPP 검사이며 이 기대값 생성에 제품을 사용하지 않는다'], sha256={name: digest(ROOT / name) for name in paths})
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n');print('HTML 비멀티 PE 이름43개 고정, MP 관련14개 제외')


if __name__ == '__main__':
    main()
