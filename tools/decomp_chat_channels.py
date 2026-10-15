# -*- coding: utf-8 -*-
"""PATCH Chat Add/Clear·이름정제·빈 refresh를 격리 실행한다. 원본 게임/GUI는 실행하지 않는다."""
import csv
import itertools
import json
import struct
import sys
from pathlib import Path

# 기존 보호 PE 격리 메모리와 동일 PC의 전체 정밀 범위를 재사용한다.
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, TEXT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

# 원본 Chatmanager의 실제 정적100×48바이트와 실제 현재 subscriber 주소다.
RECORDS, SIZE, CURRENT = 0x558ed0, 4800, 0x540c74
FIXTURE = ROOT / 'cpppj/tests/fixtures/chat-channels-x86.tsv'  # 전체 원문기록/순서/반환 기대값이다.
REPORT = ROOT / 'cpppj/chat-channels.json'  # 새 native 관찰과 제품 미빌드 상태를 구분한다.


def initial(scene):
    """검색 결과를 계산하지 않고 독립된100기록과 stale BYTE 패턴을 준비한다."""
    data = bytearray((index * 37 + scene * 11) & 255 for index in range(SIZE))
    # 모든 기록의 이름/식별자/flags를 명시 입력으로 쓴다. 나머지 stale 바이트는 보존한다.
    for index in range(100):
        record = index * 48
        data[record:record + 40] = b'OldName\0' + bytes([65 + index % 26]) * 32
        data[record + 40] = (index * 13 + 0x95) & 255
        value = 0 if scene in (0, 5) else 1000 + index
        struct.pack_into('<I', data, record + 44, value)
    # 기존 항목·앞선 빈칸·마지막 빈칸·중복·-1·0 식별자의 실제 검색 경계를 교차한다.
    if scene == 1:struct.pack_into('<I', data, 7 * 48 + 44, 77)
    if scene == 2:
        struct.pack_into('<I', data, 2 * 48 + 44, 0)
        struct.pack_into('<I', data, 7 * 48 + 44, 77)
    if scene == 3:struct.pack_into('<I', data, 99 * 48 + 44, 0)
    if scene == 4:
        struct.pack_into('<I', data, 7 * 48 + 44, 77)
        struct.pack_into('<I', data, 98 * 48 + 44, 77)
    if scene == 5:struct.pack_into('<I', data, 0 * 48 + 44, 0xffffffff)
    return bytes(data)


def inputs():
    """여덟 실제 Add 인자 중 읽는 세 인자와 현재 자기 subscriber를 독립 교차한다."""
    names = (None, b'', b'Name', b'A' * 39, b'B' * 40, b'C' * 41, b'A %.?=()[]~"`;: B\xa0', b'%~ ')
    # 자기반환·subscriber0/음수·기존/가득찬100기록·BYTE초과flags를 포함한다.
    for scene, sub, current, flags, name in itertools.product(range(6), (0, 1, 77, 0xffffffff, 0x80000080), (1, 77), (0, 0x10, 0x40, 0x80, 0x1ff), names):
        yield ('A', scene, sub, current, flags, '-' if name is None else name.hex())
    # Clear는0기록의stale flags를 보존하고높은mask비트를무시한다.
    for scene, mask in itertools.product(range(6), (0, 1, 0x10, 0x40, 0x80, 0xff, 0x100, 0x1ff)):
        yield ('C', scene, 0, 1, mask, '-')


class ChatOracle(MemoryOracle):
    """표선택·실제문자정제는 PE명령이며 CRT문자복사/검색/서식만 명시 대체한다."""
    def __init__(self):
        """같은 PC 전체 정밀 함수표의 네몸체만 허용한다."""
        super().__init__()
        self.allowed = []  # 원본 함수 밖 미검토 명령/OS 진입을 거부한다.
        self.calls = {}  # 실제 Add/Clear/이름정제/refresh 진입 횟수다.
        self.crt = {}  # 명시 대체한 CRT 주소별 호출 수다.
        self.events = []  # 개별 입력의 이름정제와 refresh 진입 순서다.
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            # 불연속 함수 범위도 원문 그대로 사용한다.
            for row in csv.DictReader(stream, delimiter='\t'):
                if int(row['entry'], 16) in (0x444ee0, 0x445080, 0x42b0e0, 0x4437f0):
                    for part in row['ranges'].split(';'):
                        low, high = (int(value, 16) for value in part.split('-'))
                        self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)

    def guard(self, mu, access, address, size, value, data):
        """명시 기록/이름정제 문자표/함수 스택 밖 쓰기는 숨기지 않고 거부한다."""
        if not (RECORDS <= address and address + size <= RECORDS + SIZE or
                0x531750 <= address and address + size <= 0x531752 or
                STACK - 4096 <= address and address + size <= STACK + 40):
            raise RuntimeError(f'Chat 관찰 범위 밖 쓰기 {address:08x}/{size}')

    def instruction(self, mu, address, size, data):
        """실제 원문 명령과 외부 CRT 대체를 별도로 기록한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        args = self.words(esp + 4, 3)
        if address in (0x444ee0, 0x445080, 0x42b0e0, 0x4437f0):
            self.calls[address] = self.calls.get(address, 0) + 1
            if address == 0x42b0e0:self.events.append('N,' + str(args[0]))
            if address == 0x4437f0:self.events.append('R')
        if address in (0x4e51f0, 0x4e58e9, 0x4e63e0, 0x4e5b40, 0x4a89c0):
            self.crt[address] = self.crt.get(address, 0) + 1
            if address == 0x4e51f0:
                value = self.string(args[1])[:args[2]];mu.mem_write(args[0], value + bytes(args[2] - len(value)));self.ret(args[0])
            elif address == 0x4e58e9:
                if self.string(args[1]) != b'Unknown%08x':raise RuntimeError('미검토 Chat sprintf 서식')
                value = ('Unknown%08x' % args[2]).encode('ascii');mu.mem_write(args[0], value + b'\0');self.ret(len(value))
            elif address == 0x4e63e0:
                value = self.string(args[0]) + b'\0';position = value.find(bytes([args[1] & 255]));self.ret(0 if position < 0 else args[0] + position)
            elif address == 0x4e5b40:
                mu.mem_write(args[0], bytes(mu.mem_read(args[1], args[2])));self.ret(args[0])
            else:
                if self.string(args[1]) != b'Cheat%d':raise RuntimeError('미검토 Chat 이름 서식')
                value = ('Cheat%d' % args[2]).encode('ascii');mu.mem_write(args[0], value + b'\0');self.ret(args[0])
            return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'미검토 Chat 명령 {address:08x}')

    def sample(self, case):
        """초기 입력을 기록한 뒤 실제 정상 반환/스택/보존 레지스터와전체4800바이트를 읽는다."""
        kind, scene, subscriber, current, flags, name = case
        self.events = [];self.mu.mem_write(RECORDS, initial(scene));self.put(CURRENT, current)
        self.mu.mem_write(TEXT, (b'' if name == '-' else bytes.fromhex(name)) + b'\0')
        # 사용되지않는5개인자에도서로다른비영패턴을넣어실사용스택위치를확인한다.
        args = (subscriber, 0 if name == '-' else TEXT, 0x12345678, 0x87654321, 0xabcdef01, flags, 0xfedcba98, 0x55555555) if kind == 'A' else (flags,)
        preserved = {UC_X86_REG_EBX:0x11112222, UC_X86_REG_ESI:0x33334444, UC_X86_REG_EDI:0x55556666, UC_X86_REG_EBP:0x77778888}
        # 호출 규약의 보존 대상4개를 정상 반환 직후 비교한다.
        for register, value in preserved.items():self.mu.reg_write(register, value)
        self.mu.reg_write(UC_X86_REG_EAX, 0xccccdddd);self.mu.reg_write(UC_X86_REG_ESP, STACK)
        self.mu.mem_write(STACK, struct.pack('<' + 'I' * (len(args) + 1), STOP, *args))
        self.mu.emu_start(0x444ee0 if kind == 'A' else 0x445080, STOP, count=100000)
        if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4:raise RuntimeError('Chat 반환/스택 오류')
        for register, value in preserved.items():
            if self.mu.reg_read(register) != value:raise RuntimeError('Chat 보존 레지스터 오류')
        result = str(struct.unpack('<i', struct.pack('<I', self.mu.reg_read(UC_X86_REG_EAX)))[0]) if kind == 'A' else '-'
        before = initial(scene);after = bytes(self.mu.mem_read(RECORDS, SIZE))
        # 전체4800바이트를읽고다른바이트만손실없이직렬화한다. 검사는원래입력에적용해전체를비교한다.
        changes = ','.join(f'{offset}:{byte:02x}' for offset, byte in enumerate(after) if byte != before[offset])
        return result, changes or '-', ';'.join(self.events) or '-'


def main():
    """실제 고유입력과 전체 원문출력을 보존한다. 제품 빌드는 root의 공용 단계에서 수행한다."""
    if sys.argv[1:] == ['--verify']:
        facts = json.loads(REPORT.read_text(encoding='utf-8'))
        # 현재 보호PE/동일PC범위/도구/제품/검사의바이트를감사한다.
        for name, expected in facts['sha256'].items():
            if digest(ROOT / name) != expected:raise RuntimeError('Chat SHA 불일치: ' + name)
        rows = list(csv.reader(FIXTURE.open(encoding='utf-8'), delimiter='\t'))[1:]
        if len(rows) != facts['observations'] or len({tuple(row[:6]) for row in rows}) != len(rows):raise RuntimeError('Chat 고유입력 수 오류')
        print('Chat native/SHA verified:', len(rows));return
    if sys.argv[1:]:raise SystemExit('사용법: decomp_chat_channels.py [--verify]')
    oracle = ChatOracle();rows = []
    # 원본기대값은C++규칙으로계산하지않고매입력의실제출력을읽는다.
    for case in inputs():rows.append((*case, *oracle.sample(case)))
    with FIXTURE.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n');writer.writerow(('kind','scene','subscriber','current','flags','nameHex','result','changes','calls'));writer.writerows(rows)
    sources = ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_chat_channels.py','tools/decomp_priestmenu.py','cpppj/src/client/RawChatChannels.h','cpppj/src/client/RawChatChannels.cpp','cpppj/tests/ChatChannelsTests.cpp','cpppj/tests/fixtures/chat-channels-x86.tsv')
    facts = {'host':'HJOW-Athlon','date':'2026-10-15','status':'native-observed-product-unbuilt','observations':len(rows),'native_calls':{f'{address:08x}':count for address,count in oracle.calls.items()},'explicit_crt_substitutions':{f'{address:08x}':count for address,count in oracle.crt.items()},'normal_return':True,'game_gui_os_calls':0,'sha256':{name:digest(ROOT/name) for name in sources if (ROOT/name).exists()}}
    REPORT.write_text(json.dumps(facts, ensure_ascii=False, indent=2)+'\n',encoding='utf-8');print('Chat native observations:',len(rows))


if __name__ == '__main__':main()
