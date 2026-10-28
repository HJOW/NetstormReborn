#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Shift F7의 선행 저장·안내·정지 순서와 GIF 파일 번호를 독립 관측한다."""
import hashlib
import itertools
import json
import socket
from pathlib import Path
import decomp_banner_key as base
from unicorn.x86_const import UC_X86_REG_ESP

# 기존 관측기는 보호 PE와 Guide/Gump/일반 F7의 실제 본체를 제공한다.
ROOT = Path(__file__).resolve().parents[1]  # 보호 입력과 새 산출물의 저장소 루트다.
OUTPUT = ROOT / 'extracted/cpp-screenshot-key-548'  # 이번 단위의 원시 관측 폴더다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/screenshot-key-x86.tsv'  # 제품과 무관하게 먼저 동결하는 새 입력/관측이다.
EXTRA = (0x4db140, 0x4db206)  # 번호 선택과 실제 화면 저장 호출의 원문 범위다.
# 앞14열은 입력이고 나머지는 실제 원문 관측이다.
SCHEMA = ('name', 'events', 'guideResult', 'gumpResult', 'pauseDepth', 'console', 'banners',
          'counter', 'existing', 'tellPauses', 'spec', 'width', 'height', 'pitch',
          'counterout', 'bannersout', 'trace', 'completed', 'stackdelta', 'nativeinstructions')


class Oracle(base.Oracle):
    """번호 선택과 호출 순서는 원문을 실행하고 파일·설정·화면 장치만 제공한다."""

    def __init__(self):
        """원문 저장 루틴의 불변 범위를 기존 키 관측기에 추가한다."""
        super().__init__()
        self.original[EXTRA[0]] = bytes(self.mu.mem_read(EXTRA[0], EXTRA[1]-EXTRA[0]))

    def guard(self, mu, access, address, size, value, data):
        """저장 번호 DWORD의 실제 증가만 추가 허용한다."""
        if address == 0x5e1830 and size == 4:
            self.writes['screenshotCounter'] += 1
        else:
            super().guard(mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """외부 파일·문자열 경계에 실제 전달된 인자와 순서를 기록한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4a89e0:
            dest, form, value = (self.number(esp+offset) for offset in (4, 8, 12))
            assert self.cstring(form) == 'pic%d.gif'
            signed = value if value < 0x80000000 else value-0x100000000
            self.mu.mem_write(dest, f'pic{signed}.gif'.encode('ascii')+b'\0')
            self.returned(dest)
            return
        if address == 0x4414d0:
            key, value = (self.cstring(self.number(esp+offset)) for offset in (4, 8))
            self.event('set', (key, value))
            assert key == 'GifOutName'
            self.filename = value
            self.returned()
            return
        if address == 0x441300:
            key, dest = self.cstring(self.number(esp+4)), self.number(esp+8)
            assert key == 'GifOutSpec1'
            value = self.case['spec'].replace('{GifOutName}', self.filename)
            self.event('get', (key, value))
            self.mu.mem_write(dest, value.encode('ascii')+b'\0')
            self.returned(1)
            return
        if address == 0x41ab10:
            path = self.cstring(self.number(esp+4))
            exists = self.checks < self.case['existing']
            self.checks += 1
            self.event('exists', (path, int(exists)))
            self.returned(int(exists))
            return
        if address in (0x4a14e0, 0x4a1550):
            self.event('lock' if address == 0x4a14e0 else 'unlock')
            self.returned()
            return
        if address == 0x4daff0:
            self.event('gif', (self.cstring(self.number(esp+4)), *(self.number(esp+offset) for offset in (8, 12, 16, 20))))
            self.returned()
            return
        if address == 0x4c2630:
            self.event('log', (self.number(esp+4), self.cstring(self.number(esp+8)).replace('\n', r'\n'), self.cstring(self.number(esp+12))))
            self.returned()
            return
        if address == 0x4cf960:
            self.event('tell', (self.cstring(self.number(esp+4)),))
            if self.case['tellPauses']:
                self.number(base.base.view.gate.PAUSE, 1)
            self.returned()
            return
        if address in (0x4437f0, 0x4d6020, 0x471890):
            self.event('diagnostic', (hex(address),))
            self.returned()
            return
        if EXTRA[0] <= address < EXTRA[1]:
            self.instructions += 1
            self.visited.add(address)
            return
        super().instruction(mu, address, size, data)

    def run(self, case, control):
        """같은 장치 입력에서 실제 번호·현재 배너와 호출열만 결과로 읽는다."""
        self.filename, self.checks = '', 0  # 현재 시도 이름과 실제 존재 검사 횟수다.
        self.number(0x5e1830, case['counter'])
        # 실제 논리 화면의 주소/폭/높이/행 간격은 인자 관측용 명시 입력이다.
        for address, value in ((0x5c78f8, 0x1003000), (0x531860, case['width']),
                               (0x531864, case['height']), (0x5c78ec, case['pitch'])):
            self.number(address, value)
        result = super().run(case, control)
        result['counterout'] = self.number(0x5e1830)
        return result


def cases():
    """모달 선행 소비·정지·수정키·뗌·번호 충돌의 입력만 고유화한다."""
    rows, seen = [], set()

    def add(code, **changes):
        """관측 결과를 참조하지 않고 독립 사건과 장치 조건을 구성한다."""
        row = dict(name='shot-'+str(len(rows)), events=f'{code}:17:23', guideResult=0,
                   gumpResult=0, pauseDepth=0, console=0, banners=1, writeUpdates=1,
                   counter=0, existing=0, tellPauses=1, spec='shots/{GifOutName}',
                   width=1024, height=768, pitch=1024, command=0x020000f4,
                   read=0, filled=0, hide=0, redraw=0, themeRead=0, themeValue=0,
                   fort=0, metamap=0, editor=0)
        row.update(changes)
        key = tuple(str(row[field]) for field in SCHEMA[1:14])
        if key not in seen:
            seen.add(key)
            rows.append(row)

    # 모든 상위 비트와뗌·정지를 교차하고 Ctrl Shift는 호출 인자 경계만 관측한다.
    for flags, release, pause in itertools.product(range(32), (0, 0x80000000), (0, 3)):
        add(0x760000 | flags << 26 | release, pauseDepth=pause)
    # Guide/Gump의 선행 소비와console, Tell이 정지를 만들지 않는 장치를 구별한다.
    for guide, gump, console, pauses in itertools.product((0, 7), (0, 11), (0, 1), (0, 1)):
        add(0x20760000, guideResult=guide, gumpResult=gump, console=console, tellPauses=pauses)
    # 번호는 unsigned 증가 후 signed 서식이며 이미 있는 경로는 건너뛴다.
    for counter, existing, dimensions in itertools.product((0, 9, 0x7fffffff, 0xffffffff), (0, 2),
                                                          ((3, 2, 8), (1280, 720, 1280), (1280, 800, 1280))):
        add(0x20760000, counter=counter, existing=existing, width=dimensions[0], height=dimensions[1], pitch=dimensions[2])
    return rows


def main():
    """제품 연결 전에두 정밀도 관측을새 fixture와 보존 보고서에 고정한다."""
    assert base.base.gate.digest(base.base.gate.PE_PATH) == base.base.gate.PE_SHA
    oracle, observed = Oracle(), []
    # 동일 입력마다 장치/메모리를 다시 준비해두 x87 결과를 독립 대조한다.
    for row in cases():
        first, second = oracle.run(row, 0x27f), oracle.run(row, 0x37f)
        assert first == second, row['name']
        observed.append({**row, **first})
    FIXTURE.write_text('\t'.join(SCHEMA)+'\n'+'\n'.join('\t'.join(str(row[key]) for key in SCHEMA) for row in observed)+'\n', encoding='utf-8', newline='\n')
    report = dict(host=socket.gethostname(), inputs=len(observed), observations=len(observed)*2,
                  precisionDifferences=0, peSha256=base.base.gate.PE_SHA,
                  fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(),
                  nativeBodiesUnchanged=True, externalDevices=['Config', 'sprintf', 'file exists', 'screen lock/unlock', 'GIF encoder', 'log', 'Tell', 'CtrlShift diagnostic calls'])
    (OUTPUT/'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
