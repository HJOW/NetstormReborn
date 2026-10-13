#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 상위 Renderer 정상 반환으로 공통 병합/CRT/종류 분배를 관찰한다. 수집/장치 내부만 명시 응답 경계다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, STACK, STOP, digest, fbits,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 격리된 Renderer 객체/5,000항목/8바이트 묶음 헤더/내부 배열이다. 호스트 주소가 아니다.
DATA, FRAME, ARRAY, GROUPS, MEMBERS = 0x15000000, 0x15000000, 0x15001000, 0x15018000, 0x15019000
CAPACITY, STRIDE, SEED = 5000, 16, 0x994b031b
FIXTURE = ROOT / 'cpppj/tests/fixtures/drawsubmission-x86.tsv'
REPORT = ROOT / 'cpppj/drawsubmission.json'
# 명시 응답 경계의 주소다. 좌표/클립/CRT/묶음 순회/분배의 실제 명령은 대체하지 않는다.
SPECS = {
    'originals': dict(root=0x4994b0, flags=0x5c78d4, previous=0x59a9cc, full=0x59a8b0,
        width=0x5c78f0, height=0x5c78f4, viewport=0x5ca9cc, camera=0x59a900,
        array=0x59a8c0, groups=0x59a8c8, group_data=0x59a8c4, hidden=0x59a8b4, debug=0x59a8cc,
        sort_option=0x540ce8, clip=0x5c7a44, counter=0x59a8d4, capture=0x499bfd,
        ui=0x466070, construction=0x487f10, group=0x433410, objects=0x433660, preview=0x446060,
        sprite=0x498220, draw_ui=0x466310, draw_construction=0x4877c0, draw_preview=0x446270,
        reset=0x470c80, lock=0x4a14e0, unlock=0x4a1550, set_clip=0x4a1580,
        debug_rect=0x4a1800, fill=0x4a2060, flush=0x4a4150, finish=0x470ca0),
    'originalCD': dict(root=0x4575e0, flags=0x516af0, previous=0x5203d0, full=0x52039c,
        width=0x516b10, height=0x516b14, viewport=0x583e20, camera=0x565c18,
        array=0x5203ac, groups=0x5203b4, group_data=0x5203b0, hidden=0x5203a4, debug=0x5203b8,
        sort_option=0x520418, clip=0x552880, counter=0x5203cc, capture=0x4582b0,
        ui=0x492750, construction=0x45c4b0, group=0x4884a0, objects=0x4887a0, preview=0x43da30,
        sprite=0x455f60, draw_ui=0x492aa0, draw_construction=0x45c5e0, draw_preview=0x43dbd0,
        reset=0x4b0b50, lock=0x422d70, unlock=0x422de0, set_clip=0x422e10,
        debug_rect=0x423130, fill=0x423960, flush=0x426390, finish=0x4b1030),
}
SPECS['original1037'] = dict(SPECS['originalCD'])
# 수집기 응답은 완성 입력 바이트이며 기대 병합/분배를 계산하지 않는다.
COLLECTORS = ('ui', 'construction', 'objects', 'preview')
BOUNDARIES = COLLECTORS + ('group', 'sprite', 'draw_ui', 'draw_construction', 'draw_preview',
    'reset', 'lock', 'unlock', 'set_clip', 'debug_rect', 'fill', 'flush', 'finish')


def plan():
    """빈/단일/동률/혼합/개수 변경과 네 옵션 및 두 장치 잠금 위치를 교차한다."""
    result = []
    # 서로 독립적인 dirty bit 2/정렬/묶음 숨김/debug/장치 bit 7 분기를 모두 공급한다.
    for profile in range(10):
        # 각 입력 계획에 모든 옵션 조합을 넣는다.
        for flags in (0, 4):
            # 정렬 전 수집 순서와 실제 CRT 결과를 각각 관찰한다.
            for sort in (0, 1):
                # 묶음 숨김은 나머지 종류의 분배에 영향을 주지 않아야 한다.
                for hidden in (0, 1):
                    # debug는 알려지지 않은 kind에도 실행된다.
                    for debug in (0, 1):
                        # 장치 잠금/해제가 dirty 안과 바깥에 있는 두 원본 경로다.
                        for mode in (0, 128): result.append((profile, flags, sort, hidden, debug, mode))
    result.extend([(10, 0, 0, 0, 0, 0), (10, 4, 0, 0, 0, 128)])
    return result


def inputs(profile):
    """수집기 완성 응답과 묶음 저장소를 만든다. 토큰/키/종류/tail은 입력이고 결과 순서는 만들지 않는다."""
    rng = random.Random(SEED + profile);serial = 0
    # 입력 레코드는 모든 필드를 채워 전체 16바이트 보존을 관찰한다.
    def record(kind, tail=0):
        """합성 키와 식별 토큰을 서로 다르게 공급한다."""
        nonlocal serial
        serial += 1
        x, y, z = rng.randrange(-3, 4), rng.randrange(-3, 4), rng.randrange(-4, 5)
        if profile == 3: x = y = z = 0
        if profile == 7: z = (-32768, 32767)[serial % 2]
        return struct.pack('<2fIhBB', x, y, serial, z, kind, tail & 255)
    sizes = (0, 0, 0, 0) if profile == 0 else (2, 2, 3, 2)
    if profile == 1: sizes = (1, 1, 1, 1)
    if profile == 8: sizes = (17, 15, 21, 13)
    if profile == 9: sizes = (5, 7, 8, 0)
    if profile == 10: sizes = (4997, 1, 0, 1)
    responses = {}
    # 네 수집기의 결과 레코드는 해당 경계 호출 시에만 가상 작업 배열에 쓰인다.
    for key, size in zip(COLLECTORS, sizes, strict=True):
        kind = dict(ui=2, construction=3, objects=0, preview=4)[key]
        responses[key] = [record(255 if profile == 10 and key == 'ui' else
            (5 if key == 'objects' and index == 1 and profile in (2, 8) else kind), index ^ 0x72) for index in range(size)]
    groups = []
    # 헤더의 개수와 배열의 실제 저장소 크기는 다르다. 4/5는 그리기 중 개수가 바뀐다.
    for index in range(0 if profile == 0 else 1 if profile in (1, 10) else 2):
        count = (-1 if index else 0) if profile == 6 else 1 if profile in (1, 5, 10) else 4
        members = [record((offset + 1) % 5, 0xb0 + offset) for offset in range(4)]
        groups.append((GROUPS + index * 8, count, members))
    return responses, groups


def packed_inputs(profile):
    """네 수집기 응답과 묶음 헤더/저장소의 완성 입력을 문자열로 저장한다. 기대 결과는 넣지 않는다."""
    responses, groups = inputs(profile)
    return '|'.join([b''.join(responses[key]).hex() or '-' for key in COLLECTORS] +
        [f'{token}={count}=' + b''.join(members).hex() for token, count, members in groups])


class DrawSubmissionOracle(ProcessOracle):
    """상위 Renderer/CRT/좌표/클립/CD 묶음 전체를 실제 실행하고 외부 수집/그림 장치만 대체한다."""
    def __init__(self, edition):
        """읽기 전용 내보내기로 실행 범위를 한정하고 모든 응답 경계를 이름으로 기록한다."""
        super().__init__(edition);self.d = SPECS[edition]
        self.extra = [ROOT / f'extracted/drawsubmission/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entry_names = [], {}
        with self.extra[1].open(encoding='utf-8') as fp:
            # 한 함수의 불연속 명령 범위도 각각 허용한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entry_names[int(row['entry'], 16)] = row['entry']
                # 함수 사이 빈 구간은 실행하지 못한다.
                for part in row['ranges'].split(';'):
                    low, high = (int(v, 16) for v in part.split('-'));self.allowed.append((low, high + 1))
        self.mu.mem_map(DATA, 0x40000);self.boundary_addresses = {self.d[name]: name for name in BOUNDARIES}
        self.returns, self.boundaries = collections.Counter(), collections.Counter()

    def on_instruction(self, mu, address, size, data):
        """실제 상위 흐름의 인자/바이트/개수와 호출 순서를 관찰한다. 내부 결과/순서를 계산하지 않는다."""
        d = self.d;esp = mu.reg_read(UC_X86_REG_ESP)
        if address == d['capture']:
            self.count = mu.reg_read(UC_X86_REG_EBX) if self.patch else self.u32(esp + 8)
            self.before = bytes(mu.mem_read(ARRAY, self.count * STRIDE)).hex() or '-'
        if address in self.boundary_addresses:
            name = self.boundary_addresses[address];self.boundaries[name] += 1;self.calls[name] += 1
            result, purge = 0, 0
            if name in COLLECTORS:
                args = struct.unpack('<6I', mu.mem_read(esp + 4, 24));rect, pointer, limit = args[:4], args[4], args[5]
                self.collection.append(f'{name}:{(pointer-ARRAY)//STRIDE}:{limit}:' + ','.join(map(str, rect)))
                self.rectangles[name] = list(rect)
                supplied = self.responses[name]
                if len(supplied) > limit or pointer < ARRAY or pointer + len(supplied)*STRIDE > ARRAY + CAPACITY*STRIDE:
                    raise RuntimeError('표시 수집 응답 저장소/용량 오류')
                if supplied: mu.mem_write(pointer, b''.join(supplied))
                result = len(supplied)
            elif name == 'group':
                args = struct.unpack('<8I', mu.mem_read(esp + 4, 32))
                if args[4] != GROUPS: raise RuntimeError('묶음 헤더 포인터 오류')
                self.collection.append('group:' + str(args[6]) + ':' + ','.join(map(str, args[:4])))
                self.put(args[7], self.covered);result = len(self.groups)
            elif name in ('sprite', 'draw_preview'):
                pointer = mu.reg_read(UC_X86_REG_ECX) if name == 'sprite' else self.u32(esp + 4)
                self.draws.append(('S' if name == 'sprite' else 'P') + ':' + bytes(mu.mem_read(pointer, STRIDE)).hex())
                if name == 'sprite' and pointer == MEMBERS and self.profile in (4, 5):
                    self.put(GROUPS + 4, 1 if self.profile == 4 else 4)
            elif name in ('draw_ui', 'draw_construction'):
                self.draws.append(('U' if name == 'draw_ui' else 'C') + ':' + str(self.u32(esp + 4)))
                if self.profile == 9 and name == 'draw_ui' and not self.toggled:
                    self.put(d['hidden'], self.u32(d['hidden']) ^ 1);self.put(d['debug'], self.u32(d['debug']) ^ 1);self.toggled = True
            elif name == 'set_clip':
                rectangle = bytes(mu.mem_read(esp + 4, 16));mu.mem_write(d['clip'], rectangle)
            elif name == 'debug_rect': self.draws.append('D')
            elif name in ('lock', 'unlock', 'fill', 'flush', 'finish'): self.devices.append(name)
            ret = self.u32(esp);mu.reg_write(UC_X86_REG_EAX, result)
            mu.reg_write(UC_X86_REG_ESP, esp + 4 + purge);mu.reg_write(UC_X86_REG_EIP, ret);return
        if not any(low <= address and address + size <= high for low, high in self.allowed):
            raise RuntimeError(f'표시 제출 허용하지 않은 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]] += 1;self.entries[self.entry_names[address]] += 1

    def run(self, case, control):
        """원본 Renderer 루트까지 정상 반환하고 원본이 쓴 병합/정렬 배열과 모든 분배 인자를 기록한다."""
        self.profile, flags, sort, hidden, debug, mode = case;mu, d = self.mu, self.d
        self.responses, self.groups = inputs(self.profile);self.covered = self.profile % 2
        self.collection, self.draws, self.devices, self.rectangles = [], [], [], {}
        self.calls, self.entries = collections.Counter(), collections.Counter();self.count, self.before = 0, '-';self.toggled = False
        mu.mem_write(FRAME, b'\0' * 0x900);mu.mem_write(ARRAY - 16, b'\xa5' * (CAPACITY * STRIDE + 32))
        self.put(FRAME + 8, 16);self.put(FRAME + 12, 11);self.put(FRAME + 100, 1)
        mu.mem_write(FRAME + 0x68, struct.pack('<4i', 0, 0, 320, 200));self.put(FRAME + 0x6a8, flags)
        # 원본 모드 전환/강제 갱신/구름/격자 분기는 입력에서 끄고 유효 dirty 한 개를 공급한다.
        for key, value in dict(flags=mode, previous=mode, full=0, width=640, height=480,
            array=ARRAY, groups=GROUPS, group_data=MEMBERS, hidden=hidden, debug=debug, sort_option=sort).items(): self.put(d[key], value)
        mu.mem_write(d['viewport'], struct.pack('<4i', 100, 50, 250, 400));mu.mem_write(d['camera'], struct.pack('<2f', 1.5, 2.25))
        if self.patch: self.put(0x531860, 128);self.put(0x531864, 33)
        # 묶음의 내부 저장소는 최초 개수보다 넓다. 기대 제출 순서는 원본이 결정한다.
        for index, (token, count, members) in enumerate(self.groups):
            self.put(token, MEMBERS + index * 0x100);self.put(token + 4, count);mu.mem_write(MEMBERS + index * 0x100, b''.join(members))
        self.writes = [(0, 4), (STACK - 0xf000, STACK + 0x1000), (FRAME, FRAME + 0x900),
            (ARRAY, ARRAY + CAPACITY * STRIDE), (d['counter'], d['counter'] + 4)]
        # ABI 보존 레지스터/SEH/x87을 비영 값으로 검사한다. 응답 경계는 보존 레지스터를 건드리지 않는다.
        for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EDX): mu.reg_write(reg, 0)
        mu.reg_write(UC_X86_REG_ECX, FRAME);mu.reg_write(UC_X86_REG_ESP, STACK);mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_FPCW, control);mu.reg_write(UC_X86_REG_FPSW, 0);self.put(0, 0xffffffff);self.put(STACK, STOP)
        mu.emu_start(d['root'], STOP, timeout=30000000, count=20000000)
        if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK + 4 or any(mu.reg_read(r) != v for r, v in REGISTERS.items()):
            raise RuntimeError('표시 제출 정상 반환/스택/보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW) & 0x3800 or mu.reg_read(UC_X86_REG_FPCW) != control or self.u32(0) != 0xffffffff:
            raise RuntimeError('표시 제출 x87/SEH 복구 오류')
        self.returns[f'{d["root"]:08x}'] += 1
        guard = bytes(mu.mem_read(ARRAY - 16, 16)) + bytes(mu.mem_read(ARRAY + self.count * STRIDE, 16))
        if guard != b'\xa5' * 32: raise RuntimeError('표시 제출 미사용/보호 바이트 오류')
        return [packed_inputs(self.profile), str(self.count), str(self.covered if not flags & 4 else 0), self.before,
            bytes(mu.mem_read(ARRAY, self.count * STRIDE)).hex() or '-', ';'.join(self.collection) or '-',
            ';'.join(self.draws) or '-', ','.join(self.devices),
            json.dumps(dict(self.calls), sort_keys=True, separators=(',', ':')),
            json.dumps(dict(self.entries), sort_keys=True, separators=(',', ':')), guard.hex()]


def generate(smoke):
    """두 실제 x87 결과가 동일한 독립 관찰만 저장한다. C++는 실행하거나 가져오지 않는다."""
    rows, editions = [], {};files = {ROOT / 'tools/decomp_drawsubmission_oracle.py', ROOT / 'tools/decomp_drawsort_oracle.py',
        ROOT / 'tools/decomp_process_oracle.py', ROOT / 'tools/ghidra/drawsubmission-functions.json', FIXTURE}
    # 세 PE는 독립된 메모리에서 상위 Renderer 정상 반환을 관찰한다.
    for edition in BINARIES:
        oracle = DrawSubmissionOracle(edition);cases = plan()
        if smoke: cases = cases[::41] + cases[-2:]
        # 모든 입력의 배열/인자/순서/보호 바이트/실제 진입을 두 정밀도에서 비교한다.
        for index, case in enumerate(cases, 1):
            observed = [oracle.run(case, control) for control in CONTROLS]
            if observed[0] != observed[1]: raise RuntimeError(f'표시 제출 x87 차이: {edition} {case}')
            rows.append([edition, ','.join(map(str, case)), *observed[0]])
            if index % 100 == 0: print(f'{edition}: {index}/{len(cases)}', flush=True)
        editions[edition] = dict(cases=len(cases), entries=dict(oracle.hits), returns=dict(oracle.returns), boundaries=dict(oracle.boundaries))
        files.update(oracle.exports + oracle.extra);files.add(ROOT / BINARIES[edition]);print(f'{edition}: {len(cases)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 상위 Renderer의 실제 병합/정렬/분배/정상 반환 관찰. 수집기·화면 출력은 명시 응답 경계.\n' +
        '\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows), host='HJOW-Athlon', date='2026-10-11', controls=list(CONTROLS),
        os_calls=0, assertions=0, boundaries=list(BOUNDARIES), editions=editions,
        files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(files)},
        limits=['상위 Renderer 루트 정상 반환, 좌표/클립/병합/CRT/종류 분배/CD 묶음은 실제 전체 몸체',
            '모드 0/128·유효 dirty 한 개·격자/모드 전환/강제 갱신/구름 끔; 전체 함수의 모든 분기 복원을 주장하지 않는다',
            '수집기 완성 바이트/묶음·배경 덮음 응답, 그림/장치 출력만 명시 응답 경계; 실제 SHP/팔레트/GUI 프레임은 범위 밖',
            '묶음 개수 증가/감소/0/음수, tail 보존, 알 수 없는 kind, 5,000항목 용량, 두 x87·ABI·실행/쓰기·SHA 감사']),
        ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def verify():
    """입력 계획/응답/배열 폭/실제 진입/정상 반환/경계 총계를 감사한다. 기대 병합/정렬/분배는 계산하지 않는다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본/도구/내보내기/fixture의 정확한 바이트를 검증한다.
    for name, sha in report['files'].items():
        if digest(ROOT / name) != sha: raise RuntimeError('표시 제출 SHA 불일치: ' + name)
    if report['total'] != len(rows) or report['controls'] != list(CONTROLS) or report['os_calls'] or report['assertions'] or report['boundaries'] != list(BOUNDARIES):
        raise RuntimeError('표시 제출 총계/경계 오류')
    if set(report['editions']) != set(BINARIES): raise RuntimeError('표시 제출 판본 누락')
    # 각 PE의 전체 계획/관찰 폭과 실제 실행 집계를 확인한다.
    for edition, item in report['editions'].items():
        chosen = [row for row in rows if row[0] == edition];entries, boundaries = collections.Counter(), collections.Counter()
        if len(chosen) != len(plan()) or item['cases'] != len(chosen): raise RuntimeError('표시 제출 행 수 오류')
        # 저장한 기대 배열을 생성하지 않고 같은 입력 시나리오가 누락/중복 없이 있는지 확인한다.
        for row, case in zip(chosen, plan(), strict=True):
            if len(row) != 13 or row[1] != ','.join(map(str, case)): raise RuntimeError('표시 제출 입력 순서/폭 오류')
            n = int(row[3])
            if row[2] != packed_inputs(case[0]): raise RuntimeError('표시 제출 완성 입력 오류')
            if not 0 <= n <= CAPACITY or row[12] != (b'\xa5' * 32).hex(): raise RuntimeError('표시 제출 용량/보호 오류')
            if any(len(b'' if value == '-' else bytes.fromhex(value)) != n * STRIDE for value in row[5:7]): raise RuntimeError('표시 제출 배열 폭 오류')
            if row[4] != str(case[0] % 2 if not case[1] & 4 else 0): raise RuntimeError('표시 제출 배경 응답 오류')
            # 정상 반환마다 실제 함수 진입과 명시 경계 호출 집계를 합산한다.
            for address, count in json.loads(row[11]).items(): entries[address] += count * 2
            for name, count in json.loads(row[10]).items(): boundaries[name] += count * 2
        if dict(entries) != item['entries'] or dict(boundaries) != item['boundaries'] or item['returns'] != {f'{SPECS[edition]["root"]:08x}': len(chosen)*2}:
            raise RuntimeError('표시 제출 진입/경계/정상 반환 집계 오류')
    print(f'drawsubmission 검증 통과: {len(rows)}개, 두 정밀도 정상 반환 {len(rows)*2}회, OS/assert 0')


def main():
    """전체 관찰/저장 감사/무저장 표본 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke', action='store_true');parser.add_argument('--verify', action='store_true')
    args = parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
