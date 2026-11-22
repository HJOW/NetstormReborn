#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""REA로 읽은 원본 Player Pre 바이트만 격리 실행해 독립 필드/버퍼 기대값을 저장한다."""
from pathlib import Path
import csv
import hashlib
import itertools
import json
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_EDI

# 모든 실행 메모리는 호스트/게임 프로세스와 분리한 가상 주소다.
ROOT = Path(__file__).resolve().parents[1]
# 이번 Native 근거와 실행 감사의 독립 출력 경로다.
OUT = ROOT / 'extracted/dotnet-0813'
# C++/DN이 함께 읽을 원본 관측 자료다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/player-pre-destroy-x86.tsv'
# Source가 지정한 진입과 실제 공통 Pre 경계다.
ENTRY, BASE_PRE = 0x4906c0, 0x4b0950
# 실제 PE 명령이 읽는 Player base와 Pool base 전역이다.
RECORDS_GLOBAL, POOL_GLOBAL = 0x59534c, 0x5c8464
# 고유 Pre만 접근하는 격리 배열 세 개와 슬롯/스택/정상 반환 주소다.
BUFFERS, POOL, STACK, STOP = (0x10000000, 0x10010000, 0x10020000), 0x11000000, 0x3000f000, 0x20000000
# 전체 BYTE owner의 실제 주소 계산을 담는 각 관측 버퍼 크기다.
BUFFER_SIZE = 0x10000
# 원본 포인터 차이/50이 계산할 명시 SID다.
SID = 5


def sha(data):
    """실제 관측 전체 바이트의 해시만 계산하고 결과 모델은 만들지 않는다."""
    return hashlib.sha256(data).hexdigest()


def main():
    """불변 PE/REA 바이트를 대조하고 허용한 명령·두 명시 경계에서만 실행한다."""
    binary = ROOT / 'originals/Netstorm.exe'
    original = binary.read_bytes()
    evidence = json.loads((OUT / 'rea-evidence.json').read_text(encoding='utf-8'))['records']
    assert len(evidence) == 2 and all(row['subject']['digest']['sha256'] == sha(original) for row in evidence)
    observed = {int(row['normalized_result']['address'], 16): bytes.fromhex(row['normalized_result']['bytes_hex']) for row in evidence}
    combined = observed[ENTRY] + observed[ENTRY + 128]
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_32).disasm(combined, ENTRY))
    end_index = next(i for i, instruction in enumerate(instructions) if instruction.mnemonic == 'ret')
    body = instructions[:end_index + 1]
    end = body[-1].address + body[-1].size
    assert body[-1].op_str == '4' and end == 0x490747
    and_instruction = next(instruction for instruction in body if instruction.mnemonic == 'and' and instruction.op_str.startswith('dword ptr [eax],'))
    probe = and_instruction.address + and_instruction.size
    assert len([instruction for instruction in body if instruction.mnemonic == 'call']) == 1
    pe = pefile.PE(data=original)
    image = pe.get_memory_mapped_image()
    image_base = pe.OPTIONAL_HEADER.ImageBase
    assert image[ENTRY - image_base:ENTRY - image_base + len(combined)] == combined
    mu = Uc(UC_ARCH_X86, UC_MODE_32)
    mu.mem_map(image_base, (max(len(image), pe.OPTIONAL_HEADER.SizeOfImage) + 4095) & ~4095)
    mu.mem_write(image_base, image)
    mu.mem_map(BUFFERS[0], 3 * BUFFER_SIZE)
    mu.mem_map(POOL, 0x10000)
    mu.mem_map(0x30000000, 0x10000)
    mu.mem_map(STOP, 0x1000)
    raw = POOL + SID * 50
    state = {}
    visited = set()

    def read(address):
        """실제 가상 메모리의 little endian DWORD를 읽는다."""
        return struct.unpack('<I', bytes(mu.mem_read(address, 4)))[0]

    def write(address, value):
        """입력/명시 경계의 한 DWORD만 설정한다."""
        mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def hook(machine, address, size, data):
        """공통 Pre 호출과 두 flags 쓰기 사이의 명시 변경만 제공하며 결과는 원본 명령이 쓴다."""
        if address == STOP:
            machine.emu_stop()
            return
        if address == BASE_PRE:
            state['base_calls'] += 1
            sp = machine.reg_read(UC_X86_REG_ESP)
            assert machine.reg_read(UC_X86_REG_ECX) == raw and read(sp + 4) == state['flags']
            if state['variant']:
                machine.mem_write(raw + 34, bytes((state['next_owner'],)))
                write(RECORDS_GLOBAL, BUFFERS[1])
            machine.reg_write(UC_X86_REG_EIP, read(sp))
            machine.reg_write(UC_X86_REG_ESP, sp + 8)
            return
        if not (ENTRY <= address < end):
            raise AssertionError(f'허용하지 않은 실행 주소: {address:08x}')
        visited.add(address)
        if address == probe:
            state['probe_calls'] += 1
            if state['variant'] == 2:
                machine.mem_write(raw + 34, bytes((state['third_owner'],)))
                write(RECORDS_GLOBAL, BUFFERS[2])

    mu.hook_add(UC_HOOK_CODE, hook)
    rows = []
    cases = itertools.product((0, 1, 8, 9, 255), (0, 0xffffffff, 0x1234, 0x80000000), (0, 1), (0, 1, 0xabcdef01), (0, 1, 2))
    for owner, flags, matches, record_flags, variant in cases:
        # 입력 세 버퍼와 slot은 매번 새 패턴이며 기대 변경을 Python으로 계산하지 않는다.
        next_owner, third_owner = (owner + 1) & 255, (owner + 2) & 255
        for address, color in zip(BUFFERS, (0xa5, 0x5a, 0x3c)):
            mu.mem_write(address, bytes((color,)) * BUFFER_SIZE)
        mu.mem_write(raw, bytes(50))
        mu.mem_write(raw + 34, bytes((owner,)))
        write(BUFFERS[0] + owner * 180 + 0x9c, SID if matches else 0x87654321)
        for address, selected, value in zip(BUFFERS, (owner, next_owner, third_owner), (record_flags, record_flags ^ 0x55555555, record_flags ^ 0xaaaaaaaa)):
            # 기록 flags의 원시 입력만 설정하며 정리 결과를 미리 쓰지 않는다.
            write(address + selected * 180 + 0x50, value)
        write(POOL_GLOBAL, POOL)
        write(RECORDS_GLOBAL, BUFFERS[0])
        write(STACK, STOP)
        write(STACK + 4, flags)
        mu.reg_write(UC_X86_REG_ECX, raw)
        mu.reg_write(UC_X86_REG_ESP, STACK)
        mu.reg_write(UC_X86_REG_ESI, 0x12345678)
        mu.reg_write(UC_X86_REG_EDI, 0x87654321)
        state = dict(flags=flags, variant=variant, next_owner=next_owner, third_owner=third_owner, base_calls=0, probe_calls=0)
        mu.emu_start(ENTRY, STOP, count=1000)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP and mu.reg_read(UC_X86_REG_ESP) == STACK + 8
        assert mu.reg_read(UC_X86_REG_ESI) == 0x12345678 and mu.reg_read(UC_X86_REG_EDI) == 0x87654321
        assert state['base_calls'] == state['probe_calls'] == 1
        rows.append([owner, flags, matches, record_flags, variant, bytes(mu.mem_read(raw, 50)).hex(), *(sha(bytes(mu.mem_read(address, BUFFER_SIZE))) for address in BUFFERS)])
    assert len(rows) == 360 and visited == {instruction.address for instruction in body}
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(['# owner', 'flags', 'matches', 'recordFlags', 'variant', 'raw50', 'recordsAsha256', 'recordsBsha256', 'recordsCsha256'])
        writer.writerows(rows)
    result = {'observations': len(rows), 'target_sha256': sha(original), 'body_start': hex(ENTRY), 'body_end_exclusive': hex(end), 'instruction_count': len(body), 'all_instructions_visited': True, 'base_boundary': hex(BASE_PRE), 'between_flags_probe': hex(probe), 'all_stack_and_saved_registers_preserved': True, 'fixture_sha256': sha(FIXTURE.read_bytes()), 'source_unchanged': binary.read_bytes() == original, 'rea_evidence_ids': [row['evidence_id'] for row in evidence], 'limitations': ['Ghidra는 이 주소를 함수로 자동 등록하지 않았다. REA read_bytes의 두 범위/PE 바이트와 Capstone의ret4까지를 사용했다.', '공통 Pre는 명시 경계이며 실제 공통 삭제 전체/게임 프로세스/Root를 실행한 관측이 아니다.', 'variant2는 두 flags 쓰기 사이의 명시 메모리 변경 입력이다.']}
    (OUT / 'native-player-pre-summary.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (OUT / 'native-player-pre-assembly.tsv').write_text('\n'.join(f'{instruction.address:08x}\t{instruction.bytes.hex()}\t{instruction.mnemonic}\t{instruction.op_str}' for instruction in body) + '\n', encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
