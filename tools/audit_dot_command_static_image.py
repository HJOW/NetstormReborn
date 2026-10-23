#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""점 명령 인접 표의 PE 초기 데이터만 동결하며 게임·기계어는 실행하지 않는다."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE와 저장소 상대 경로의 기준이다.
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile

FIXTURE = ROOT / 'cpppj/tests/fixtures/dot-command-static-image.hex'  # 제품 소유자를 만들기 전에 읽은 전체2568BYTE다.
REPORT = ROOT / 'cpppj/dot-command-static-image.json'  # 정적 데이터 관측이며 원문 함수 실행 보고서와 구별한다.


def sha(path):
    """실제 파일 바이트의 SHA256을 반환한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    """최초 PE 추출과 이후 불변 SHA 검증을 구별하며 기존 관측을 덮어쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, expected in report['sha256'].items():  # 최초 관측의 모든 입력을 현재 실제 바이트와 대조한다.
            if sha(ROOT / name) != expected:
                raise ValueError('정적 점 명령 표 SHA 불일치: ' + name)
        print(json.dumps({'verified': True, 'bytes': report['bytes'], 'count': report['count']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise FileExistsError('최초 정적 표 관측은 덮어쓰지 않습니다')
    pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'))  # 실제 게임 프로세스가 아닌 읽기 전용 PE 파서다.
    raw = pe.get_data(0x531b30 - pe.OPTIONAL_HEADER.ImageBase, 0xa08)  # 이름/콜백/표시/count를 같은 물리 범위로 읽는다.
    if len(raw) != 0xa08:
        raise ValueError('PE 점 명령 초기 표의 물리 길이 부족')
    FIXTURE.write_text(raw.hex() + '\n', encoding='utf-8')
    report = {'kind': 'static-pe-data-no-execution', 'address': '00531b30', 'bytes': len(raw),
              'count': int.from_bytes(raw[0xa04:0xa08], 'little'),
              'firstName': raw[:32].split(b'\0')[0].decode('ascii'),
              'firstCallback': hex(int.from_bytes(raw[0x800:0x804], 'little')),
              'firstVisible': hex(int.from_bytes(raw[0x900:0x904], 'little')),
              'sha256': {str(path.relative_to(ROOT)).replace('\\', '/'): sha(path)
                         for path in (ROOT / 'originals/Netstorm.exe', Path(__file__).resolve(), FIXTURE)}}  # 제품 파일을 기대의 입력으로 사용하지 않는다.
    REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
