#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 썬 디스크 Handle을 실행해 실제 프레임 지정 요청과 float 반환을 관찰한다."""
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, PLAYERS, TARGET, STOP, STACK, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

SPEC={  # 실제 가상 Handle/공통 프레임 지정 주소다.
    'originals':dict(handle=0x469540,setter=0x4acee0,getter=0x4ac550),
    'originalCD':dict(handle=0x437290,setter=0x4acbc0,getter=0x4abf50),
}
CALLER, RESULT=PLAYERS+0x6000,PLAYERS+0x6100  # 반환 float10을 binary32로 관찰하는 격리 도구 코드다.
FIXTURE=ROOT/'cpppj/tests/fixtures/disc-regular-x86.tsv'  # 새 입력만 기록한다.
REPORT=ROOT/'cpppj/disc-regular.json'  # 기존 프레임 지정 검증은 재합산하지 않는다.


class DiscRegularOracle(OwnerOracle):
    """실제 Handle/TYPE 조회를 실행하고 프레임 지정만 명시 관찰 경계로 둔다."""
    def __init__(self, edition):
        """정밀 디스크/포구 getter export와 반환 관찰 명령을 준비한다."""
        super().__init__(edition)
        self.paths=[]  # 현재 export 전체 원문이다.
        # 실제 추가 몸체의 정확 불연속 범위를 읽는다.
        for folder in ('projectile-disc','projectile-help'):
            self.paths += [ROOT/f'extracted/{folder}/{edition}/{name}' for name in ('creation.c','functions.tsv')]
            with self.paths[-1].open(encoding='utf-8') as fp:
                for row in csv.DictReader(fp,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'))
                        self.allowed.append((low,high+1))
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))
        self.allowed.append((CALLER,CALLER+11));self.write_ranges=[(RESULT,RESULT+4)]
        self.request=None  # 마지막 실제 프레임 지정의 인자다.

    def on_instruction(self, mu, address, size, data):
        """공통 지정기의 별도 검증과 분리하여 실제 요청 frame/flags만 관찰한다."""
        if address==SPEC[self.edition]['setter']:
            sp=mu.reg_read(UC_X86_REG_ESP)
            ret,frame,flags=struct.unpack('<III',mu.mem_read(sp,12))
            assert mu.reg_read(UC_X86_REG_ECX)==self.slot(TARGET)
            self.request=(frame,flags)
            mu.reg_write(UC_X86_REG_ESP,sp+12);mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def sample(self, frame, clusters, directions, event, count, payload):
        """명시 TYPE/frame/event/count/payload 입력만 만들고 실제 계산/반환을 실행한다."""
        raw=bytearray(self.stride);raw[10]=83
        if self.edition=='originals':struct.pack_into('<I',raw,36,frame)
        else:raw[34]=frame
        self.mu.mem_write(self.slot(TARGET),bytes(raw))
        self.mu.mem_write(TYPES+83*self.type_stride+0x110,struct.pack('<ii',directions,clusters))
        self.mu.mem_write(STACK,struct.pack('<IIII',CALLER,event,count,payload))
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(TARGET))
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f);self.mu.reg_write(UC_X86_REG_FPSW,0);self.request=None
        self.mu.emu_start(SPEC[self.edition]['handle'],STOP,count=2000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+16
        assert self.mu.reg_read(UC_X86_REG_FPCW)==0x027f and self.request is not None
        return (*self.request,int.from_bytes(self.mu.mem_read(RESULT,4),'little'))


def main():
    """새 고유192입력과 actual Handle 정상반환/요청을 별도로 보존한다."""
    cases=[(f,c,d,e,n,p) for f,c,d,(e,n),p in itertools.product((0,3,34,255),(4,12,35),(1,2),((0,0),(99,9)),(0x3ca3d70a,0xbf800000))]
    rows,editions=[],{}  # 양 판본의 새 독립 결과다.
    # 보호 원본은 각 격리 메모리에서 읽기 전용으로 실행한다.
    for edition in SPEC:
        oracle=DiscRegularOracle(edition)
        for values in cases:rows.append((edition,*values,*oracle.sample(*values)))
        editions[edition]=dict(samples=len(cases),returns=len(cases),assertions=oracle.assertions,peSha=oracle.sha256,
            exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.paths},nativeCalls=dict(oracle.native_calls))
        print(edition,len(cases),'정상 반환',flush=True)
    FIXTURE.write_text('# 실제 썬 디스크 Handle/TYPE 조회. 공통 frame setter는 명시 외부 요청 관찰. x87=027f.\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),editions=editions,
        fixtureSha=digest(FIXTURE),toolSha=digest(ROOT/'tools/decomp_disc_regular.py'),
        limitations=['합성 TYPE83/현재 frame과 유한 payload. 실제 Handle/TYPE getter를 실행한다.',
            '공통 setter는 요청 관찰이다. 그 기존 Native 검증 입력은 재합산하지 않는다.',
            '방향0/분모0/음수frame/정수overflow와 전체 캠페인 충돌은 별도 범위다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
