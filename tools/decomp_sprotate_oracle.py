#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 SP 회전/초기화/저장/읽기의 실제 PE 몸체를 격리 x86으로 실행한다. 게임/GUI 실행은 없다.

사용: python -X utf8 tools/decomp_sprotate_oracle.py [--verify]
기존 보상 oracle의 PE 로더/명시 malloc·시각 재시드 경계를 재사용한다. 기대 회전/비트/난수는 계산하지 않는다.
"""
import argparse
import collections
import csv
import hashlib
import json
import struct
from pathlib import Path

from decomp_reward_oracle import RewardOracle, ROOT, ARENA, ARENA_SIZE, STACK

# 고유 입력/근거/내보내기 경로와 실제 회전 진입 주소다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/sprotate-x86.tsv'
EVIDENCE = ROOT / 'cpppj/sprotate.json'
EXPORT = ROOT / 'extracted/sprotate/originals'
ROTATE = 0x491680
# SHA를 고정할 코드/원본 경계 자료다. 실제 원본 파일은 읽기만 한다.
SOURCES = ['tools/decomp_sprotate_oracle.py', 'tools/decomp_reward_oracle.py',
           'tools/ghidra/sprotate-functions.json', 'cpppj/src/o/SpStore.h', 'cpppj/src/o/SpStore.cpp']


def sha(path):
    """지정 경로의 현재 바이트 SHA256을 계산한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases():
    """정상 초기화·모든 소유자/부호 비트·반복 회전/읽기/재저장·9600 경계의 입력만 만든다."""
    result = []
    # 힙 채움의 각 sign/shift 분기와 lazy 회전 초기화를 실제 몸체에서 확인한다.
    for seed in (0, 1, 7, 42, 0x12345678, 0xffffffff):
        for fill in (0, 0xffffffff, 0x80000001, 0x55555555):
            result.append((seed, fill, 'R:1'))
            result.append((seed, fill, 'I;R:1;R:31;R:1'))
    # 소유자마다 전혀 다른 DWORD를 저장하고 회전 사이에도 다시 저장한다.
    for index, seed in enumerate((0, 1, 2, 3, 7, 42, 99, 0x12345678)):
        actions = [f'S:{owner}:{((0x80000001 * (owner+1)) ^ (0x01010101*index)) & 0xffffffff}' for owner in range(9)]
        actions += ['R:1', 'R:17', f'S:{index%9}:{(0xf000000f ^ seed) & 0xffffffff}', 'R:32', 'R:64']
        result.append((seed, 0xcdcdcdcd, ';'.join(actions)))
    # offset0의 마지막 9600 포함/그 다음 회전과 offset99의 비정렬 경계를 원본에서 실행한다.
    # 시드로 특정 offset을 찾는 것은 입력 구성일 뿐이고 회전 기대값은 실제 PE가 돌려준다.
    for target in (0, 97, 98, 99):
        seed = next(value for value in range(1, 100000) if (((value*0x10003+3)&0xffffffff)>>16)%100 == target)
        steps = (9600-target)//3
        result.append((seed, 0x80000001, f'S:0:2147483649;S:8:4294967295;R:{steps};R:1;R:1'))
    # 재초기화는 기존 offset/저장내용을 유지하는 회전과 달리 실제 새 힙 채움으로 교체한다.
    result.append((42, 0xabcdef01, 'S:1:1065353216;R:17;I;R:1'))
    return result


class RotateOracle(RewardOracle):
    """허용 실행 목록을 고유 SP 함수 다섯 개로 좁혀 기존 PE 로더만 재사용한다."""
    def __init__(self):
        """내보낸 실제 함수 범위/허용 쓰기와 malloc/재시드 경계만 준비한다."""
        super().__init__('originals')
        self.allowed, self.entry_names = [], {}
        with (EXPORT/'functions.tsv').open(encoding='utf-8') as handle:
            # 각 실제 몸체의 분리 범위를 모두 허용하고 그 밖 실제 실행은 즉시 거부한다.
            for row in csv.DictReader(handle, delimiter='\t'):
                self.entry_names[int(row['entry'],16)] = row['entry']
                for part in row['ranges'].split(';'):
                    low, high = (int(value,16) for value in part.split('-'))
                    self.allowed.append((low, high+1))
        self.stub_addresses = {self.s['stubs']['reseed'], self.s['stubs']['malloc']}
        g = self.s['g']
        self.writes = [(ARENA, ARENA+ARENA_SIZE), (STACK-0xf000, STACK+0x1000),
                       (g['perm'],g['perm']+64), (g['pools'],g['pools']+128)]
        # 초기화와 회전이 실제로 쓰는 전역만 별도로 허용한다.
        for name in ('rng','inited','offset','init_copy'):
            self.writes.append((g[name],g[name]+4))

    def run(self, seed, fill, actions):
        """처음 상태에서 명령 목록을 실제 실행하고 각 효과 뒤 전체 288 DWORD/offset/SP/난수를 관찰한다."""
        g, s = self.s['g'], self.s
        self.seed, self.fill, self.malloc_next = seed, fill, 0
        self.mu.mem_write(ARENA, bytes(ARENA_SIZE))
        self.stub_hits.clear(); self.hits.clear()
        # 호스트 준비 쓰기는 실행 훅이 아닌 합성 입력이며 원본 파일과 관계없다.
        for name in ('rng','inited','offset','init_copy'):
            self.put(g[name],0)
        self.mu.mem_write(g['perm'],bytes(64)); self.mu.mem_write(g['pools'],bytes(128))
        snapshots = []
        # 각 명령은 원본 진입점을 통하여 실행한다. R의 내부 반복도 매번 실제 회전을 호출한다.
        for action in actions.split(';'):
            fields = action.split(':')
            if fields[0] == 'I':
                self.call(0x40eba0,[],0x027f)
            elif fields[0] == 'S':
                self.call(0x40edb0,[int(fields[1]),int(fields[2])],0x027f)
            else:
                for _ in range(int(fields[1])):
                    self.call(ROTATE,[],0x027f)
            values = []
            # 모든 소유자의 논리 값도 별도 actual Get 몸체에서 관찰한다.
            for owner in range(9):
                self.call(0x40ec50,[owner,ARENA+0x1f000],0x027f)
                values.append(self.u32(ARENA+0x1f000))
            memory = b''.join(bytes(self.mu.mem_read(self.u32(g['pools']+bit*4),36)) for bit in range(32))
            snapshots.append(f"{self.u32(g['offset'])},{self.u32(g['init_copy'])},{self.u32(g['rng'])},"+
                             ','.join(map(str,values))+':'+memory.hex())
        return '|'.join(snapshots), dict(self.hits), dict(self.stub_hits)


def generate():
    """원본 결과를 fixture/근거에 보존한다. 실패는 중간 파일로 성공 자료를 대체하지 않는다."""
    oracle = RotateOracle(); rows = []; hits = collections.Counter(); stubs = collections.Counter()
    for index, (seed,fill,actions) in enumerate(cases()):
        result, observed, boundaries = oracle.run(seed,fill,actions)
        rows.append('\t'.join(map(str,(seed,fill,actions,result))))
        hits.update(observed); stubs.update(boundaries)
        if index%10 == 0:
            print(f'SP 실제 PE 진행 {index+1}/{len(cases())}',flush=True)
    payload = '# 10.78 SP 실제 회전/초기화/읽기/쓰기; seed fill actions snapshots\n'+'\n'.join(rows)+'\n'
    FIXTURE.write_text(payload,encoding='utf-8',newline='\n')
    files = SOURCES+['originals/Netstorm.exe',str(EXPORT.relative_to(ROOT)/'creation.c'),str(EXPORT.relative_to(ROOT)/'functions.tsv')]
    report = dict(host='VM-W11-CODEX',date='2026-10-13',cases=len(rows),entries=dict(hits),boundaries=dict(stubs),
                  fixture_sha256=sha(FIXTURE),files={name:sha(ROOT/name) for name in files},
                  limits=['malloc(0x24)/시각 재시드만 대체','게임/GUI 실행 없음','CD에는 이 SP 회전 몸체가 없음'])
    EVIDENCE.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'SP 회전 독립 원본 입력 {len(rows)}개 저장',flush=True)


def verify():
    """현재 원본/몸체/코드/fixture와 입력 수·9600 경계·모든 함수 실제 호출을 감사한다."""
    report = json.loads(EVIDENCE.read_text(encoding='utf-8'))
    assert report['fixture_sha256'] == sha(FIXTURE)
    for name,expected in report['files'].items():
        assert sha(ROOT/name) == expected, name
    rows = [row for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    assert len(rows) == len(cases()) == report['cases']
    assert set(report['entries']) == {'00491680','0040eba0','0040ec50','0040edb0','004558c0'}
    assert any('R:3200' in row for row in rows)
    assert set(report['boundaries']) == {'malloc','reseed'}
    print(f"SP 회전 SHA/실제 함수/입력 감사 {len(rows)}개 통과")


def main():
    """생성/재감사 명령을 분리해 사용한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    options = parser.parse_args()
    verify() if options.verify else generate()


if __name__ == '__main__':
    main()
