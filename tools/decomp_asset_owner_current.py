#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""생산 접두/중립 소유자의 실제 두 PE 몸체를 관측한다. 게임/OS/MP는 실행하지 않는다."""
import argparse
import hashlib
import json
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/asset-owner-current-local-x86.tsv'  # 최초 유효 관측은 덮어쓰지 않는다.
SPEC={  # 생산 접두, 고정 중립, 공통 부모, 생산 카운터, 슬롯 폭, owner, extra다.
    'originals':(0x454cb0,0x462580,0x4adf00,0x55a4a4,50,34,40),
    'originalCD':(0x47b580,0x416f20,0x4aefa0,0x520c10,36,32,35),
}

class AssetOwnerOracle(Oracle):
    """짧은 재정의 본문/ret4만 실행하고 공통 부모는 명시 변경/중단 장치로 구분한다."""
    def __init__(self, edition):
        """현재 호스트의 정밀 자료와 보호 PE를 읽기 전용으로 사용한다."""
        self.s=SPEC[edition]  # 판본별 원본 주소와 실제 필드 폭이다.
        super().__init__(edition,[dict(name='common',**{edition:f'{self.s[2]:08x}'})],helpers=[])
        self.allowed=[(self.s[0],self.s[0]+(17 if edition=='originals' else 26)),(self.s[1],self.s[1]+7)]  # 원문 wrapper의 명시 opcode 범위만 허용한다.
        self.events=[];self.stop=False;self.cause='returned'  # 현재 부모 진입 기록과 부분 중단 입력이다.

    def get(self, address, width=4):
        """명시 폭의 현재 메모리를 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self, address, value, width=4):
        """선택한 폭만 써 이웃 payload를 보존한다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def on_instruction(self, mu, address, size, data):
        """공통 부모 진입 시 이미 증가한 카운터를 기록하고 명시 BYTE 변경 또는 중단한다."""
        if address==self.s[2]:
            sp=mu.reg_read(UC_X86_REG_ESP);owner=self.get(sp+4)
            self.events.append(f'B{owner},{self.get(self.s[3])}')
            if self.stop:
                self.cause='base';mu.emu_stop();return
            self.put(SCRATCH+self.s[5],owner,1)
            mu.reg_write(UC_X86_REG_EIP,self.get(sp));mu.reg_write(UC_X86_REG_ESP,sp+8);return
        super().on_instruction(mu,address,size,data)

    def observe(self, kind, extra, owner, dirty, stop):
        """독립 입력의 호출 순서·전체 슬롯·카운터를 관측 행으로 반환한다."""
        self.events=[];self.stop=stop;self.cause='returned'
        raw=bytearray((i*29+7)&255 for i in range(self.s[4]));raw[self.s[5]]=3;raw[self.s[6]]=extra
        self.mu.mem_write(SCRATCH,bytes(raw));self.put(self.s[3],dirty);self.entries['derived']=self.s[kind]
        try:
            self.call('derived',[owner],SCRATCH,4)
        except RuntimeError:
            if self.cause=='returned': raise
        return [self.edition,','.join(map(str,(kind,extra,owner,dirty,int(stop)))),self.cause,';'.join(self.events) or '-',str(self.get(self.s[3])),bytes(self.mu.mem_read(SCRATCH,self.s[4])).hex()]

def main():
    """최초 관측 생성 또는 같은 입력의 재관측 일치만 수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args();rows=[]
    for edition in SPEC:  # 두 보호 PE의 같은 로컬 자산 몸체만 대조한다.
        oracle=AssetOwnerOracle(edition)
        for kind in (0,1):  # 생산 접두와 고정 중립을 구분한다.
            for extra in (0,1,8,9):  # 두 비트와 무비트의 선택을 확인한다.
                for owner in (2,0xffffffff):  # 원본 인자 전체와 BYTE 쓰기를 구분한다.
                    for dirty in (0,0xffffffff):  # unsigned DWORD 감김도 독립 관측한다.
                        for stop in (False,True):  # 공통 부모 중단 전의 부분 쓰기를 보존한다.
                            rows.append(oracle.observe(kind,extra,owner,dirty,stop))
    rendered='# 파생Owner 로컬128행; 공통부모는 명시장치; 게임/MP 미실행\n'+'\n'.join('\t'.join(row) for row in rows)+'\n'
    if args.verify:
        assert FIXTURE.read_text(encoding='utf-8')==rendered;print(f'verified {len(rows)} local derived Owner inputs');return
    assert not FIXTURE.exists(),'최초 관측 덮어쓰기 금지'
    FIXTURE.write_bytes(rendered.encode('utf-8'))
    paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_asset_owner_current.py',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    (ROOT/'cpppj/asset-owner-current-native-20261106.json').write_text(json.dumps({'unit':962,'rows':len(rows),'scope':'생산접두/중립owner 실제 wrapper/ret4, 공통 부모 명시장치. 게임/MP 없음.','sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'froze {len(rows)} local derived Owner inputs')

if __name__=='__main__':
    main()
