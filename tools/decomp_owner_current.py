#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 소유자의 현재 모드/TYPE와 진단 정상 복귀를 두 PE에서 관측한다. challenge/MP는 실행하지 않는다."""
import argparse
import hashlib
import json
import socket
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

POOL, TYPE = 0x13000000, SCRATCH+0x1000  # 실제 게임/힙과 분리한 가상 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/owner-current-local-x86.tsv'  # 최초 유효 관측만 쓰며 이후 변경하지 않는다.
REPORT=ROOT/'cpppj/owner-current-native-20261106.json'  # 원문/장치/최초 관측의 정체성이다.
SPEC={  # 부모/진단/genus/제거/추가/풀/fort/battle/challenge/슬롯/owner 위치다.
 'originals':(0x4adf00,0x4e0620,0x49a840,0x4900a0,0x490050,0x5c8464,0x594fb8,0x594fbc,0x594fc8,50,34),
 'originalCD':(0x4aefa0,0x490040,0x4abae0,0x407250,0x4071f0,0x5395dc,0x540a1c,0x540a20,0x540a24,36,32),
}

class OwnerCurrentOracle(Oracle):
    """부모 본문만 실행하고 genus/목록 함수는 변경/중단 가능한 명시 경계로 구별한다."""
    def __init__(self, edition):
        """보호 PE와 같은 PC의 정밀 함수 범위를 읽는다."""
        self.s=SPEC[edition]  # 현재 판본의 실제 주소다.
        super().__init__(edition,[dict(name='owner',**{edition:f'{self.s[0]:08x}'})],helpers=[])
        self.mu.mem_map(POOL,0x10000)
        self.events=[];self.profile=0;self.genus=0x4000;self.cause='returned'  # 외부 장치 입력과 실제 호출 기록이다.

    def get(self, address, width=4):
        """가상 메모리의 명시 폭만 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self, address, value, width=4):
        """지정 BYTE/DWORD만 써 인접 payload를 보존한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def resume(self, value=0):
        """명시 cdecl/무인자 thiscall 자식만 복귀시키고 부모 ret4는 직접 실행한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_EIP,self.get(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4)

    def on_instruction(self, mu, address, size, data):
        """선택된 실제 위치에서 진단/현재 genus/목록 자식 인자를 관측한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);slot=POOL+5*self.s[9]
        if address==self.s[1]:
            self.events.append('D'+self.read_string(self.get(sp+4)).decode('ascii')+'|'+self.read_string(self.get(sp+8)).decode('ascii')+'|'+str(self.get(sp+12)))
            if self.profile==1:
                self.put(self.s[6],1);self.put(slot+10,86,1);self.genus=0
            self.resume();return
        if address==self.s[2]:
            kind=self.get(sp+4) if self.edition=='originals' else self.get(slot+10,1)
            self.events.append(f'G{kind}');self.put(TYPE+0xec,self.genus)
            self.resume(TYPE if self.edition=='originals' else self.genus);return
        if address in (self.s[3],self.s[4]):
            remove=address==self.s[3];self.events.append(f'{"M" if remove else "A"}{self.get(sp+4)},{self.get(sp+8)}')
            if remove and self.profile==2:
                self.put(slot+10,86,1);self.genus=0
            if remove and self.profile==3:
                self.put(self.s[6],0)
            if (remove and self.profile==4) or (not remove and self.profile==5):
                self.cause='remove' if remove else 'append';mu.emu_stop();return
            self.resume();return
        super().on_instruction(mu,address,size,data)

    def observe(self, fort, player, profile):
        """정상/무효 소유자 진단 뒤의 현재값과 부분 중단을 새 입력에서 관측한다."""
        self.events=[];self.profile=profile;self.genus=0x4000;self.cause='returned'
        self.put(self.s[5],POOL);self.put(self.s[6],fort);self.put(self.s[7],0);self.put(self.s[8],0)
        raw=bytearray((i*29+7)&255 for i in range(self.s[9]));raw[10]=85;raw[11]=0;raw[self.s[10]]=3;raw[40 if self.edition=='originals' else 35]=0
        self.mu.mem_write(POOL+5*self.s[9],bytes(raw))
        try:
            self.call('owner',[player],POOL+5*self.s[9],4)
        except RuntimeError:
            if self.cause=='returned': raise
        return [self.edition,f'{fort},{player},{profile}',self.cause,';'.join(self.events) or '-',str(self.get(self.s[6])),bytes(self.mu.mem_read(POOL+5*self.s[9],self.s[9])).hex()]

def main():
    """최초 관측 생성과 불변 파일 재관측 비교만 선택한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();rows=[]
    for edition in SPEC:  # 실제 게임/사제/신전/MP 가상 몸체는 실행하지 않는다.
        oracle=OwnerCurrentOracle(edition)
        for fort in (0,1):  # 전투0/challenge0의 편집·비편집 입력이다.
            for player in (2,9,0xffffffff):  # 범위 진단이 정상 복귀해 원래 하위 BYTE를 쓰는 경우도 포함한다.
                for profile in range(6):  # 현재 TYPE/모드 변경과 제거/추가 중단을 구별한다.
                    rows.append(oracle.observe(fort,player,profile))
    rendered='# 공통Owner 현재값/진단72행; battle0/challenge0; genus/목록은 명시 장치; MP 미실행\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==rendered;print(f'verified {len(rows)} local Owner inputs');return
    assert not FIXTURE.exists() and not REPORT.exists(),'최초 관측 덮어쓰기 금지'
    FIXTURE.write_bytes(rendered.encode('utf-8'))
    paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','extracted/refined/originals/Netstorm.c','extracted/refined/originalCD/Netstorm.c','tools/decomp_oracle.py','tools/decomp_owner_current.py',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    REPORT.write_text(json.dumps({'unit':961,'host':socket.gethostname(),'lastPreciseDecompile':{'host':'HJOW-Athlon','date':'2026-11-03'},'rows':len(rows),'scope':'공통Owner004adf00/CD004aefa0 본문/ret4와현재TYPE·모드/진단복귀·부분중단. genus/목록은명시장치. battle0/challenge0, 게임/MP미실행.','sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'froze {len(rows)} local Owner inputs')

if __name__=='__main__':
    main()
