#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 PATCH 초기화/임시 재시드/CRT srand/rand를 격리 실행하여 전체 세 상태를 관찰한다."""
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle,ROOT,PLAYERS,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

MODE,GAME,TLS=0x5c89a8,0x532710,PLAYERS+0x8000  # 실제 전역 두 개와 명시 CRT thread data 입력이다.
TIME,GETPTD=0x4e4f1f,0x4ea61d  # OS Unix 시각/TLS 포인터 공급만 외부 경계다.
FIXTURE=ROOT/'cpppj/tests/fixtures/campaign-seed-x86.tsv'  # 이전 SP fixture는 불변이다.
REPORT=ROOT/'cpppj/campaign-seed.json'  # 새 직접 고유 입력96개만 기록한다.


class CampaignSeedOracle(OwnerOracle):
    """게임 초기화/재시드와 CRT 계산은 모두 실제 PE 몸체로 실행한다."""
    def __init__(self):
        """현재 같은 PC의 정확4함수 범위와 원본 상태 쓰기만 허용한다."""
        super().__init__('originals')
        self.paths=[ROOT/f'extracted/campaign-seed3/originals/{part}' for part in ('creation.c','functions.tsv')]
        with self.paths[1].open(encoding='utf-8') as fp:
            # 내보낸 각 함수의 불연속 몸체 범위를 그대로 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.write_ranges=[(MODE,MODE+4),(GAME,GAME+4),(TLS+0x14,TLS+0x18)]
        self.epoch=0;self.timeCalls=0;self.tlsCalls=0  # 명시 외부 입력과 각 실제 요청 수다.

    def on_instruction(self, mu, address, size, data):
        """시각/TLS만 실제 ABI로 공급하고 srand/rand 계산을 대체하지 않는다."""
        if address in (TIME,GETPTD):
            sp=mu.reg_read(UC_X86_REG_ESP);ret=int.from_bytes(mu.mem_read(sp,4),'little')
            if address==TIME:
                assert int.from_bytes(mu.mem_read(sp+4,4),'little')==0
                self.timeCalls+=1;value=self.epoch&0xffffffff
            else:self.tlsCalls+=1;value=TLS
            mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def sample(self, temporary, mode, epoch, crt):
        """명시 초기 상태 뒤 원본 정상반환과 mode/game/CRT 전체DWORD 및 요청 수를 얻는다."""
        self.epoch=epoch;self.timeCalls=self.tlsCalls=0
        self.mu.mem_write(MODE,struct.pack('<I',mode));self.mu.mem_write(GAME,struct.pack('<I',0x12345678))
        self.mu.mem_write(TLS+0x14,struct.pack('<I',crt));self.mu.mem_write(STACK,struct.pack('<I',STOP))
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.emu_start(0x4559d0 if temporary else 0x455970,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        values=[int.from_bytes(self.mu.mem_read(p,4),'little') for p in (MODE,GAME,TLS+0x14)]
        return (*values,self.timeCalls,self.tlsCalls)


def main():
    """직접96입력/정상반환/assert와 기존 보호PE/현재 정확 export/도구 SHA를 고정한다."""
    oracle=CampaignSeedOracle();rows=[]
    # mode0/1/기타DWORD·시각경계·기존CRT·임시모드 여부만 입력한다. 결과 계산은 원본이 수행한다.
    for values in itertools.product((0,1),(0,1,7,0xffffffff),(0,1,-1,946684800,2147483647,-2147483648),(0,0xfeedface)):
        rows.append((*values,*oracle.sample(*values)))
    FIXTURE.write_text('# 실제 PATCH 게임seed/CRT. __time32(NULL)와CRT TLS 포인터만 외부 입력.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),returns=len(rows),assertions=oracle.assertions,
        peSha=oracle.sha256,nativeCalls=dict(oracle.native_calls),exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.paths},
        fixtureSha=digest(FIXTURE),toolSha=digest(ROOT/'tools/decomp_campaign_seed.py'),
        limitations=['PATCH10.78만 직접 관찰했다. CD10.72 재시드 진입은 이번 비교 범위 밖이다.',
            'time32 signed Unix 초와 TLS 포인터는 명시 외부 입력이다. 실제초기화/helper/srand/rand 계산은 원본 몸체다.',
            'GUI/운영체제시계/스레드초기화/세션전체 및 SP전체 caller는 별도 연결 범위다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('실제 PATCH 재시드:',len(rows),'정상/assert',oracle.assertions)


if __name__=='__main__':main()
