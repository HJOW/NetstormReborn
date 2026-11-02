#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문441790의6경로 탐색/실제Probe/디렉터리 확정 구간을 CPU로 관측한다."""
import argparse
import json
import socket
from pathlib import Path
from decomp_file_directory import DirectoryOracle
from decomp_file_name_leaf import add_ranges
from decomp_file_name_formatter import Oracle as NameOracle, REGISTERS, RING, INDEX
from decomp_priestmenu import ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

FIXTURE = ROOT / 'cpppj/tests/fixtures/startup-location-x86.tsv'  # 최초 전체 고리와 실제 방문 인자를 덮지 않는 입력이다.
GAME_ROOT = 0x54d988  #435cd0이 이미 채운 실행 위치 문자열이다.


class LocationOracle(DirectoryOracle):
    """CD 검색 직전4418bd 또는 원문assert에서 멈추며 Probe의 분기는 그대로 실행한다."""
    def __init__(self):
        """기존 실제 작성기/디렉터리에 탐색 부모와Probe/정규화/FileSpec만 추가한다."""
        super().__init__()
        self.mu.mem_map(0, 0x1000)
        add_ranges(self, (0x441790, 0x459520, 0x45a220, 0x459610, 0x4595f0, 0x459900, 0x41ab10, 0x4e7620))
        self.probes = []  # 실제 원문이 Probe에 넘긴 지역 사본의 순서다.
        self.hit = 0  # 이 명시 입력에서 첫 fopen이 성공할 후보 번호다.6은모두실패다.
        self.status = ''  # 정상 위치 확정과 원문assert를 구별한다.

    def guard(self, mu, access, address, size, value, data):
        """SEH 첫 슬롯 외에는 기존 스택/고리 쓰기 제한을 유지한다."""
        if address == 0 and size == 4:
            return
        NameOracle.guard(self, mu, access, address, size, value, data)

    def instruction(self, mu, address, size, data):
        """파일 열기/목록 검색 장치만 응답하고 부모·Probe·모든 경로 BYTE는 실제 명령이다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x41ab10:
            self.probes.append(self.string(self.words(esp + 4, 1)[0]).hex())
        if address == 0x41a340:
            assert mu.reg_read(UC_X86_REG_ECX) in (0x5450b8, 0x544f80)
            self.ret(0, 12)  # 시작 전 두TAFF목록은 비어 있는 명시 조건이다.
            return
        if address == 0x4e545a:
            name, mode = self.words(esp + 4, 2)
            assert self.string(mode) == b'rb' and self.string(name)
            self.ret(0x1234 if len(self.probes) - 1 == self.hit else 0)
            return
        if address == 0x4e54b9:
            assert self.words(esp + 4, 1) == (0x1234,)
            self.ret(0)
            return
        if address == 0x41aae0:
            self.ret(0)  # 보조 디스크는 없는 명시 조건이며 앞의 실제 경로 작성은 그대로 관측한다.
            return
        if address == 0x4e0620:
            expression, source, line = self.words(esp + 4, 3)
            assert self.string(expression) == b'i<COUNTOF(searchPaths)' and line == 0xc3
            assert self.string(source) == b'\\Ns\\O\\Configinterface.cpp'
            self.status = 'assert'
            mu.emu_stop()
            return
        if address == 0x4418bd:
            self.status = 'ok'
            mu.emu_stop()
            return
        NameOracle.instruction(self, mu, address, size, data)

    def locate(self, root, hit, index, precision):
        """여섯 후보 각각의 첫 성공/모두 실패에서 prefix·번호·전체10칸을 고정한다."""
        self.probes.clear()
        self.hit, self.status, self.thrown = hit, '', False
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, index)
        self.put(0x532728, 92, 1)
        self.put(0, 0)
        self.mu.mem_write(GAME_ROOT, root + b'\0')
        self.mu.mem_write(0x544d80, bytes(512))  # 두기본경로는 원문최초BSS의빈문자열이다.
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        for number, register in enumerate(REGISTERS):  # 부분 부모의 실제 스택/고리 쓰기를 비영 입력에서 관측한다.
            self.mu.reg_write(register, 0x12345678 + number * 0x1020304)
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.put(STACK, STOP)
        self.mu.emu_start(0x441790, STOP, count=200000)
        assert not self.thrown and self.status
        assert len(self.probes) == min(hit + 1, 6)
        assert self.mu.reg_read(UC_X86_REG_FPCW) == precision
        assert bytes(self.mu.mem_read(GAME_ROOT, len(root) + 1)) == root + b'\0'
        prefix = self.string(self.mu.reg_read(UC_X86_REG_ESP) + 8) if self.status == 'ok' else b''
        return dict(status=self.status, probes=','.join(self.probes), prefix=prefix.hex(),
                    index=self.words(INDEX, 1)[0], ring=bytes(self.mu.mem_read(RING, 2560)).hex())


def main():
    """최초fixture생성과 재검증은 구별하고 기존 행을 다시 쓰지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    if not args.verify:
        assert not FIXTURE.exists()
    oracle = LocationOracle()
    lines = ['# root_hex hit initial_index status probes_hex prefix_hex final_index ring_hex']
    for root in (b'', b'.', b'Game', b'C:\\Game', b'C:\\Game\\', b'\\\\server\\share\\Game'):  # 상대/장치/UNC 루트의 다른Probe소모를 구별한다.
        for hit in range(7):  # 각각 첫 성공하는 여섯 후보와전부실패다.
            for index in (0, 9):  # 고리의 첫 순환도 별도 입력으로 고정한다.
                row = oracle.locate(root, hit, index, 0x027f)
                assert row == oracle.locate(root, hit, index, 0x037f)
                lines.append('\t'.join((root.hex() or '-', str(hit), str(index), row['status'], row['probes'], row['prefix'] or '-', str(row['index']), row['ring'])))
    content = '\n'.join(lines) + '\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8') == content
    else:
        FIXTURE.write_text(content, encoding='utf-8', newline='\n')
    report = dict(unit=735, date='2026-11-02', host=socket.gethostname(),
                  lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                  rows=len(lines)-1, observations=(len(lines)-1)*2, normalRows=72, assertionRows=12,
                  boundary='실제441790~4418bd/실제Probe/정규화/FileSpec/Directory/Device/Format/CRT. fopen/fclose/두빈TAFF목록/없는보조디스크만명시경계. CD/CWD/TARC등록/설정Load/전체부모반환규약은범위밖',
                  gameExecutions=0, multiplayerTests=0,
                  sha256={str(p.relative_to(ROOT)): digest(p) for p in (ROOT/'originals/Netstorm.exe', Path(__file__), FIXTURE)})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(rows=report['rows'], observations=report['observations'])))


if __name__ == '__main__':
    main()
