#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""BaseFile의 여섯 원문 몸체를 격리 실행하고 CRT/진단 경계의 인자와 현재 필드를 동결한다."""
import argparse
import hashlib
import itertools
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, PLAYERS, TYPES, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EAX

ENTRIES = dict(seek=0x41a700, eof=0x41a6c0, length=0x41aa80, close=0x41a680, destruct=0x41b410, write=0x41a910)  # 판본10.78의 실제 주소다.
RANGES = ((0x41a680, 0x41a6f1), (0x41a700, 0x41a79c), (0x41a910, 0x41aac4), (0x41b410, 0x41b458), (0x4e4a40, 0x4e4a7d))  # __chkstk도 실제 원문을 실행한다.


class Oracle(MemoryOracle):
    """계산·분기·XOR는 PE 명령으로 실행하며 CRT 호출만 명시 장치로 받는다."""
    def instruction(self, mu, address, size, data):
        """실제 경계의 현재 인자와 데이터를 기록하고 원문 복귀 규약으로 돌아간다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp+4, 4)
        if address == 0x4e573c:
            file, offset, origin = args[:3]
            self.events.append(f'S:{file}:{offset}:{origin}')
            if not self.failed:
                self.position = (offset + (self.position if origin == 1 else self.end if origin == 2 else 0)) & 0xffffffff
            self.ret(0xffffffff if self.failed else 0)
            return
        if address == 0x4e566c:
            self.events.append(f'T:{args[0]}')
            if self.scenario == 2:
                self.put(POOL+4, self.words(POOL+4, 1)[0] ^ 1)
                self.put(POOL+8, 9)
                self.put(POOL+12, 11)
            self.ret(self.position)
            return
        if address == 0x4e44a6:
            pointer, length, count, file = args
            sent = bytes(mu.mem_read(pointer, length*count))
            self.events.append(f'W:{file}:{length}:{count}:{sent.hex() or "-"}')
            self.position = (self.position + (length*count//2 if self.failed else length*count)) & 0xffffffff
            self.ret(0 if self.failed or not length else count)
            return
        if address == 0x4e54b9:
            self.events.append(f'C:{args[0]}')
            if self.scenario == 1:
                for at, value in enumerate((PLAYERS+64, 255, 99, 100, TYPES+128, 5)):
                    self.put(POOL+at*4, value)  # fclose 중 변이도 뒤 원문 store/키 조회와 구별한다.
            self.ret(0xffffffff if self.failed else 0)
            return
        if address == 0x4e4354:
            self.events.append(f'F:{args[0]}')
            self.ret()
            return
        if address == 0x4e0620:
            line = args[2]
            self.events.append(f'A:{line}')
            if self.scenario == 4 and line == 0x3aa:
                self.put(POOL+20, 3)
            if self.scenario == 5 and line == 0x3a4:
                self.put(POOL+4, self.words(POOL+4, 1)[0] & ~1)
            self.ret(1)
            return
        if not any(low <= address < high for low, high in RANGES):
            raise RuntimeError(f'원문 BaseFile 범위 밖 {address:08x}')

    def run(self, case):
        """명시 FILE 필드/입력을 설치하고 실제 최종 상태와 호출 전체를 반환한다."""
        operation, file, flags, base, length, key, key_length, position, end, file_flags, offset, origin, size, scenario, failed = case
        self.events, self.completed = [], False
        self.position, self.end, self.scenario, self.failed = position, end, scenario, failed
        self.mu.mem_write(STACK-0x10000, bytes(0x10100))
        for index, value in enumerate((file, flags, base, length, key, key_length)):
            self.put(POOL+index*4, value)  # 호스트 기대값을 작성하지 않고 입력 여섯 필드만 공급한다.
        self.put(PLAYERS+12, file_flags)
        self.mu.mem_write(TYPES, b'abc')
        self.mu.mem_write(TYPES+128, b'vwxyz')
        self.mu.mem_write(TEXT, bytes((index*29+7) & 255 for index in range(size)))
        arguments = [offset, origin] if operation == 'seek' else [TEXT, size, offset] if operation == 'write' else []
        self.put(STACK, STOP)
        for index, value in enumerate(arguments):
            self.put(STACK+4+index*4, value)  # 원문의 thiscall 인자 순서다.
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, POOL)
        self.mu.emu_start(ENTRIES[operation], STOP+1, count=2000000)
        assert self.completed, case
        result = self.mu.reg_read(UC_X86_REG_EAX) if operation in ('eof', 'length') else None
        return dict(result=result, final=list(self.words(POOL, 6)), position=self.position, events=';'.join(self.events) or '-')


def cases():
    """원문 출력과 독립인 정상/끝/감김/부분 쓰기/콜백 변이 입력을 구성한다."""
    def case(operation, flags=0x80, file=PLAYERS, key=TYPES, position=15, end=666, file_flags=16, offset=0xffffffff, origin=0, size=0, scenario=0, failed=0, key_length=3):
        """고정된 부분 파일 base7/length9와 명시 조합의 입력 한 행이다."""
        return [operation, file, flags, 7, 9, key, key_length, position, end, file_flags, offset, origin, size, scenario, failed]
    result = [case('seek', flags=f, offset=o, origin=r, failed=fail) for f, o, r, fail in itertools.product((0x80, 0x81), (0xfffffffb, 0, 2, 0x7fffffff), (0xffffffff, 0, 1, 2, 3), (0, 1))]
    result += [case('eof', flags=f, position=p, file_flags=v, scenario=s) for f, p, v, s in itertools.product((0x80, 0x81), (15, 16, 17, 0xffffffff, 0x80000000), (0, 16, 17), (0, 2))]
    result += [case('length', flags=f, position=p, end=e, failed=fail) for f, p, e, fail in itertools.product((0x80, 0x81), (0xffffffff, 12), (0, 666), (0, 1))]
    result += [case(op, flags=f, file=p, key=k, scenario=s, failed=fail) for op, f, p, k, s, fail in itertools.product(('close', 'destruct'), (0x80, 0x81), (0, PLAYERS), (0, TYPES), (0, 1), (0, 1))]
    result += [case('write', flags=f, key=k, size=n, offset=o, failed=fail) for f, k, n, o, fail in itertools.product((0x80, 0x81), (0, TYPES), (0, 1, 4095, 4096, 4097, 8193), (0xffffffff, 2), (0, 1))]
    result += [case('write', flags=f, size=4097, scenario=s, key_length=0 if s == 4 else 3) for f, s in itertools.product((0x80, 0x81), (2, 4, 5))]
    return result


def main():
    """한 프로세스 안의 재관측을 먼저 대조하고 새 출력 디렉터리에만 저장한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    out = parser.parse_args().output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    inputs = cases()
    oracle = Oracle()
    observed = [oracle.run(case) for case in inputs]
    assert observed == [oracle.run(case) for case in inputs]
    with (out/'basefile-io-x86.jsonl').open('x', encoding='utf-8', newline='\n') as stream:
        for case, result in zip(inputs, observed):
            stream.write(json.dumps(dict(input=case, **result), separators=(',', ':'))+'\n')  # 전체 쓰기 BYTE도 원시 관측으로 보존한다.
    with (out/'basefile-io-x86.tsv').open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# op/file/flags/base/length/key/keyLength/position/end/fileFlags/offset/origin/size/scenario/failure / result/final6/position/events\n')
        for case, result in zip(inputs, observed):
            stream.write('\t'.join(map(str, (*case, '-' if result['result'] is None else result['result'], ','.join(map(str, result['final'])), result['position'], result['events'])))+'\n')
    report = dict(unit=651, host=platform.node(), lastFullDecompileHost='HJOW-Athlon', lastFullDecompileDate='2026-10-23', cases=len(inputs), observations=len(inputs)*2, differences=0,
                  executableSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),
                  scope='6개 BaseFile 몸체와 실제 __chkstk. CRT/진단만 명시 경계. 게임/GUI/네트워크 실행 없음.')
    (out/'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
