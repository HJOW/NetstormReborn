#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""REA의 실제 두 지갑 함수만 격리 실행해 전체 본체/Player 기대 해시를 저장한다."""
from pathlib import Path
import csv
import hashlib
import itertools
import json
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ESI

# 읽기 전용 원본과 이번 REA 근거/감사 경로다.
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'extracted/dotnet-0815'
# Core의 단일 소비자가 읽을 독립 원본 관측이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/ai-wallet-x86.tsv'
# Source 진입과 Ghidra 함수 본문 전체 길이다.
FUNCTIONS = {'Credit': (0x413bb0, 98), 'Balance': (0x413c20, 54)}
# 유일한 명시 외부 호출 경계이며 실제 게임/OS 호출은 실행하지 않는다.
DEMO = 0x463b50
# 격리 Player1620BYTE·두 실제0xb08 본체와 스택/반환 주소다.
PLAYERS, FIRST, SECOND, STACK, STOP = 0x10000000, 0x20000000, 0x20010000, 0x3000f000, 0x40000000


def sha(data):
    """모델 결과를 만들지 않고 전체 관측 바이트만 식별한다."""
    return hashlib.sha256(data).hexdigest()


def main():
    """REA/PE 바이트·함수 경계를 확인하고 허용한 명령 주소 안에서만 실행한다."""
    original = (ROOT / 'originals/Netstorm.exe').read_bytes()
    evidence = json.loads((OUT / 'rea-evidence.json').read_text(encoding='utf-8'))['records']
    assert len(evidence) == 4 and all(r['subject']['digest']['sha256'] == sha(original) for r in evidence)
    pe = pefile.PE(data=original); image = pe.get_memory_mapped_image(); base = pe.OPTIONAL_HEADER.ImageBase
    functions = {}; listings = []
    # 메타데이터의 완전 본문 범위와 두 rawbytes 전체를 원본 PE와 직접 대조한다.
    for name, (entry, size) in FUNCTIONS.items():
        fact = next(r['normalized_result'] for r in evidence if r['operation'] == 'procedure_info' and int(r['normalized_result']['entrypoint'], 16) == entry)
        assert fact['body']['ranges'] == [{'start': hex(entry), 'end': hex(entry + size - 1)}] and fact['length'] == size
        code = image[entry-base:entry-base+size]; instructions = list(Cs(CS_ARCH_X86, CS_MODE_32).disasm(code, entry))
        assert sum(i.size for i in instructions) == size and instructions[-1].mnemonic == 'ret'
        functions[name] = {i.address for i in instructions}
        listings += [f'{name}\t{i.address:08x}\t{i.mnemonic}\t{i.op_str}' for i in instructions]
    # 로더 바이트가 PE 원문과 같은 경우만 원본 관측으로 취급한다.
    for record in evidence:
        if record['operation'] != 'read_bytes': continue
        result = record['normalized_result']; data = bytes.fromhex(result['bytes_hex']); address = int(result['address'], 16)
        assert result['complete'] and image[address-base:address-base+len(data)] == data
    mu = Uc(UC_ARCH_X86, UC_MODE_32); mu.mem_map(base, (len(image)+4095)&~4095); mu.mem_write(base, image)
    # 서로 떨어진 입력 배열은 각 허용 크기만 관측하며 다른 포인터를 합성하지 않는다.
    for address, size in [(PLAYERS, 0x1000), (FIRST, 0x1000), (SECOND, 0x1000), (0x30000000, 0x10000), (STOP, 0x1000)]: mu.mem_map(address, size)
    state = {}; visited = {'Credit': set(), 'Balance': set()}

    def read(address):
        """가상 메모리의 실제 DWORD를 읽는다."""
        return struct.unpack('<I', bytes(mu.mem_read(address, 4)))[0]

    def write(address, value):
        """입력 또는 명시 경계의 한 DWORD만 설정한다."""
        mu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def hook(current, address, size, unused):
        """데모 반환/메모리 변경 입력 외에는 함수의 실제 명령만 실행한다."""
        if address == STOP: current.emu_stop(); return
        if address == DEMO:
            assert state['operation'] == 'Credit'; state['demo_calls'] += 1
            if state['variant']:
                write(PLAYERS + state['owner'] * 180 + 0xb0, SECOND)
                write(FIRST + 0x2d8, state['initial'] ^ 0x80000000)
            sp = current.reg_read(UC_X86_REG_ESP); target = read(sp)
            current.reg_write(UC_X86_REG_EAX, state['demo']); current.reg_write(UC_X86_REG_ESP, sp+4); current.reg_write(UC_X86_REG_EIP, target)
            return
        assert address in functions[state['operation']], hex(address)
        visited[state['operation']].add(address)

    mu.hook_add(UC_HOOK_CODE, hook)
    rows = []
    # 양수 실제 Player1/8과 비양수 자연 반환을 signed 감김·데모·저장 포인터 변경 입력에서 관측한다.
    for disabled, owner, brain, initial, amount, demo, variant in itertools.product([0, 7], [0, 1, 8, 0xffffffff, 0x80000000], [0, 1], [0, 1190, 1191, 0x7fffffff, 0x80000000, 0xffffffff], [0, 1, 0xffffffff, 0x80000000, 0x7fffffff], [0, 1], [0, 1]):
        rows.append(('Credit', disabled, owner, brain, initial, amount, demo, variant))
    # 잔액 함수는 데모를 호출하지 않는 원본 전체 경로를 독립적으로 관측한다.
    for disabled, owner, brain, initial in itertools.product([0, 7], [0, 1, 8, 0xffffffff, 0x80000000], [0, 1], [0, 1190, 1191, 0x7fffffff, 0x80000000, 0xffffffff]):
        rows.append(('Balance', disabled, owner, brain, initial, 0, 0, 0))
    output = []
    # 기대 결과는 C++/C# 계산기로 만들지 않고 실행 후 전체 배열과 호출 수만 기록한다.
    for operation, disabled, owner, brain, initial, amount, demo, variant in rows:
        mu.mem_write(PLAYERS, bytes(1620)); mu.mem_write(FIRST, bytes([0xa7])*0xb08); mu.mem_write(SECOND, bytes([0x5b])*0xb08)
        write(FIRST+0x2d8, initial); write(SECOND+0x2d8, 5000)
        if owner in (1, 8): write(PLAYERS+owner*180+0xb0, FIRST if brain else 0)
        write(0x594fc8, disabled); write(0x59534c, PLAYERS); write(STACK, STOP); write(STACK+4, owner); write(STACK+8, amount)
        state.update(operation=operation, owner=owner, initial=initial, demo=demo, variant=variant, demo_calls=0)
        mu.reg_write(UC_X86_REG_ESP, STACK); mu.reg_write(UC_X86_REG_ESI, 0xabcdef01); mu.reg_write(UC_X86_REG_EAX, 0x31415926)
        mu.emu_start(FUNCTIONS[operation][0], STOP, count=300)
        assert mu.reg_read(UC_X86_REG_EIP) == STOP and mu.reg_read(UC_X86_REG_ESP) == STACK+4 and mu.reg_read(UC_X86_REG_ESI) == 0xabcdef01
        output.append((operation, disabled, owner, brain, initial, amount, demo, variant, mu.reg_read(UC_X86_REG_EAX) if operation == 'Balance' else 0, state['demo_calls'], sha(bytes(mu.mem_read(PLAYERS, 1620))), sha(bytes(mu.mem_read(FIRST, 0xb08))), sha(bytes(mu.mem_read(SECOND, 0xb08)))))
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n'); writer.writerow(['# operation', 'disabled', 'owner', 'brain', 'initial', 'amount', 'demo', 'variant', 'balance', 'demo_calls', 'players_sha256', 'first_sha256', 'second_sha256']); writer.writerows(output)
    assert visited == functions and (ROOT / 'originals/Netstorm.exe').read_bytes() == original
    (OUT/'native-wallet-assembly.tsv').write_text('\n'.join(listings)+'\n', encoding='utf-8')
    summary = {'observations':len(output), 'operations':{name:sum(r[0]==name for r in output) for name in functions}, 'function_instructions':{name:len(addresses) for name,addresses in functions.items()}, 'all_instructions_visited':visited==functions, 'source_unchanged':True, 'source_sha256':sha(original), 'fixture_sha256':sha(FIXTURE.read_bytes()), 'limits':['Player9 물리 계약 안의 owner1/8과 비양수 관문이며 범위 밖 호스트 예외는 별도 검사한다.', '데모463b50은 명시 반환/변경 입력 경계이며 게임/OS 프로세스를 실행하지 않는다.']}
    (OUT/'native-wallet-summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2)+'\n', encoding='utf-8'); print(json.dumps(summary, ensure_ascii=False))


if __name__ == '__main__': main()
