#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 PE의 499fe0 앞단에서 dirty 출력·두 요청 소비·첫 flags 보존을 관측한다. 게임 실행은 없다."""
import argparse
import hashlib
import itertools
import json
import platform
from pathlib import Path
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

ENTRY, END = 0x499fe0, 0x49a081  # 뒤 카메라/flip과 분리한 실제 화면 출력 구간이다.


class Oracle(MemoryOracle):
    """원문의 전체 dirty 계산·flags 검사·저장을 실행하고 OS 장치 호출만 기록한다."""
    def instruction(self, mu, address, size, data):
        """정상 경계에서 멈추며 출력 시점의 요청/count/첫 flags도 함께 기록한다."""
        if address == END:
            self.completed = True
            mu.emu_stop()
            return
        if address in (0x497770, 0x4a17c0, 0x4a17e0):
            self.events.append(f'{address:x}')
            self.ret()
            return
        if address == 0x4a1800:
            esp = mu.reg_read(UC_X86_REG_ESP)
            self.events.append('U:' + ':'.join(map(str, self.words(esp+4, 4))))
            self.at_update.append(list(self.words(POOL+0x5c, 3)) + [self.words(POOL+0x6a8, 1)[0]])
            self.ret()
            return
        if not ENTRY <= address < END:
            raise RuntimeError(f'원문 dirty 출력 범위 밖 {address:08x}')

    def run(self, case):
        """명시 화면/요청/표를 실제 주소에 공급한 후 원문의 최종 저장소를 읽는다."""
        width, height, camera, background, debug, count, first_flags, second_flags = case
        self.events, self.at_update, self.completed = [], [], False
        self.mu.mem_write(POOL, bytes(0x900))
        self.mu.mem_write(STACK-0x1000, bytes(0x1100))
        for address, value in ((0x5c78f0, width), (0x5c78f4, height), (0x59a8a8, debug), (0x59a8b0, 7),
                (POOL+0x5c, camera), (POOL+0x60, background), (POOL+0x64, count), (POOL+0x6a8, first_flags), (POOL+0x6ac, second_flags)):
            self.put(address, value)  # 기대값 계산 없이 명시 입력만 공급한다.
        for index, value in enumerate((2, 3, 7, 8, 20, 30, 40, 50)):
            self.put(POOL+0x68+index*4, value)  # 두 표 항목은 서로 겹치지 않는다.
        self.put(STACK, STOP)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, POOL)
        self.mu.emu_start(ENTRY, STOP, count=10000)
        assert self.completed, case
        return dict(events=';'.join(self.events), atUpdate=self.at_update,
                    final=list(self.words(POOL+0x5c, 7))+[self.words(POOL+0x6a8, 1)[0], self.words(0x59a8b0, 1)[0]])


def main():
    """최초/재관측을 대조하고 새 디렉터리에 원문 출력과 입력만 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    cases = [[width, height, camera, background, debug, count, first, second]
             for (width, height), (camera, background), debug, count, (first, second) in itertools.product(
                 ((1024, 768), (1280, 720), (1280, 800)), ((0, 0), (1, 0), (0, 1), (0xffffffff, 2)),
                 (0, 1), (0, 1, 2), ((0, 0), (1, 4), (4, 1), (2, 2)))]  # 출력 결과와 독립인 교차 입력이다.
    oracle = Oracle()
    first = [oracle.run(case) for case in cases]
    replay = [oracle.run(case) for case in cases]
    assert first == replay
    with (out/'renderer-present-dirty-x86.jsonl').open('x', encoding='utf-8', newline='\n') as stream:
        for case, result in zip(cases, first):
            stream.write(json.dumps(dict(input=case, **result), separators=(',', ':'))+'\n')  # 원문 호출/저장 그대로다.
    with (out/'renderer-present-dirty-x86.tsv').open('x', encoding='utf-8', newline='\n') as stream:
        stream.write('# 화면WH/두 요청/debug/count/첫째둘째flags / 최종9DWORD / 실제 호출\n')
        for case, result in zip(cases, first):
            stream.write('\t'.join(map(str, (*case, *result['final'], result['events'])))+'\n')  # C++도 같은 최초 관측을 읽는다.
    report = dict(unit=648, host=platform.node(), lastFullDecompileHost='HJOW-Athlon', lastFullDecompileDate='2026-10-23',
                  cases=len(cases), observations=len(cases)*2, differences=0,
                  executableSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),
                  scope='499fe0..49a081 원문 저장/검사/출력 인자. OS 장치는 명시 경계. 게임/GUI/네트워크/자연 전투 실행 없음.')
    (out/'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
