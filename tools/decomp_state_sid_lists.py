#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제품 변경 전에 PATCH의 실제 20포인터 등록·해제·개수 정리를 격리 CPU로 관측한다."""
import argparse
import json
import socket
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 원본 세 몸체의 실제 끝 RET까지이며 20개 표/명시 가상 헤더는 서로 겹치지 않는다.
SPECS = {'reset': (0x472830, 0x472873), 'register': (0x472880, 0x4728a0), 'unregister': (0x4728a0, 0x4728c5)}
TABLE, HEADERS = 0x592938, POOL + 256
FIXTURE = ROOT / 'cpppj/tests/fixtures/state-sid-lists-x86.tsv'  # 최초 독립 관측을 제품 출력으로 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/state-sid-lists-native-20261026.json'  # CD에 같은 몸체를 가정하지 않는다.


class Oracle(MemoryOracle):
    """실제 세 몸체만 실행하며 호출/효과 치환 없이 현재 포인터의 순차 쓰기를 기록한다."""
    def __init__(self):
        """보호 원본의 메모리 사본에만 입력하고 CPU의 실제 store를 감사한다."""
        super().__init__()
        self.running = False
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def instruction(self, mu, address, size, data):
        """현재 몸체 이외의 게임·OS 진입과 무한 실행을 거부한다."""
        self.instructions += 1
        low, high = SPECS[self.kind]
        if not low <= address < high or self.instructions > 250:
            raise RuntimeError('등록 목록 몸체 이탈: ' + hex(address))

    def write(self, mu, access, address, size, value, data):
        """스택 이외의 실제 모든 DWORD 쓰기를 주소·폭·값 순서로 보존한다."""
        if self.running and not STACK - 256 <= address < STACK + 64:
            if size != 4 or not (TABLE <= address < TABLE + 80 or HEADERS <= address < HEADERS + 640):
                raise RuntimeError('등록 목록의 예상 밖 store')
            self.stores.append(f'{address}:{size}:{value}')

    def observe(self, kind, case, control):
        """빈/전체/구멍/중복/마지막/자기수정 입력과 이웃 canary를 실제 명령으로 실행한다."""
        self.kind = kind
        pattern = bytes((case * 31 + index * 17) & 255 for index in range(672))
        self.mu.mem_write(HEADERS - 16, pattern)
        self.mu.mem_write(TABLE - 16, bytes([0xa5]) * 112)
        slots = [HEADERS + index * 32 for index in range(20)]
        if case == 0:
            slots = [0] * 20
        elif case == 1:
            slots = [slots[0]] * 20
        elif case == 2:
            slots = [slot if index % 2 else 0 for index, slot in enumerate(slots)]
        elif case == 3:
            slots[19] = 0
        elif case == 4:
            slots[0] = 0
        elif case == 5:
            slots[7] = 0
        elif case == 6:
            slots[0] = TABLE + 4 - 12
        elif case == 7:
            slots[0] = TABLE + 19 * 4 - 12
        elif case == 8:
            slots = [0] * 19 + [slots[19]]
        elif case == 9:
            slots[3] = slots[8] = slots[19] = slots[0]
        for index, slot in enumerate(slots):  # 각 슬롯의 명시 DWORD를 원본 표에 제공한다.
            self.put(TABLE + index * 4, slot)
        argument = 0 if case in (2, 8) else slots[0] if case in (1, 9) else HEADERS + 32 * 19
        table_before = bytes(self.mu.mem_read(TABLE - 16, 112))
        headers_before = bytes(self.mu.mem_read(HEADERS - 16, 672))
        self.mu.mem_write(STACK - 256, b'\xcd' * 320)
        self.put(STACK, STOP)
        self.put(STACK + 4, argument)
        saved = ((UC_X86_REG_EBX, 0x456789ab), (UC_X86_REG_ESI, 0x56789abc), (UC_X86_REG_EDI, 0x6789abcd), (UC_X86_REG_EBP, 0x789abcde))
        for register, value in ((UC_X86_REG_EAX, 0x89abcdef), (UC_X86_REG_ECX, 0x12345678), (UC_X86_REG_EDX, 0x3456789a), *saved):  # 비영 호출 환경이다.
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.mu.reg_write(UC_X86_REG_FPSW, 0)
        self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff)
        self.stores = []
        self.instructions = 0
        self.running = True
        try:
            self.mu.emu_start(SPECS[kind][0], STOP)
        finally:
            self.running = False
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('등록 목록 호출 복귀 차이')
        for register, value in saved:  # 실제 비휘발 레지스터를 대조한다.
            if self.mu.reg_read(register) != value:
                raise RuntimeError('등록 목록 비휘발 보존 차이')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) or self.mu.reg_read(UC_X86_REG_FPTAG) != 0xffff:
            raise RuntimeError('등록 목록 FPU 보존 차이')
        return [kind, case, control, argument, table_before.hex(), headers_before.hex(), bytes(self.mu.mem_read(TABLE - 16, 112)).hex(), bytes(self.mu.mem_read(HEADERS - 16, 672)).hex(), ';'.join(self.stores) or '-', self.instructions]


def main():
    """새 독립 관측과 저장된 SHA 검증을 구분하고 원본 게임/창을 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, wanted in report['sha256'].items():  # 최초 도구·원문·관측은 모두 바이트로 대조한다.
            if digest(ROOT / name) != wanted:
                raise RuntimeError('등록 목록 근거 SHA 차이: ' + name)
        print(json.dumps({'verified': True, 'observations': report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 등록 목록 관측 덮어쓰기 금지')
    oracle = Oracle()
    rows = [oracle.observe(kind, case, control) for kind in SPECS for case in range(11) for control in (0x27f, 0x37f)]
    FIXTURE.write_bytes(('# kind / case / control / argument / tableInput / headersInput / tableOutput / headersOutput / stores / instructions\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows)).encode('utf-8'))
    names = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_state_sid_lists.py', FIXTURE.relative_to(ROOT).as_posix()]
    report = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'edition': 'PATCH 10.78', 'observations': len(rows), 'effectsStubbed': False, 'originalGameExecuted': False, 'functions': {kind: {'entry': hex(low), 'bytes': high-low, 'bodyHex': bytes(oracle.mu.mem_read(low, high-low)).hex()} for kind, (low, high) in SPECS.items()}, 'sha256': {name: digest(ROOT / name) for name in names}, 'cdNote': 'CD 10.72의 등록 목록 몸체를 실행하지 않았으므로 동등성 근거에 포함하지 않는다.'}
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print(json.dumps({'passed': True, 'observations': len(rows), 'host': report['host']}))


if __name__ == '__main__':  # CLI 명시 실행에서만 격리 CPU 관측을 수행한다.
    main()
