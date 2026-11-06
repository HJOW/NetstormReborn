#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""BaseProcess의 신규 로컬 Form 부착과 생성 경계의 부분 상태를 양판본 PE에서 관측한다."""
import argparse
import hashlib
import json
import socket
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

POOL, PROC, VT, DEVICE = 0x13000000, SCRATCH+0x1000, SCRATCH+0x2000, SCRATCH+0x3000  # 실제 프로세스/힙/게임과 분리한 입력이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/baseprocess-attach-current-local-x86.tsv'  # 신규 로컬 Form만 최초 관측하며 수신/MP 입력은 만들지 않는다.
REPORT = ROOT/'cpppj/baseprocess-attach-current-native-20261106.json'  # 관측 경계와 정체성이다.
SPEC = {  # 부모/Kernel등록/Create/타입쓰기/진단/풀/상한/Form번호/슬롯 폭이다.
    'originals': (0x41bd20,0x471950,0x4af530,0x4ae2e0,0x4e0620,0x5c8464,0x54116c,0x541078,50),
    'originalCD': (0x48f590,0x4ed500,0x4ab390,0x4af4f0,0x490040,0x5395dc,0x51ca58,0x51c968,36),
}

class BaseAttachOracle(Oracle):
    """원문 부모/타입쓰기만 실행하고 Kernel/Create/가상부착/통지는 명시 장치로 구별한다."""
    def __init__(self, edition):
        """현재 PC의 정밀 함수 범위와 보호 PE를 사용한다."""
        self.s = SPEC[edition]  # 원문 판본 주소다.
        super().__init__(edition,[dict(name='base',**{edition:f'{self.s[0]:08x}'})],helpers=[self.s[3]])
        self.mu.mem_map(POOL,0x10000)
        self.events = []  # 실제 선택한 호출 순서다.
        self.profile = 0  # Create 진입에서만 중단하는 명시 경계다.
        self.cause = 'returned'  # 정상 부모 반환과 자식 경계 중단을 구별한다.

    def read(self, address, width=4):
        """가상 메모리의 원문 폭만 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self, address, value, width=4):
        """명시 입력의 폭만 기록한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def resume(self, value=0, purge=0):
        """명시 자식만 반환시키며 실제 부모 ret16을 별도 검증한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_EIP,self.read(sp))
        self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def on_instruction(self, mu, address, size, data):
        """허용 부모/타입쓰기 밖에서는 검토한 경계만 대체한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.s[4]:
            self.events.append('D'+self.read_string(self.read(sp+4)).decode('ascii')+'|'+self.read_string(self.read(sp+8)).decode('ascii')+'|'+str(self.read(sp+12)))
            self.resume();return
        if address==self.s[1]:
            self.events.append('K');self.resume(7);return
        if address==self.s[2]:
            self.events.append(f'C{self.read(sp+4)},{self.read(sp+8)}')
            if self.profile:
                self.cause='create';mu.emu_stop();return
            self.resume(POOL+5*self.s[8]);return  # Create는 cdecl이며 원문 부모가 두 인자를 반납한다.
        if address==DEVICE:
            self.events.append(f'A{self.read(sp+4)},{self.read(sp+8)}');self.resume(0,8);return
        if address==DEVICE+0x40:
            self.events.append('N');self.resume(1);return
        super().on_instruction(mu,address,size,data)

    def observe(self, state, kind, profile):
        """신규Form0/로컬flags50만 실행하고 반환 또는 Create 경계의 필드/슬롯을 저장한다."""
        self.events=[];self.profile=profile;self.cause='returned'
        self.mu.mem_write(POOL,bytes(16*self.s[8]));self.put(self.s[5],POOL)
        self.put(self.s[6],63 if self.edition=='originals' else 62);self.put(self.s[7],2)
        self.put(POOL+7*self.s[8]+11,state,1)
        raw=bytearray((i*29+7)&255 for i in range(self.s[8]));raw[0:4]=VT.to_bytes(4,'little');raw[10]=2
        self.mu.mem_write(POOL+5*self.s[8],bytes(raw));self.put(VT+0x74,DEVICE);self.put(VT+0x14,DEVICE+0x40)
        self.mu.mem_write(PROC,bytes([0xab])*16);self.put(PROC,VT)
        try:
            self.call('base',[kind,7,0,0x50],PROC,16)
        except RuntimeError:
            if self.cause!='create': raise
        return [self.edition,f'{state},{kind},{profile}',self.cause,';'.join(self.events) or '-',f'{self.read(PROC+4)},{self.read(PROC+8)}',bytes(self.mu.mem_read(POOL+5*self.s[8],self.s[8])).hex()]

def main():
    """기존 fixture를 변경하지 않고 최초 생성 또는 재관측 비교를 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();rows=[]
    for edition in SPEC:  # 지정Form/수신/직렬화/패킷은 실행하지 않는다.
        oracle=BaseAttachOracle(edition)
        for state in (0,1,2,3):  # PATCH 조기 반환과 CD 두 진단을 구별한다.
            for kind in (46,9,0xffffffff):  # signed 타입 진단과 실제 BYTE 저장이다.
                for profile in (0,1):  # 정상 자식 반환 또는 Create 진입 중단이다.
                    rows.append(oracle.observe(state,kind,profile))
    rendered='# 신규 로컬Form0/flags50 BaseProcess; Kernel/Create/가상부착은 명시 경계; MP/수신 미실행\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==rendered;print(f'verified {len(rows)} local BaseProcess inputs');return
    assert not FIXTURE.exists() and not REPORT.exists(),'최초 관측 덮어쓰기 금지'
    FIXTURE.write_bytes(rendered.encode('utf-8'))
    paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originalCD/Netstorm.c','tools/decomp_oracle.py','tools/decomp_baseprocess_attach_current.py',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    REPORT.write_text(json.dumps({'unit':960,'host':socket.gethostname(),'lastPreciseDecompile':{'host':'HJOW-Athlon','date':'2026-11-03'},'rows':len(rows),'scope':'신규로컬Form0/flags50의0041bd20/CD0048f590와실제타입쓰기. Kernel/Create/가상부착/통지는명시장치. 생성중단은자식진입부분상태이며전체SEH/힙실패아님. MP/수신미실행.','sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'froze {len(rows)} local BaseProcess inputs')

if __name__=='__main__':
    main()
