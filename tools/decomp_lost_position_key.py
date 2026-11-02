#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""U/u의 실제 최근 손실 좌표 키와 Point 유효성·선행 뗌/모달 관문을 동결한다(751)."""
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys
from decomp_oracle import Oracle as BaseOracle, STACK
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT = Path(__file__).resolve().parents[1]  # 같은 PC의 보호 PE와 정밀 내보내기만 읽는다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/lost-position-key-x86.tsv'  # 최초 원문 관측을 제품 결과에 맞춰 덮어쓰지 않는다.
REPORT = ROOT / 'extracted/cpp-resume-751/native.json'  # 큰 원시 관측은 Git 제외 경로에 보존한다.
LOST, CELLS, MODAL = 0x5c848c, 0x531928, 0x565dd4  # 실제 두 float·세계 크기 signed DWORD·Console/모달 전역이다.
RANGES = ((0x4d6eca, 0x4d6ed6), (0x4d8c60, 0x4d9bbb), (0x40e800, 0x40e84d), (0x4d53f0, 0x4d542e))  # 공통 뗌과 키 부모 사이의 일반 명령/Guide/Gump는 제외하며 다른 가상키의 기호 검색만 순수 몸체로 실행한다.
FIELDS = ('event', 'xBits', 'yBits', 'cells', 'paused', 'modal', 'mutatePause', 'pauseX', 'pauseY', 'stopCenter')  # 입력은 원시 DWORD로 직렬화한다.
OUTPUTS = ('outX', 'outY', 'trace', 'completed', 'pointChecks', 'fpsw', 'stackDelta', 'gate')  # Python 기대 계산 대신 기계어 관측만 저장한다.


def digest(path):
    """지정된 파일의 실제 물리 바이트 SHA-256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """입력용 단정도 비트를 만들며 관측 결과를 예측하지 않는다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


class PartialStop(Exception):
    """실제 Center 경계에서 의도한 중단을 정상 복귀와 구별한다."""


class Oracle(BaseOracle):
    """U/u·정지·Point 기계어를 실행하고 현재 정지/Center/strchr만 명시 경계로 둔다."""
    def __init__(self):
        """보호 PE의 검토된 코드 범위와 스택만 쓰기 허용한다."""
        super().__init__('originals', [dict(name='unused', originals='004d62b0')], helpers=[])
        self.allowed = list(RANGES)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def dword(self, address, value=None):
        """명시 입력/외부 경계 변경에만 네 바이트를 기록한다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def guard(self, mu, access, address, size, value, data):
        """기계어의 쓰기는 실제 지역 스택으로만 제한한다."""
        assert STACK - 0x100 <= address and address + size <= STACK + 0x1000, (hex(address), size)

    def returned(self, value=0):
        """외부 cdecl 경계의 반환만 구현하고 인자 정리는 원문에 맡긴다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4)
        self.mu.reg_write(UC_X86_REG_EIP, self.dword(sp))

    def on_instruction(self, mu, address, size, data):
        """공통 종료와 외부 효과만 포착하며 U/u 선택·유효성은 실제 CPU가 판정한다."""
        if address == 0x4da477:
            self.gate = 'release' if self.stage == 'release' else 'modal'
            mu.emu_stop()
            return
        if address == 0x460de0:
            self.trace.append('P:' + str(self.case['paused']))
            if self.case['mutatePause']:
                self.dword(LOST, self.case['pauseX'])
                self.dword(LOST + 4, self.case['pauseY'])
            self.returned(self.case['paused'])
            return
        if address == 0x40e800:
            assert mu.reg_read(UC_X86_REG_ECX) == LOST
            self.point_checks += 1
        if address == 0x433aa0:
            sp = mu.reg_read(UC_X86_REG_ESP)
            self.trace.append(f'C:{self.dword(sp + 4)}:{self.dword(sp + 8)}')
            if self.case['stopCenter']:
                raise PartialStop()
            self.returned()
            return
        if address == 0x4e63e0:
            sp = mu.reg_read(UC_X86_REG_ESP)
            pointer, character = self.dword(sp + 4), self.dword(sp + 8) & 255
            source = self.read_string(pointer) + b'\0'
            found = source.find(bytes([character]))
            self.trace.append('F')
            self.returned(0 if found < 0 else pointer + found)
            return
        return super().on_instruction(mu, address, size, data)

    def run(self, case, control):
        """현재 사건·두 좌표와 공유 세계 크기에서 선행 뗌/모달·정지·U/u를 관측한다."""
        self.case, self.trace, self.point_checks, self.gate = case, [], 0, 'key'
        self.dword(LOST, case['xBits'])
        self.dword(LOST + 4, case['yBits'])
        self.dword(CELLS, case['cells'])
        self.dword(MODAL, case['modal'])
        self.mu.mem_write(STACK - 0x100, bytes(0x1100))
        self.dword(STACK + 0x14, case['event'])
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # 이전 반복 레지스터가 현재 사건 분기에 영향을 주지 않게 한다.
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.stage = 'release'
        self.mu.emu_start(0x4d6eca, 0x4d6ed6, timeout=1000000, count=4000)
        completed = True
        if self.gate != 'release':
            assert self.mu.reg_read(UC_X86_REG_EIP) == 0x4d6ed6
            self.stage = 'key'
            try:
                self.mu.emu_start(0x4d8c60, 0x4d9bbb, timeout=1000000, count=4000)
            except PartialStop:
                completed = False
            if completed and self.gate != 'modal':
                assert self.mu.reg_read(UC_X86_REG_EIP) == 0x4d9bbb
        assert self.mu.reg_read(UC_X86_REG_FPCW) == control
        if completed:
            assert self.mu.reg_read(UC_X86_REG_ESP) == STACK
        return dict(outX=self.dword(LOST), outY=self.dword(LOST + 4), trace=';'.join(self.trace) or '-', completed=int(completed), pointChecks=self.point_checks, fpsw=self.mu.reg_read(UC_X86_REG_FPSW), stackDelta=self.mu.reg_read(UC_X86_REG_ESP) - STACK, gate=self.gate)


def inputs():
    """수정키·뗌·outside·정지/모달·raw float 경계를 만들되 기대값은 계산하지 않는다."""
    def case(event=ord('U'), x=bits(41.25), y=bits(73.5), cells=256, paused=0, modal=0, mutate=0, pause_x=bits(111.25), pause_y=bits(121.5), stop=0):
        """한 입력의 원시 비트만 구성하는 지역 생성기다."""
        return dict(zip(FIELDS, (event, x, y, cells, paused, modal, mutate, pause_x, pause_y, stop)))
    for key, modifiers, paused, modal in itertools.product((ord('U'), ord('u')), range(64), (0, 1), (0, 1)):  # Shift/Ctrl/Alt/PressedSince/Outside/Release의 모든 조합이다.
        flags = sum(bit for index, bit in enumerate((0x20000000, 0x10000000, 0x08000000, 0x40000000, 0x04000000, 0x80000000)) if modifiers & (1 << index))
        yield case(event=key | flags, paused=paused, modal=modal)
    edges = ((0, bits(1)), (0x80000000, bits(1)), (1, 1), (bits(1), 0), (bits(1), 0x80000000), (bits(-10000), bits(2)), (bits(1), bits(-10000)), (bits(1), bits(1)), (0x437fffff, bits(1)), (bits(256), bits(1)), (0x43800001, bits(1)), (bits(1), 0x437fffff), (bits(1), bits(256)), (0x7f800000, bits(1)), (0xff800000, bits(1)), (bits(1), 0x7f800000), (0x7fc12345, bits(1)), (bits(1), 0x7fc12345), (0x7f812345, bits(1)), (bits(1), 0x7f812345), (bits(1), 0xff812345), (0x4f000000, bits(1)), (bits(1), 0x4f000000))
    for key, point, cells in itertools.product((ord('U'), ord('u')), edges, (0, 1, 2, 255, 256, 0x7fffffbf, 0x7fffffff, 0xffffffff, 0x80000000)):  # signed 세계 크기의 float 반올림과 양 축 단락을 실제 x87로 관측한다.
        yield case(event=key, x=point[0], y=point[1], cells=cells)
    for key, paused, point, stop in itertools.product((ord('U'), ord('u')), (0, 1), ((bits(111.25), bits(121.5)), (bits(-10000), bits(13))), (0, 1)):  # 정지 판정 이후 같은 현재 좌표 재조회와 센터의 부분 중단을 구별한다.
        yield case(event=key, paused=paused, mutate=1, pause_x=point[0], pause_y=point[1], stop=stop)
    for key, flags, paused, modal in itertools.product((ord('V'), ord('v'), 0x550000, 0x02000055, 0xffff), (0, 0x20000000, 0x04000000, 0x80000000), (0, 1), (0, 1)):  # 다른 문자/가상키/프로그램 명령은 U의 하위BYTE와 혼동하지 않는다.
        yield case(event=key | flags, paused=paused, modal=modal)


def observe():
    """두 x87 정밀도와 같은 입력 재실행을 대조하여 실제 관측 TSV/원시를 구성한다."""
    oracle, lines, observations = Oracle(), [], []
    for case in inputs():  # 원문 결과를 독립 두 정밀도와 재반복에서 얻으며 구현 모델은 쓰지 않는다.
        results = [oracle.run(case, control) for control in (0x27f, 0x37f, 0x27f)]
        assert results[0] == results[1] == results[2], case
        result = results[0]
        lines.append('\t'.join(str(case[field]) for field in FIELDS) + '\t' + '\t'.join(str(result[field]) for field in OUTPUTS) + '\n')
        observations.append(dict(input=case, observations=results))
        if len(observations) % 256 == 0:
            print('원문 관측 ' + str(len(observations)) + '행', flush=True)
    return '# ' + ' '.join(FIELDS + OUTPUTS) + '\n' + ''.join(lines), observations


def main():
    """최초 fixture를 한 번 만들고 재실행은 기존 물리 바이트/원문 코드를 검증한다."""
    fixture, observations = observe()
    native = Oracle()
    code = {f'{start:08x}..{end - 1:08x}': hashlib.sha256(bytes(native.mu.mem_read(start, end - start))).hexdigest() for start, end in RANGES}  # 실제 실행 허용 범위의 코드 바이트를 별도 식별한다.
    if '--verify' in sys.argv:
        record = json.loads(REPORT.read_text('utf-8'))
        assert FIXTURE.read_bytes() == fixture.encode('utf-8')
        assert code == record['codeSha256']
        for name, expected in record['sha256'].items():  # 원문/도구/첫 fixture가 바뀌지 않았는지 확인한다.
            assert digest(ROOT / name) == expected, name
        print(json.dumps(dict(inputs=len(observations), repeatedFixtureMatches=True, codeShaMatches=True, gameExecutions=0, multiplayerTests=0), ensure_ascii=False))
        return
    assert not FIXTURE.exists() and not REPORT.exists()
    FIXTURE.write_text(fixture, 'utf-8', newline='\n')
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/raw.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_oracle.py', 'tools/decomp_lost_position_key.py', FIXTURE.relative_to(ROOT).as_posix())
    record = dict(unit=751, date='2026-11-02', host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'), inputs=len(observations), nativeExecutions=len(observations) * 3, precisionOrRepeatDifferences=0, gameExecutions=0, multiplayerTests=0, initialFailure='첫 실행은 다른 가상키의 Shift 기호 검색004d53f0을 허용 범위 밖으로 거부했다. 실제 순수 검색 몸체004d53f0..004d542d를 정적으로 확인해 추가했고 최초 fixture 생성 전 재실행했다.', ranges=[f'{start:08x}..{end - 1:08x}' for start, end in RANGES], boundaries=['뗌 gate와 Console/모달→정지→일반 키 부모 사이의 일반 명령/Guide/Gump는 실행하지 않는다.', '정지00460de0·즉시중심00433aa0·다른키의strchr만 명시 외부 장치다. 실제433aa0 내부·GUI·OS·게임전체 관측은 아니다.', '큐 사건 outside 비트는 원문 뗌 gate와 키mask에서 실제 관측하며 일반 폴링/OS 비활성 정책과 구별한다.', 'CD1072는 이번 기계어 실행 대상이 아니며 정적 대응만 별도 문서화한다.'], codeSha256=code, zeroComparisonBits=native.dword(0x500fc4), sha256={name: digest(ROOT / name) for name in paths}, observations=observations)
    REPORT.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps({key: record[key] for key in ('unit', 'inputs', 'nativeExecutions', 'precisionOrRepeatDifferences', 'gameExecutions', 'multiplayerTests', 'zeroComparisonBits')}, ensure_ascii=False))


if __name__ == '__main__':
    main()
