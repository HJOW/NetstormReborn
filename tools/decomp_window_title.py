#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 보호 PE의 창 제목 선택을 격리 x86에서 실행하고 설정/창 경계만 관찰한다."""
import csv
import itertools
import json
import socket
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, PLAYERS, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

SPECS = {  # 정밀 내보내기로 확인한 제목 함수/미션 getter와 현재 전역 주소다.
    'originals': dict(entry=0x460730, path=0x4410f0, apply=0x435c60, get=0x482c90,
        fort=0x55b490, editor=0x5c85a4, mission=0x594fa4, owner=0x540cac, templates=0x5c89f0, system=0x562ca0),
    'originalCD': dict(entry=0x4ca4e0, path=0x4a9460, apply=0x4856f0, get=0x480cf0,
        fort=0x53f458, editor=0x518904, mission=0x52e170, owner=0x50f6d4, templates=0x564bd0, system=0x549208),
}
FORT, MISSION, TITLE, RESULT = (PLAYERS+value for value in (0x8000, 0x9000, 0xa000, 0xb000))  # 명시 문자열/미션 외부 입력 저장소다.
FIXTURE = ROOT/'cpppj/tests/fixtures/window-title-x86.tsv'  # 새 고유 입력/관찰이며 이전 fixture를 덮지 않는다.
REPORT = ROOT/'cpppj/window-title.json'  # 실제 PE/도구/정밀 원문 SHA와 관찰 범위를 기록한다.


class TitleOracle(OwnerOracle):
    """문자열 복사/분기와 미션 getter는 원본 명령을 실행한다."""
    def __init__(self, edition):
        """정확한 내보내기 함수만 허용하고 기존 공용 격리 PE 메모리를 빌린다."""
        super().__init__(edition)
        self.title = SPECS[edition]  # 현재 판본의 실제 주소 표다.
        self.paths = [ROOT/f'extracted/window-title/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]  # 현재 호스트의 정밀 근거다.
        self.allowed, self.entries = [], set()  # 이 실행에서 허용하는 제목/미션 getter 몸체만 보관한다.
        with self.paths[-1].open(encoding='utf-8') as fp:
            # 각 불연속 함수 범위도 Ghidra 기록 그대로 사용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 한 함수의 분리된 범위도 각각 허용하며 그 사이 코드는 실행하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high+1))
        self.external_calls = 0  # 설정 조회/미션 문자열 조회/창 적용의 명시 경계 호출 수다.

    def put(self, address, value):
        """명시 입력 DWORD만 격리 메모리에 쓴다."""
        self.mu.mem_write(address, struct.pack('<I', value))

    def string(self, address):
        """명시 외부 문자열을 NUL까지 Windows-1252로 읽는다."""
        return bytes(self.mu.mem_read(address, 512)).split(b'\0', 1)[0].decode('cp1252')

    def returned(self, sp, value=0, purge=0):
        """cdecl은 caller 정리, 실제 미션 Get의 thiscall은 ret4 정리를 반영한다."""
        ret = struct.unpack('<I', self.mu.mem_read(sp, 4))[0]  # 현재 실제 반환 주소다.
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp+4+purge)
        self.mu.reg_write(UC_X86_REG_EIP, ret)

    def on_instruction(self, mu, address, size, data):
        """설정/미션 title 조회/실제 창 장치만 대체하고 원본 제목 알고리즘은 실행한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)  # 외부 경계 진입 시 원본 스택이다.
        if address == self.title['path']:
            key, argument, second, third = struct.unpack('<IIII', mu.mem_read(sp+4, 16))  # 원본 PathSpec의 네 인자다.
            assert second == third == 0
            name = self.string(key)  # 실제 원본이 선택한 설정 키다.
            arg = self.string(argument) if argument else '<null>'  # NULL과 빈 문자열을 구분하는 외부 입력이다.
            self.trace.append(name+'='+arg)
            output = '<'+name+':'+arg+'>'  # 설정 소비자가 반환하는 명시 입력이며 분기 기대를 계산하지 않는다.
            mu.mem_write(RESULT, output.encode('cp1252')+b'\0')
            self.external_calls += 1
            self.returned(sp, RESULT)
            return
        if address == self.title['get']:
            assert mu.reg_read(UC_X86_REG_ECX) == MISSION and self.string(struct.unpack('<I', mu.mem_read(sp+4, 4))[0]) == 'title'
            self.trace.append('title')
            self.external_calls += 1
            self.returned(sp, TITLE, 4)
            return
        if address == self.title['apply']:
            self.output = self.string(struct.unpack('<I', mu.mem_read(sp+4, 4))[0])  # 실제 완성 버퍼를 그대로 읽는다.
            self.external_calls += 1
            self.returned(sp)
            return
        super().on_instruction(mu, address, size, data)

    def sample(self, fort, editor, mission, present, readonly, demo, test, title):
        """서로 독립인 모드/Template/빈 문자열 입력을 원본에 공급한다."""
        self.trace, self.output = [], None  # 이번 호출만의 조회 순서와 최종 제목 관찰이다.
        self.mu.mem_write(FORT, ('' if fort == '#NULL' else fort).encode('cp1252')+b'\0')
        self.mu.mem_write(TITLE, title.encode('cp1252')+b'\0')
        self.put(self.title['fort'], 0 if fort == '#NULL' else FORT)
        self.put(self.title['editor'], editor)
        self.put(self.title['mission'], mission)
        self.put(self.title['owner'], 3)
        self.put(self.title['templates']+60, present)
        self.put(self.title['templates']+68, readonly)
        self.put(self.title['system']+0x800, MISSION)
        self.put(MISSION+0x64, test)
        self.put(MISSION+0x68, demo)
        self.mu.mem_write(STACK, struct.pack('<I', STOP))
        self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.reg_write(UC_X86_REG_ECX, 0)
        self.mu.emu_start(self.title['entry'], STOP, count=20000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK+4 and self.output is not None
        return '|'.join(self.trace), self.output


def main():
    """두 판본 새 입력만 기록하며 game/OS 창 프로세스는 실행하지 않는다."""
    rows, editions = [], {}  # 각 고유 입력의 실제 반환과 판본별 근거다.
    cases = list(itertools.product(('#NULL', '', 'Saved Fort'), (0, 1), (0, 1), (0, 1), (0, 1), (0, 1), (0, 1), ('', 'The War Begins', 'M'*200)))  # 원본 판단을 포함하지 않는 독립 입력 곱이다.
    # 판본마다 현재 PE의 독립 고유 입력을 한 번씩 실행한다.
    for edition in SPECS:
        oracle = TitleOracle(edition)  # 판본마다 격리 PE/호출 집계를 새로 만든다.
        # 입력 분기를 추정하지 않고 실제 명령이 반환한 trace/제목을 기록한다.
        for case in cases:
            rows.append((edition, *case, *oracle.sample(*case)))
        editions[edition] = dict(inputs=len(cases), returns=len(cases), assertions=oracle.assertions, peSha=oracle.sha256,
            nativeCalls=dict(oracle.native_calls), externalCalls=oracle.external_calls, exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.paths})
        print(edition, len(cases), '창 제목 실제 정상 반환', flush=True)
    assert len(rows) == len(set(row[:-2] for row in rows))
    FIXTURE.write_text('# 실제 제목 함수/미션 getter; 설정 반환/미션 title 문자열/창 장치는 명시 외부 경계.\n'+'\n'.join('\t'.join(map(str, row)) for row in rows)+'\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(), uniqueInputs=len(rows), editions=editions,
        fixtureSha=digest(FIXTURE), toolSha=digest(__file__), supportSha=digest(ROOT/'tools/decomp_owner_oracle.py'),
        limitations=['256BYTE 안의 정의된 ASCII 문자열 입력이다. 초과/잘못된 포인터는 실행하지 않았다.', '원본 설정 알고리즘/Win32 창 출력의 재관찰이 아니며 선택된 키/인자/복사/최종 적용 요청을 관찰했다.']), ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')


if __name__ == '__main__':
    main()
