#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 GunHit→Apply→PriestHit→HPsetter/burning 호출 사슬의 새 입력을 두 PE에서 실행한다."""
import csv
import itertools
import json
import struct
from decomp_priestdamage_oracle import DamageOracle, KEYS, CREATED
from decomp_gunhit_oracle import SPEC as GUN, CD_DPS, VALUE
from decomp_owner_oracle import ROOT, TYPES, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

TABLE=CREATED+0x400  # 발사체의 실제 bc/c0 진입을 연결하는 명시 격리 가상 표 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/campaign-hit-x86.tsv'  # 기존 개별 함수 fixture는 불변이다.
REPORT=ROOT/'cpppj/campaign-hit.json'  # 새 compound 고유96입력만 기록한다.


class CampaignHitOracle(DamageOracle):
    """표적98을 외부 피해 함수로 대체하지 않고 실제 사제 몸체에 연결한다."""
    def __init__(self, edition):
        """기존 정확 범위에 GunHit 함수 범위와 DPS 입력 경계를 추가한다."""
        super().__init__(edition)
        self.gun=GUN[edition]
        self.exports.extend(ROOT/f'extracted/gunhit/{edition}/{part}' for part in ('creation.c','functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # 실제 내보낸 불연속 몸체 범위만 추가로 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        if self.gun['rate']:
            assert bytes(self.mu.mem_read(self.gun['rate'],4)).hex()=='d94148c3'
            self.allowed.append((self.gun['rate'],self.gun['rate']+4));self.entries.add(self.gun['rate'])
        self.mu.mem_write(CD_DPS,b'\xd9\x05'+struct.pack('<I',VALUE)+b'\xc2\x04\x00')
        self.allowed.append((CD_DPS,CD_DPS+9));self.queryCalls=0  # 명시 group/genus/type/통지 경계 요청 수다.

    def on_instruction(self, mu, address, size, data):
        """현재 타입 query/공간통지는 명시 입력이며 실제 피해/HP 함수는 대체하지 않는다."""
        if hasattr(self,'gun') and address in (self.gun['group'],self.gun['genus'],self.gun['type'],self.gun['notify']):
            sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX);ret=self.read(sp)
            value=TYPES+self.read(this+10,1)*self.type_stride if address==self.gun['type'] else 0
            self.queryCalls+=1
            mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def sample(self, word, now, dps, regen):
        """준비 helper의 명시 합성 입력 뒤 실제 전체 피해 사슬의 HP/시간WORD를 관찰한다."""
        values=dict(zip(KEYS,(0,100,0,0,0,2,1,1,0,1,regen,0,0,0,0,0,158,100)))
        # 기존 입력 준비는 actual Adjust(0) 한 번으로 마친다. 이 준비 반환은 compound 정상반환 수로 세지 않는다.
        self.run_body(values)
        self.events=[];patch=self.edition=='originals'
        raw=bytearray(self.stride);struct.pack_into('<I',raw,0,TABLE);raw[10]=71;raw[34 if patch else 32]=2;struct.pack_into('<H',raw,12,word)
        self.mu.mem_write(self.slot(60),bytes(raw))
        source=TYPES+71*self.type_stride;self.mu.mem_write(source,bytes(self.type_stride));self.mu.mem_write(source+0x48,struct.pack('<f',dps))
        self.mu.mem_write(TABLE+0xbc,struct.pack('<I',self.gun['dps'] or CD_DPS));self.mu.mem_write(TABLE+0xc0,struct.pack('<I',self.gun['entries'][3]))
        self.mu.mem_write(VALUE,struct.pack('<f',dps));self.mu.mem_write(self.gun['now'],struct.pack('<d',now))
        self.write_ranges.append((self.slot(60)+12,self.slot(60)+14))
        self.mu.mem_write(STACK,struct.pack('<II',STOP,50));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(60))
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(self.gun['entries'][2],STOP,count=20000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8
        assert self.mu.reg_read(UC_X86_REG_FPCW)==0x027f and not(self.mu.reg_read(UC_X86_REG_FPSW)&0x3800)
        return (self.hp(),self.read(self.slot(60)+12,2),sum(e.startswith('N:') for e in self.events))


def main():
    """이전 개별 입력을 다시 합산하지 않고 새 사슬96입력/정상반환 및 정확 export SHA를 기록한다."""
    cases=list(itertools.product((1,17,81),(0.125,172.0/60.0,31.25,4096.0),(15,30),(0,1)))
    rows,editions=[],{}  # 새 compound 결과와 실제 함수 실행 근거다.
    # 보호 두 PE를 각 격리 메모리에서 실행한다. 창/OS/파일 쓰기 호출은 허용하지 않는다.
    for edition in ('originals','originalCD'):
        oracle=CampaignHitOracle(edition)
        for values in cases:rows.append((edition,*values,*oracle.sample(*values)))
        editions[edition]=dict(samples=len(cases),returns=len(cases),assertions=oracle.assertions,peSha=oracle.sha256,
            nativeCalls=dict(oracle.native_calls),queryRequests=oracle.queryCalls,
            exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.exports})
        print(edition,len(cases),'사슬 정상 반환',flush=True)
    FIXTURE.write_text('# 실제 GunHit→PriestHit→HPsetter. 초기HP100/owner2→1/level차단/damageEffect0/감시empty, x87=027f.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),editions=editions,
        fixtureSha=digest(FIXTURE),toolSha=digest(ROOT/'tools/decomp_campaign_hit.py'),
        limitations=['발사 group/genus/type와 통지는 명시 query 입력이며 CD DPS 반환은 입력 thunk다.',
            '실제 GunHit/Time/SetTime/Apply/PriestHit/HPsetter/burning body를 실행한다. 표적98은 실제 PE 사제 진입이다.',
            '회복 조회/취소/프레임/소유자 통지/가상 공간은 기존 DamageOracle의 명시 관찰 경계다. nonempty 감시는 C++ 통합 검사 범위다.',
            '각 입력 준비의 Adjust(0) 한 번은 준비용 실행이며 새 사슬 정상반환 수에 재합산하지 않는다.',
            '초기HP100/미션레벨차단/damageEffect0/empty감시의 합성 입력이며 전체 게임 동일성은 별도다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
