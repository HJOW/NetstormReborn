#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Process 부착의 서버0 접두만 양판본 PE로 관측한다. 직렬화/송신 분기는 실행하지 않는다."""
import argparse
import hashlib
import json
import socket
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

FIXTURE = ROOT/'cpppj/tests/fixtures/process-attach-current-local-x86.tsv'  # 구현 전 최초 서버0 출력만 동결한다.
REPORT = ROOT/'cpppj/process-attach-current-native-20261106.json'  # 관측 범위와 파일 정체성을 기록한다.
SPEC = {  # 정밀 디컴파일과 현재 PE의 두 부모/공통 자식/서버 전역이다.
    'originals': (0x4b1440, 0x4ac800, 0x540bc0, 0x4e0620, 50),
    'originalCD': (0x4af380, 0x4ac2e0, 0x540a28, 0x490040, 36),
}

class ProcessAttachOracle(Oracle):
    """진단 정상복귀와 공통 부착의 호출 인자를 관측하며 서버 분기는 항상0이다."""
    def __init__(self, edition):
        """원문 부모 외의 게임/OS/직렬화 몸체를 허용하지 않는다."""
        self.spec = SPEC[edition]  # 실제 판본의 고정 주소와 슬롯 폭이다.
        super().__init__(edition, [dict(name='attach', **{edition: f'{self.spec[0]:08x}'})], helpers=[])
        self.events = []  # 원문 진단과 공통 부착 순서다.
        self.profile = 0  # 진단 뒤 현재 raw 부모를 바꾸는 명시 장치 입력이다.

    def read(self, address, width=4):
        """에뮬레이터 내부의 비정렬 입력만 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, width), 'little')

    def resume(self, purge=0):
        """자식 호출의 스택만 복구하며 부모의 실제 ret8도 검증한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, 0)
        self.mu.reg_write(UC_X86_REG_EIP, self.read(sp))
        self.mu.reg_write(UC_X86_REG_ESP, sp+4+purge)

    def on_instruction(self, mu, address, size, data):
        """서버0에서 도달하는 진단/공통 자식만 명시 경계로 대체한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == self.spec[3]:
            self.events.append('D'+self.read_string(self.read(sp+4)).decode('ascii')+'|'+self.read_string(self.read(sp+8)).decode('ascii')+'|'+str(self.read(sp+12)))
            if self.profile:
                mu.mem_write(SCRATCH+16, (11).to_bytes(2, 'little'))
            self.resume()
            return
        if address == self.spec[1]:
            self.events.append(f'A{self.read(sp+4)},{self.read(sp+8)},{self.read(SCRATCH+16,2)}')
            self.resume(8)
            return
        super().on_instruction(mu, address, size, data)

    def observe(self, stored, flags, profile):
        """매 입력을 초기 상태로 되돌리고 실제 부모의 ret8까지 실행한다."""
        self.events = []
        self.profile = profile
        raw = bytearray((i*29+7)&255 for i in range(self.spec[4]))
        raw[16:18] = stored.to_bytes(2, 'little')
        self.mu.mem_write(SCRATCH, bytes(raw))
        self.mu.mem_write(self.spec[2], bytes(4))
        self.call('attach', [7, flags], SCRATCH, 8)
        return [self.edition, f'{stored},{flags},{profile}', ';'.join(self.events), bytes(self.mu.mem_read(SCRATCH, self.spec[4])).hex()]

def main():
    """최초 생성 또는 불변 fixture의 재관측 비교만 선택한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    rows = []
    for edition in SPEC:  # 서버/직렬화/패킷 관측은 생성하지 않는다.
        oracle = ProcessAttachOracle(edition)
        for stored in (0, 7):  # 실제 부모 변경과 동일 부모다.
            for flags in (0, 0x80, 0x100, 0x180, 0xffffff7f, 0xffffff80):  # 낮은BYTE의 debug비트와 전체DWORD 보존을 대조한다.
                for profile in (0, 1):  # 정상 진단과 진단 중 raw 변경이다.
                    rows.append(oracle.observe(stored, flags, profile))
    rendered = '# 서버0 Process 파생부착 접두; 공통 자식은 명시 경계; MP/직렬화/송신 미실행\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8') == rendered
        print(f'verified {len(rows)} local Process inputs')
        return
    assert not FIXTURE.exists() and not REPORT.exists(), '최초 관측 덮어쓰기 금지'
    FIXTURE.write_bytes(rendered.encode('utf-8'))
    paths = ['originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originalCD/Netstorm.c', 'tools/decomp_oracle.py', 'tools/decomp_process_attach_current.py', str(FIXTURE.relative_to(ROOT)).replace('\\', '/')]
    report = {'unit': 959, 'host': socket.gethostname(), 'lastPreciseDecompile': {'host': 'HJOW-Athlon', 'date': '2026-11-03'}, 'rows': len(rows), 'server': 0, 'scope': '004b1440/CD004af380의 비서버 진단→공통부착 호출/ret8. 공통자식은 명시 장치. 서버/MP/직렬화/송신 미실행.', 'sha256': {path: hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in paths}}
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(f'froze {len(rows)} local Process inputs')

if __name__ == '__main__':
    main()
