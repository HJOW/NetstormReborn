#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""게임/OS 없이 원본 State39~41과 다섯 상태 블록을 독립 x86에서 관측한다."""
import csv
import hashlib
import json
import socket
import struct
from pathlib import Path
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

BLOCKS = (0x5c8630, 0x5c8758, 0x5c86c8, 0x5c87e8, 0x5c8878)  # old/phase/immediate/current/next의 실제 순서다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/native-intro-state-x86.tsv'  # 기존 fixture와 별개 최초 관측이다.
REPORT = ROOT / 'cpppj/native-intro-state-native-20261029.json'  # 원문/도구/관측 해시와 실행 범위를 기록한다.


class Oracle(Base):
    """큰 장치만 명시 입력으로 대체하고 분기/요청/144BYTE 복사는 실제 PE로 실행한다."""
    def __init__(self):
        """같은 PC의 정밀 함수표에서 선택된 실제 범위만 허용한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for address in (0x4b6dd0, 0x4b2bc0, 0x483db0):  # 부모/요청/현재 MovieGump 조회만 CPU에서 실행한다.
            for part in facts[address]['ranges'].split(';'):  # 불연속 범위의 빈틈을 실행 허용에 포함하지 않는다.
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))

    def event(self, value):
        """선택 장치의 순서와 명시 중단을 기록한다."""
        self.events.append(value)
        if self.stop == value:
            self.status = 'stop';self.mu.emu_stop();return False
        return True

    def instruction(self, mu, address, size, data):
        """검토된 원문만 실행하며 외부 함수 인자를 매번 확인한다."""
        args = self.words(mu.reg_read(UC_X86_REG_ESP) + 4, 4)
        if address == 0x45ad00:
            state = self.words(0x5c87e8)[0];self.events.append('state:' + str(state))
            if state == 3: self.status = 'menu';mu.emu_stop();return
            self.ret();return
        if address == 0x4b2df0:
            if args[0] != 0: raise RuntimeError('인트로 cleanup 인자')
            if self.event('cleanup'): self.ret()
            return
        if address in (0x42f270, 0x42e440, 0x48f3b0, 0x4993c0, 0x466280):
            names = {0x42f270: 'input', 0x42e440: 'hide', 0x48f3b0: 'core', 0x4993c0: 'renderer', 0x466280: 'hud'}
            if address == 0x42f270 and args[0] != 0: raise RuntimeError('Chat 인자')
            if address == 0x48f3b0 and args[:2] != (1, 1): raise RuntimeError('Core 인자')
            if address == 0x4993c0 and args[0] != 1: raise RuntimeError('Renderer 인자')
            if address == 0x466280 and args != (0x0200003d, 1, 0, 1): raise RuntimeError('HUD 인자')
            if self.event(names[address]): self.ret(purge=4 if address == 0x4993c0 else 0)
            return
        if address == 0x441300:
            if self.string(args[0]) != b'introMovie': raise RuntimeError('인트로 설정 이름')
            if self.event('path'): mu.mem_write(args[1], (b'MOVIE/missing.smk' if self.named else b'') + b'\0');self.ret()
            return
        if address == 0x4414d0:
            if self.string(args[0]) != b'intro' or self.string(args[1]) != b'1': raise RuntimeError('인트로 설정 쓰기')
            if self.event('mark'): self.ret()
            return
        if address == 0x484010:
            if self.string(args[0]) != b'MOVIE/missing.smk': raise RuntimeError('영상 이름')
            if self.event('open'): self.ret(self.openResult)
            return
        if address == 0x460bb0:
            if self.string(args[0]) != b'': raise RuntimeError('메뉴 복귀 pending 이름')
            if self.event('pending'): self.put(TEXT, 0, 1);self.ret()
            return
        if address == 0x4437f0: self.ret();return  # 공통 ret 전의 Guide 후처리는 이 관측의 명시 장치다.
        if not any(low <= address < high for low, high in self.allowed): raise RuntimeError(f'인트로 실행 범위 밖 {address:08x}')

    def sample(self, selected, prepare, named, active, result, stop):
        """각 관측마다 구별 가능한 상태 블록/이름 꼬리와 외부 장치 입력을 설치한다."""
        self.events=[];self.status='normal';self.named=named;self.openResult=result;self.stop=stop
        for index, base in enumerate(BLOCKS):  # 다섯 블록의 모든 DWORD를 서로 다르게 채운다.
            self.mu.mem_write(base, struct.pack('<36I', *[index * 1000 + word for word in range(36)]))
        for address, value in ((0x5c87e8, selected), (0x5c8874, prepare), (0x594fb8, 0), (0x540bc0, 0), (0x540bc4, 0), (0x595074, active), (0x5427ec, 0x0200003d), (0x55b494, TEXT)):
            self.put(address, value)  # 각 물리의 명시 현재값만 초기화한다.
        self.mu.mem_write(TEXT, b'Next\0' + b'\xa5' * 11)
        for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
            self.mu.reg_write(register, 0)  # 이전 관측의 레지스터를 입력으로 사용하지 않는다.
        self.mu.reg_write(UC_X86_REG_ESP, STACK);self.put(STACK, STOP)
        self.mu.emu_start(0x4b6dd0, STOP, count=100000)
        if self.status == 'normal' and (self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK + 4): raise RuntimeError('인트로 ret/스택 불일치')
        raw=':'.join(bytes(self.mu.mem_read(base, 144)).hex() for base in BLOCKS)
        modes=','.join(str(self.words(address)[0]) for address in (0x594fb8, 0x540bc0, 0x540bc4))
        return '\t'.join((str(selected), str(prepare), str(named), str(active), str(result), stop, self.status, raw, modes, bytes(self.mu.mem_read(TEXT, 16)).hex(), ';'.join(self.events)))


def main():
    """최초 자료를 보존하며 --verify는 독립 CPU 관측을 다시 실행해 바이트 대조한다."""
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify', action='store_true');args=parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('인트로 최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=[]
    cases=[(39,0,0,0,0,'-'),(39,1,0,0,0,'-'),(39,1,1,0,0,'-'),(39,1,1,0,1,'-'),(39,0xffffffff,1,1,1,'-'),(40,0,0,0,0,'-'),(40,0,0,1,0,'-'),(41,0,0,0,0,'-')]
    cases += [(39,1,1,0,0,stop) for stop in ('cleanup','input','hide','core','renderer','hud','path','mark','open')]
    for case in cases: rows.append(oracle.sample(*case))  # 성공뿐 아니라 앞선 쓰기를 보존한 선택 경계 중단도 관측한다.
    content='# state / prepare / named / active / openResult / stop / status / five blocks / fort,server,boss / pending / trace\n'+'\n'.join(rows)+'\n'
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_transport.py','tools/decomp_native_intro_state.py')
    hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        if hashes != report['sha256'] or content != FIXTURE.read_text(encoding='utf-8'): raise RuntimeError('인트로 독립 재관측 차이')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n')
        report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','observations':len(rows),'sha256':hashes,'fixtureSha256':hashlib.sha256(content.encode()).hexdigest(),'scope':'State39~41/실제 요청/다섯144BYTE 공통ret. 큰 cleanup/Chat/Core/Renderer/HUD/Config/영상/pending/Guide는 명시 장치. State3 진입에서 중단. 실제 게임/OS/네트워크 실행 없음.'}
        REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'observations':len(rows),'verified':args.verify,'differences':0}))


if __name__ == '__main__':
    main()
