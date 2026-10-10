#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""소리 제어 여섯 몸체를 세 원본 PE에서 실행해 사건·반환·표·전역을 관찰한다.

재생 수 재계산, 이름 정지/조회/켜기, 반복 정지, 전체 음량 변경은 실제 명령이다.
호출되는 조회/재생/정지와 ASCII 비교도 soundplay 몸체를 실행한다. 장치 COM·적재·
기록·assert·패치 CRT 로캘 선택은 기존 실행기의 명시 기록 대체를 재사용한다.
게임·창·OS·실제 소리 장치는 실행하지 않는다. --verify는 SHA와 전체 입력을 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
import zlib
from decomp_soundplay_oracle import (SoundPlayOracle, BODY, ROOT, SPECS, DEFAULTS,
    CONTROLS, SOUNDS, NAMES, DEVICE, LIST_BYTES, FALLBACK_NAME, BUFFERS,
    FAIL, named, ready, signed, digest)

# 제어 몸체와 보류 깊이/예약 음량 전역이다. 다른 몸체/전역은 기존 재생 실행기에서 읽는다.
CONTROL = {
    'originals': dict(recount=0x4a8e60, stopname=0x4a9d20, isname=0x4a9da0,
        switch=0x4a9de0, stoploops=0x4a9e50, master=0x4a9f10, depth=0x5c7b28, pending=0x5c7b2c),
    'originalCD': dict(recount=0x437c80, stopname=0x438bc0, isname=0x438c30,
        switch=0x438c80, stoploops=0x438d20, master=0x438e50, depth=0x51a728, pending=0x51a72c),
}
CONTROL['original1037'] = dict(CONTROL['originalCD'])
# 새 단계의 관찰/근거만 쓴다. 기존 재생 fixture와 감사 기록은 바꾸지 않는다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/soundcontrol-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-soundcontrol-evidence.json'


def inputs():
    """자연 종료/복제/상태 비트/옵션/이름/보류/넘침을 교차한 고정 순서의 독립 입력이다."""
    scripts = []
    a, bad = named('a.wav'), named('bad.WAV')
    # 버퍼가 없는 항목, 소리 없음 항목, 실제 버퍼와 복제 세 개를 함께 둔다. 옵션과 무관한 전체 순회도 확인한다.
    for root, copy, last, initialized, enabled in itertools.product(range(8), (0, 1, 5, 7), (0, 1, 4, 5), (0, 1), (0, 1)):
        scripts.append(ready('a.wav', 'bad.WAV', 'absent.wav') + ['g:max:0', 'ld:b:120',
            'play:$6:0:0:0:0:0', 'play:$6:1:0:0:0:0', 'play:$6:1:0:0:0:0',
            'ld:s:70', 'play:$7:0:0:0:0:0', f'st:0:{root}', f'st:1:{copy}', f'st:2:{last}',
            f'g:initialized:{initialized}', f'g:enabled:{enabled}', 'g:playing:2147483648',
            'recount', 'stoploops', 'recount', f'stopname:{a}', f'isname:{a}', 'recount'])
    # 이름 켜기는 성공 여부 대신 on 자체를 돌려준다. 준비/옵션/null은 조회 전 거르고, 기존 재생은 변경하지 않는다.
    for kind, on, loop, initialized, enabled in itertools.product(('empty', 'playing', 'ended', 'copy', 'silent'),
            (0, 1, 8, 0xffffffff), (0, 8), (0, 1), (0, 1)):
        script = ready('a.wav') + ['g:max:1', 'ld:b:340']
        if kind != 'empty': script.append('play:$6:0:0:0:0:0')
        if kind == 'ended': script.append('st:0:0')
        elif kind == 'copy': script += ['g:max:0', 'play:$6:1:0:0:0:0', 'st:0:0']
        elif kind == 'silent': script += ['st:0:0', 'clr:$6', 'ld:s:0', 'g:playing:0', 'play:$6:0:0:0:0:0']
        scripts.append(script + [f'g:initialized:{initialized}', f'g:enabled:{enabled}',
            f'switch:{on}:{a}:{loop}:4294966596', f'switch:{on}:-:{loop}:0',
            f'isname:{bad}', f'stopname:{a}', 'recount'])
    # 전체 음량은 자르지 않고, 복제 항목의 감쇠 0과 멈춘 버퍼에도 적용한다. 실패 HRESULT를 기록하지 않는다.
    for volume, attenuation, depth, initialized, enabled in itertools.product(
            (0, -10000, -10001, 3000, 0x7fffffff, -0x80000000), (0, 800, -300, 0x7fffffff),
            (0, 1, 2, -1), (0, 1), (0, 1)):
        scripts.append(ready('a.wav', 'bad.WAV', 'empty.wav') + ['g:max:0', f'ld:b:{attenuation & 0xffffffff}',
            'play:$6:1:4294967096:0:0:0', 'play:$6:0:700:0:0:0', 'st:0:0',
            'ld:s:90', 'play:$7:0:0:0:0:0', f'g:initialized:{initialized}', f'g:enabled:{enabled}',
            f'g:depth:{depth & 0xffffffff}', 'g:pending:123', f'hr:gain:{FAIL}', f'master:{volume & 0xffffffff}',
            'recount', 'g:depth:0', 'master:4294966796', 'stoploops'])
    # 빈 표와 미적재 이름에서도 제어 호출이 안전하고, 조회/정지는 옵션이 꺼져 있어도 새 이름을 등록한다.
    scripts += [['recount', 'stoploops', 'master:123', f'isname:{bad}', f'stopname:{a}',
                 f'switch:8:{a}:1:0', 'recount', 'g:initialized:1', 'master:0'],
                ready('a.wav') + ['ld:b:0', f'hr:copy:{FAIL}', 'play:$6:0:0:0:0:0',
                 f'switch:8:{a}:1:0', 'recount', 'stoploops'],
                ready('a.wav') + ['ld:b:0', 'play:$6:1:0:0:0:0', 'g:playing:0', 'stoploops', 'recount']]
    # 장치가 연속 조회 사이에 상태를 바꾸는 입력이다. 정지 판정·잃음 검사·켜기의 재생 실패 반환을 구별한다.
    for sequence in ('1,0', '1,4,2', '1,5,0'):
        scripts.append(ready('a.wav') + ['play:$6:1:0:0:0:0', f'sq:0:{sequence}', 'stoploops', 'recount'])
    scripts.append(ready('a.wav') + ['play:$6:0:0:0:0:0', f'hr:copy:{FAIL}', 'sq:0:0,1',
        f'switch:8:{a}:0:0', 'recount'])
    return [('Script', '|'.join(script)) for script in scripts]


class SoundControlOracle(SoundPlayOracle):
    """제어 몸체를 허용 범위에 추가하고 재생 실행기의 장치 기록/ABI 검사를 그대로 사용한다."""
    def __init__(self, edition):
        """현재 PC의 여섯 내보내기를 기존 호출 몸체와 함께 읽는다."""
        super().__init__(edition)
        self.c = CONTROL[edition]
        self.g = dict(self.g, depth=self.c['depth'], pending=self.c['pending'])
        added = [ROOT / f'extracted/soundcontrol/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        with added[1].open(encoding='utf-8') as fp:
            # 새 실제 몸체의 불연속 구간만 허용하며 임의 주소의 실행은 계속 거부한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entries.add(int(row['entry'], 16))
                # 몸체가 끊긴 사이의 코드는 허용하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(value, 16) for value in part.split('-'))
                    self.allowed.append((low, high + 1))
        self.exports += added

    def device(self, mu, name, purge):
        """조회 순서 입력이 있으면 다음 상태를 적용한 뒤 기존 명시 장치 대체를 실행한다."""
        if name == 'status':
            index = (self.arguments(mu, 2)[0] - BUFFERS) // 16
            queue = self.queues.get(index, [])
            if queue: self.status[index] = queue.pop(0)
        super().device(mu, name, purge)

    def run_script(self, row, control):
        """각 연산의 실제 진입을 실행하고 반환/사건과 표 전체·세 음량 전역을 관찰한다."""
        g = self.g
        self.mu.mem_write(SOUNDS, bytes(0x10000))
        self.mu.mem_write(DEVICE + 0x1000, bytes(0xf000))
        self.mu.mem_write(g['list'], struct.pack('<III', SOUNDS, SOUNDS, 0))
        # 모든 입력 전역을 초기화해 앞 스크립트의 상태가 섞이지 않게 한다.
        for name, value in dict(DEFAULTS, depth=0, pending=0).items(): self.put_global(name, value)
        self.status, self.load, self.results = [], ('b', 0), dict(copy=0, gain=0, balance=0)
        self.queues = {}
        self.write_ranges = [(0, 4), (SOUNDS, SOUNDS + LIST_BYTES)] + [
            (g[name], g[name] + 4) for name in ('free', 'playing', 'serial', 'master', 'pending')]
        results, tokens = [], []

        def handle(text):
            """$k는 k번째 연산 반환, 그 밖의 값은 항목 숫자다."""
            return results[int(text[1:])] if text.startswith('$') else int(text)

        def pointer(text):
            """-는 null이고 다른 이름은 외부 NUL 문자열로 준비한다."""
            if text == '-': return 0
            self.mu.mem_write(NAMES, bytes.fromhex(text) + b'\0')
            return NAMES

        # 입력 설정과 실제 호출을 같은 순서로 실행한다. 기대 결과는 Python으로 계산하지 않는다.
        for operation in row[1].split('|'):
            op = operation.split(':')
            self.events, result, shown = [], 0, '-'
            if op[0] in ('i', 'n'):
                name = named(FALLBACK_NAME) if op[0] == 'i' else op[1]
                result = self.call(self.b['lookup'], 0, (pointer(name),), 0, control)
                if op[0] == 'i': self.mu.mem_write(g['fallback'], struct.pack('<I', result))
            elif op[0] == 'g': self.put_global(op[1], int(op[2]))
            elif op[0] in ('view', 'cam'):
                names = ('left', 'top', 'right', 'bottom') if op[0] == 'view' else ('camx', 'camy')
                # 준비 스크립트의 화면/카메라 입력을 그대로 대입한다.
                for name, value in zip(names, op[1:]): self.put_global(name, int(value))
            elif op[0] == 'st': self.status[int(op[1])] = int(op[2])
            elif op[0] == 'sq': self.queues[int(op[1])] = list(map(int, op[2].split(',')))
            elif op[0] == 'hr': self.results[op[1]] = int(op[2])
            elif op[0] == 'ld': self.load = (op[1], int(op[2]))
            elif op[0] == 'clr': self.mu.mem_write(handle(op[1]), bytes(4))
            elif op[0] == 'play': result = self.call(self.b['play'], 0, (handle(op[1]), *map(int, op[2:7])), 0, control)
            elif op[0] == 'recount':
                result = self.call(self.c['recount'], 0, (), 0, control)
                shown = str(signed(result))
            elif op[0] == 'stopname': self.call(self.c['stopname'], 0, (pointer(op[1]),), 0, control)
            elif op[0] == 'isname':
                result = self.call(self.c['isname'], 0, (pointer(op[1]),), 0, control)
                shown = str(result)
            elif op[0] == 'switch':
                result = self.call(self.c['switch'], 0, (int(op[1]), pointer(op[2]), int(op[3]), int(op[4])), 0, control)
                shown = str(signed(result))
            elif op[0] == 'stoploops': self.call(self.c['stoploops'], 0, (), 0, control)
            elif op[0] == 'master': self.call(self.c['master'], 0, (int(op[1]),), 0, control)
            else: raise RuntimeError(f'알 수 없는 연산: {operation}')
            if op[0] in ('i', 'n', 'play'): shown = 'null' if result == 0 else str(result - SOUNDS)
            results.append(result)
            tokens.append((';'.join(self.events) or '-') + '=' + shown)
        if self.u32(g['list']) != SOUNDS: raise RuntimeError('표 전역 변경')
        return ['|'.join(tokens), self.u32(g['free']) - SOUNDS, zlib.adler32(bytes(self.mu.mem_read(SOUNDS, LIST_BYTES))),
            signed(self.u32(g['playing'])), self.u32(g['serial']), *(signed(self.u32(g[name])) for name in ('master', 'depth', 'pending'))]


def generate(smoke=False):
    """두 x87 제어값에서 동일하게 정상 반환한 관찰만 저장한다."""
    cases = inputs()[::113] if smoke else inputs()
    rows, editions = [], {}
    paths = {FIXTURE, ROOT / 'tools/decomp_soundcontrol_oracle.py', ROOT / 'tools/decomp_soundplay_oracle.py',
        ROOT / 'tools/decomp_owner_oracle.py', ROOT / 'tools/ghidra/soundcontrol-functions.json', ROOT / 'tools/ghidra/soundplay-functions.json'}
    # 각 PE에서 실제 제어·호출 몸체를 독립으로 실행한다.
    for edition in SPECS:
        oracle = SoundControlOracle(edition)
        # 같은 입력의 두 정밀도 관찰끼리 비교한다. C++ 결과를 생성에 사용하지 않는다.
        for case in cases:
            first, second = (oracle.run(case, control) for control in CONTROLS)
            if first != second: raise RuntimeError(f'정밀도 관찰 불일치: {edition}/{case}')
            rows.append([edition, *case, *first])
        editions[edition] = dict(cases=len(cases), returns=dict(oracle.returns), calls=oracle.calls,
            native_calls=dict(oracle.native_calls), substitutions=dict(oracle.substitutions), assertions=oracle.assertions, instructions=oracle.instructions)
        paths.update(oracle.exports)
        paths.add(ROOT / oracle.spec['binary'])
        print(f'{edition}: 제어 {len(cases)}개 정상 반환', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 소리 제어 명령 관찰. COM/적재/기록/assert/CRT 로캘 선택만 명시 대체.\n'
        '# edition Script ops tokens firstFree adler playing serial master depth pending\n'
        '# ops: soundplay 준비/재생 연산 + recount, stopname:이름, isname:이름, switch:on:이름(-=null):반복:음량, stoploops, master:음량\n'
        '# 이름은 Latin-1 바이트 hex, signed 인자는 DWORD decimal. !는 계속 실행한 assert 줄, 사건 순서는 실제 호출 순서.\n'
        + '\n'.join('\t'.join(map(str, row)) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(schema=1, host='HJOW-Athlon', decompile_date='2026-10-10', total=len(rows), controls=list(CONTROLS),
        os_calls=0, editions=editions,
        stubbed=['장치 COM 상태/재생/정지/음량/좌우/복제', 'WAVE 항목 적재', '기록', 'assert(기록 후 계속)',
            '패치 CRT 로캘 선택(ASCII 몸체는 실제 명령)', '초기 표 확보/0 채움/대체 이름 대입'],
        limitations=['실제 소리 장치/소리 출력 제외', '음악과 음소거 깊이 증감 호출 계층 제외(전역 깊이를 명시 입력)',
            'null 표·손상된 표/복제 사슬·빈 이름의 계약 위반 제외'],
        files={path.relative_to(ROOT).as_posix(): digest(path) for path in sorted(paths)}), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """SHA·누락/중복 없는 전체 입력·제어 몸체 진입·정상 반환·OS 0을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    expected = inputs()
    if report['schema'] != 1 or report['controls'] != list(CONTROLS) or report['os_calls'] or set(report['editions']) != set(SPECS):
        raise RuntimeError('감사 스키마/정밀도/판본/OS 오류')
    # 실행기/PE/내보내기/fixture의 바이트를 감사한다.
    for name, value in report['files'].items():
        if digest(ROOT / name) != value: raise RuntimeError(f'SHA 불일치: {name}')
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows) != len(expected) * 3 or report['total'] != len(rows): raise RuntimeError('행 개수 오류')
    entered = collections.Counter(op.split(':')[0] for case in expected for op in case[1].split('|'))
    # 각 판본이 같은 순서의 모든 입력을 두 정밀도로 정상 반환했는지 확인한다.
    for edition, item in report['editions'].items():
        selected = [row for row in rows if row[0] == edition]
        if len(selected) != len(expected) or [row[1:3] for row in selected] != [list(case) for case in expected] or item['cases'] != len(expected):
            raise RuntimeError(f'입력 오류: {edition}')
        if item['assertions'] or item['returns'] != {'Script': len(expected) * 2}: raise RuntimeError(f'assert/반환 오류: {edition}')
        # 직접 호출한 제어 몸체는 입력 연산 수 이상 진입해야 한다. 내부 호출이 추가될 수 있다.
        for key in ('recount', 'stopname', 'isname', 'switch', 'stoploops', 'master'):
            if item['native_calls'].get(f'{CONTROL[edition][key]:08x}', 0) < entered[key] * 2:
                raise RuntimeError(f'제어 몸체 진입 오류: {edition}/{key}')
    print(f'soundcontrol 검증 통과: {len(rows)}개')


def main():
    """기본 생성·축소 실행·저장 기록 감사 중 하나를 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
