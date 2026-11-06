#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 소유자의 현재 진단/알림/공통 부모 뒤 부분 효과만 두 PE에서 관측한다. 게임/MP는 실행하지 않는다."""
import argparse
import hashlib
import json
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/priest-owner-current-local-x86.tsv'  # 최초 유효 PE 관측은 덮어쓰지 않는다.
SPEC={  # 사제/공통/알림/진단/풀/전투/편집/로딩/편집로컬/전투로컬/슬롯/owner/VT다.
 'originals':(0x491790,0x4adf00,0x4214a0,0x4e0620,0x5c8464,0x594fbc,0x594fb8,0x5c89b8,0x540cac,0x540c70,50,34,0x50f210),
 'originalCD':(0x40bda0,0x4aefa0,0x448c10,0x490040,0x5395dc,0x540a20,0x540a1c,0x5178d4,0x50f6d4,0x50f6c8,36,32,0x5003e0),
}

class PriestOwnerOracle(Oracle):
    """사제 본문과 ret4는 실제 실행하고 공통 부모/알림/진단은 명시 변경 장치로 구별한다."""
    def __init__(self, edition):
        """같은 PC의 정밀 함수 범위와 보호 PE를 읽는다."""
        self.s=SPEC[edition]  # 판본별 실제 필드와 진입 주소다.
        super().__init__(edition,[dict(name='priest',**{edition:f'{self.s[0]:08x}'})],helpers=[])
        self.events=[];self.profile=0;self.notices=0;self.cause='returned'  # 현재 입력의 호출 순서와 중단 표식이다.

    def get(self, address, width=4):
        """명시 BYTE/WORD/DWORD의 현재 원문 값을 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self, address, value, width=4):
        """선택 폭만 써 이웃 payload를 보존한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def resume(self, purge=0):
        """명시 자식의 실제 호출 규약으로 복귀하며 사제의 ret4는 직접 실행한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EIP,self.get(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def on_instruction(self, mu, address, size, data):
        """선택된 진단/알림/부모의 당시 WORD와 owner 및 부분 중단을 기록한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==self.s[3]:
            self.events.append('D'+self.read_string(self.get(sp+4)).decode('ascii')+'|'+self.read_string(self.get(sp+8)).decode('ascii')+'|'+str(self.get(sp+12)))
            if self.profile==1:
                self.put(self.s[5],1);self.put(self.s[6],0);self.put(self.s[7],0);self.put(self.s[9],0);self.put(SCRATCH+12,0xcc81,2)
            self.resume();return
        if address==self.s[2]:
            self.events.append(f'N{self.get(sp+4)},{self.get(SCRATCH+12,2)},{self.get(SCRATCH+self.s[11],1)}');self.notices+=1
            if self.profile==4:
                self.cause='notify';mu.emu_stop();return
            if self.profile==2 and self.notices==1:
                self.put(self.s[5],0);self.put(self.s[6],0);self.put(self.s[7],1);self.put(SCRATCH+12,0xbb07,2)
            self.resume();return
        if address==self.s[1]:
            owner=self.get(sp+4);self.events.append(f'B{owner},{self.get(SCRATCH+12,2)},{self.get(SCRATCH+self.s[11],1)}')
            if self.profile==5:
                self.cause='base';mu.emu_stop();return
            self.put(SCRATCH+self.s[11],owner,1)
            if self.profile==3:
                self.put(SCRATCH+11,self.get(SCRATCH+11,1)|1,1);self.put(SCRATCH+10,255,1);self.put(SCRATCH+12,0xcc81,2)
                self.put(self.s[5],0);self.put(self.s[6],1);self.put(self.s[7],0);self.put(self.s[8],2)
            self.resume(4);return
        super().on_instruction(mu,address,size,data)

    def observe(self, battle, loading, owner, profile):
        """독립 입력에서 사제 본문의 현재 상태와 전체 슬롯 결과를 관측한다."""
        self.events=[];self.profile=profile;self.notices=0;self.cause='returned'
        self.put(self.s[4],SCRATCH-5*self.s[10]);self.put(self.s[5],battle);self.put(self.s[6],1);self.put(self.s[7],loading);self.put(self.s[8],2);self.put(self.s[9],3)
        raw=bytearray((i*29+7)&255 for i in range(self.s[10]));raw[:4]=self.s[12].to_bytes(4,'little');raw[10]=158;raw[11]=4;raw[12:14]=(0xab83).to_bytes(2,'little');raw[self.s[11]]=3
        self.mu.mem_write(SCRATCH,bytes(raw))
        try:
            self.call('priest',[owner],SCRATCH,4)
        except RuntimeError:
            if self.cause=='returned': raise
        current=','.join(str(self.get(self.s[i])) for i in (6,5,7,8,9))
        return [self.edition,','.join(map(str,(battle,loading,owner,profile))),self.cause,';'.join(self.events) or '-',current,bytes(self.mu.mem_read(SCRATCH,self.s[10])).hex()]

def main():
    """최초 관측 또는 같은 원문 입력의 재관측 검증만 수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();rows=[]
    for edition in SPEC:  # 두 판본의 사제 부모만 관측하고 게임/OS/통신은 실행하지 않는다.
        oracle=PriestOwnerOracle(edition)
        for battle,loading in ((0,0),(0,1),(1,1),(1,0)):  # 원래 소유자 변경과 전투의 기억값 경로를 구분한다.
            for owner in (0,2,9):  # 진단0/정상/공통 호출 필터의 판본 차이다.
                for profile in range(6):  # 정상/진단변경/알림변경/부모변경/알림중단/부모중단이다.
                    rows.append(oracle.observe(battle,loading,owner,profile))
    rendered='# 사제Owner 현재진단/부분효과 로컬144행; 자식은 명시장치; 게임/MP 미실행\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==rendered;print(f'verified {len(rows)} local Priest Owner inputs');return
    assert not FIXTURE.exists(),'최초 관측 덮어쓰기 금지'
    FIXTURE.write_bytes(rendered.encode('utf-8'))
    paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_priest_owner_current.py',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    (ROOT/'cpppj/priest-owner-current-native-20261106.json').write_text(json.dumps({'unit':963,'rows':len(rows),'scope':'실제 사제 부모/ret4, 진단/알림/공통 부모는 명시 변경·중단 장치. 게임/MP 실행 없음.','sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'froze {len(rows)} local Priest Owner inputs')

if __name__=='__main__':
    main()
