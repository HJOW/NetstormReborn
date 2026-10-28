#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 고정 금액 송금 명령에서 실제 분기·인자·패킷을 독립 관측한다."""
import hashlib
import itertools
import json
import socket
from pathlib import Path
import decomp_production_shortcuts as gate
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX

# 현재 사건 부모와 분리한 Player9 기록 및 새 관측 출력이다.
ROOT = Path(__file__).resolve().parents[1]
PLAYERS = gate.HEAP + 0x3000
FIXTURE = ROOT / 'cpppj/tests/fixtures/send-money-input-x86.tsv'
REPORT = ROOT / 'extracted/cpp-send-money-547/native-input.json'
HEADER = (0x40af90, 0x40afcf)
SCHEMA = ('name', 'code', 'target', 'local', 'flags', 'afford', 'guide', 'gump', 'console',
          'pause', 'mission', 'single', 'diagnostic', 'effects', 'completed', 'stackdelta')


class Oracle(gate.Oracle):
    """메뉴/자금 판정·가산·전송은 명시 장치로 두고 원본 송금 분기를 실행한다."""
    def __init__(self):
        """실제 메시지 생성자는 추가 허용하고 본체 바이트를 보존한다."""
        super().__init__()
        self.original[HEADER[0]] = bytes(self.mu.mem_read(HEADER[0], HEADER[1] - HEADER[0]))

    def instruction(self, mu, address, size, data):
        """실제 CALL의 금액 float 비트와 Player 번호 및11바이트 패킷을 읽는다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if HEADER[0] <= address < HEADER[1]:
            self.instructions += 1
            return
        if address == 0x425e90:
            self.event('afford', (self.number(sp+4),))
            self.returned(self.input['afford'])
            return
        if address in (0x41dff0, 0x40eec0, 0x413bb0):
            name = {0x41dff0: 'addOwner', 0x40eec0: 'addLocal', 0x413bb0: 'wallet'}[address]
            count = 1 if address == 0x40eec0 else 2
            self.event(name, tuple(self.number(sp+4+i*4) for i in range(count)))
            self.returned()
            return
        if address == 0x4df230:
            # 길이 설정 장치만 대체하고 생성자의 나머지 BYTE 쓰기는 원문을 실행한다.
            self.number(mu.reg_read(UC_X86_REG_ECX)+2, self.number(sp+4), 2)
            self.returned(cleanup=4)
            return
        if address == 0x4e2dc0:
            self.event('send', (self.number(sp+4), bytes(mu.mem_read(self.number(sp+8), 11)).hex()))
            self.returned()
            return
        if address == 0x4e0620:
            self.event('assert')
            self.returned()
            return
        super().instruction(mu, address, size, data)

    def observe(self, row, control):
        """독립 입력의 현재 로컬/Player flags와 실제 Guide 관문을 설치한다."""
        self.input = row
        self.number(0x59534c, PLAYERS)
        self.mu.mem_write(PLAYERS, bytes(9*180))
        self.number(PLAYERS+row['target']*180+0x50, row['flags'])
        self.number(0x540c70, row['local'])
        self.number(0x594fa4, row['mission'])
        self.number(0x5e4794, row['single'])
        self.number(0x5c85a4, 0)
        self.number(0x55a480, row['diagnostic'])
        self.number(0x55a484, 0)
        case = dict(command=0x020000f4, read=0, filled=0,
                    guideResult=row['guide'], gumpResult=row['gump'], console=row['console'],
                    pauseDepth=row['pause'], events=f"{row['code']}:71:{row['target']}")
        result = super().run(case, control)
        wanted = {'afford', 'addOwner', 'addLocal', 'wallet', 'send', 'assert'}
        return dict(effects=';'.join(x for x in self.trace if x.split(':', 1)[0] in wanted) or '-',
                    completed=result['completed'], stackdelta=result['stackdelta'])


def cases():
    """금액·자금·AI/연결 플래그·수정키와 선행 소비의 실제 입력만 생성한다."""
    rows, seen = [], set()

    def add(name, **changes):
        """기대 효과를 만들지 않고 이름을 제외한 중복 입력만 제거한다."""
        row = dict(name=name, code=0x02000120, target=2, local=1, flags=0x22, afford=1,
                   guide=0, gump=0, console=0, pause=0, mission=1, single=1, diagnostic=0)
        row.update(changes)
        key = tuple(row[n] for n in SCHEMA[1:13])
        if key not in seen:
            seen.add(key)
            rows.append(row)

    # 현재 명령은 release와 별도 수정키가 있어도 공통 입력 부모가 판단한다.
    for code, flags, afford, target, keyflags, pause in itertools.product(
            (0x02000120, 0x02000108, 0x020000ec), (0, 1, 0x20, 0x21, 0x22, 0xffffffff),
            (0, 1), (1, 2, 8), (0, 0x80000000, 0x10000000, 0x20000000), (0, 3)):
        add(f'fixed-{code:x}-{flags:x}-{afford}-{target}-{keyflags:x}-{pause}',
            code=code|keyflags, flags=flags, afford=afford, target=target, pause=pause)
    # Guide/Gump/모달 전역 및 원문 AI 진단의 현재 모드를 따로 관측한다.
    for code, guide, gump, console in itertools.product((0x02000120, 0x02000108, 0x020000ec), (0, 7), (0, 11), (0, 1)):
        add(f'gates-{code:x}-{guide}-{gump}-{console}', code=code, guide=guide, gump=gump, console=console)
    for mission, single in itertools.product((0, 1), (0, 1)):
        add(f'AI-mode-{mission}-{single}', mission=mission, single=single)
    return rows


def main():
    """원문 해시와 두 정밀도 관측 일치를 확인한 뒤 새 자료만 저장한다."""
    assert gate.digest(gate.PE_PATH) == gate.PE_SHA
    oracle, output = Oracle(), []
    # 같은 각 입력을 두 x87 정밀도에서 처음부터 실행한다.
    for row in cases():
        try:
            first, second = oracle.observe(row, 0x27f), oracle.observe(row, 0x37f)
        except Exception as error:
            raise RuntimeError(row['name']) from error
        assert first == second, row['name']
        output.append({**row, **first})
    FIXTURE.write_bytes(('\t'.join(SCHEMA)+'\n'+''.join('\t'.join(str(row[n]) for n in SCHEMA)+'\n' for row in output)).encode('utf8'))
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = dict(host=socket.gethostname(), peSha256=gate.PE_SHA, uniqueInputs=len(output),
                  nativeObservations=len(output)*2, precisionDifferences=0, nativeBodiesUnchanged=True,
                  fixtureSha256=hashlib.sha256(FIXTURE.read_bytes()).hexdigest(), ranges=gate.RANGES+(HEADER,),
                  externalDevices=['CanAfford', 'AddSp', 'AI wallet', 'packet length', 'send', 'assert', 'Guide/Gump'],
                  scope='일반 로컬 미션의 고정 송금 입력. 실제 네트워크/멀티플레이 실행 없음')
    REPORT.write_bytes((json.dumps(report, ensure_ascii=False, indent=2)+'\n').encode('utf8'))
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
