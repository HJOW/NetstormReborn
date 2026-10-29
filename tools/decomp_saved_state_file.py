#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 저장파일 이름과 unlink/오류 매핑을 격리 CPU로 관측한다. 게임/OS는 실행하지 않는다."""
import argparse
import csv
import json
import socket
import struct
from decomp_priestmenu import Oracle as Base, ROOT, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

TLS = TEXT + 0x8000  # 원문 CRT가 쓰는 현재 스레드의 errno/doserrno 명시 물리다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/saved-state-file-x86.tsv'  # 최초 독립 오류 기대값이며 기존 fixture를 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/saved-state-file-native-20261029.json'  # 실행 범위/경계/해시와 이름 관측을 기록한다.
REGISTERS = (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)  # 호출자가 보존해야 하는 네 레지스터다.


class Oracle(Base):
    """파일명 부모와 실제 CRT unlink/45항목 매핑만 실행하고 외부 OS/TLS/formatter를 관찰한다."""
    def __init__(self):
        """PE를 격리 복사하고 파일 OS 두 경계만 명시 가상 주소로 연결한다."""
        super().__init__()
        facts = {int(row['entry'], 16): row for row in csv.DictReader((ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8'), delimiter='\t')}
        self.allowed = []  # 실제 함수표가 허용한 불연속 명령 범위다.
        for entry in (0x4b5470, 0x4e58ba, 0x4e58e4, 0x4ed5ee):  # 원문 본체 범위 밖 실행은 전부 거부한다.
            for part in facts[entry]['ranges'].split(';'):  # 불연속 범위를 임의 연속 코드로 넓히지 않는다.
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))
        self.put(0x5000b0, STOP + 0x100)
        self.put(0x5000a0, STOP + 0x104)
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """원문 스택/명시 TLS와 formatter 출력 외 쓰기를 거부한다."""
        allowed = ((STACK - 0x2000, STACK + 0x100), (TLS, TLS + 0x100), (TEXT, TEXT + 0x1000))
        if not any(low <= address and address + size <= high for low, high in allowed):
            raise RuntimeError(f'저장파일 CPU 쓰기 범위 밖 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """원문 분기/오류 표는 CPU로 실행하며 이름 서식/경로/OS/TLS만 명시 장치로 둔다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 8)
        if address == STOP:
            mu.emu_stop()
            return
        if address == 0x4a89c0:
            target, pattern, name = args[:3]
            observed = {'format': self.string(pattern).decode(), 'name': self.string(name).decode(), 'zero': args[3]}
            self.events.append(observed)
            mu.mem_write(target, observed['format'].replace('%s', observed['name']).encode() + b'\0')
            self.ret(target)
        elif address == 0x459c60:
            expression = self.words(esp + 4, 7)
            self.events.append({'tokens': [expression[index] for index in (0, 2, 4, 6)],
                                'root': self.string(expression[1]).decode(), 'data': self.string(expression[3]).decode(),
                                'file': self.string(expression[5]).decode()})
            self.ret(TEXT)
        elif address == STOP + 0x100:
            self.events.append('delete:' + self.string(args[0]).hex())
            self.ret(self.success, 4)
        elif address == STOP + 0x104:
            self.events.append('lastError')
            self.ret(self.error)
        elif address == 0x4ea61d:
            self.events.append('tls')
            self.ret(TLS)
        elif not any(low <= address < high for low, high in self.allowed):
            raise RuntimeError(f'저장파일 CPU 실행 범위 밖 {address:08x}')
        self.instructions += 1

    def sample(self, entry, success=0, error=0):
        """서로 다른 스택/보존 레지스터와 고정 기존 errno를 매번 새 입력으로 공급한다."""
        self.events = []  # 이번 단일 반환에서 발생한 순서/인자만 보존한다.
        self.success, self.error = success, error  # 명시 DeleteFile BOOL과 GetLastError 외부 입력이다.
        self.mu.mem_write(TEXT, b'd\\!state.dat\0')
        self.mu.mem_write(TLS, bytes(0x100))
        self.put(TLS + 8, 0x3456)
        self.put(TLS + 12, 0x11223344)
        for index, register in enumerate(REGISTERS):  # 0으로 보존 오류를 가리지 않는다.
            self.mu.reg_write(register, 0x10203040 + index * 0x11111111)
        saved = [self.mu.reg_read(register) for register in REGISTERS]
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP)
        self.put(STACK + 4, TEXT)
        self.mu.emu_start(entry, STOP, count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert saved == [self.mu.reg_read(register) for register in REGISTERS]
        return self.mu.reg_read(UC_X86_REG_EAX), *self.words(TLS + 8, 2), self.events


def main():
    """최초 기대값/보고서를 보존하며 --verify는 같은 CPU 관측을 다시 대조한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()):
        raise RuntimeError('저장파일 최초 관측 덮어쓰기 거부')
    oracle = Oracle()
    _, _, _, path = oracle.sample(0x4b5470)
    cases = [(1, error) for error in (0, 2, 5, 32)] + [(0, error) for error in (*range(257), 9001, 0xffffffff)]  # 성공의 과거 오류 보존과 실패의 표/범위/기본 매핑을 분리한다.
    rows = []
    for success, error in cases:  # 실제 thunk→unlink→dosmaperr의 반환/현재 TLS만 관측한다.
        result, errno, doserrno, events = oracle.sample(0x4e58e4, success, error)
        rows.append('\t'.join(map(str, (success, error, result, errno, doserrno))) + '\t' + ';'.join(events))
    content = '# success error result errno doserrno trace\n' + '\n'.join(rows) + '\n'
    hashes = {name: digest(ROOT / name) for name in ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_priestmenu.py', 'tools/decomp_saved_state_file.py')}  # 실제 실행한 근거만 고정한다.
    report = {'host': socket.gethostname(), 'lastDecompileHost': 'HJOW-Athlon / 2026-10-23', 'observations': len(rows),
              'pathObservation': path, 'sha256': hashes, 'scope': '실제4b5470/4e58e4/4e58ba/4ed5ee. formatter/path/Win32/TLS getter는명시장치. 실제파일/게임/OS/통신실행없음.'}
    if args.verify:
        previous = json.loads(REPORT.read_text(encoding='utf-8'))
        assert previous == report and FIXTURE.read_text(encoding='utf-8') == content
    else:
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
        REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'observations': len(rows), 'path': path, 'verified': args.verify}))


if __name__ == '__main__':
    main()
