#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH 두 함수의 새 독립 관측 도구다. 실제 결과는 새 출력에 보존하며 기존 fixture는 수정하지 않는다.

실제 실행 대상으로 허용하는 코드는 REA에서 확인한 00449220/0042cbd0의 두 몸체뿐이다.
사제 찾기/반경3 finder/가상Count/LocalPlayer는 인자·순서와 명시 변이만 기록하는 격리 경계다.
그러므로 이 출력은 두 상위 본체의 검증이며 하위 월드 본체/실제Game 결선 검증을 대신하지 않는다.
보호PE는 읽기 전용으로 적재하고 호스트 원본 게임/장치/네트워크 코드는 실행하지 않는다.
출력은 이 ignored 초안 디렉터리에만 새 JSON으로 허용한다. 기존 fixture와 기대값은 바꾸지 않는다.
"""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

# 기존 도구의 격리 Python 의존성과 PE/Unicorn 로더 모듈 위치다.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_owner_oracle import (pefile, Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 정적 REA의 정확한 보호PE SHA이며 다른 판본을 우연히 실행하지 않는다.
EXPECTED_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
# REA의 실제 body 끝은 포함 주소이며 두 함수 사이의 다른 코드는 허용하지 않는다.
BODIES = ((0x449220, 0x4492ea), (0x42cbd0, 0x42cc09))
# 호스트 주소와 관계없는 격리 에뮬레이터 입력 저장소다.
POOL, VTABLE, COUNT = 0x16000000, 0x16010000, 0x16011000
CS_A, CS_B, STOP, STACK = 0x16100000, 0x16110000, 0x20000000, 0x3003f000
# 원문 외부 경계의 실제 주소다. Count만 명시 합성 가상 진입점이며 원문 본체로 세지 않는다.
PRIEST, ALTAR, LOCAL = 0x4921f0, 0x4b2140, 0x40da40
# 현재 전역 포인터/관문의 원문 주소다.
POOL_GLOBAL, CHALLENGE, KNOWN, CURRENT_CS = 0x5c8464, 0x594fc8, 0x595344, 0x5467d8


def digest(path):
    """새 출력에 실제 입력 파일의 SHA를 기록한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class SceneWorldOracle:
    """두 실제 body와 명시 외부 경계만 허용하는 제한 x86 관측기다."""
    def __init__(self):
        """원본을 읽기만 하며 실행 주소와 메모리 쓰기 범위를 제한한다."""
        binary = ROOT / 'originals/Netstorm.exe'
        if digest(binary) != EXPECTED_SHA:
            raise RuntimeError('정적 REA와 다른 원본 SHA')
        pe = pefile.PE(str(binary)); image = pe.get_memory_mapped_image(); base = pe.OPTIONAL_HEADER.ImageBase
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.mu.mem_map(base, (len(image) + 4095) & ~4095); self.mu.mem_write(base, image)
        # 원문 주소 공간과 겹치지 않는 입력/스택/가상 경계만 별도로 확보한다.
        for address, size in ((POOL, 0x20000), (CS_A, 0x6000), (CS_B, 0x6000), (STOP, 0x1000), (0x30000000, 0x40000)):
            self.mu.mem_map(address, size)
        self.events = []; self.case = {}; self.entries = {}
        self.mu.hook_add(UC_HOOK_CODE, self.on_instruction); self.mu.hook_add(UC_HOOK_MEM_WRITE, self.on_write)

    def read(self, address):
        """정렬되지 않은 현재 DWORD를 원문폭대로 읽는다."""
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def put(self, address, value):
        """명시 입력/경계 변이만 격리 메모리에 쓴다."""
        self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def leave(self, result):
        """cdecl 인자를 지우지 않고 복귀하며 호출자가 보존을 기대하지 않는 ECX/EDX를 흐린다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, result); self.mu.reg_write(UC_X86_REG_ESP, sp + 4)
        self.mu.reg_write(UC_X86_REG_ECX, 0xdeadbeef); self.mu.reg_write(UC_X86_REG_EDX, 0xfeedface)
        self.mu.reg_write(UC_X86_REG_EIP, self.read(sp))

    def on_write(self, _mu, _access, address, size, _value, _data):
        """두 원문 함수는 스택 밖을 쓰지 않는다. 예기치 않은 원본 쓰기는 즉시 거부한다."""
        if not 0x30000000 <= address <= 0x30040000 - size:
            raise RuntimeError(f'원문 조회의 예상 밖 쓰기 {address:08x}+{size}')

    def on_instruction(self, mu, address, _size, _data):
        """명시 경계의 인자/재진입 변이를 기록하고 그 외 주소는 두 원문 body만 실행한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == PRIEST:
            self.events.append(['priest', self.read(sp + 4)]); self.leave(self.case['priest']); return
        if address == ALTAR:
            args = [self.read(sp + at) for at in (4, 8, 12, 16)]
            self.events.append(['altar', *args]); self.leave(self.case['altar']); return
        if address == COUNT:
            self.events.append(['count', mu.reg_read(UC_X86_REG_ECX)])
            if self.case['mutate']:
                self.put(POOL + self.case['altar'] * 50 + 36, 0)
            self.leave(self.case['count']); return
        if address == LOCAL:
            self.events.append(['local'])
            if self.case['mutate']:
                self.put(CURRENT_CS, CS_B)
            self.leave(self.case['owner']); return
        if not any(low <= address < high for low, high in BODIES):
            raise RuntimeError(f'허용하지 않은 코드 {address:08x}')
        if address in (0x449220, 0x42cbd0):
            self.entries[f'{address:08x}'] = self.entries.get(f'{address:08x}', 0) + 1

    def run(self, case):
        """원시 입력을 적재하고 실제 반환/스택/순서/변이 후 raw값을 관찰한다. 기대 bool은 계산하지 않는다."""
        self.case = case; self.events = []; self.put(POOL_GLOBAL, POOL)
        self.mu.mem_write(POOL, bytes(500)); self.put(VTABLE + 0x80, COUNT)
        self.put(POOL + 5 * 50 + 14, 0x42210000); self.put(POOL + 5 * 50 + 18, 0x42250000)
        self.put(POOL + 6 * 50, VTABLE); self.put(POOL + 6 * 50 + 36, case.get('frame', 0))
        self.put(CHALLENGE, case.get('challenge', 0)); self.put(KNOWN, case.get('known', 0)); self.put(CURRENT_CS, CS_A)
        self.mu.mem_write(CS_A, bytes(0x5350)); self.mu.mem_write(CS_B, bytes(0x5350))
        cs_owner = case['owner'] if case['operation'] == 'waiting' else 0
        self.put(CS_A + cs_owner * 0x1cc + 0xc14, case.get('flag', 0))
        self.put(CS_B + cs_owner * 0x1cc + 0xc14, case.get('replacement_flag', 0x80000000))
        # 이전 호출의 보존 레지스터/인자/분기 플래그가 다음 입력을 오염시키지 않게 한다.
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)
        self.mu.reg_write(UC_X86_REG_EFLAGS, 2); self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP); self.put(STACK + 4, case['owner'])
        entry = 0x449220 if case['operation'] == 'sacrificing' else 0x42cbd0
        self.mu.emu_start(entry, STOP, count=10000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise RuntimeError('원문 조회의 실제 cdecl 반환/스택 불일치')
        eax = self.mu.reg_read(UC_X86_REG_EAX)
        return dict(input=case, eax=eax, bool_byte=eax & 255, events=self.events,
            frame_after=self.read(POOL + 6 * 50 + 36), current_cs_after=self.read(CURRENT_CS))


def inputs():
    """양/음 signed 경계·조기반환·전체DWORD·명시경계변이 입력만 생성하며 기대값은 만들지 않는다."""
    # 현재 소유자 범위는 두 함수를 분리한다. 희생의 DWORD비교는0x101도 입력한다.
    for owner, priest, altar, frame, count, mutate in itertools.product((0, 1, 0x101), (0, 5), (0, 6),
            (13, 14, 74, 75, 119, 120, -1, -2147483648), range(4), (0, 1)):
        yield dict(operation='sacrificing', owner=owner, priest=priest, altar=altar, frame=frame, count=count, mutate=mutate)
    # 현재 CS의 실제40참가자 범위 안에서 모드/known/필드의 전체 비영DWORD를 교차한다.
    for challenge, known, owner, flag, mutate in itertools.product((0, 1, 0x100), (0, 1, 0x80000000), (0, 1, 8, 39), (0, 1, 0x80000000), (0, 1)):
        yield dict(operation='waiting', owner=owner, challenge=challenge, known=known, flag=flag, mutate=mutate)


def main():
    """수동 검토 후 선택할 새 출력만 허용하며 기존fixture 경로는 거부한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--output', type=Path, required=True); args = parser.parse_args()
    output = args.output.resolve(); draft = ROOT / 'extracted/dotnet-scene-world-draft'
    if output.parent != draft or output.exists():
        raise RuntimeError('출력은 같은 ignored 초안 폴더의 아직 없는 새 파일이어야 한다')
    oracle = SceneWorldOracle(); rows = [oracle.run(case) for case in inputs()]
    payload = dict(status='new-original-observations-not-registered-fixture', binary_sha256=EXPECTED_SHA,
        helper_sha256=digest(__file__), bodies=BODIES, recorded_boundaries=['priest', 'altar', 'count', 'local'],
        entries=oracle.entries, rows=rows)
    output.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
