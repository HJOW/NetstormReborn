#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""음악 파일 읽기/정지와 공유 음량/음소거를 실제 세 PE에서 정상 반환까지 실행한다.

장치 COM, Winmm 읽기/seek/닫기, 재귀 잠금, 기록/예정 assert만 명시 대체한다.
CD 시작은 공개 파일 열기 함수에 인라인되어 시작/되감기 단독 입력은 10.78에만 둔다.
게임/장치/스레드/OS를 실행하지 않는다. --verify는 입력/진입/반환/SHA 근거를 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_soundcontrol_oracle import SoundControlOracle
from decomp_soundplay_oracle import ROOT, SPECS, DEFAULTS, CONTROLS, SOUNDS, NAMES, BUFFERS, STUBS, named, signed, digest
from decomp_owner_oracle import pefile

# 관찰 대상 채널과 제어 몸체/전역이다. 실제 원본의 기본 채널 주소를 사용한다.
BODY = {
 'originals': dict(channel=0x5c7b60, read=0x4aa0e0, close=0x4aa410, stop=0x4aa9c0,
    start=0x4aaa40, rewind=0x4a9fe0, mv=0x4aa5d0, push=0x4aa900, pop=0x4aa970, allstop=0x4aad70,
    music=0x5c7b24, pendingmusic=0x5c7b30, musicinit=0x5c7b5c),
 'originalCD': dict(channel=0x5650b8, read=0x439fb0, close=0x43a130, stop=0x439ea0,
    mv=0x439dd0, push=0x438f10, pop=0x438f60, allstop=0x439910,
    music=0x51a724, pendingmusic=0x51a730, musicinit=0x51a764),
}
BODY['original1037'] = dict(BODY['originalCD'])
# 파일 토큰·출력 메모리와 전체 관찰 크기다. 호스트 파일/포인터와 무관하다.
FILE, OUTPUT, OUT_BYTES, CHANNEL_BYTES = 0x17001000, 0x17100000, 128, 0x60
# 허용한 잠금/Winmm stdcall 경계의 인자 개수다. 그 밖의 API는 대체하지 않는다.
IMPORTS = dict(EnterCriticalSection=1, LeaveCriticalSection=1, mmioRead=3, mmioSeek=3, mmioClose=2)
FIXTURE = ROOT / 'cpppj/tests/fixtures/musiccontrol-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-musiccontrol-evidence.json'


def inputs(edition):
    """읽기 경계/반복/부분 실패와 중첩 음소거/음량 보류/재진입을 교차한 고정 입력이다."""
    scripts = []
    # 파일 끝의 정확한 경계·0 길이·부분 읽기 실패·되감기 실패를 교차한다. 반복해도 한 번만 되감는 원본 계약이다.
    for length, flags, cap, seek in itertools.product((0, 1, 8, 16), (0, 1, 2, 3, 0xa5000003), (-2, -1, 0, 2), (0, -1)):
        # 중복 없는 시작 위치와 읽기 크기를 만든다.
        for offset, count in itertools.product(sorted({0, length // 2, length}), sorted({0, 1, length, length + 1, length * 2 + 1})):
            scripts.append(['seed', f'file:{length}:{offset}:0', f'f:8:{flags}', f'cap:{cap}', f'seek:{seek}',
                f'read:{count}', 'read:1', 'close', 'stop'])
    # 정지는 버퍼 유무/중첩 깊이/참조 해제 실패와 무관하게 정확한 필드만 지운다. 두 판본 공개 정지의 패딩 보존도 관찰한다.
    for buffer, depth, release, initialized in itertools.product((0, 1), (0, 1, 0xffffffff), (0, 0x80004005), (0, 1)):
        scripts.append(['seed', f'mb:{buffer}', f'f:64:{depth}', f'hr:release:{release}', f'g:musicinit:{initialized}', 'allstop', 'stop', 'close'])
    # 공유 보류 전역의 signed 깊이·음악/효과음 초기화와 음악 음량 실패/범위를 섞는다.
    for depth, initialized, sound, volume, result in itertools.product((0, 1, 2, 0xffffffff, 0x7fffffff), (0, 1), (0, 1),
            (0, 3000, 4294957295), (0, 0x80004005)):
        scripts.append(['seed', f'g:depth:{depth}', f'g:musicinit:{initialized}', f'g:initialized:{sound}', f'hr:gain:{result}',
            f'mv:{volume}', 'push', 'push', 'master:4294965296', 'mv:4294964296', 'pop', 'pop', 'pop'])
    # COM 음량 콜백이 깊이/예약 음악을 바꾸는 명시 재진입 입력이다. patch와 CD pop의 전역 재검사를 관찰한다.
    for when, depth in itertools.product(('fx', 'music'), (0, 1, 0xffffffff)):
        scripts.append(['seed', f'cb:{when}:{depth}:4294966496', 'push', 'pop', 'pop'])
        scripts.append(['seed', 'g:depth:1', f'cb:{when}:{depth}:4294966496', 'pop', 'mv:4000'])
    if edition == 'originals':
        # 10.78의 독립 시작/되감기 몸체다. CD 같은 구간은 파일 열기 함수 안에 있어 단독 fixture로 주장하지 않는다.
        for length, buffer, loop, seek in itertools.product((0, 8), (0, 1), (0, 1, 8), (0, -1)):
            scripts.append(['seed', f'file:{length}:{length}:0', f'mb:{buffer}', f'seek:{seek}', f'start:{loop}', 'rewind', 'read:1', 'stop'])
    return ['|'.join(script) for script in scripts]


class MusicOracle(SoundControlOracle):
    """기존 실제 효과음 제어와 음악 몸체를 함께 실행하고 파일/잠금/버퍼 경계를 기록한다."""
    def __init__(self, edition):
        """현재 호스트의 음악 내보내기와 제한된 IAT 대체를 추가한다."""
        super().__init__(edition)
        self.m = BODY[edition]
        self.g = dict(self.g, **{k: self.m[k] for k in ('music', 'pendingmusic', 'musicinit')})
        self.mu.mem_map(OUTPUT, 0x1000)
        added = [ROOT / f'extracted/musiccontrol/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        with added[1].open(encoding='utf-8') as fp:
            # 실제 불연속 몸체만 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 몸체 사이의 임의 코드는 계속 거부한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(v, 16) for v in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.exports += added
        self.api = {}
        pe = pefile.PE(str(ROOT / self.spec['binary']))
        # 원본 IAT의 다섯 API만 독립 메모리 파일/잠금 경계로 바꾼다.
        for directory in pe.DIRECTORY_ENTRY_IMPORT:
            # import 이름이 정확히 일치할 때만 허용한다.
            for item in directory.imports:
                name = item.name.decode() if item.name else ''
                if name in IMPORTS:
                    address = STUBS + 0x400 + len(self.api) * 16
                    self.api[address] = name
                    self.mu.mem_write(item.address, struct.pack('<I', address))
        if set(self.api.values()) != set(IMPORTS): raise RuntimeError('필수 IAT 경계 누락')
        self.stubs[STUBS + 0x100] = ('release', 4)
        self.mu.mem_write(0x16000200 + 8, struct.pack('<I', STUBS + 0x100))
        self.direct = collections.Counter()

    def device(self, mu, name, purge):
        """음악 버퍼 참조 해제와 한 번의 명시 재진입만 추가하고 나머지 COM은 기존 대체를 재사용한다."""
        if name == 'release':
            (buffer,) = self.arguments(mu, 1)
            self.events.append('B:' + self.ordinal(buffer))
            return self.leave(mu, self.release_result, purge)
        buffer = self.arguments(mu, 2)[0] if name == 'gain' else None
        super().device(mu, name, purge)
        if name == 'gain' and self.callback and buffer == BUFFERS + (0 if self.callback[0] == 'fx' else 16):
            _, depth, pending = self.callback
            self.callback = None
            self.put_global('depth', depth);self.put_global('pendingmusic', pending)
            self.events.append(f'J:{signed(depth)}:{signed(pending)}')

    def on_instruction(self, mu, address, size, data):
        """실제 코드와 구별해 잠금/파일 대체를 기록하며 스택/쓰기 검사는 유지한다."""
        name = self.api.get(address)
        if not name: return super().on_instruction(mu, address, size, data)
        self.substitutions[name] += 1
        args = self.arguments(mu, IMPORTS[name]);result = 0
        if name in ('EnterCriticalSection', 'LeaveCriticalSection'):
            if args[0] != self.m['channel'] + 0x44: raise RuntimeError('잠금 채널 오류')
            self.lock_depth += 1 if name.startswith('Enter') else -1
            if self.lock_depth < 0: raise RuntimeError('잠금 초과 해제')
            self.events.append('E' if name.startswith('Enter') else 'L')
        elif name == 'mmioSeek':
            token, offset, origin = args
            if origin != 0: raise RuntimeError('절대 seek 계약 오류')
            result = offset if self.opened and token == FILE and self.seek_result != -1 else 0xffffffff
            self.events.append(f'S:{token}:{offset}:{signed(result)}')
            if result != 0xffffffff: self.position = offset
        elif name == 'mmioRead':
            token, target, count = args
            old = self.position
            result = -1
            if self.opened and token == FILE and self.cap != -1:
                sample = self.file[self.position:self.position + count]
                if self.cap >= 0: sample = sample[:self.cap]
                if sample: mu.mem_write(target, sample)
                result = len(sample);self.position += result
            self.events.append(f'R:{token}:{old}:{count}:{result}')
        elif name == 'mmioClose':
            token, flags = args
            if flags != 0: raise RuntimeError('mmio close 플래그 오류')
            self.events.append(f'C:{token}');self.opened = False
        self.leave(mu, result & 0xffffffff, IMPORTS[name] * 4)

    def invoke(self, key, args, control):
        """공개/채널 ABI로 직접 호출하고 정상 반환 횟수를 별도로 기록한다."""
        channel = key in ('read', 'close', 'stop', 'start', 'rewind')
        result = self.call(self.m[key], self.m['channel'] if channel else 0, tuple(args),
            8 if key == 'read' else 4 if key == 'start' else 0, control)
        self.direct[key] += 1
        return result

    def run_script(self, script, control):
        """스크립트의 원본 관찰을 반환한다. C++ 결과나 음악 상태 전이를 Python으로 계산하지 않는다."""
        g, channel = self.g, self.m['channel']
        self.mu.mem_write(SOUNDS, bytes(0x8000));self.mu.mem_write(g['list'], struct.pack('<III', SOUNDS, SOUNDS, 0))
        self.mu.mem_write(OUTPUT, bytes([0xa7]) * OUT_BYTES)
        self.mu.mem_write(channel, bytes((i * 17 + 3) & 255 for i in range(CHANNEL_BYTES)))
        # 호출할 여섯 음악/보류 전역과 효과음 전역을 초기화한다.
        for name, value in dict(DEFAULTS, initialized=1, device=1, enabled=1, master=-400,
                depth=0, pending=123, music=-600, pendingmusic=222, musicinit=1).items(): self.put_global(name, value)
        self.status, self.load, self.results, self.queues = [], ('b', 0), dict(copy=0, gain=0, balance=0), {}
        self.release_result, self.cap, self.seek_result, self.callback, self.lock_depth = 0, -2, 0, None, 0
        self.file, self.position, self.opened = b'', 5, True
        self.write_ranges = [(0, 4), (SOUNDS, SOUNDS + 0x8000), (OUTPUT, OUTPUT + OUT_BYTES), (channel, channel + CHANNEL_BYTES)] + [
            (g[name], g[name] + 4) for name in ('free', 'master', 'pending', 'depth', 'music', 'pendingmusic')]
        tokens = []

        def field(offset, value):
            """입력 DWORD만 채널 상태에 대입한다."""
            self.mu.mem_write(channel + offset, struct.pack('<I', value & 0xffffffff))

        # 입력 설정과 실제 함수 호출을 차례로 실행하며 각 호출 뒤 잠금 균형을 확인한다.
        for operation in script.split('|'):
            op = operation.split(':');self.events, shown = [], '-'
            if op[0] == 'seed':
                # 표/가짜 버퍼 초기 배치만 공급한다. Lookup은 원본 명령을 실행한다.
                for text in ('nonexistant.wav', 'effect.wav'):
                    self.mu.mem_write(NAMES, text.encode() + b'\0')
                    entry = self.call(self.b['lookup'], 0, (NAMES,), 0, control)
                    if text == 'nonexistant.wav': self.mu.mem_write(g['fallback'], struct.pack('<I', entry))
                self.mu.mem_write(entry, struct.pack('<I', self.create()))
                self.mu.mem_write(entry + 12, struct.pack('<II', 300, -200 & 0xffffffff))
                field(0, self.create());field(4, 16);field(8, 3);field(12, FILE);field(16, 16);field(32, 0);field(36, 9);field(60, 5);field(64, 0)
                self.file = bytes((i * 7 + 11) & 255 for i in range(21))
            elif op[0] == 'g': self.put_global(op[1], int(op[2]))
            elif op[0] == 'f': field(int(op[1]), int(op[2]))
            elif op[0] == 'mb': field(0, BUFFERS + 16 if int(op[1]) else 0)
            elif op[0] == 'file':
                length, offset, tail = map(int, op[1:]);field(16, length);field(32, offset)
                self.file = bytes((i * 7 + 11) & 255 for i in range(5 + length + tail));self.position = 5 + offset
            elif op[0] == 'cap': self.cap = int(op[1])
            elif op[0] == 'seek': self.seek_result = int(op[1])
            elif op[0] == 'hr':
                if op[1] == 'release': self.release_result = int(op[2])
                else: self.results[op[1]] = int(op[2])
            elif op[0] == 'cb': self.callback = (op[1], int(op[2]), int(op[3]))
            elif op[0] == 'read': shown = str(signed(self.invoke('read', (OUTPUT, int(op[1])), control)))
            elif op[0] in ('start', 'rewind'): shown = str(self.invoke(op[0], tuple(map(int, op[1:])), control))
            elif op[0] == 'master': self.call(self.c['master'], 0, (int(op[1]),), 0, control)
            elif op[0] in ('close', 'stop', 'mv', 'push', 'pop', 'allstop'): self.invoke(op[0], tuple(map(int, op[1:])), control)
            else: raise RuntimeError('알 수 없는 연산: ' + operation)
            if self.lock_depth: raise RuntimeError(f'반환 시 잠금 잔류: {operation}')
            tokens.append((';'.join(self.events) or '-') + '=' + shown)
        return ['|'.join(tokens), bytes(self.mu.mem_read(channel, CHANNEL_BYTES)).hex(), bytes(self.mu.mem_read(OUTPUT, OUT_BYTES)).hex(),
            zlib.adler32(bytes(self.mu.mem_read(SOUNDS, 0x8000))), self.position, int(self.opened),
            *(signed(self.u32(g[name])) for name in ('master', 'pending', 'depth', 'music', 'pendingmusic'))]


def generate(smoke=False):
    """두 제어값에서 동일하게 정상 반환한 원본 관찰과 파일 SHA를 기록한다."""
    rows, editions = [], {}
    paths = {Path(__file__), FIXTURE, ROOT / 'tools/ghidra/musiccontrol-functions.json', ROOT / 'tools/decomp_musiccontrol_oracle.py',
        ROOT / 'tools/decomp_soundcontrol_oracle.py', ROOT / 'tools/decomp_soundplay_oracle.py', ROOT / 'tools/decomp_owner_oracle.py'}
    # 세 PE의 원본 결과를 독립으로 수집한다.
    for edition in SPECS:
        oracle = MusicOracle(edition);cases = inputs(edition)
        if smoke: cases = cases[::73] + cases[-2:]
        # 두 정밀도/ABI 관찰은 원본끼리 비교한다.
        for case in cases:
            first, second = (oracle.run_script(case, control) for control in CONTROLS)
            if first != second: raise RuntimeError(f'정밀도 관찰 불일치: {edition}/{case}')
            rows.append([edition, case, *first])
        editions[edition] = dict(cases=len(cases), calls=oracle.calls, direct_returns=dict(oracle.direct), native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.substitutions), assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports);paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 음악 제어 {len(cases)}개 정상 반환', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 음악 채널/공유 음소거 관찰. 파일/COM/잠금/기록만 명시 대체. CD 시작 인라인은 단독 대조에서 제외.\n'
        '# edition ops tokens raw96 output128 tableAdler filePosition fileOpen master pending depth music pendingMusic\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1, host='HJOW-Athlon', decompile_date='2026-10-10', total=len(rows), controls=list(CONTROLS),
        os_calls=0, editions=editions, stubbed=['COM 음량/정지/참조 해제', 'Winmm read/seek/close', '잠금(중첩/균형 확인)',
            '기록/예정 assert/패치 CRT 로캘', '표/채널/파일/버퍼 초기 배치', '명시 COM 재진입 깊이/예약 전역 입력'],
        limitations=['실제 장치/파일/스레드 실행 제외', '음악 파일 열기/버퍼 생성/스트리밍 갱신/스레드 초기화·종료 후속',
            'CD 시작/독립 되감기는 인라인이므로 10.78에만 단독 진입 대조', '손상된 음수/범위 밖 길이/포인터 제외'],
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(paths)}), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """SHA·전체 입력/행·직접 정상 반환·실제 진입·잠금/OS/예기치 않은 assert 0을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    if report['schema'] != 1 or report['controls'] != list(CONTROLS) or report['os_calls'] or set(report['editions']) != set(SPECS):
        raise RuntimeError('감사 스키마/판본/제어값 오류')
    # 원본/모든 실행기/내보내기/fixture 바이트를 확인한다.
    for name, value in report['files'].items():
        if digest(ROOT / name) != value: raise RuntimeError('SHA 불일치: ' + name)
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != report['total'] or len(rows) != sum(len(inputs(e)) for e in SPECS): raise RuntimeError('관찰 개수 오류')
    # 판본마다 정확한 입력 순서/직접 반환/몸체 진입을 확인한다.
    for edition, item in report['editions'].items():
        cases = inputs(edition);selected = [row for row in rows if row[0] == edition]
        if [row[1] for row in selected] != cases or item['cases'] != len(cases) or item['assertions']: raise RuntimeError('입력/assert 오류')
        counts = collections.Counter(op.split(':')[0] for case in cases for op in case.split('|'))
        expected = {key: counts[key] * 2 for key in BODY[edition] if key in ('read', 'close', 'stop', 'mv', 'push', 'pop', 'allstop', 'start', 'rewind')}
        if item['direct_returns'] != expected: raise RuntimeError('직접 정상 반환 개수 오류')
        # 각 직접 호출 몸체는 대응 횟수 이상 진입해야 한다(내부 호출이 더해진다).
        for key, count in expected.items():
            if item['native_calls'].get(f'{BODY[edition][key]:08x}', 0) < count: raise RuntimeError(f'몸체 진입 누락: {edition}/{key}')
        if item['substitutions']['EnterCriticalSection'] != item['substitutions']['LeaveCriticalSection']: raise RuntimeError('잠금 균형 오류')
    print(f'musiccontrol 감사 통과: {len(rows)}개')


def main():
    """생성·축소 진입 확인·저장 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
