#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Missile 충돌의 Early5 명령 생산자를 독립 PE 실행으로 관찰한다. 게임/OS는 실행하지 않는다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_thunder_missile import Oracle as BaseOracle, KEYS as BASE_KEYS, sample
from decomp_owner_oracle import ROOT, TYPES, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

KEYS = (*BASE_KEYS, 'targetType', 'displayType', 'firingState')  # 기존 독립 숫자 입력 뒤 실제 타입/발사자 상태를 추가한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/thunder-hit-event-x86.tsv'  # 기존 썬더 관측은 덮어쓰지 않는다.
REPORT = ROOT / 'cpppj/thunder-hit-event-native.json'  # 실행 전 고정한 기대값과 실제 코드/명시 경계의 출처다.


def cases():
    """충돌/미충돌·dead/free·코너·타입 불일치·종료·억제 플래그를 독립 입력으로 구성한다."""
    rows = []  # 반환 전 고유 입력만 남길 작업 목록이다.
    # 보간 시각과 유효/무효 이전 좌표를 타입/수명/코너와 모두 교차한다.
    for fraction, state, previous, corner, target, display, source in itertools.product(
            (0, 3, 4, 8), range(4), (0, 1), (0, 1), (77, 78), (80, 132), (0, 2)):
        rows.append(sample(kind=1, endx=sample()['x'], endy=struct.unpack('<I', struct.pack('<f', 90))[0],
                           fraction=fraction, targetstate=state, previous=previous, corner=corner,
                           range=struct.unpack('<I', struct.pack('<f', 40))[0], impactSound=456,
                           targetType=target, displayType=display, firingState=source))
    # 이미 통과한 투사체와 충돌 억제 비트는 실제 정면 충돌 입력에서 별도로 교차한다.
    for passed, extra, corner, target, display, source in itertools.product((0, 1), (0, 16), (0, 1), (77, 78), (80, 132), (0, 2)):
        rows.append(sample(kind=1, endx=sample()['x'], endy=struct.unpack('<I', struct.pack('<f', 90))[0],
                           fraction=4, previous=1, corner=corner, passed=passed, extra=extra,
                           range=struct.unpack('<I', struct.pack('<f', 40))[0], impactSound=456,
                           targetType=target, displayType=display, firingState=source))
    # 사거리 종료가 겹친 충돌에서도 사건과 피해·소리의 순서를 실제 원본에서 읽는다.
    for fraction, state, corner, target, display, source in itertools.product((4, 8), range(4), (0, 1), (77, 78), (80, 132), (0, 2)):
        rows.append(sample(kind=1, endx=sample()['x'], endy=struct.unpack('<I', struct.pack('<f', 90))[0],
                           fraction=fraction, targetstate=state, previous=1, corner=corner,
                           range=struct.unpack('<I', struct.pack('<f', 9))[0], impactSound=456,
                           targetType=target, displayType=display, firingState=source))
    return list({tuple(row[key] for key in KEYS): row for row in rows}.values())


class Oracle(BaseOracle):
    """기존 실제 비행 몸체를 재사용하되 함수 진입 입력과 외부 명령 큐만 확장한다."""
    def on_instruction(self, mu, address, size, data):
        """프레임 첫 명령 전에 입력을 구성하고 00452fe0의 세 인자를 그대로 기록한다."""
        if address == self.p['frame']:
            # 원본 숫자 산술/분기를 실행하기 전에 실제 슬롯/타입 자료만 공급한다.
            for sid, offset, value in ((60, 10, self.case['targetType']), (90, 10, self.case['displayType']), (50, 11, self.case['firingState'])):
                mu.mem_write(self.slot(sid) + offset, bytes([value]))
            target = TYPES + self.case['targetType'] * self.type_stride  # 발자국은 기존 독립 입력2×3과 같다.
            display = TYPES + self.case['displayType'] * self.type_stride  # 소리는 두 표시 타입 모두 같은 독립 토큰이다.
            mu.mem_write(target, bytes(self.type_stride))
            mu.mem_write(target + 0x1d4, struct.pack('<2i', 2, 3))
            mu.mem_write(display + 0x74, struct.pack('<I', self.case['impactSound']))
            mu.mem_write(0x541190, struct.pack('<I', 78))
        if address == 0x452fe0:
            esp = mu.reg_read(UC_X86_REG_ESP)  # 실제 cdecl 세 인자와 반환 주소를 읽는다.
            args = [self.read(esp + 4 + index * 4) for index in range(3)]
            self.events.append('E:' + ':'.join(map(str, args)))
            self.boundaries['00452fe0'] += 1
            mu.reg_write(UC_X86_REG_EAX, 0)
            mu.reg_write(UC_X86_REG_ESP, esp + 4)
            mu.reg_write(UC_X86_REG_EIP, self.read(esp))
            return
        super().on_instruction(mu, address, size, data)


def main():
    """기존 자료를 변경하지 않고 새 원본 기대값을 고정하거나 저장된 해시를 감사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 실제 원본/수출 범위/실행기/새 fixture를 각각 바이트 단위로 대조한다.
        for name, expected in report['sources'].items():
            assert digest(ROOT / name) == expected, name
        rows = [row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows) == report['uniqueInputs'] == len({tuple(row[:len(KEYS)]) for row in rows})
        assert all(len(row) == len(KEYS) + 4 for row in rows)
        assert sum('E:' in row[len(KEYS)] for row in rows) == report['eventRows'] > 0
        print('native thunder hit event verified:', len(rows))
        return
    assert not FIXTURE.exists() and not REPORT.exists(), '기존 원본 관측을 덮어쓰지 않는다.'
    oracles = [Oracle(0x027f), Oracle(0x037f)]  # 같은 입력을 두 x87 정밀도에서 독립 실행한다.
    assert oracles[0].read(0x5429c8) == 0x020000b5 and oracles[0].read(0x541190) == 78
    rows = []  # 원본이 반환한 이벤트/128BYTE/150BYTE/좌표를 그대로 저장한다.
    differences = []  # 정밀도 차이가 있다면 숨기지 않고 별도 관측으로 보존한다.
    # 기대값은 원본 CPU 실행 출력만 사용하며 제품 C++는 실행하거나 읽지 않는다.
    for index, case in enumerate(cases()):
        try:
            output = [oracle.run_body(dict(case)) for oracle in oracles]
        except Exception as error:
            raise RuntimeError(f'입력{index} {case}: {error}') from error
        if output[0] != output[1]:
            differences.append(dict(index=index, secondary=output[1]))
        rows.append('\t'.join([str(case[key]) for key in KEYS] + output[0]))
    FIXTURE.write_text('# 독립10.78 실제 Missile.Frame/타입 조회/좌표/충돌. 명령 큐는 외부 경계다.\n' + '\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    sources = ('originals/Netstorm.exe', 'extracted/refined/originals/functions.tsv',
               'tools/decomp_thunder_hit_event.py', 'tools/decomp_thunder_missile.py', 'tools/decomp_crossbow_missile.py',
               'tools/decomp_gunmissile_oracle.py', 'tools/decomp_owner_oracle.py', FIXTURE.relative_to(ROOT).as_posix())
    report = dict(host=socket.gethostname(), lastDecompileHost='VM-W11-Codex', lastDecompileDate='2026-10-19',
                  uniqueInputs=len(rows), returnsPerPrecision=[oracle.returns for oracle in oracles], precisionDifferences=differences,
                  eventRows=sum('E:' in row.split('\t')[len(KEYS)] for row in rows),
                  nativeCalls=dict(oracles[0].native_calls), boundaries=dict(oracles[0].boundaries),
                  sources={name: digest(ROOT / name) for name in sources},
                  originalGlobals={'005429c8': '020000b5', '00541268': 132, '00541190': 78},
                  limitations=['독립 입력의 제한 x86 실행이며 실제 게임/OS 실행이나 자연 완주가 아니다.',
                               'SHP/공간 효과/피해/사운드/입력 큐는 명시 외부 경계다. 큐 본문이 아닌 실제 호출 인자와 순서를 고정한다.'])
    REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('native thunder hit event:', len(rows), 'rows;', report['eventRows'], 'event rows;', len(differences), 'precision differences')


if __name__ == '__main__':
    main()
