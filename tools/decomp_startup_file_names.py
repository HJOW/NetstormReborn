#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원문 Startup의 위치 확정 뒤 파일명 작성/지역 복사→실제Load 구간을 관측한다."""
import argparse
import json
import socket
from pathlib import Path

from decomp_config_path_load import LoadOracle
from decomp_file_name_leaf import add_ranges
from decomp_priestmenu import ROOT, TEXT, STACK, digest
from decomp_file_name_formatter import RING, INDEX
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

FIXTURE = ROOT / 'cpppj/tests/fixtures/startup-file-names-x86.tsv'  # 원문 지역 인자/작성 순서와 전체10칸의 최초 출력이다.
FRAME = STACK - 0x1000  # 위치 탐색이 끝난 원문 안정 스택에 해당하는 명시 입력 프레임이다.
RETURNS = {0x441aa3, 0x441ad9, 0x441b02, 0x441b2f, 0x441b5f, 0x441b8f, 0x441bbf}  # 실제459c60 반환 위치다.


class StartupOracle(LoadOracle):
    """441a44~441c5e만 실행하며 앞선 위치 탐색과 뒤의Set/반납은 범위 밖이다."""
    def __init__(self):
        """기존 실제Load/이름/CRT·검색 경계에 Startup의 실제 범위만 더한다."""
        super().__init__()
        add_ranges(self, (0x441790,))
        self.environment = {}  # 현재 명시한 IDENTITY/USER 원래 C문자열 포인터다.
        self.formats = []  # 실제 작성기 반환에서 읽은 경로 바이트다.
        self.arguments = []  # 실제440e00에 전달된 지역 소유 인자다.
        self.env_events = []  # 원문 getenv의 실제 선택 순서다.

    def instruction(self, mu, address, size, data):
        """고정 getenv/sprintf 경계 외 원문 작성/복사/Load를 그대로 실행한다."""
        esp = mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4e72c1:
            key = self.string(self.words(esp + 4, 1)[0])
            assert key in (b'IDENTITY', b'USER')
            self.env_events.append(key.decode('ascii'))
            self.ret(self.environment[key])
            return
        if address == 0x4e58e9:
            target, pattern, major, minor, again_major, again_minor = self.words(esp + 4, 6)
            assert self.string(pattern) == b'MAJOR_VERSION=%d;MINOR_VERSION=%d;version="v%d.%d";'
            assert (major, minor) == (again_major, again_minor) == (10, 78)
            value = b'MAJOR_VERSION=10;MINOR_VERSION=78;version="v10.78";'
            mu.mem_write(target, value + b'\0')
            self.ret(len(value))
            return
        if address in RETURNS:
            self.formats.append(self.string(mu.reg_read(UC_X86_REG_EAX)).hex())
        if address == 0x440e00:
            args = self.words(esp + 4, 11)
            assert args[0] == args[-1] == 0
            self.arguments = [self.string(pointer).hex() for pointer in args[1:-1]]
            for pointer in args[2:-1]:  # 원문 version/@/파일 인자는 PE 리터럴 또는 부모 지역이며 고리 뷰가 아니다.
                assert not RING <= pointer < RING + 2560
        if address == 0x441c5e:
            mu.emu_stop()
            return
        super().instruction(mu, address, size, data)

    def suffix(self, prefix, identity, user, index, precision):
        """이미 정한 data prefix와 두 getenv 응답에서 파일명/Load 전체 고리를 관찰한다."""
        self.formats.clear()
        self.arguments.clear()
        self.env_events.clear()
        self.events.clear()
        self.found = b''
        self.thrown = False
        self.mu.mem_write(STACK - 0x2000, bytes([0x65]) * 0x2000)
        self.mu.mem_write(RING, bytes(2560))
        self.put(INDEX, index)
        self.put(0x532728, 92, 1)
        self.put(0, 0)
        self.mu.mem_write(FRAME + 8, prefix + b'\0')
        self.environment = {}
        for number, (key, value) in enumerate(((b'IDENTITY', identity), (b'USER', user))):  # NULL과 유효 빈 문자열 포인터를 구별한다.
            pointer = 0 if value is None else TEXT + number * 512
            if pointer:
                self.mu.mem_write(pointer, value + b'\0')
            self.environment[key] = pointer
        self.mu.mem_write(TEXT + 2048, b'cheat=1;SCREENW=800\0')
        self.put(FRAME + 0xe28, 0)
        self.put(FRAME + 0xe2c, TEXT + 2048)
        self.put(FRAME + 0xe30, 10)
        self.put(FRAME + 0xe34, 78)
        self.put(FRAME - 4, 0)  # 첫 getenv 뒤 원문 add esp,8이 반납하는 앞선CD 설정 인자다.
        self.mu.reg_write(UC_X86_REG_ESI, TEXT + 4096)
        self.mu.reg_write(UC_X86_REG_ESP, FRAME - 4)
        self.mu.reg_write(UC_X86_REG_FPCW, precision)
        self.mu.emu_start(0x441a44, 0x441c5e, count=100000)
        assert not self.thrown and self.mu.reg_read(UC_X86_REG_EIP) == 0x441c5e
        assert self.mu.reg_read(UC_X86_REG_ESP) == FRAME - 44 and self.mu.reg_read(UC_X86_REG_FPCW) == precision
        assert self.words(0, 1) == (0,)
        assert bytes(self.mu.mem_read(FRAME + 8, len(prefix) + 1)) == prefix + b'\0'
        for key, value in ((b'IDENTITY', identity), (b'USER', user)):  # 환경 입력의 첫NUL 뒤 물리도 변경하지 않는다.
            if value is not None:
                assert bytes(self.mu.mem_read(self.environment[key], len(value) + 1)) == value + b'\0'
        advances = len(self.formats) + self.events.count('name')
        assert self.words(INDEX, 1)[0] == (index + advances) % 10
        return dict(formats=list(self.formats), arguments=list(self.arguments),
                    environment=list(self.env_events), advances=advances, events=list(self.events),
                    ringHex=bytes(self.mu.mem_read(RING, 2560)).hex())


def main():
    """identity와 root·초기 고리 위치를 바꿔 작성/읽기 순서와 전체 버퍼를 고정한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    if not args.verify:
        assert not FIXTURE.exists()
    cases = [(b'd\\', None, None), (b'd\\', b'alice', b'bob'), (b'd\\', b'', b'bob'),
             (b'd\\', None, b''), (b'd\\', b'.odd.', None), (b'', b'name', None),
             (b'C:/data//', b'name', None), (b'\\\\server\\share\\', None, b'person'),
             (b'\\', b'foo/bar', None), (b'D:', b'\xe9cole', None),
             (b'relative\\\\', b'first\0ignored', None), (b'.', b'\0ignored', None)]
    oracle = StartupOracle()
    fixture = ['# prefix_hex identity_hex_or_NULL user_hex_or_NULL index format_hex arguments_hex advances ring_hex']
    records = []
    for prefix, identity, user in cases:  # 환경 변수는 첫NUL이 비어 있으면USER로 재조회한다.
        for index in (0, 9):  # 실제 선증가와 wrap을 별도 입력으로 확인한다.
            result = oracle.suffix(prefix, identity, user, index, 0x027f)
            assert result == oracle.suffix(prefix, identity, user, index, 0x037f)
            encoded = lambda value: 'NULL' if value is None else value.hex() or '-'
            fixture.append('\t'.join((prefix.hex() or '-', encoded(identity), encoded(user), str(index),
                                      ','.join(result['formats']), ','.join(value or '-' for value in result['arguments']),
                                      str(result['advances']), result['ringHex'])))
            records.append(dict(prefixHex=prefix.hex(), identityHex=encoded(identity), userHex=encoded(user),
                                index=index, formats=result['formats'], arguments=result['arguments'],
                                environment=result['environment'], advances=result['advances'], events=result['events']))
    content = '\n'.join(fixture) + '\n'
    if args.verify:
        assert FIXTURE.read_text('utf-8') == content
    else:
        FIXTURE.write_text(content, 'utf-8', newline='\n')
    report = dict(unit=732, date='2026-11-02', host=socket.gethostname(),
                  lastFullDecompilation=dict(host='HJOW-Athlon', date='2026-10-23'),
                  fixtureRows=24, observations=48, rows=records,
                  boundary='실제441a44~441c5e의post-location구간과440e00/FileSpec/이름getter/CRT. prefix는확정입력, getenv/고정sprintf/설정parser·read·log/Find·Close는명시경계. 앞선탐색/CWD/TARC등록과뒤Set/반납·전체함수ABI/실제OS는범위밖',
                  gameExecutions=0, multiplayerTests=0,
                  sha256={str(path.relative_to(ROOT)): digest(path) for path in (
                      ROOT / 'originals/Netstorm.exe', Path(__file__), ROOT / 'tools/decomp_config_path_load.py', FIXTURE)})
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', 'utf-8', newline='\n')
    print(json.dumps(dict(fixtureRows=24, observations=48, advances=sorted({row['advances'] for row in records}))))


if __name__ == '__main__':
    main()
