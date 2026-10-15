#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 빈 부모 통지와 PATCH 피해 통지 래퍼의 분기/AI 호출 인자를 격리 관찰한다."""
import csv
import json
import random
import socket
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, PLAYERS, POOL, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

FIXTURE = ROOT / 'cpppj/tests/fixtures/player-damage-notice-x86.tsv'  # 실제 래퍼가 요청한 AI 인자의 기록이다.
REPORT = ROOT / 'cpppj/campaign-notices.json'  # 관찰 범위와 코드·입력 해시를 보관한다.
PRESERVED = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789,
             UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}  # 호출 규약의 보존 레지스터 입력이다.


class NoticeOracle(OwnerOracle):
    """실제 통지 몸체를 실행하고 미복원 AI 몸체 요청만 외부 경계에서 관찰한다."""
    def __init__(self, edition):
        """현재 호스트에서 내보낸 작은 통지 범위만 실행하도록 초기화한다."""
        super().__init__(edition)
        self.paths = [ROOT / f'extracted/campaign-notices/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed = []  # 다른 단위의 몸체는 실행 허용에 포함하지 않는다.
        self.entries = set()  # 이번 정밀 내보내기의 진입점만 센다.
        self.ai_calls = []  # 실제 ECX와 두 스택 인자를 순서대로 관찰한다.
        with self.paths[1].open(encoding='utf-8') as stream:
            # 실제 불연속 함수 범위를 그대로 사용한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 범위 중 빈틈의 코드는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))

    def on_instruction(self, mu, address, size, data):
        """AI 게임 규칙은 실행하지 않고 래퍼의 실제 __thiscall 요청만 경계로 기록한다."""
        if self.edition == 'originals' and address == 0x415460:
            sp = mu.reg_read(UC_X86_REG_ESP)
            ret, victim, attacker = struct.unpack('<III', mu.mem_read(sp, 12))
            self.ai_calls.append((mu.reg_read(UC_X86_REG_ECX), victim, attacker))
            mu.reg_write(UC_X86_REG_ESP, sp + 12)
            mu.reg_write(UC_X86_REG_EIP, ret)
            return
        super().on_instruction(mu, address, size, data)

    def invoke(self, entry, arguments):
        """명시 인자를 스택에 두고 정상 반환·보존 레지스터를 확인한다."""
        self.ai_calls = []
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.reg_write(UC_X86_REG_EAX, 0xdeadbeef)
        self.mu.reg_write(UC_X86_REG_EDX, 0xbaadf00d)
        # 비영 패턴으로 callee-saved 레지스터의 보존을 관찰한다.
        for register, value in PRESERVED.items():
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ECX, POOL + 50 * self.stride)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack(f'<{len(arguments)+1}I', STOP, *(arg & 0xffffffff for arg in arguments)))
        self.mu.emu_start(entry, STOP, count=2000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP
        # 현재 내보낸 기본 빈 몸체는 ret8, 피해 래퍼는 cdecl ret0이다.
        expected = STACK + (4 if entry == 0x416760 else 12)
        assert self.mu.reg_read(UC_X86_REG_ESP) == expected
        # 호출 규약이 보존하는 모든 범용 레지스터를 비교한다.
        for register, value in PRESERVED.items():
            assert self.mu.reg_read(register) == value
        return list(self.ai_calls)

    def damage(self, owner, challenge, token, victim, attacker, index):
        """같은 현재 Player9의 실제 +b0를 입력하고 래퍼 결과와 전체 불변 바이트를 기록한다."""
        rng = random.Random(0x416760 + index)
        players = bytearray(rng.randbytes(9 * 0xb4))
        struct.pack_into('<I', players, max(owner, 0) * 0xb4 + 0xb0, token)
        self.mu.mem_write(PLAYERS, bytes(players))
        self.mu.mem_write(self.g['challenge'], struct.pack('<I', challenge))
        calls = self.invoke(0x416760, (owner, victim, attacker))
        assert bytes(self.mu.mem_read(PLAYERS, len(players))) == players
        assert len(calls) <= 1
        observed = calls[0] if calls else (0, 0, 0)
        return (owner, challenge, token, victim, attacker, len(calls), *observed)

    def empty(self, form, reason):
        """두 판본의 실제 기본 빈 통지에서 raw 슬롯 불변과 ret8을 확인한다."""
        raw = bytes((index * 17 + 3) & 255 for index in range(self.stride))
        self.mu.mem_write(POOL + 50 * self.stride, raw)
        self.invoke(0x41c100 if self.edition == 'originals' else 0x401c20, (form, reason))
        assert bytes(self.mu.mem_read(POOL + 50 * self.stride, self.stride)) == raw


def main():
    """고유 래퍼 입력144개와 판본별 빈 몸체6개를 실행하고 새 관찰만 저장한다."""
    patch = NoticeOracle('originals')
    cd = NoticeOracle('originalCD')
    rows = []
    index = 0
    # owner0/음수, 로컬/상대8명, 모드와 실제 NULL/비영 AI를 독립 입력으로 조합한다.
    for owner in (-2147483648, -1, 0, *range(1, 9), -2):
        # challenge 모드에서는 Player 번호/AI를 읽지 않는 순서도 관찰한다.
        for challenge in (0, 1):
            # 비영 opaque 주소는 실제 AI 몸체를 호출했다는 관찰에만 사용한다.
            for token in (0, 0x1234abcd):
                # 두 SID의0/서버/최대WORD 조합을 고유 입력으로 만든다.
                for victim, attacker in ((0, 0), (20000, 20001), (65535, 7)):
                    rows.append(patch.damage(owner, challenge, token, victim, attacker, index))
                    index += 1
    # 원본 빈 몸체가 인자를 읽거나 부모를 변경하지 않는 경로를 판본별로 관찰한다.
    for oracle in (patch, cd):
        # 내용물0/서버/최대WORD의 raw 인자를 그대로 전달한다.
        for form in (0, 20000, 65535):
            # 부착1/제거2의 두 정상 사건을 관찰한다.
            for reason in (1, 2):
                oracle.empty(form, reason)
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(('# owner', 'challenge', 'ai', 'victim', 'attacker', 'calls', 'seenAi', 'seenVictim', 'seenAttacker'))
        writer.writerows(rows)
    report = {'host': socket.gethostname(), 'date': '2026-10-15', 'damage_wrapper_inputs': len(rows),
              'empty_notice_inputs': 12, 'ai_body_executed': False,
              'ai_boundary_calls': sum(row[5] for row in rows),
              'assertions': patch.assertions + cd.assertions,
              'tool_sha256': digest(Path(__file__)), 'helper_sha256': digest(ROOT / 'tools/decomp_owner_oracle.py'),
              'fixture_sha256': digest(FIXTURE), 'editions': {}}
    # 실제 실행 범위와 보호 PE를 함께 기록한다.
    for oracle in (patch, cd):
        report['editions'][oracle.edition] = {'binary_sha256': oracle.sha256,
            'exports': {str(path.relative_to(ROOT)): digest(path) for path in oracle.paths},
            'native_entries': dict(oracle.native_calls)}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
