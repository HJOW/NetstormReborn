#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제품 변경 전에 PATCH0043dad0과 실제 serial 조회를 격리 실행한다."""
import argparse
import csv
import itertools
import json
import socket
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

# 실제 부모/조회와 명시된 두 가상 장치의 격리 주소다. 게임/OS 함수는 실행하지 않는다.
ENTRY, LOOKUP, TABLE, SERIAL = 0x43dad0, 0x464020, 0x563e38, 0x557d74
NODES, VTABLE, PUSH, ATTACH = POOL + 256, TEXT + 256, STOP + 256, STOP + 512
FIXTURE = ROOT / 'cpppj/tests/fixtures/budget-refresh-x86.tsv'  # 최초 CPU 관측이며 제품 출력으로 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/budget-refresh-native-20261026.json'  # CD는 이번 실제 실행 범위에 포함하지 않는다.


class Oracle(MemoryOracle):
    """정상/미존재/죽음/중복 serial과 가상 호출의 현재 필드 재조회·중단을 관측한다."""
    def __init__(self):
        """읽은 PE의 사본에 실제 조회표와 가상 장치 주소를 둔다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = [(entry, entry + int(facts[entry]['size'])) for entry in (ENTRY, LOOKUP)]

    def instruction(self, mu, address, size, data):
        """부모와 조회는 실제 CPU, 선택된 Push/Attach만 인자를 보존하는 외부 장치로 둔다."""
        self.instructions += 1
        if self.instructions > 16000:
            raise RuntimeError('예산 조회 명령 한도')
        if address in (PUSH, ATTACH):
            target = mu.reg_read(UC_X86_REG_ECX)
            args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 4) if address == ATTACH else ()
            self.calls.append(('P' if address == PUSH else 'A') + ':' + ':'.join(map(str, (target, *args))))
            if self.change:
                self.put(target + 0x30, 0x89abcdef if address == PUSH else 0xfedcba98)
                if address == PUSH:
                    self.put(target + 0x4c, 0xc2300000)
                    self.put(target + 0x50, 0x42b00000)
                    self.put(SERIAL, 0xfedcba98)
            if self.stop == ('push' if address == PUSH else 'attach'):
                self.stopped = True
                mu.emu_stop()
                return
            self.ret(purge=16 if address == ATTACH else 0)
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError('예산 몸체 이탈: ' + hex(address))

    def observe(self, case, change, stop, control):
        """명시 raw 입력을 준비하고 전체 대상 BYTE·호출 순서·정상복귀를 읽는다."""
        before = bytearray((case * 31 + index * 17) & 255 for index in range(3 * 192))
        self.mu.mem_write(NODES, bytes(before))
        self.mu.mem_write(TABLE, b'\0' * 4000)
        self.mu.mem_write(VTABLE, b'\0' * 64)
        self.put(VTABLE + 0x18, PUSH)
        self.put(VTABLE + 0x20, ATTACH)
        self.put(SERIAL, 71)
        for index in range(3):  # 같은 vtable과 서로 다른 serial/깊이/좌표를 가진 실제 크기 이상의 노드다.
            base = NODES + index * 192
            self.put(base, VTABLE)
            self.put(base + 4, 0)
            self.put(base + 0x2c, 71 if index != 2 else 72)
            self.put(base + 0x30, 0x12345678 + index)
            self.put(base + 0x4c, 0x3fc00000 + index)
            self.put(base + 0x50, 0x80000000 if index == 0 else 0x40400000)
        if case == 1:
            self.put(TABLE, NODES)
        elif case == 2:
            self.put(TABLE + 4, NODES)
        elif case == 3:
            self.put(TABLE + 999 * 4, NODES)
        elif case == 4:
            self.put(TABLE + 4, NODES)
            self.put(NODES + 4, 0x10000)
        elif case == 5:
            self.put(TABLE + 4, NODES)
            self.put(TABLE + 8, NODES + 192)
            self.put(NODES + 4, 0x10000)
        elif case == 6:
            self.put(TABLE + 4, NODES + 192)
            self.put(TABLE + 8, NODES)
        elif case == 7:
            self.put(SERIAL, 0)
            self.put(TABLE + 4, NODES)
            self.put(NODES + 0x2c, 0)
        elif case == 8:
            self.put(TABLE + 4, NODES + 384)
        elif case == 9:
            self.put(TABLE + 4, NODES)
            self.put(NODES + 4, 0x80000000)
        before = bytes(self.mu.mem_read(NODES, 576))
        registry = bytes(self.mu.mem_read(TABLE, 4000))
        initial_serial = self.words(SERIAL, 1)[0]
        self.put(STACK, STOP)
        saved = ((UC_X86_REG_EBX, 0x456789ab), (UC_X86_REG_ESI, 0x56789abc), (UC_X86_REG_EDI, 0x6789abcd), (UC_X86_REG_EBP, 0x789abcde))
        for register, value in saved:  # 정상 복귀에서 비휘발 레지스터도 확인한다.
            self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, control)
        self.change, self.stop, self.stopped, self.calls, self.instructions = change, stop, False, [], 0
        self.mu.emu_start(ENTRY, STOP)
        if not self.stopped:
            if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
                raise RuntimeError('예산 호출 정상 복귀 차이')
            for register, value in saved:  # 장치가 보존하는 비휘발 레지스터를 부모도 유지해야 한다.
                if self.mu.reg_read(register) != value:
                    raise RuntimeError('예산 비휘발 보존 차이')
        if bytes(self.mu.mem_read(TABLE, 4000)) != registry or self.mu.reg_read(UC_X86_REG_FPCW) != control:
            raise RuntimeError('예산 조회표/FPU 변경')
        return [case, change, stop, control, initial_serial, before.hex(), registry.hex(), bytes(self.mu.mem_read(NODES, 576)).hex(), self.words(SERIAL, 1)[0], ';'.join(self.calls) or '-', int(self.stopped), self.instructions]


def main():
    """최초 관측과 근거 검증을 구별하며 원본 파일에는 쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, wanted in report['sha256'].items():  # 최초 코드·원문·관측의 실제 바이트를 검사한다.
            if digest(ROOT / name) != wanted:
                raise RuntimeError('예산 근거 SHA 차이: ' + name)
        print(json.dumps({'verified': True, 'observations': report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 예산 관측 덮어쓰기 금지')
    oracle = Oracle()
    rows = [oracle.observe(*values) for values in itertools.product(range(10), (0, 1), ('none', 'push', 'attach'), (0x27f, 0x37f))]
    FIXTURE.write_bytes(('# case / change / stop / control / serialInput / nodesInput / registryInput / nodesOutput / serialOutput / calls / stopped / instructions\n' + ''.join('\t'.join(map(str, row)) + '\n' for row in rows)).encode('utf-8'))
    names = ['originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_budget_refresh.py', FIXTURE.relative_to(ROOT).as_posix()]
    report = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'edition': 'PATCH10.78', 'observations': len(rows), 'realFunctions': [hex(ENTRY), hex(LOOKUP)], 'effectsStubbed': ['virtual+18 Push', 'virtual+20 Attach'], 'originalGameExecuted': False, 'sha256': {name: digest(ROOT / name) for name in names}, 'scope': '실제 serial 조회·현재 필드·정상 깊이 복구·중단 prefix만 관측한다. 개별 가상 Push/Attach 몸체 동등성은 기존 GumpTree Native 근거를 재사용한다.'}
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))
    print(json.dumps({'passed': True, 'observations': len(rows), 'host': report['host']}))


if __name__ == '__main__':  # 명시 CLI에서만 실제 PE 사본의 CPU 명령을 실행한다.
    main()
