#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""게임을 시작하지 않고 일반 큐의 Guide/Gump/뗌과 키·커서 분류만 격리 관측한다."""
import hashlib
import argparse
import itertools
import json
from pathlib import Path
import struct
import sys

ROOT = Path('H:/T5Workspace/NetstormReborn')  # 같은 호스트의 보호 PE와 정밀 내보내기를 읽는다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_oracle import Oracle as BaseOracle, STACK, SCRATCH
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

STACK -= 0x3000  # 원문 부모의 큰 지역 서식을 수용하되 기존 격리 스택 매핑 내부에 둔다.

RANGES = ((0x4d6e61, 0x4d6ed6), (0x4d8c60, 0x4d8c6d), (0x4d96b6, 0x4d9c63),
          (0x4d53f0, 0x4d542e), (0x40e800, 0x40e84d), (0x4d5020, 0x4d5080))  # 일반 명령·개발자 키 부모는 명시적으로 생략한다.
WRITES = {0x54db60, 0x594fc0, 0x558e78, 0x5caf40}  # C/T/Tab/사제의 실제 현재 DWORD만 허용한다.


class Oracle(BaseOracle):
    """분류 기계어와 현재 인자를 실행하고 월드·Gump·큐 효과는 호출 경계에서 기록한다."""
    def __init__(self):
        """보호 PE의 검사 범위를 읽으며 실제 OS/창/게임/통신은 호출하지 않는다."""
        super().__init__('originals', [dict(name='unused', originals='004d62b0')], helpers=[])
        self.allowed = list(RANGES)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def word(self, address, value=None):
        """명시 입력과 관측 DWORD를 같은 원시 비트로 읽고 쓴다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def guard(self, mu, access, address, size, value, data):
        """기계어 쓰기를 지역 스택과 검토된 네 전역으로만 제한한다."""
        assert (STACK - 0x100 <= address and address + size <= STACK + 0x3000) or (address in WRITES and size == 4) or (0x5caeb0 <= address < 0x5caf00 and size == 4), (hex(address), size)
        if 0x5caeb0 <= address < 0x5caf00:
            self.saved_writes += 1

    def returned(self, value=0, purge=0):
        """외부 효과의 반환 규약만 구현하고 호출자 정리는 실제 CPU에 맡긴다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, self.word(sp))

    def on_instruction(self, mu, address, size, data):
        """일반 사건 앞단과 실제 부모 분류에서 외부 호출/첫 세계 분기만 기록한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4d9a8e:
            self.trace.append('Language:' + str(self.word(0x542384)))
        if address == 0x4da477:
            self.gate = self.phase
            mu.emu_stop()
            return
        if address in (0x4d9ff8, 0x4d9d49, 0x4d9c63):
            self.trace.append({0x4d9ff8: 'WorldLeft', 0x4d9d49: 'WorldRight', 0x4d9c63: 'WorldRightMode'}[address])
            mu.emu_stop()
            return
        if address == 0x463e40:
            self.trace.append('Guide')
            self.returned(self.case['guide'], 4)
            return
        if address == 0x466200:
            assert [self.word(sp + offset) for offset in (4, 8, 12)] == [self.case['event'], 37, 0xffffffed]
            self.trace.append('Gump')
            self.returned(self.case['gump'])
            return
        if address == 0x466710:
            self.trace.append('ClosePopups')
            self.returned()
            return
        if address == 0x460de0:
            self.trace.append('Pause:' + str(self.case['paused']))
            self.returned(self.case['paused'])
            return
        if address == 0x445330:
            self.trace.append('Cursor:' + str(self.case['cursor']))
            self.returned(self.case['cursor'])
            return
        if address == 0x452fe0:
            self.trace.append('Queue:' + ':'.join(str(self.word(sp + offset)) for offset in (4, 8, 12)))
            self.returned()
            return
        if address in (0x4da880, 0x4d5c90, 0x4473e0, 0x446a70, 0x433aa0):
            name = {0x4da880: 'Next', 0x4d5c90: 'SelectPriest', 0x4473e0: 'Drop', 0x446a70: 'Rotate', 0x433aa0: 'Center'}[address]
            self.trace.append(name + (':' + str(self.word(sp + 4)) if name == 'Rotate' else ''))
            self.returned()
            return
        if address == 0x494f10:
            self.trace.append('Priest')
            self.returned(1)
            return
        if address == 0x4a89c0:
            self.trace.append('FormatRotation:' + str(self.word(0x54db60)))
            self.returned(SCRATCH)
            return
        if address == 0x441580:
            self.trace.append('Config')
            self.returned()
            return
        if address == 0x4e63e0:
            source = self.read_string(self.word(sp + 4)) + b'\0'
            found = source.find(bytes([self.word(sp + 8) & 255]))
            self.returned(0 if found < 0 else self.word(sp + 4) + found)
            return
        if address == 0x4a9cb0:
            self.trace.append('Sound')
            self.returned()
            return
        if address == 0x497e50:
            self.trace.append('Capture')
            self.returned(SCRATCH, 4)
            return
        return super().on_instruction(mu, address, size, data)

    def run(self, case):
        """원시 사건을 Guide/Gump/뗌→모달→정지/키→커서/마우스 분류 순서로 관측한다."""
        self.case, self.trace, self.gate, self.saved_writes = case, [], '-', 0
        self.mu.mem_write(STACK - 0x100, bytes(0x3100))
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # 이전 사건의 레지스터를 이어받지 않는다.
            self.mu.reg_write(reg, 0)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_FPCW, 0x27f)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        for address, value in ((STACK + 0x14, case['event']), (STACK + 0x18, 37), (STACK + 0x1c, -19), (0x565dd4, case['modal']), (0x54db60, 0), (0x594fc0, 0), (0x558e78, 0), (0x5caf40, 0), (0x5cae90, 0), (0x5caea0, 0), (0x531928, 256)):
            self.word(address, value)
        for address in (0x558ea4, 0x558ea8, 0x5c848c, 0x5c8490, 0x5caeb0, 0x5caeb4):  # 원문 Point의 유효한 명시 입력이다.
            self.mu.mem_write(address, struct.pack('<f', 41.25))
        for slot in range(10):  # 모든 저장 슬롯을 같은 독립 초기상으로 고정하고 관측 쓰기 수와 전체 물리 바이트를 기록한다.
            self.mu.mem_write(0x5caeb0 + slot * 8, struct.pack('<ff', 41.25 + slot, 71.5 + slot))
        self.mu.mem_write(SCRATCH, struct.pack('<ff', 61.25, 71.5))
        # 실제 앞단/모달/정지 뒤 분류를 차례로 실행하며 선행 소비 시 다음 구간을 실행하지 않는다.
        for phase, start, end in (('prefix', 0x4d6e61, 0x4d6ed6), ('modal', 0x4d8c60, 0x4d8c6d), ('body', 0x4d96b6, 0x4da477)):
            self.phase = phase
            self.mu.emu_start(start, end, timeout=1000000, count=8000)
            if phase == 'body' and self.mu.reg_read(UC_X86_REG_EIP) == 0x4da477:
                self.gate = 'body'
            if self.gate != '-' or self.trace[-1].startswith('World'):
                break
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK
        return dict(trace=';'.join(self.trace), gate=self.gate, rotation=self.word(0x54db60), gameTime=self.word(0x594fc0), tabIndex=self.word(0x558e78), slotWrites=self.saved_writes, savedSha256=hashlib.sha256(bytes(self.mu.mem_read(0x5caeb0,80))).hexdigest())


def main():
    """동일 입력을 두 번 실행해 새 준비 fixture와 원시 경계 보고서만 기록한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    output_directory=parser.parse_args().output;output_directory.mkdir(parents=True,exist_ok=True)
    # 새 관측 경로만 허용해 최초fixture와 기존 보고서의 바이트를 덮어쓰지 않는다.
    for name in ('native.json','outside-input-x86.tsv'): assert not (output_directory/name).exists()
    oracle, rows = Oracle(), []
    keys = tuple(map(ord, 'DdQqWwAaSsZzYyXxEeHhNnPpRrCcTtUu')) + (9, 0x730000, 0x740000, 0x750000, 0x250000, 0x270000, 0x10000, 0x20000, ord('1'), 0x20000021)
    for key, outside, release, paused, cursor in itertools.product(keys, (0, 0x4000000), (0, 0x80000000), (0, 1), (0, 1)):  # outside만 제거한 쌍에서 분류와 효과가 같은지를 따로 비교한다.
        case = dict(event=key | outside | release, guide=0, gump=0, modal=0, paused=paused, cursor=cursor)
        result = oracle.run(case)
        assert result == oracle.run(case)
        rows.append(dict(input=case, output=result))
    for key, outside, gate in itertools.product((ord('D'), 0x10000), (0, 0x4000000), ('guide', 'gump', 'modal')):  # 선행 소비가 뒤의 outside 정책과 독립임을 관측한다.
        case = dict(event=key | outside, guide=0, gump=0, modal=0, paused=0, cursor=1)
        case[gate] = 1
        rows.append(dict(input=case, output=oracle.run(case)))
    pairs = {tuple(case['input'][field] for field in ('event', 'guide', 'gump', 'modal', 'paused', 'cursor')): case['output'] for case in rows}
    differences = []
    for key, output in pairs.items():  # 숫자키의 원시 슬롯 계산은 outside를 보존하므로 허용 분류와 슬롯 효과를 구별한다.
        if key[0] & 0x4000000:
            plain = (key[0] & ~0x4000000,) + key[1:]
            if plain in pairs and output != pairs[plain]:
                differences.append(dict(input=key, inside=pairs[plain], outside=output))
    report = dict(unit=761, gameExecutions=0, multiplayerTests=0, executions=len(rows) * 2 - 12, observations=len(rows), ranges=[f'{a:08x}..{b-1:08x}' for a,b in RANGES], codeSha256={f'{a:08x}..{b-1:08x}': hashlib.sha256(bytes(oracle.mu.mem_read(a,b-a))).hexdigest() for a,b in RANGES}, outsidePairDifferences=differences, boundaries=['Guide/Gump/큐/소리/설정/카메라/선택·Drop·회전은 실제 호출 인자만 기록하는 명시 경계다.', '일반 명령 및 개발자 키 사슬은 실행하지 않으며 4d8c60 모달 뒤에서 4d96b6 정지 앞까지 생략한다.', '세계 좌/우 분기는 첫 마커에서 중단하며 실제 선택·이동·세계 효과를 실행하지 않는다.', '물리 OS 입력/게임 전체/MP 검사와 CD1072 관측은 아니다.'], rows=rows)
    (output_directory / 'native.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), 'utf-8')
    (output_directory / 'outside-input-x86.tsv').write_text('# event\tguide\tgump\tmodal\tpaused\tcursor\ttrace\tgate\trotation\tgameTime\ttabIndex\tslotWrites\tsavedSha256\n' + ''.join('\t'.join(str(row['input'][field]) for field in ('event','guide','gump','modal','paused','cursor')) + '\t' + '\t'.join(str(row['output'][field]) for field in ('trace','gate','rotation','gameTime','tabIndex','slotWrites','savedSha256')) + '\n' for row in rows), 'utf-8', newline='\n')
    print(json.dumps({key: report[key] for key in ('unit','observations','executions','gameExecutions','multiplayerTests')}, ensure_ascii=False))
    print('outside 쌍 효과 차이:', len(differences))


if __name__ == '__main__':
    main()
