#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 훨리기그 caller의20BYTE 방송 패킷 작성 구간과 미정 필드를 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, PROCESS, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

POOL = 0x14000000  # 최대 원본SID32767을 담는 별도 격리 풀이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/flyer-command-x86.tsv'  # 제품 결과를 읽기 전에 동결하는 새 원문이다.
REPORT = ROOT/'cpppj/flyer-command-native.json'  # 구간 실행 범위와 미정 바이트 경계를 기록한다.


def cases():
    """정상 SID·두 실제 명령과 서로 다른 기존 패킷 바이트를 교차한다."""
    return list(itertools.product((0,50,999,15065,32767),(2,3),(0,0xa5,0xff)))


class Oracle(BaseOracle):
    """실제45c946..45c987 caller 구간과 헤더/부모 getter만 실행한다."""
    def __init__(self):
        """동일 보호 PE와 기존 격리 ABI 도구를 재사용한다."""
        super().__init__()
        self.mu.mem_map(POOL,0x190000)
        self.allowed = []  # 기존 커서 몸체의 실행 허용을 가져오지 않는다.
        ranges(self,(0x40a890,0x4df230,0x41bfb0))
        self.allowed.append((0x45c946,0x45c987))
        self.allowed.append((0x45cc29,0x45cc62))  # 귀환 명령3의 실제 caller 구간이다.
        self.instructions = 0  # 이번 실제 구간의 명령 수다.

    def instruction(self,mu,address,size,data):
        """원격 방송만 외부 관측 경계로 두고 실제 작성 명령은 그대로 실행한다."""
        self.instructions += 1
        if address==0x4e2e30:
            pointer = self.get(mu.reg_read(UC_X86_REG_ESP)+4)  # cdecl 방송의 실제 패킷 인수다.
            self.packet = bytes(mu.mem_read(pointer,20)).hex()
            self.ret()
            return
        if address in (0x45c987,0x45cc62):
            self.ret()  # 원래 함수 전체 반환이 아니라 검토한 caller 구간의 완료 경계다.
            return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'훨리기그 명령 미검토 주소 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """실행 중 caller 스택/패킷 외 쓰기를 거부한다."""
        if not STACK-0x1000<=address or address+size>STACK+0x1000:
            raise RuntimeError(f'훨리기그 명령 스택 밖 쓰기 {address:x}')

    def sample(self,case,precision):
        """입력 스택을 채운 뒤 실제 구간·헤더·부모 getter·방송 인수를 읽는다."""
        sid, command, seed = case  # 미정 패킷 바이트도 호출자 입력으로 구별한다.
        self.mu.mem_write(STACK-0x1000,bytes([0xcc])*0x2000)
        self.put(STACK,STOP)
        offset = 0x74 if command==2 else 0x38  # 각 실제 caller의 원래 스택 패킷 위치다.
        entry = 0x45c946 if command==2 else 0x45cc29  # 두 실제 즉시 명령을 그대로 실행한다.
        source = 0x23456789 if command==2 else POOL+sid*50  # 명령3은 이미 얻은 부모 포인터를ESI로 받는다.
        self.mu.mem_write(STACK+offset,bytes([seed])*20)
        self.put(PROCESS+8,sid)
        self.put(0x5c8464,POOL)
        self.put(POOL+sid*50+6,1,2)
        self.put(0x52edf2,0x86,1)
        self.put(0x52edf3,1,1)
        self.put(0x52edf4,20,2)
        self.packet = None  # 방송 호출이 없으면 성공으로 세지 않는다.
        registers = {UC_X86_REG_EAX:0,UC_X86_REG_EDX:0,UC_X86_REG_ECX:STACK+offset,UC_X86_REG_ESP:STACK,
            UC_X86_REG_EDI:PROCESS,UC_X86_REG_ESI:source,UC_X86_REG_EBP:0x456789ab,
            UC_X86_REG_EFLAGS:2,UC_X86_REG_FPCW:precision,UC_X86_REG_FPSW:0,UC_X86_REG_FPTAG:0xffff}
        # 입력 레지스터를 매 구간 실행 전에 독립 설정한다.
        for register,value in registers.items():
            self.mu.reg_write(register,value)
        self.mu.emu_start(entry,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_EDI)==PROCESS and self.mu.reg_read(UC_X86_REG_ESI)==source and self.mu.reg_read(UC_X86_REG_EBP)==0x456789ab
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        assert bytes(self.mu.mem_read(STACK+offset+20,4))==bytes([0xcc])*4
        assert self.packet is not None
        return self.packet


def main():
    """새 독립 관측을 동결하거나 보존된 입력/원문 해시만 검사한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--replay',action='store_true')
    args = parser.parse_args()
    if args.verify:
        report = json.loads(REPORT.read_text(encoding='utf-8'))
        # 원본 기대값을 다시 쓰지 않는다.
        for name,expected in report['sha256'].items():
            assert digest(ROOT/name)==expected,name
        assert report['inputs']==len(cases())
        print('훨리기그 방송 패킷 구간 원문 감사 통과')
        return
    if not args.replay:
        assert not FIXTURE.exists() and not REPORT.exists()
    oracles = [Oracle(),Oracle()]  # 두 정밀도는 같은 초기 입력으로 별개 실행한다.
    rows = []  # C++ 출력을 사용하지 않는 실제 패킷 관측이다.
    # 입력별 원본 구간 완료 결과를 두 정밀도에서 비교한다.
    for case in cases():
        outputs = [oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
        assert outputs[0]==outputs[1]
        rows.append((','.join(map(str,case)),*outputs))
    content = '# sid,command,seed packet53 packet64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'  # 최종 실제 caller의 관측 원문이다.
    if args.replay:
        assert content.encode('utf-8')==FIXTURE.read_bytes()
        print('원본 구간 재생/기존 기대값 보존',len(rows),len(rows)*2)
        return
    FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    names = ['originals/Netstorm.exe','tools/decomp_flyer_command.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py',
        'tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/flyer-command-x86.tsv']  # 같은 실제 PE/실행 허용/격리 ABI의 현재 근거다.
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',
        inputs=len(rows),segmentCompletions=len(rows)*2,instructions=sum(oracle.instructions for oracle in oracles),
        scope='명령2의45c946..45c987와 명령3의45cc29..45cc62 실제 caller 구간/40a890/4df230/41bfb0. 코드 바이트를 바꾸지 않는다. 방송4e2e30은 인수 관측 경계다. 전체 함수 정상 반환이나 실제 게임 실행으로 합산하지 않는다. 초기 코드 즉시값 변경 모형은 명령3 입력에서도2를 기록하여 실제 두 caller로 대체했다. 초기 도구/관측은 extracted/goal-early5-combat-20261019-command-inputpatched-v1.*에 보존한다. 최종 원문을 C++ 출력에서 만들지 않는다.',
        sha256={name:digest(ROOT/name) for name in names}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(len(rows),len(rows)*2)


if __name__=='__main__':
    main()
