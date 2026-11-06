#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 정렬 Content 부착의 링크·Pop·부모 통지까지 실제 PE로 관측한다."""
import argparse
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS

POOL, VT, DEVICE = 0x13000000, 0x14000000, SCRATCH + 0x1000  # 게임/호스트와 분리한 슬롯·가상 표·외부 장치다.
FIXTURE = ROOT/'cpppj/tests/fixtures/content-attach-current-x86.tsv'  # 최초41행을 덮어쓰지 않는 관측 파일이다.
REPORT = ROOT/'cpppj/content-attach-current-native-20261106.json'  # 원문/장치 경계/도구와 최초 관측의 정체성이다.


def digest(path):
    """근거 파일의 전체 바이트를 고정한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class ContentOracle(Oracle):
    """004aec10과 실제 setNext만 실행하며 가상 getter/setter·Pop·통지는 명시 장치다."""
    def __init__(self):
        """같은 PC의 정밀 함수 범위와 격리 물리 입력만 준비한다."""
        super().__init__('originals', [dict(name='content', originals='004aec10')], helpers=[0x4aba70])
        self.mu.mem_map(POOL, 0x10000)
        self.mu.mem_map(VT, 0x1000)
        self.events = []  # 원문이 실제 선택한 호출 순서다.
        self.cause = 'returned'  # 조기 반환과 통지까지 진행한 경계를 구별한다.
        self.profile = 0  # 명시 외부 장치의 현재값 변경/중단 입력이다.

    def put(self, at, value, width=4):
        """지정 BYTE/WORD/DWORD 폭만 쓴다."""
        self.mu.mem_write(at, (value & ((1 << (width*8))-1)).to_bytes(width, 'little'))

    def get(self, at, width=4):
        """정렬되지 않은 실제 입력 메모리를 읽는다."""
        return int.from_bytes(self.mu.mem_read(at, width), 'little')

    def slot(self, sid, offset=0):
        """실제 PATCH50BYTE 슬롯을 계산한다."""
        return POOL + sid*50 + offset

    def resume(self, value=0, purge=0):
        """자식 장치만 원래 호출 규약으로 복귀시킨다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_EIP, self.get(sp))
        self.mu.reg_write(UC_X86_REG_ESP, sp+4+purge)

    def on_instruction(self, mu, address, size, data):
        """각 소비 위치의 현재 가상 토큰과 앞선 링크 쓰기를 관측한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e0620:
            line = self.get(sp+12)
            self.events.append(f'D{line}')
            if self.profile in (4, 5):
                self.put(self.slot(9 if line == 0xbe6 else 5, 11), 4, 1)
                if self.profile == 5:
                    self.put(0x541088, 1)
                    self.put(self.slot(7, 10), 1, 1)
                    self.put(self.slot(11, 10), 1, 1)
            self.resume()
            return
        if DEVICE <= address < DEVICE+0x200:
            table_index, kind = divmod(address-DEVICE, 0x40)
            sid = (mu.reg_read(UC_X86_REG_ECX)-POOL)//50
            value = self.get(sp+4)
            token = VT+table_index*0x100
            if kind == 0:
                self.events.append(f'G{sid},{token}')
                if self.profile == 7:
                    self.cause = 'previous';mu.emu_stop();return
                if self.profile == 6:
                    self.put(self.slot(5), VT+0x400)
                self.resume(self.get(self.slot(sid, 14), 2))
            elif kind in (8, 16):
                self.events.append(f'{"P" if kind == 8 else "R"}{sid},{token},{value}')
                self.put(self.slot(sid, 16 if kind == 8 else 14), value, 2)
                self.resume(purge=4)
            elif kind == 24:
                self.events.append(f'N{sid},{self.get(sp+4)},{self.get(sp+8)}')
                self.cause = 'notice' if self.profile == 2 else 'attached'
                mu.emu_stop()  # 부모 통지 경계 뒤의 사제/전투 꼬리는 실행하지 않는다.
            else:
                raise RuntimeError('미검토 Content 가상 장치')
            return
        if address == 0x4ac560:
            self.events.append('O'+str(self.get(sp+4)))
            if self.profile == 1:
                self.cause = 'pop';mu.emu_stop();return
            self.put(self.slot(5, 11), self.get(self.slot(5, 11), 1) & ~4, 1)
            self.resume(purge=4)
            return
        super().on_instruction(mu, address, size, data)

    def observe(self, case):
        """정렬/상태/장치 입력만 설치하고 원문의 다섯 슬롯 전체를 반환한다."""
        layout, state, self.profile = case
        self.events, self.cause = [], 'returned'
        self.mu.mem_write(POOL, bytes(16*50))
        self.put(0x5c8464, POOL);self.put(0x541088, 6)
        for index in range(5):  # 서로 다른 가상 토큰을 각 getter/setter/통지 장치에 연결한다.
            for offset, kind in ((4, 0), (8, 8), (12, 16), (36, 24)):
                self.put(VT+index*0x100+offset, DEVICE+index*0x40+kind)
        for index, sid in enumerate((5, 7, 9, 11)):  # 순서/미수정 payload도 원문 결과에 포함한다.
            self.mu.mem_write(self.slot(sid), bytes([0x71])*50)
            self.put(self.slot(sid), VT+index*0x100)
            for offset, value, width in ((4, 0, 2), (6, 0, 2), (10, 6, 1), (11, 4, 1), (14, 0, 2), (16, 0, 2), (18, 85, 2), (20, 0, 2), (23, 0, 1)):
                self.put(self.slot(sid, offset), value, width)
        self.put(self.slot(5, 11), state, 1)
        if layout:
            self.put(self.slot(9, 6), 7, 2);self.put(self.slot(7, 4), 11, 2);self.put(self.slot(11, 14), 7, 2)
            self.put(self.slot(7, 23), 2 if layout == 1 else 0, 1);self.put(self.slot(11, 23), 3 if layout == 1 else 1, 1)
            if layout == 3:
                self.put(self.slot(7, 10), 1, 1);self.put(self.slot(11, 23), 2, 1)
        if self.profile in (3, 4, 5):
            self.put(self.slot(9, 11), 6, 1);self.put(self.slot(5, 11), 6, 1)
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):  # 이전 입력의 레지스터 흔적을 남기지 않는다.
            self.mu.reg_write(reg, 0)
        self.put(STACK, STOP);self.put(STACK+4, 9);self.put(STACK+8, 0x10)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_ECX, self.slot(5));self.mu.reg_write(UC_X86_REG_EFLAGS, 2)
        self.mu.emu_start(0x4aec10, STOP, count=10000)
        if self.cause == 'returned' and (self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK+12):
            raise RuntimeError('Content 조기 반환/스택 불일치')
        raw = ';'.join(bytes(self.mu.mem_read(self.slot(sid), 50)).hex() for sid in (0, 5, 7, 9, 11))
        return '\t'.join((*(str(x) for x in case), self.cause, ';'.join(self.events) or '-', raw))


def main():
    """최초 관측은 배타 생성하고 verify는 원문 출력·근거 SHA를 재대조한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args = parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()):
        raise RuntimeError('Content 최초 관측 덮어쓰기 금지')
    if args.verify:
        for path, expected in json.loads(REPORT.read_text('utf-8'))['sha256'].items():  # 재관측도 동일 원문/도구를 사용한다.
            assert digest(ROOT/path) == expected, path
    cases = [(layout, state, profile) for layout in range(4) for state in (4, 0x14, 0) for profile in range(3)] + [(1, 4, profile) for profile in range(3, 8)]  # 정상·늦은 실패·진단 복귀·가상 토큰 재독 입력이다.
    oracle = ContentOracle();observed = '# layout/state/profile/cause/trace/5x50 raw; 부모 통지까지, 전투·사제 꼬리 미실행\n'+'\n'.join(oracle.observe(case) for case in cases)+'\n'
    if args.verify:
        assert observed == FIXTURE.read_text('utf-8');print('verified 41 Content inputs');return
    with FIXTURE.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write(observed)
    paths = ('originals/Netstorm.exe', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originals/functions.tsv', 'tools/decomp_oracle.py', 'tools/decomp_content_attach_current.py', FIXTURE.relative_to(ROOT).as_posix())
    result = {'unit':958, 'host':socket.gethostname(), 'lastPreciseDecompile':{'host':'HJOW-Athlon','date':'2026-11-03'}, 'rows':len(cases), 'scope':'004aec10의 부모 통지 진입까지와 실제004aba70. 가상 getter/setter/Pop/통지는 명시 장치. 사제/전투 꼬리·게임/MP/통신 미실행.', 'sha256':{path:digest(ROOT/path) for path in paths}}
    with REPORT.open('x', encoding='utf-8', newline='\n') as stream:
        stream.write(json.dumps(result, ensure_ascii=False, indent=2)+'\n')
    print('captured 41 Content inputs')


if __name__ == '__main__':
    main()
