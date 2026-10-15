#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""로컬 미션 Zone/로그인 작성과 Zone 설치의 원본 기계어만 격리 관찰한다."""
import csv
import hashlib
import json
import socket
import struct
import sys
from pathlib import Path

# 보호 PE와 같은 PC의 전체 정밀 범위 외 코드는 실행하지 않는다.
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

# 게임 주소/OS와 분리한 이름·이미지·복제·Player·스택 저장소다.
TEXT, IMAGE, HEAP, PLAYERS, TABLE, ZONE, STACK, STOP = 0x12000000, 0x12100000, 0x12200000, 0x12300000, 0x12400000, 0x12500000, 0x3007f000, 0x20000000


def digest(path):
    """현재 근거 파일의 전체 바이트 SHA를 구한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Oracle:
    """작성/설치 판단은 실제 명령을 쓰고 파일·설정·힙·전송만 명시 입력으로 공급한다."""
    def __init__(self):
        """현재 PC 정밀 전체 함수표로 검토한 함수의 불연속 범위를 확인한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32)
        pe = pefile.PE(str(ROOT / 'originals/Netstorm.exe'));image = pe.get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095);self.mu.mem_write(0x400000, image)
        self.mu.mem_map(0, 0x1000)
        for address in (TEXT, IMAGE, HEAP, PLAYERS, TABLE, ZONE):self.mu.mem_map(address, 0x100000)
        self.mu.mem_map(STACK - 0x7f000, 0x80000);self.mu.mem_map(STOP, 0x1000)
        selected = {0x4b51e0, 0x4b63d0, 0x4b4be0, 0x48f3b0, 0x4df590, 0x40a220, 0x4dece0, 0x40a1e0, 0x40a450, 0x40a4b0, 0x40a580, 0x4e4a40, 0x4df740, 0x4df240, 0x4df010}
        self.allowed = []
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        for address in selected:
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.hook_add(UC_HOOK_CODE, self.instruction)

    def put(self, address, value, width=4):
        """원본 정수폭의 낮은 바이트만 격리 메모리에 쓴다."""
        self.mu.mem_write(address, (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little'))

    def words(self, address, count=1):
        """정렬되지 않은 스택/원본 DWORD를 읽는다."""
        return struct.unpack('<' + 'I' * count, self.mu.mem_read(address, count * 4))

    def string(self, address):
        """실제 NUL 종료 바이트까지 읽고 범위 없는 문자열은 거부한다."""
        result = bytearray()
        for offset in range(4096):
            byte = self.mu.mem_read(address + offset, 1)[0]
            if not byte:return bytes(result)
            result.append(byte)
        raise ValueError('NUL 없는 원본 문자열')

    def ret(self, value=0, purge=0):
        """외부 경계의 원래 호출 규약으로 반환한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP);target = self.words(esp)[0]
        self.mu.reg_write(UC_X86_REG_EAX, value);self.mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge);self.mu.reg_write(UC_X86_REG_EIP, target)

    def instruction(self, mu, address, size, data):
        """검토하지 않은 진입은 실패시키고 실제 패킷 관찰기를 원래 쓰기 전에 호출한다."""
        esp = mu.reg_read(UC_X86_REG_ESP);ecx = mu.reg_read(UC_X86_REG_ECX);args = self.words(esp + 4, 8)
        if address == 0x45ad00:self.ret();return
        if address == 0x4e0940:self.ret(TEXT + 0x9000);return
        if address == 0x4e59a3:self.ret();return
        if address == 0x4e49c0:self.now += 1;self.events.append('Now,' + str(self.now));self.ret(self.now);return
        if address == 0x4dec30:
            pointer = args[3];self.events.append(f'Record,{self.words(pointer + 1)[0] & 255},{args[4] & 65535},{args[2]}');self.ret();return
        if address == STOP + 0x200:
            connection, pointer = args[1:3];self.events.append(f'Receive,{args[0]},{self.words(pointer + 1)[0] & 255},{self.words(connection + 0x8c)[0]}')
            if self.mutated is False and self.image_size:self.put(connection + 0x40, 1);self.mutated = True
            self.ret();return
        if address == 0x4df9f0:self.events.append('Terminate');self.put(ecx + 0x34, self.words(ecx + 0x34)[0] | 0x40);self.ret();return
        if address == 0x4e03d0:self.events.append('Close');self.ret();return
        if address == 0x460980:self.events.append('Fort,' + self.string(args[0]).hex());self.ret();return
        if address == 0x4bd2e0:self.events.append('Template');self.ret(ecx);return
        if address == 0x460680:self.events.append('FortName');self.ret(TEXT);return
        if address == 0x4bf550:self.events.append('Load,' + self.string(args[0]).hex());self.ret();return
        if address == 0x4bf3e0:
            for offset, value in enumerate((IMAGE, 0, self.image_size, self.image_size)):self.put(args[0] + offset * 4, value)
            self.events.append('Image,' + str(self.image_size));self.ret();return
        if address == 0x4bf690:self.events.append('TemplateFree');self.ret();return
        if address == 0x4df590:self.packet_address = args[4];self.reserve = args[3]
        if address == 0x4e4391:self.events.append('Allocate,' + str(args[0]));self.ret(HEAP);return
        if address == 0x4e4354:self.events.append('Free' if self.kind.startswith('Batch') else 'FreeImage');self.ret();return
        if address == 0x4e0620:self.events.append('Assert,' + str(args[2]));self.ret();return
        if address == 0x4e2dc0:
            assert args[0] == 9;self.send_packet = args[1];self.events.append('Send,9')
            self.put(esp - 8, STOP + 0x100);self.put(esp - 4, 1)
            mu.reg_write(UC_X86_REG_ECX, args[1]);mu.reg_write(UC_X86_REG_ESP, esp - 8);mu.reg_write(UC_X86_REG_EIP, 0x40a450);return
        if address == STOP + 0x100:
            self.sent = bytes(mu.mem_read(self.send_packet, int.from_bytes(mu.mem_read(self.send_packet + 2, 2), 'little')));self.ret();return
        if address == 0x441090:
            assert self.string(args[0]) == b'loginTimeout';self.events.append('Timeout');self.ret(30);return
        if address == 0x460d70:self.events.append('Time');self.ret(TEXT + 0x800);return
        if address == 0x42b0e0:
            self.events.append('Name,' + str(args[0]) + ',' + self.string(args[1]).hex())
            if self.kind == 'InstallMutate' and not self.mutated:self.put(ZONE + 16, 9);self.put(ZONE + 14, 2, 1);self.mutated = True
            self.ret();return
        if address == 0x4e2430:self.events.append('Serve,' + str(args[0]));self.ret();return
        if not any(low <= address < high for low, high in self.allowed):raise RuntimeError(f'미검토 로컬 미션 명령 {address:08x}')
        self.instructions += 1
        if self.instructions > 500000:raise RuntimeError('원본 로컬 미션 명령 상한')

    def run(self, kind, seed, name_size, image_size):
        """정상 입력과 선언 길이 경계에서 전체 작성 바이트/설치 기록/효과를 보존한다."""
        self.events = [];self.instructions = 0;self.reserve = 0;self.sent = b'';self.image_size = image_size;self.packet_address = 0;self.kind = kind;self.mutated = False;self.now = 104
        self.mu.mem_write(TEXT, b'mission.fort\0');self.mu.mem_write(IMAGE, bytes((index * 17 + seed) & 255 for index in range(image_size)))
        self.mu.mem_write(PLAYERS, b'\0' * 1620);self.put(0x59534c, PLAYERS);self.put(0x595344, 0);self.put(0x540c70, 8);self.put(0x540cac, 8);self.put(0x540c74, 1);self.put(0x594fc8, 0);self.put(0x540a2c, 0)
        self.put(0x5e477c, TABLE);self.put(TABLE + 0x1a * 4, 0x40a1e0);self.put(TABLE + 0x27 * 4, 0x40a450);self.put(0x544c30, 0);self.put(0x544c44, 0);self.put(0x5c85c0, seed);self.put(0x5c85e4, seed + 3);self.put(0x5318e8, 6799)
        self.mu.mem_write(TEXT + 0x800, struct.pack('<d', 100.5));self.mu.mem_write(0x5c8618, struct.pack('<d', 0))
        name = b'N' * name_size;self.mu.mem_write(TEXT + 0x1000, name + b'\0');self.put(0x540ca8, TEXT + 0x1000)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):self.mu.reg_write(register, 0x76543210 + register)
        args = [TEXT] if kind == 'Zone' else [ZONE] if kind.startswith('Install') else []
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<' + 'I' * (1 + len(args)), STOP, *args))
        if kind.startswith('Install'):
            zone = bytearray(b'\xff' * 512);struct.pack_into('<BBHIHI', zone, 0, 1, 0x1a, 86, 0x82000004, 0, 0);zone[14] = 1;zone[15] = 0;struct.pack_into('<II', zone, 16, 1, seed & 1);zone[24:24 + name_size + 1] = name + b'\0';zone[56] = 0;self.mu.mem_write(ZONE, bytes(zone));self.put(PLAYERS + 180 + 0x50, seed << 12)
            if kind == 'InstallMutate':
                self.put(ZONE + 86, 1, 1);self.put(ZONE + 87, 42);self.put(ZONE + 91, 0);self.mu.mem_write(ZONE + 95, b'Second\0');self.put(ZONE + 127, 0, 1)
        if kind.startswith('Batch'):
            packet = bytearray(b'\xff' * 19);struct.pack_into('<BBH', packet, 0, seed, 0x27, 19);struct.pack_into('<I',packet,8,1);packet[12] = 1;struct.pack_into('<I',packet,13,1);packet[17] = 0;packet[18] = 7
            frame = bytearray(struct.pack('<BBH',255,4,28) + packet + bytes([1,0x24,5,0,77]));self.mu.mem_write(ZONE,bytes(frame));self.mu.mem_write(HEAP,b'\0' * 0x94)
            self.put(HEAP + 0x60,ZONE);self.put(HEAP + 0x34,0x64 if kind=='BatchClosed' else 0x24);self.put(HEAP + 0x40,0xffffffff);self.put(HEAP + 0x44,103);self.put(HEAP + 0x48,104)
            self.put(0x5e4778,TABLE + 0x4000);self.put(TABLE + 0x4000 + 0x27 * 4,STOP + 0x200);self.put(TABLE + 0x4000 + 0x24 * 4,STOP + 0x200)
            self.put(0x5e4780,TABLE + 0x5000);self.put(TABLE + 0x5000 + 0x27 * 2,1,1);self.put(TABLE + 0x5000 + 0x27 * 2 + 1,name_size,1);self.put(TABLE + 0x5000 + 0x24 * 2,1,1)
            self.mu.reg_write(UC_X86_REG_ECX,HEAP)
        self.mu.emu_start(0x4df740 if kind.startswith('Batch') else 0x4b4be0 if kind.startswith('Install') else {'Zone':0x4b51e0, 'Login':0x4b63d0}[kind], STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        for register in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):assert self.mu.reg_read(register) == 0x76543210 + register
        if kind.startswith('Batch'):
            storage = bytes(self.mu.mem_read(ZONE,28));saved = b'';state = [self.mu.reg_read(UC_X86_REG_EAX)] + [self.words(HEAP + a)[0] for a in (0x34,0x40,0x44,0x48,0x8c)]
        elif kind.startswith('Install'):
            storage = bytes(self.mu.mem_read(PLAYERS, 1620));saved = b''
            state = [self.words(a)[0] for a in (0x540c70, 0x540cac, 0x595344, 0x540bc0, 0x540bc4, 0x5c8540, 0x5c8544, 0x5c86c0)] + [int.from_bytes(self.mu.mem_read(0x5c8618, 8), 'little')]
        else:
            storage = bytes(self.mu.mem_read(self.packet_address, self.reserve));saved = bytes(self.mu.mem_read(HEAP, int.from_bytes(storage[2:4], 'little'))) if kind == 'Zone' else self.sent
            state = [self.words(a)[0] for a in (0x5c8878, 0x5c85c0)] if kind == 'Zone' else []
        return [kind, f'{seed},{name_size},{image_size}', storage.hex(), saved.hex() or '-', ','.join(map(str,state)) or '-', ';'.join(self.events) or '-']


def cases():
    """이미지 바이트/이름/기존 상태의 서로 다른 입력을 한 번씩만 공급한다."""
    for seed in (0, 7):yield 'Zone', seed, 0, 0
    for seed in (0, 7):
        for name in (0, 8, 31, 49):
            for size in (1, 7, 256):yield 'Login', seed, name, size
    for seed in (0, 7):
        for name in (0, 8, 30):yield 'Install', seed, name, 0
        yield 'InstallMutate', seed, 8, 0
    for version in (0, 1, 2):
        for policy in (0, 1, 2, 4, 8):
            for mutate in (0, 1):yield 'Batch',version,policy,mutate
    for mutate in (0, 1):yield 'BatchClosed',1,0,mutate


def main():
    """기존 원본은 읽기만 하고 전체 관찰/현재 소스 SHA를 생성하거나 검증한다."""
    fixture = ROOT / 'cpppj/tests/fixtures/local-mission-transport-x86.tsv';report = ROOT / 'cpppj/local-mission-transport.json'
    sources = ['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_transport.py','cpppj/src/zacket/LocalMissionTransport.h','cpppj/src/zacket/LocalMissionTransport.cpp','cpppj/tests/LocalMissionTransportTests.cpp','cpppj/tests/fixtures/local-mission-transport-x86.tsv']
    if '--verify' in sys.argv:
        facts = json.loads(report.read_text(encoding='utf-8'))
        for path, sha in facts['sha256'].items():assert digest(ROOT / path) == sha,path
        with fixture.open(encoding='utf-8') as stream:rows = list(csv.DictReader(stream, delimiter='\t'))
        assert len(rows) == facts['nativeUniqueInputs'] == len({(row['kind'],row['input']) for row in rows});print('로컬 미션 원본 관찰/SHA 검증:',len(rows));return
    oracle = Oracle();rows = [oracle.run(*case) for case in cases()]
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer = csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['kind','input','storage','saved','state','trace']);writer.writerows(rows)
    facts = {'host':socket.gethostname(),'date':'2026-10-15','nativeUniqueInputs':len(rows),'status':'native-observed-cpp-validation-pending','boundaries':['전체 정밀 함수표의 검토한 불연속 범위만 실행; 게임/OS 진입 없음; 정상 cdecl스택/EBX·ESI·EDI·EBP 보존 검사','Fort 경로 선택/Template 파일 읽기/이미지 공급/Template 정리/할당/전송/로그인 설정/시간/Chat 이름 등록/서버 시작은 명시 외부 경계','Zone512 전체 stack과 observer후86BYTE 복제,0x27 전체 예약/observer후송신,Zone설치 전체1620BYTE/현재전역/효과순서','004df740 집합분배의 실제검증/버전/observer/즉시depth 몸체 실행; 시계·기록·등록receive·종료·반납은 명시 외부 효과','원본0x27 handler004a7800/Socket/집합 송신버퍼 전체는 이 native 관찰 범위가 아님'],'sha256':{path:digest(ROOT/path) for path in sources if (ROOT/path).is_file()}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('로컬 미션 원본 고유 입력:',len(rows))


if __name__ == '__main__':main()
