#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 Gump 위치/경계/트리 수집과 CRT 절삭을 격리 실행한다. 월드 투영만 명시 응답 경계다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, STACK, STOP, digest, fbits,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 원본 노드/출력/위치 반환 저장소다. 주소는 에뮬레이터 안에서만 사용한다.
DATA, ARRAY, RESULT, POOL = 0x15000000, 0x15001000, 0x15002000, 0x15003000
NODE_SIZE, CAPACITY, SEED = 128, 32, 0x46607020
FIXTURE = ROOT / 'cpppj/tests/fixtures/gumpcollection-x86.tsv'
REPORT = ROOT / 'cpppj/recovery-gumpcollection-evidence.json'
# 함수/전역과 월드 투영의 실제 경계 주소다. 나머지 호출은 모두 원본 명령으로 실행한다.
SPECS = {
    'originals': dict(Collect=0x466070, Node=0x464b30, Position=0x464740, Bounds=0x4648e0,
        root=0x563e38, override=0x565dd8, pool=0x5c8464, stride=50, project=0x497220),
    'originalCD': dict(Collect=0x492750, Node=0x4938c0, Position=0x4934c0, Bounds=0x493670,
        root=0x56b748, override=0x535e90, pool=0x5395dc, stride=36, project=0x455c90),
}
SPECS['original1037'] = dict(SPECS['originalCD'])


def node(index, **fields):
    """완성 노드 입력을 만든다. 캐시는 독립된 초기값이며 기대 위치/목록은 계산하지 않는다."""
    value = dict(flags=1, next=0, parent=0, child=0, object=index, depth=0x12348000 + index,
        width=fbits(12.75), height=fbits(9.5), x=fbits(0), y=fbits(0), cached_x=17, cached_y=-19)
    value.update(fields);return value


def plan():
    """엄격한 경계/플래그/제한/override/큰 좌표/현재 링크의 유한 독립 입력 계획이다."""
    result = []
    # 입력을 깊게 복사해 서로 다른 시나리오가 저장소를 공유하지 않게 한다.
    def add(op, nodes, target=1, limit=32, view=(0, 0, 100, 100), override=0, anchors=()):
        """기대 결과 없이 함수와 완성 노드/투영 응답만 저장한다."""
        result.append(dict(op=op, nodes=[dict(n) for n in nodes], target=target, limit=limit,
            view=view, override=override, anchors=list(anchors)))
    # 최상위 A/B와 A의 두 자식이다. root flags는 꺼져 있어도 수집한다.
    for outside in (False, True):
        # 최상위 visible/dead/passChildren을 독립적으로 바꾼다.
        for flags in (0, 1, 0x21, 0x10001, 0x10021):
            # 자식에는 삭제 필터가 없음을 직접 관찰한다.
            for child_flags in (0, 1, 0x10001):
                nodes = [node(0, flags=0, child=1, x=fbits(.75), y=fbits(-.25)),
                    node(1, flags=flags, parent=0, next=2, child=3, x=fbits(150 if outside else 10.9), y=fbits(100 if outside else 8.25)),
                    node(2, parent=0, x=fbits(60), y=fbits(10)),
                    node(3, flags=child_flags, parent=1, next=4, x=fbits(-145 if outside else -1.25), y=fbits(-95 if outside else .5)),
                    node(4, parent=1, x=fbits(15), y=fbits(-.25))]
                # 같은 limit을 각 루트에 주므로 전체 개수는 limit보다 클 수 있다.
                for limit in (-1, 0, 1, 2, 4): add('Collect', nodes, limit=limit)
                add('Node', nodes);add('Collect', nodes, override=3)
    # 맞닿음, 내부 점/선, 역전 영역/크기, 음수 좌표를 실제 정수 교차로 판별한다.
    for width, height, x, y in ((10, 10, 100, 1), (10, 10, -10, 1), (10, 10, 1, 100),
        (10, 10, 1, -10), (0, 0, 20, 20), (0, 10, 20, 20), (-1, 5, 20, 20), (5, -1, 20, 20),
        (10.99, 10.99, -1.99, -.99)):
        nodes = [node(0, child=1), node(1, parent=0, width=fbits(width), height=fbits(height), x=fbits(x), y=fbits(y))]
        # 유효/역전/점 화면 영역을 각각 공급한다.
        for view in ((0, 0, 100, 100), (10, 0, 5, 100), (0, 10, 100, 5), (20, 20, 20, 20)):
            add('Collect', nodes, view=view)
    # float32 저장마다 생기는 반올림과 CRT의 64비트 절삭/하위 DWORD를 관찰한다.
    floats = (fbits(.9), fbits(-.9), fbits(16777216), fbits(-16777216), fbits(2147483648),
        fbits(4294967296), fbits(9223372036854775808), fbits(-9223372036854775808), 0x7fc00001, 0x7f800000, 1, 0x80000000)
    for value in floats:
        nodes = [node(0, x=fbits(.75), y=fbits(-.75)), node(1, parent=0, x=value, y=value, width=fbits(.75), height=fbits(-.75))]
        # 같은 부동소수 입력을 위치/경계/수집의 세 루트에서 관찰한다.
        for op in ('Position', 'Bounds', 'Node'): add(op, nodes)
    # 로컬 float의 절삭과 부모별 저장을 무작위 다단계 입력에서도 확인한다.
    rng = random.Random(SEED)
    for index in range(60):
        nodes = [node(0, x=fbits(rng.randrange(-40, 41)/7), y=fbits(rng.randrange(-40, 41)/9))]
        # 다섯 부모 수준에서 저장 반올림과 절삭을 구분한다.
        for level in range(1, 6):
            nodes.append(node(level, parent=level-1, x=fbits(rng.choice((1, 16777216, -16777216))*rng.randrange(-8, 9)/3),
                y=fbits(rng.randrange(-500, 501)/7), width=fbits(rng.randrange(-30, 101)/9), height=fbits(rng.randrange(-30, 101)/11)))
        add('Position' if index%2 else 'Bounds', nodes, target=5)
    # 월드 부착 자신은 root 위치/캐시를 무시하고, 그 자식은 root까지 계속 누적한다.
    for point in ((30, -20), (16777217, -16777217), (2147483647, -2147483648)):
        # 분수 절삭과 정수 덧셈 넘침 입력을 교차한다.
        for offset in (.9, -1.9, 2147483648):
            nodes = [node(0, child=1, x=fbits(.75), y=fbits(-.75)),
                node(1, flags=0x2001, parent=0, child=2, x=fbits(offset), y=fbits(-offset)),
                node(2, parent=1, x=fbits(-16777216), y=fbits(16777216))]
            # 부착 자신과 일반 자식의 캐시/부모 누적을 나눠 관찰한다.
            for target in (1, 2):
                # 위치와 크기 합산 루트 모두 정상 반환까지 실행한다.
                for op in ('Position', 'Bounds'): add(op, nodes, target=target, anchors=[(1, *point, -1, 0)])
            add('Collect', nodes, view=(-2147483648, -2147483648, 2147483647, 2147483647), anchors=[(1, *point, -1, 0)])
    # 위치 조회의 투영 응답이 next를 바꾼다. 최상위는 저장한 next, 자식은 현재 next를 쓴다.
    for child in (False, True):
        nodes = [node(0, child=1), node(1, parent=0, next=2, child=3 if child else 0, flags=1 if child else 0x2001),
            node(2, parent=0, x=fbits(30)), node(3, parent=0, next=4, flags=0x2001), node(4, parent=0, x=fbits(40))]
        add('Collect', nodes, anchors=[(3 if child else 1, 10, 10, 3 if child else 1, 0)])
    return result


def token(index):
    """입력 번호를 고정한 가상 노드 주소로 바꾼다. 0번도 유효한 root다."""
    return DATA + index*NODE_SIZE


def raw(n):
    """같은 128바이트 배치에 완성 입력을 쓰며 기대 결과는 넣지 않는다."""
    value = bytearray(NODE_SIZE)
    # 링크의 0은 null, parent의 0은 root 번호다. root 자신의 parent만 null이다.
    for name, offset in dict(flags=4, next=12, parent=20, child=24, object=36, depth=48,
        width=68, height=72, x=76, y=80, cached_x=108, cached_y=112).items():
        field = n[name]
        if name in ('next', 'child'): field = token(field) if field else 0
        if name == 'parent': field = 0 if n['object'] == 0 else token(field)
        struct.pack_into('<I', value, offset, field & 0xffffffff)
    return bytes(value)


def encoded(case):
    """입력과 명시 응답을 C++가 재생할 완성 바이트/정수열로 저장한다."""
    config = ','.join(map(str, (token(0), token(case['override']) if case['override'] else 0,
        token(case['target']), case['limit'], *case['view'])))
    nodes = ';'.join(f'{token(i)}={raw(n).hex()}' for i, n in enumerate(case['nodes']))
    anchors = ';'.join(','.join(map(str, (sid, x, y, token(mutate) if mutate >= 0 else 0,
        token(next_) if next_ else 0))) for sid, x, y, mutate, next_ in case['anchors']) or '-'
    return config, nodes, anchors


class GumpCollectionOracle(ProcessOracle):
    """Gump/CRT의 실제 명령만 허용하고 월드 투영 응답과 메모리 변경을 기록한다."""
    def __init__(self, edition):
        """읽기 전용 내보내기의 불연속 범위만 실행을 허용한다."""
        super().__init__(edition);self.d = SPECS[edition]
        self.extra = [ROOT / f'extracted/gumpcollection/{edition}/{name}' for name in ('creation.c', 'functions.tsv')]
        self.allowed, self.entry_names = [], {}
        with self.extra[1].open(encoding='utf-8') as fp:
            # 함수 사이의 코드는 허용하지 않고 실제 함수 범위만 등록한다.
            for row in csv.DictReader(fp, delimiter='\t'):
                self.entry_names[int(row['entry'], 16)] = row['entry']
                # 불연속 몸체도 각각 허용하며 사이 주소는 실행하지 않는다.
                for part in row['ranges'].split(';'):
                    low, high = (int(v, 16) for v in part.split('-'));self.allowed.append((low, high+1))
        self.mu.mem_map(DATA, 0x10000);self.returns, self.boundaries = collections.Counter(), collections.Counter()

    def on_instruction(self, mu, address, size, data):
        """월드 투영은 공급한 정수점으로 응답하며 나머지는 실제 명령을 실행한다."""
        if address == self.d['project']:
            esp, ecx = mu.reg_read(UC_X86_REG_ESP), mu.reg_read(UC_X86_REG_ECX)
            pointer, x, y = struct.unpack('<3I', mu.mem_read(esp+4, 12));sid = x
            response = self.anchors[sid];self.projected.append(str(sid));self.boundaries['project'] += 1
            # 두 판본의 투영은 this와 반환 인자 양쪽에 같은 정수점을 쓴다.
            destination = ecx
            mu.mem_write(ecx, struct.pack('<2i', response[0], response[1]))
            mu.mem_write(pointer, struct.pack('<2i', response[0], response[1]))
            if response[2]: self.put(response[2]+12, response[3])
            mu.reg_write(UC_X86_REG_EAX, destination);mu.reg_write(UC_X86_REG_ESP, esp+16);mu.reg_write(UC_X86_REG_EIP, self.u32(esp));return
        if not any(low <= address and address+size <= high for low, high in self.allowed):
            raise RuntimeError(f'Gump 허용하지 않은 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]] += 1;self.entries[self.entry_names[address]] += 1

    def run(self, case, control):
        """루트 정상 반환/ABI/캐시/전체 출력/보호 바이트를 실제 PE에서 관찰한다."""
        mu, d = self.mu, self.d;self.entries, self.projected = collections.Counter(), []
        config, nodes, anchors = encoded(case);values = list(map(int, config.split(',')))
        self.anchors = {sid: (x, y, token(mutate) if mutate >= 0 else 0, token(next_) if next_ else 0)
            for sid, x, y, mutate, next_ in case['anchors']}
        mu.mem_write(DATA, b'\0' * 0x1000);mu.mem_write(ARRAY-16, b'\xa5' * (CAPACITY*16+32));mu.mem_write(RESULT-16, b'\xa5'*48)
        # 모든 노드와 SID 원본 좌표는 완성 입력이다. 투영 시 x가 SID를 식별한다.
        for i, n in enumerate(case['nodes']):
            mu.mem_write(token(i), raw(n));self.put(POOL+i*d['stride']+14, i);self.put(POOL+i*d['stride']+18, 0)
        # 원본 루트/임시 머리/SID 풀 전역을 입력 값으로 설정한다.
        for address, value in ((d['root'], token(0)), (d['override'], values[1]), (d['pool'], POOL)): self.put(address, value)
        self.writes = [(0, 4), (STACK-0xf000, STACK+0x1000), (ARRAY, ARRAY+CAPACITY*16), (RESULT, RESULT+16)]
        self.writes += [(token(i)+108, token(i)+116) for i in range(len(case['nodes']))]
        # 비영 보존 레지스터로 실제 호출 규약/x87/SEH의 복구를 확인한다.
        for reg, value in REGISTERS.items(): mu.reg_write(reg, value)
        # 호출자의 임시 레지스터는 0에서 시작한다.
        for reg in (UC_X86_REG_EAX, UC_X86_REG_EDX): mu.reg_write(reg, 0)
        mu.reg_write(UC_X86_REG_ECX, values[2]);mu.reg_write(UC_X86_REG_ESP, STACK);mu.reg_write(UC_X86_REG_EFLAGS, 2)
        mu.reg_write(UC_X86_REG_FPCW, control);mu.reg_write(UC_X86_REG_FPSW, 0);self.put(0, 0xffffffff)
        args = [*case['view'], ARRAY, case['limit']] if case['op'] in ('Collect', 'Node') else [RESULT]
        mu.mem_write(STACK, struct.pack('<'+'I'*(len(args)+1), STOP, *(v & 0xffffffff for v in args)))
        mu.emu_start(d[case['op']], STOP, timeout=30000000, count=2000000)
        purge = 0 if case['op'] == 'Collect' else 24 if case['op'] == 'Node' else 4
        if mu.reg_read(UC_X86_REG_EIP) != STOP or mu.reg_read(UC_X86_REG_ESP) != STACK+4+purge or any(mu.reg_read(r) != v for r, v in REGISTERS.items()):
            raise RuntimeError('Gump 정상 반환/스택/보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW) != control or self.u32(0) != 0xffffffff:
            raise RuntimeError('Gump x87/SEH 오류')
        self.returns[f'{d[case["op"]]:08x}'] += 1
        result = str(mu.reg_read(UC_X86_REG_EAX)) if case['op'] in ('Collect', 'Node') else ','.join(map(str,
            struct.unpack('<'+'i'*(2 if case['op']=='Position' else 4), mu.mem_read(RESULT, 8 if case['op']=='Position' else 16))))
        guard = bytes(mu.mem_read(ARRAY-16, 16))+bytes(mu.mem_read(ARRAY+CAPACITY*16, 16))+bytes(mu.mem_read(RESULT-16, 16))+bytes(mu.mem_read(RESULT+16, 16))
        if guard != b'\xa5'*64: raise RuntimeError('Gump 보호 바이트 오류')
        # 위치 반환의 미사용 칸과 수집 출력의 미사용 칸도 보존해야 한다.
        used = int(result)*16 if case['op'] in ('Collect', 'Node') else 0
        if not 0 <= used <= CAPACITY*16 or bytes(mu.mem_read(ARRAY+used, CAPACITY*16-used)) != b'\xa5'*(CAPACITY*16-used):
            raise RuntimeError('Gump 미사용 표시 바이트 오류')
        result_size = 8 if case['op']=='Position' else 16 if case['op']=='Bounds' else 0
        if bytes(mu.mem_read(RESULT+result_size, 16-result_size)) != b'\xa5'*(16-result_size): raise RuntimeError('Gump 미사용 위치 바이트 오류')
        # 캐시와 명시 next 변경 이외의 노드 바이트가 보존됐는지 확인한다.
        caches = []
        for i, n in enumerate(case['nodes']):
            actual = bytearray(mu.mem_read(token(i), NODE_SIZE));caches.append(f'{token(i)},'+','.join(map(str, struct.unpack_from('<2i', actual, 108))))
            expected = bytearray(raw(n));actual[108:116] = expected[108:116]
            # 투영 경계에 명시한 next 변경만 검사에서 제외한다.
            for response in self.anchors.values():
                if response[2] == token(i): actual[12:16] = expected[12:16]
            if actual != expected: raise RuntimeError('Gump 노드의 허용 밖 변경')
        return [config, nodes, anchors, result, bytes(mu.mem_read(ARRAY, CAPACITY*16)).hex(), ';'.join(caches),
            ','.join(self.projected) or '-', json.dumps(dict(self.entries), sort_keys=True, separators=(',', ':')), guard.hex()]


def generate(smoke):
    """두 x87 정밀도의 실제 관찰만 저장한다. C++나 기대 위치/수집 계산은 사용하지 않는다."""
    rows, editions = [], {};files = {ROOT/'tools/decomp_gumpcollection_oracle.py', ROOT/'tools/decomp_process_oracle.py',
        ROOT/'tools/decomp_drawsort_oracle.py', ROOT/'tools/ghidra/gumpcollection-functions.json', FIXTURE}
    # 세 독립 PE에서 같은 완성 입력을 실행한다.
    for edition in BINARIES:
        oracle = GumpCollectionOracle(edition);cases = plan();indexed = list(enumerate(cases))
        if smoke: indexed = indexed[::47]+indexed[-2:]
        # 같은 입력을 두 정밀도에서 실행하고 전체 관찰의 일치를 요구한다.
        for index, case in indexed:
            observed = [oracle.run(case, control) for control in CONTROLS]
            if observed[0] != observed[1]: raise RuntimeError(f'Gump x87 차이: {edition} {index}')
            rows.append([edition, case['op'], str(index), *observed[0]])
        editions[edition] = dict(cases=len(indexed), entries=dict(oracle.hits), returns=dict(oracle.returns), boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과', flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 Gump 위치/경계/트리 수집 관찰. 월드 투영만 명시 응답 경계.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n', encoding='utf-8', newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows), host='HJOW-Athlon', date='2026-10-11', controls=list(CONTROLS), os_calls=0, assertions=0,
        boundaries=['project'], editions=editions, files={p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(files)},
        limits=['최상위/노드 수집, 위치/경계/교차와 CRT 절삭은 실제 전체 몸체',
            '월드→화면 투영만 완성 정수점 응답 경계; SID/투영/실제 Gump 생성/GUI 프레임은 후속',
            'CD 부착 부모 int→float 저장 차이, 부모별 float 저장, 64비트 절삭 하위 DWORD, 실제 캐시/출력/링크 관찰',
            '비영 보존 레지스터/스택/x87/SEH, 실행/쓰기 범위, 전체 노드/미사용/보호 바이트와 SHA 감사']),
        ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')


def verify():
    """SHA/완성 입력/행 수/폭/실제 진입/정상 반환/투영 호출 집계를 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'));rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본/도구/내보내기/fixture의 정확한 바이트를 확인한다.
    for name, sha in report['files'].items():
        if digest(ROOT/name) != sha: raise RuntimeError('Gump SHA 불일치: '+name)
    if report['total'] != len(rows) or report['controls'] != list(CONTROLS) or report['boundaries'] != ['project'] or report['os_calls'] or report['assertions']:
        raise RuntimeError('Gump 총계/경계 오류')
    if set(report['editions']) != set(BINARIES): raise RuntimeError('Gump 판본 누락')
    # 세 판본의 계획과 실제 진입/반환을 따로 집계한다.
    for edition, item in report['editions'].items():
        chosen = [row for row in rows if row[0]==edition];entries, returns, boundaries = collections.Counter(), collections.Counter(), collections.Counter()
        if len(chosen) != len(plan()) or item['cases'] != len(chosen): raise RuntimeError('Gump 행 수 오류')
        # 순서/중복/완성 입력/배열 폭은 확인하되 기대 위치나 목록은 계산하지 않는다.
        for index, (row, case) in enumerate(zip(chosen, plan(), strict=True)):
            if len(row) != 12 or row[1:3] != [case['op'], str(index)] or tuple(row[3:6]) != encoded(case): raise RuntimeError('Gump 입력/폭 오류')
            if len(bytes.fromhex(row[7])) != CAPACITY*16 or row[11] != (b'\xa5'*64).hex(): raise RuntimeError('Gump 배열/보호 오류')
            # 저장 관찰의 진입 횟수는 두 정밀도 실행을 합산한다.
            for address, count in json.loads(row[10]).items(): entries[address] += count*2
            returns[f'{SPECS[edition][case["op"]]:08x}'] += 2
            if row[9] != '-': boundaries['project'] += len(row[9].split(','))*2
        if dict(entries) != item['entries'] or dict(returns) != item['returns'] or dict(boundaries) != item['boundaries']: raise RuntimeError('Gump 실행 집계 오류')
    print(f'gumpcollection 검증 통과: {len(rows)}개, 두 정밀도 정상 반환 {len(rows)*2}회, OS/assert 0')


def main():
    """전체 관찰/무저장 표본/저장 감사 중 하나를 실행한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke', action='store_true');parser.add_argument('--verify', action='store_true')
    args = parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__ == '__main__': main()
