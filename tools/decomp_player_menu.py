#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH Player 메뉴 전체와 실제 Identity/사제/번호/SP 비교 helper를 격리 실행한다."""
import csv
import hashlib
import json
import random
import re
import struct
import sys
from pathlib import Path
from decomp_player_identity import Oracle as IdentityOracle, ROOT, POOL, PLAYERS, META, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 Player 메뉴/두 정적 문자열/방향별 동맹/숨김/사제 상태 주소다.
ENTRY, MISSION_TEXT, LOBBY_TEXT, ALLIES, HIDDEN, PRIESTS = 0x4c9700, 0x5cad98, 0x5cae00, 0x595200, 0x52f81e, 0x595428
# 독립13열 입력의 의미다.
TARGET, MODES, FLAGS, PRIEST, ALLIED, HIDE, ALLOW, MONEY, IGNORED, WHISPER, GAMES, ADDR, MUTATE = range(13)


def bits(value):
    """SP float의 원문을 보존한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def digest(path):
    """실제 근거/도구/fixture의 바이트 SHA다."""
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


class Oracle(IdentityOracle):
    """Player 부모의 실제 제어 흐름과 남은 의존 경계만 구분한다."""
    def __init__(self):
        """현재 호스트의 같은 보호PATCH 이미지/함수 표를 사용한다."""
        super().__init__('originals')
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as fp:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(fp, delimiter='\t')}
        # 실제 부모·사제/번호·동맹·지불 가능 helper를 실행한다.
        for address in [ENTRY, 0x4922e0, 0x492340, 0x48f550, 0x48f790, 0x425e90]:
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value, 16) for value in part.split('-'))
                self.allowed.append((low, high + 1))

    def instruction(self, mu, address, size, data):
        """원본 lookup/분기/인자 조립은 실행하고 GUI·목록/주소/신뢰도·SP만 관찰한다."""
        if address == ENTRY:
            self.instructions += 1;return
        esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
        args = struct.unpack('<IIIIIII', mu.mem_read(esp + 4, 28))
        if address == 0x4e0cd0:
            self.emit('A', self.string(args[0]).hex(), args[1]);mu.mem_write(ecx, bytes(20));self.ret(purge=8)
        elif address == 0x478310:
            self.emit('R', args[0])
            if self.values[MUTATE] & 8:
                self.put(0x594fc8, 0)
            self.ret(90 + args[0])
        elif address == 0x42e390:
            self.emit('G', args[0]);self.ret(self.values[IGNORED])
        elif address == 0x42f640:
            self.emit('W', args[0]);self.ret(self.values[WHISPER])
        elif address == 0x40ec50:
            self.emit('$', args[0]);self.put(args[1], self.values[MONEY])
            if self.values[MUTATE] & 4:
                self.put(PLAYERS + self.owner * 180 + 0x50, 0, 1);self.put(0x52f7ed, 0, 1)
            self.ret()
        elif address == 0x4e0620:
            text = self.string(args[0]);self.emit('!', 1 if text == b'!inMetamapMode' else 0);self.ret()
        elif address == 0x4de9a0:
            text = self.string(args[0]);self.emit('T', text.hex())
            if self.values[MUTATE] & 1 and text == b'Allied with ~i%s~.':
                self.text(PLAYERS + 3 * 180 + 12, b'Changed')
            if self.values[MUTATE] & 2 and text == b'Version: ~i%d.%d~.\n':
                self.put(META + self.owner * 460 + 0xb0, 9, 2)
            self.ret(args[0], 4)
        elif address in (0x4a89c0, 0x4a8a00):
            if address == 0x4a8a00:
                self.text(args[0], self.string(args[0]) + self.string(args[1]));self.ret(args[0]);return
            pattern = self.string(args[1]).decode('ascii');values = [];index = 2
            # 현재 고정 printf 서식의 실제%s/%d/%% 순서를 보존한다.
            for match in re.finditer('%[%sd]', pattern):
                if match[0] == '%%':
                    continue
                word = args[index];index += 1;values.append(self.string(word).decode('ascii') if match[0] == '%s' else struct.unpack('<i', struct.pack('<I', word))[0])
            self.text(args[0], (pattern % tuple(values)).encode('ascii'));self.ret(args[0])
        elif address in (0x477ae0, 0x477bb0, 0x478260):
            if address == 0x477bb0:
                assert args[0] == 0
            self.emit({0x477ae0: 'F', 0x477bb0: 'V', 0x478260: 'X'}[address]);self.ret()
        elif address in (0x477ee0, 0x478020):
            self.emit('D' if address == 0x477ee0 else 'H', self.string(args[0]).hex());self.ret()
        elif address == 0x477cb0:
            assert args[5] == 0
            self.emit('I', self.string(args[0]).hex(), *args[1:5]);self.ret()
        elif address == 0x477dd0:
            assert args[4:6] == (1, 1)
            self.emit('C', args[0], self.string(args[1]).hex(), args[2], args[3]);self.ret()
        elif address == 0x475c00:
            self.taken += 1;self.emit('K', 100 + self.taken);self.ret(100 + self.taken)
        elif address == 0x478160:
            assert args[2:4] == (1, 0)
            self.emit('S', self.string(args[0]).hex(), args[1]);self.ret()
        else:
            super().instruction(mu, address, size, data)

    def run(self, v, precision):
        """독립 현재 자료를 설정하고 실제 부모 정상 반환/문자열 전체/효과 순서를 읽는다."""
        self.values = v  # 독립13개 입력이다.
        self.owner = 1 if v[TARGET] == 0 else 2  # 현재 입력이 채울 raw target 행이다.
        self.target = 0xfedcba98 if v[TARGET] == 0 else 0x80000002 if v[TARGET] == 1 else 0xdeadbeef  # 실제 subscriber DWORD다.
        self.taken = 0  # 명시 GUI 스택 경계가 반환할 토큰 순번이다.
        identityModes = (v[MODES] & 1) | (2 if v[MODES] & 8 else 0) | (v[MODES] & 4)
        super().prepare([identityModes, 3, 1, 0xfedcba98, 0xc0a8005e, b'LocalUser'.hex(), 0, 0, 0, '-', 0, 0, 2])
        self.put(0x594fa4, v[MODES] & 2);self.put(0x594fa0, v[MODES] & 16)
        for address, bit in [(0x52f7ed, 1), (0x52f7eb, 2), (0x52f7ec, 4)]:
            self.put(address, int(bool(v[ALLOW] & bit)), 1)
        self.mu.mem_write(ALLIES, bytes(81 * 4));self.mu.mem_write(PRIESTS, bytes(9 * 4))
        for owner in range(9):
            self.put(HIDDEN + owner, int(bool(v[HIDE] & (1 << owner))), 1)
        self.put(PLAYERS + 180 + 44, 0xfedcba98);self.text(PLAYERS + 180 + 12, b'Local1');self.put(PLAYERS + 180 + 0x50, 0x23, 1)
        self.put(PLAYERS + 3 * 180 + 44, 0x80000003);self.text(PLAYERS + 3 * 180 + 12, b'Buddy');self.put(PLAYERS + 3 * 180 + 0x50, 0x23, 1)
        self.put(PLAYERS + self.owner * 180 + 0x50, v[FLAGS], 1);self.put(PRIESTS + self.owner * 4, v[PRIEST]);self.put(PLAYERS + 180 + 0x54, 1 << self.owner if v[ALLIED] & 16 else 0)
        # 서로 다른 두 방향과 마지막 Player8 후보를 독립 설정한다.
        for source, target, bit in [(self.owner, 3, 1), (3, self.owner, 2), (self.owner, 8, 4), (8, self.owner, 8)]:
            self.put(ALLIES + (source * 9 + target) * 4, int(bool(v[ALLIED] & bit)))
        for index, sub, name in [(1, 0xfedcba98, b'LocalUser'), (2, 0x80000002, b'Meta2')]:
            row = META + index * 460;self.put(row, index);self.put(row + 4, sub);self.text(row + 38, name);self.put(row + 100, 1, 1);self.put(row + 176, 1, 2);self.put(row + 178, 78, 2);self.put(row + 84, v[GAMES], 2);self.text(row + 416, b'udp,127.0.0.1:6800')
        if v[ADDR]:
            self.text(PLAYERS + self.owner * 180 + 48, b'zlocal,127.0.0.1:6800')
        self.mu.mem_write(MISSION_TEXT, bytes([0xa5]) * 104);self.text(MISSION_TEXT, b'OLD_MISSION');self.mu.mem_write(LOBBY_TEXT, bytes([0x5a]) * 104);self.text(LOBBY_TEXT, b'OLD_LOBBY')
        saved = {UC_X86_REG_EBX: 0x123456, UC_X86_REG_ESI: 0x234567, UC_X86_REG_EDI: 0x345678, UC_X86_REG_EBP: 0x456789}
        for register, value in saved.items():
            self.mu.reg_write(register, value)
        control = 0x027f if precision == 53 else 0x037f
        self.mu.reg_write(UC_X86_REG_FPCW, control);self.mu.reg_write(UC_X86_REG_FPTAG, 0xffff);self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.mem_write(STACK, struct.pack('<IIII', STOP, self.target, 1, 0))
        self.mu.emu_start(ENTRY, STOP, count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP) == STOP and self.mu.reg_read(UC_X86_REG_ESP) == STACK + 4
        assert all(self.mu.reg_read(register) == value for register, value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW) == control
        return str(self.mu.reg_read(UC_X86_REG_EAX)), bytes(self.mu.mem_read(MISSION_TEXT, 208)).hex(), ';'.join(self.events) or '-'


def cases():
    """예상 메뉴를 만들지 않고 모든 모드와 주요 분기/재진입을 고정 입력으로 만든다."""
    base = (1, 3, 0x23, 0, 0, 0, 7, bits(1000), 0, 0, 120, 0, 0);result = [base]
    for target in range(3):
        for mode in range(32):
            row = list(base);row[TARGET] = target;row[MODES] = mode;result.append(tuple(row))
    for column, options in {FLAGS: [0, 1, 2, 0x20, 0x21], PRIEST: [1, 2], ALLIED: range(32), HIDE: [2, 4, 6], ALLOW: range(8), MONEY: [bits(0), bits(299), bits(300), bits(699), bits(700), bits(float('nan'))], IGNORED: [1], WHISPER: [1, 128, 255], GAMES: [0, 32699, 32700, 65535], ADDR: [1], MUTATE: [1, 2, 4, 8, 15]}.items():
        for value in options:
            row = list(base);row[column] = value;result.append(tuple(row))
    rng = random.Random(ENTRY)
    for index in range(128):
        result.append((rng.randrange(2), rng.randrange(32), rng.choice([0, 1, 2, 0x21, 0x23]), rng.randrange(3), rng.randrange(32), rng.randrange(8), rng.randrange(8), rng.choice([bits(0), bits(700), bits(1000), bits(float('nan'))]), rng.randrange(2), rng.choice([0, 1, 128]), rng.choice([120, 65535]), rng.randrange(2), rng.randrange(16)))
    return list(dict.fromkeys(result))


def main():
    """새 원문/버퍼/효과를 보존하고 현재 바이트 계보를 감사한다."""
    reportpath = ROOT / 'cpppj/player-menu.json';fixture = ROOT / 'cpppj/tests/fixtures/player-menu-x86.tsv'
    if '--verify' in sys.argv:
        report = json.loads(reportpath.read_text(encoding='utf-8'))
        for hashes in [report['sha256'], report.get('product_validation', {}).get('sha256', {})]:
            for path, expected in hashes.items():
                assert digest(path) == expected, f'Player 메뉴 SHA 불일치: {path}'
        with fixture.open(encoding='utf-8') as fp:
            rows = list(csv.reader(fp, delimiter='\t'))[1:]
        assert len(rows) == report['observations'] == len({tuple(row[:-3]) for row in rows})
        print(json.dumps({'verified': True, 'observations': len(rows)}));return
    oracle = Oracle();rows = []
    for values in cases():
        observed = oracle.run(values, 53);assert observed == oracle.run(values, 64), values;rows.append([*values, *observed])
    with fixture.open('w', encoding='utf-8', newline='') as fp:
        writer = csv.writer(fp, delimiter='\t', lineterminator='\n');writer.writerow(['target', 'modes', 'flags', 'priest', 'allied', 'hide', 'allow', 'money', 'ignored', 'whisper', 'games', 'address', 'mutation', 'result', 'buffers', 'trace']);writer.writerows(rows)
    paths = ['originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv', 'tools/decomp_object_menu.py', 'tools/decomp_player_identity.py', 'tools/decomp_player_menu.py', 'cpppj/tests/fixtures/player-menu-x86.tsv']
    report = dict(host='HJOW-Athlon', date='2026-10-17', last_full_decompile_host='HJOW-Athlon', observations=len(rows), normal_returns=len(rows) * 2, precisions=[53, 64], precision_differences=0, instructions=oracle.instructions, os_calls=0,
                  boundaries=['C locale stricmp·문자열CRT', 'GUI 생성/서식/스택/작성', '주소 parser·신뢰도·ignore/whisper 목록·분산SP'], limits=['실제 PATCH Player 부모/Identity/사제/번호/동맹/SP 비교 helper 실행', 'CD004c4cd0은 문서 대응만 기록하며 실행/이식하지 않음', '실제 Client Session/명령 실행/GUI 픽셀/건설은 별도'], sha256={path: digest(path) for path in paths})
    reportpath.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n');print(json.dumps({'observations': len(rows), 'normal_returns': len(rows) * 2, 'instructions': oracle.instructions}))


if __name__ == '__main__':
    main()
