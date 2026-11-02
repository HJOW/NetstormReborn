#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""D/d 최근 생산 항목 키의 정지 뒤 예약과 실제40칸 FIFO를 보호10.78 기계어로 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import decomp_production_shortcuts as base


class Oracle(base.Oracle):
    """기존 Guide/Gump/정지/큐 전체 입력 관측기를 같은 보호 PE에서 빌린다."""

    def __init__(self):
        """일반 입력 몸체와 정상 시작의 개발자0을 그대로 사용한다."""
        super().__init__()
        self.original[0x4d53f0] = bytes(self.mu.mem_read(0x4d53f0, 0x90))

    def instruction(self, mu, address, size, data):
        """숫자 fallback은 원문 그대로 실행하며 외부 효과로 합성하지 않는다."""
        if 0x4d53f0 <= address < 0x4d5480:
            self.instructions += 1
            self.visited.add(address)
            return
        return super().instruction(mu, address, size, data)

    def run(self, case, control):
        """getlastitem 명령만 독립 현재값으로 공급하며 실제 입력/예약 본체는 대체하지 않는다."""
        self.number(0x542b10, case['command'])
        self.number(0x55a480, 0)
        self.number(0x55a484, 0)
        return super().run(case, control)


def cases():
    """상위 플래그·뗌·정지·선행 소비와 현재 명령/큐 경계의 입력만 생성한다."""
    rows, seen = [], set()  # 입력 중복은 제거하고 실제 원문 결과는 계산하지 않는다.

    def add(name, events, **changes):
        """현재 사건열과 외부 Guide/Gump/Console 응답을 독립 입력으로 보관한다."""
        case = dict(name=name, events=';'.join(':'.join(str(value & 0xffffffff) for value in event) for event in events),
                    guideResult=0, gumpResult=0, pauseDepth=0, console=0,
                    command=0x02000107, read=0, filled=0)
        case.update(changes)
        key = tuple(case[field] for field in base.SCHEMA[1:9])
        if key not in seen:
            seen.add(key)
            rows.append(case)

    # outside/alt/control/shift/pressed 다섯 비트와 뗌/정지의 모든 조합이다.
    for key, flags, release, pause in itertools.product((ord('D'), ord('d')), range(32), (0, 0x80000000), (0, 3, 0xffffffff)):
        add(f'flags-{key}-{flags}-{release:x}-{pause}', [(key | (flags << 26) | release, 71, 83)], pauseDepth=pause)
    # Guide/Gump의 비영 소비와 현재Console은 D/d 예약보다 앞이다.
    for key, guide, gump, console, pause in itertools.product((ord('D'), ord('d')), (0, 7), (0, 11), (0, 1), (0, 3)):
        add(f'gates-{key}-{guide}-{gump}-{console}-{pause}', [(key, 71, 83)], guideResult=guide, gumpResult=gump, console=console, pauseDepth=pause)
    # 실제 큐의 감김/가득참과 현재 명령의 원래 마스킹을 함께 관측한다.
    for read, filled, command in itertools.product((0, 17, 39), (0, 38, 39), (0, 0x02000107, 0xabcdef12, 0xffffffff)):
        add(f'fifo-{read}-{filled}-{command}', [(ord('D'), 23, 47), (0x80000000 | ord('D'), 29, 53), (0x38000000 | ord('d'), 31, 59)], read=read, filled=filled, command=command)
    # 가상D와 같은 하위키가 들어간 일반명령은 글자 D/d가 아니다.
    for code in (ord('F'), ord('f'), 0x02000044, 0x440000):
        add(f'near-{code}', [(code, 11, 13)])
    return rows


def main():
    """두 x87 정밀도의 결과/스택/본체 불변과 새fixture SHA를 함께 기록한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    assert base.digest(base.PE_PATH) == base.PE_SHA
    oracle, inputs, observed = Oracle(), cases(), []  # 읽기 전용 원문과 새 관측 저장소다.
    # 같은 원시 입력을 두 정밀도로 실행하며 명령 예약/물리 큐가 다르면 중단한다.
    for case in inputs:
        first = oracle.run(case, 0x27f)
        second = oracle.run(case, 0x37f)
        assert first == second, (case['name'], first, second)
        assert all(value == '0' for value in first['stackdelta'].split(';')), case['name']
        observed.append({**case, **first})
    args.fixture.write_text('\t'.join(base.SCHEMA)+'\n'+'\n'.join('\t'.join(str(row[field]) for field in base.SCHEMA) for row in observed)+'\n', encoding='utf-8', newline='\n')
    record = dict(unit=756, host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'),
                  inputs=len(inputs), nativeObservations=len(inputs)*2, precisionDifferences=0, stackDelta=0,
                  nativeRanges=[[hex(low), hex(high)] for low, high in (*base.RANGES, (0x4d53f0, 0x4d5480))],
                  repeatBranch='004d96fa..004d9705', pauseQueryBefore='004d96b6',
                  commandAddress='00542b10', initialCommand=0x02000107, gameExecutions=0,
                  nativeBodiesUnchanged=True, peSha256=base.PE_SHA,
                  fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  sharedOracleSha256=hashlib.sha256(Path(base.__file__).read_bytes()).hexdigest(),
                  totals=oracle.totals, boundaries=dict(oracle.boundaries),
                  explicitBoundaries=['Guide virtual+24', 'Gump Route', 'general input cleanup'],
                  scope='비멀티 D/d 키/일반 getlastitem FIFO. OS 키/게임 실행/MP 통신/전체UI 없음')
    args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(inputs=len(inputs), nativeObservations=len(inputs)*2, fixtureSha256=record['fixtureSha256'])))


if __name__ == '__main__':
    main()
