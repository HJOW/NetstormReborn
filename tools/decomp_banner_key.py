#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 Shift 없는 F7을 Guide/Gump/console/정지 관문부터 독립 관측한다."""
import hashlib
import itertools
import json
import socket
from pathlib import Path
import decomp_f8_key_input as base
from unicorn.x86_const import UC_X86_REG_ESP

# Shift F7의 파일/안내와 Ctrl Shift 진단 파일은 별도 복원 범위다.
ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'extracted/cpp-banner-option-546'
FIXTURE = ROOT / 'cpppj/tests/fixtures/banner-key-x86.tsv'
SCHEMA = ('name', 'events', 'guideResult', 'gumpResult', 'pauseDepth', 'console', 'banners',
          'writeUpdates', 'bannersout', 'queueout', 'trace', 'completed', 'stackdelta', 'nativeinstructions')


class Oracle(base.Oracle):
    """실제5319c0 읽기와 서식/Config 외부 장치에 전달한 원문 인자를 관측한다."""

    def read_global(self, mu, access, address, size, value, data):
        """배너 읽기는 CPU가 실제로 해당 DWORD를 읽은 위치에서 기록한다."""
        if address == 0x5319c0 and size == 4:
            self.event('banners', (self.number(address),))
        super().read_global(mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """문자 서식/Config는 명시 장치이며 키/정지/논리 반전은 실제 기계어다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4a89c0:
            dest, form, value = (self.number(esp + at) for at in (4, 8, 12))
            template = self.cstring(form)
            assert template == 'useBanners="%d"' and value in (0, 1)
            self.event('format', (template, value))
            self.mu.mem_write(dest, (template % value).encode('ascii') + b'\0')
            self.returned(dest)
            return
        if address == 0x441580:
            text = self.cstring(self.number(esp + 4))
            self.event('config', (text,))
            assert text in ('useBanners="0"', 'useBanners="1"')
            if self.case['writeUpdates']:
                self.number(0x5319c0, int(text[-2]))
            self.returned()
            return
        super().instruction(mu, address, size, data)

    def run(self, case, control):
        """설정 장치의 명시 응답 외에는 원문의 현재 DWORD를 보존하며 실행한다."""
        self.number(0x5319c0, case['banners'])
        result = super().run(case, control)
        result['bannersout'] = self.number(0x5319c0)
        return result


def cases():
    """Shift 없는 상위 플래그·뗌·정지·선행 UI 소비와 연속 입력을 교차한다."""
    rows, seen = [], set()

    def add(events, **changes):
        """기대 출력을 만들지 않고 독립 사건/장치 입력만 고유화한다."""
        row = dict(name='key-'+str(len(rows)), events=events, guideResult=0, gumpResult=0,
                   pauseDepth=0, console=0, banners=1, writeUpdates=1, command=0x020000f4,
                   read=0, filled=0, hide=0, redraw=0, themeRead=0, themeValue=0, fort=0, metamap=0, editor=0)
        row.update(changes)
        key = tuple(str(row[field]) for field in SCHEMA[1:8])
        if key not in seen:
            seen.add(key)
            rows.append(row)

    # Shift를 제외한 실제 상위 비트를 정지/원래 DWORD와 교차한다.
    for flags, release, pause, enabled in itertools.product(range(32), (0, 0x80000000), (0, 3), (0, 1, 0xffffffff)):
        if flags & 8:
            continue
        add(str(0x760000 | (flags << 26) | release)+':17:23', pauseDepth=pause, banners=enabled)
    # Guide/Gump/console은 전체 F7 효과보다 앞이며 Config 응답이 현재값을 바꾸지 않을 수도 있다.
    for guide, gump, console, updates in itertools.product((0, 7), (0, 11), (0, 1), (0, 1)):
        add('7733248:17:23', guideResult=guide, gumpResult=gump, console=console, writeUpdates=updates)
    add('7733248:0:0;2155216896:0:0;276168704:0:0;1081475072:0:0')
    return rows


def main():
    """원문이 받은 Config 서식 인자와 두 정밀도 결과를 제품 작성 전에 고정한다."""
    assert base.gate.digest(base.gate.PE_PATH) == base.gate.PE_SHA
    oracle, rows, observed = Oracle(), cases(), []
    # 모든 행은 같은 초기 명시 장치에서 두 번 독립 실행한다.
    for row in rows:
        first, second = oracle.run(row, 0x27f), oracle.run(row, 0x37f)
        assert first == second, row['name']
        observed.append({**row, **first})
    FIXTURE.write_bytes(('\t'.join(SCHEMA)+'\n'+'\n'.join(
        '\t'.join(str(row[field]) for field in SCHEMA) for row in observed)+'\n').encode('utf-8'))
    report = {'host': socket.gethostname(), 'peSha256': base.gate.PE_SHA, 'inputs': len(rows), 'nativeObservations': len(rows)*2,
              'precisionDifferences': 0, 'fixtureSha256': hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
              'nativeBodiesUnchanged': True, 'externalDevices': ['sprintf', 'Config application', 'Guide/Gump devices'],
              'scope': 'Shift 없는 일반/Control/Alt F7. Shift F7 GIF와 Ctrl Shift 진단 파일의 새 실행/검사 없음'}
    (OUTPUT/'key-native.json').write_bytes((json.dumps(report, ensure_ascii=False, indent=2)+'\n').encode('utf-8'))
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
