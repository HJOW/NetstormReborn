#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 MissionFirst/현재 안내와 Guide 사건·F8의 제한된 입력 분기를 제품 작성 전에 관측한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, POOL, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESP

MISSION, GUIDE, TABLE, EVENT = POOL, POOL+0x1000, POOL+0x2000, POOL+0x3000  # 격리된 미션·Guide·가상표·사건 입력이다.
PAUSE, INPUT = STOP+0x300, STOP+0x320  # 실제 가상 효과의 명시 외부 경계다.
FIXTURE = ROOT/'cpppj/tests/fixtures/mission-first-x86.tsv'  # 제품 코드와 독립적으로 먼저 동결할 원문이다.
REPORT = ROOT/'cpppj/mission-first-native.json'  # 함수 정상 반환과 내부 분기 관측을 구분한다.


def cases():
    """첫 순번/현재 안내/원본 명령의 상위 비트와 F8 modifier만 독립 입력으로 교차한다."""
    rows = []
    # 열은 op,flags,sequence,demo,stage,shown,tellReturn,current,eventCode,eventResult다.
    for sequence, flags, demo, stage, result in itertools.product((0, 1, 4, 0xffffffff), (0, 0xffffffff), (0, 1), (65, 71, 0, 255), (0, 1, 0xffffffff)):
        rows.append((0, flags, sequence, demo, stage, 0x12345678, result, 1, 0, 0))
    # Guide 사건은 알려진 First와 미처리 사건만 관측한다. 다른 다섯 명령의 효과를 주장하지 않는다.
    for current, code, result, sequence in itertools.product((0, 1), (0x2000091, 0x82000091, 0xfa000091, 0x770000, 0, 0x2000abc), (0, 1, 0xffffffff), (0, 4)):
        rows.append((1, 0xa5a50000, sequence, 0, 68, 0x12345678, 1, current, code, result))
    # 이 내부 분기의 전제는 key-down F8 및 활성 미션이다. release·다른 키·디버그/편집/도전 모드는 별도다.
    for flags, current in itertools.product((0, 0x10000000, 0x20000000, 0x30000000, 0x8000000, 0x4000000, 0x40000000), (1, 7)):
        rows.append((2, 0, 4, 0, 65, 0, 0, current, 0x770000|flags, 0))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """세 실제 함수와 F8의 한정된 내부 명령만 실행하고 큰 UI 효과는 인자를 기록한다."""
    def __init__(self):
        """기존 읽기 전용 PE 로더와 격리 메모리를 사용하며 게임/OS는 시작하지 않는다."""
        super().__init__()
        self.allowed = [];ranges(self, (0x4c2a90, 0x463e40))
        self.allowed.append((0x4c2a70, 0x4c2a81))  # 정밀 표에서 빠진 가상+4의 실제 17바이트 몸체다.
        self.allowed.extend(((0x4d9611, 0x4d9650), (0x4d964c, 0x4d9650)))
        self.calls = collections.Counter();self.instructions = 0  # 실제 진입 횟수와 명령 수다.
        self.put(TABLE+4, 0x4c2a70);self.put(TABLE+0x24, INPUT);self.put(TABLE+0x28, PAUSE)

    def instruction(self, mu, address, size, data):
        """실제 분기/문자열/가상 분배를 유지하고 Tell·팝업·정지·하위 입력·큐만 대체한다."""
        self.instructions += 1
        if address in (0x4c2a70, 0x4c2a90, 0x463e40):self.calls[f'{address:08x}'] += 1
        sp = mu.reg_read(UC_X86_REG_ESP);args = struct.unpack('<3I', mu.mem_read(sp+4, 12))
        if self.operation == 2 and address in (0x4d9650, 0x4d9670):
            self.slice_stop = address;mu.emu_stop();return
        if address == 0x482de0:
            if mu.reg_read(UC_X86_REG_ECX) != MISSION:raise RuntimeError('Tell 대상 오류')
            text=bytes(mu.mem_read(args[0], 5)).split(b'\0', 1)[0]  # 원본 char255/0도 텍스트 변환 없이 바이트로 보존한다.
            self.event('tell', text.hex());self.ret(self.case[6], 4);return
        if address == 0x466710:self.event('close');self.ret();return
        if address == PAUSE:
            if mu.reg_read(UC_X86_REG_ECX) != MISSION:raise RuntimeError('정지 대상 오류')
            self.event('pause');self.ret();return
        if address == INPUT:
            if mu.reg_read(UC_X86_REG_ECX) != MISSION or args[0] != EVENT:raise RuntimeError('하위 입력 대상 오류')
            self.event('input', *struct.unpack('<3I', mu.mem_read(EVENT, 12)));self.ret(self.case[9], 4);return
        if address == 0x452fe0:self.event('queue', *args);self.ret(1);return
        if not any(low <= address and address+size <= high for low, high in self.allowed):raise RuntimeError(f'미검토 명령 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """원본 함수는 미션 flags/마지막 안내 결과와 한정 스택 이외에 쓸 수 없다."""
        if STACK-0x4000 <= address and address+size <= STACK+0x100:return
        if any(low <= address and address+size <= high for low, high in ((MISSION+0x54, MISSION+0x58), (MISSION+0xb4, MISSION+0xb8))):return
        raise RuntimeError(f'미션 First 쓰기 범위 밖 {address:08x}/{size}')

    def sample(self, case):
        """전체 미션 바이트와 반환/효과를 수집하고 내부 분기는 함수 반환으로 세지 않는다."""
        self.case=case;self.operation=case[0];self.events=[];self.slice_stop=0
        raw=bytearray((index*37+41)&255 for index in range(0xd0))
        # 관련 필드만 독립 입력으로 쓰고 그 밖의 stale 바이트도 최종 비교한다.
        for offset, value in ((0, TABLE), (0x54, case[1]), (0x58, case[2]), (0x68, case[3]), (0xb4, case[5])):struct.pack_into('<I', raw, offset, value)
        self.mu.mem_write(MISSION, bytes(raw));self.put(GUIDE+0x800, MISSION if case[7] else 0)
        self.mu.mem_write(EVENT, struct.pack('<3I', case[8], 0x12345678, 0x87654321));self.put(0x5ca8e8, case[4]);self.put(0x594fa0, case[7])
        self.mu.mem_write(STACK-0x4000, b'\xa5'*0x4100);self.put(STACK, STOP);self.put(STACK+4, EVENT)
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.mu.reg_write(UC_X86_REG_EAX, 0xccccdddd);self.mu.reg_write(UC_X86_REG_ECX, MISSION if case[0]==0 else GUIDE)
        preserved={UC_X86_REG_EBX:0x11112222, UC_X86_REG_EDI:0x33334444}
        # 함수가 보존해야 하는 비영 레지스터를 넣는다. F8 slice의 EBX는 원본 입력 코드다.
        for register, value in preserved.items():self.mu.reg_write(register, value)
        if case[0]==2:
            self.mu.reg_write(UC_X86_REG_EBX, case[8]);self.put(STACK+0x14, case[8]);entry=0x4d9611
        else:entry=0x4c2a70 if case[0]==0 else 0x463e40
        self.mu.emu_start(entry, STOP, count=10000)
        if case[0]==2:
            if not self.slice_stop or self.mu.reg_read(UC_X86_REG_ESP)!=STACK:raise RuntimeError('F8 내부 분기 종료/스택 오류')
            result='slice'
        else:
            if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+(8 if case[0]==1 else 4):raise RuntimeError('First 정상 반환/스택 오류')
            for register, value in preserved.items():
                if self.mu.reg_read(register)!=value:raise RuntimeError('First 보존 레지스터 오류')
            result=str(self.mu.reg_read(UC_X86_REG_EAX)) if case[0]==1 else 'void'
        return '|'.join((result, bytes(self.mu.mem_read(MISSION, 0xd0)).hex(), ';'.join(self.events) or '-'))


def main():
    """최초 관측을 저장하거나 보호 해시만 확인하며 제품/C++검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name, expected in record['sha256'].items():
            if digest(ROOT/name)!=expected:raise RuntimeError('보호 해시 불일치: '+name)
        print('MissionFirst 최초 원문·관측 도구·두 판본/정밀 해시 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 원문 덮어쓰기 금지')
    oracle=Oracle();rows=cases();lines=['# 원본 함수 First/Guide 사건 및 key-down 활성 미션 F8 내부 분기의 독립 관측이다.']
    # 기대값은 제품 코드 없이 실제 PE 실행의 전체 결과만 기록한다.
    for case in rows:lines.append(','.join(map(str, case))+'\t'+oracle.sample(case))
    FIXTURE.write_text('\n'.join(lines)+'\n', encoding='utf-8', newline='\n')
    record={'host':socket.gethostname(),'decompileHost':'HJOW-Athlon / 2026-10-20','uniqueInputs':len(rows),
        'normalReturns':sum(case[0]!=2 for case in rows),'sliceObservations':sum(case[0]==2 for case in rows),
        'instructions':oracle.instructions,'calls':dict(oracle.calls),'gameExecuted':False,'osExecuted':False,
        'boundaries':['482de0 Tell','466710 팝업 Kill','가상+28 정지','가상+24 하위 입력','452fe0 큐'],
        'scope':'First/현재 안내 두 함수와 Guide 사건의 First/미처리 입력만. F8은 4d9611~4d9650/4d9670 내부 분기이며 전체 UserInput 반환이 아니다. release·다른 키·비활성/도전·편집·디버그 효과는 관측하지 않는다.',
        'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'originalCD/Netstorm.exe',ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'tools/decomp_mission_first.py',FIXTURE)}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:record[key] for key in ('uniqueInputs','normalReturns','sliceObservations','instructions','calls')},ensure_ascii=False))


if __name__=='__main__':
    main()
