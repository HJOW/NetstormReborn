#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 두 PE의 Player9 송신 분배를 실제 명령으로 실행한다."""
import csv
import itertools
import json
import struct
from decomp_owner_oracle import OwnerOracle,ROOT,PLAYERS,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
SPEC={  # 같은 PC에서 확인한 실제 분배/전역 배치다.
    'originals':dict(all=0x4e2e30,exclude=0x4e2e80,connections=0x5e4a18,local=0x540c70,server=0x540bc0),
    'originalCD':dict(all=0x4c0420,exclude=0x4c0480,connections=0x53d610,local=0x50f6c8,server=0x540a28),
}
OBJECTS=PLAYERS+0x5000  # 실제 연결 객체 대신 쓰는 명시 외부 가상+4 입력의 주소다.
TABLE=PLAYERS+0x6000  # 각 연결의 가상+4 표다.
PACKET=PLAYERS+0x7000  # 명시 현재 packet 전체 바이트다.
CALL=STOP+0x800  # 가상 송신 경계만 관찰하는 실행 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/battle-bcast-x86.tsv'  # 이전 관찰 자료와 분리한다.
REPORT=ROOT/'cpppj/battle-bcast.json'  # 이번 새 고유 입력만 기록한다.


class BroadcastOracle(OwnerOracle):
    """실제 분배 몸체를 실행하며 가상 송신 장치 요청만 기록한다."""
    def __init__(self,edition):
        """정확 export 범위와 격리 Player/연결 표를 구성한다."""
        super().__init__(edition);self.broadcast=SPEC[edition]
        self.paths=[ROOT/f'extracted/battle-bcast/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.paths[-1].open(encoding='utf-8') as fp:
            # Ghidra의 실제 각 함수 몸체 범위만 추가한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.externalCalls=0  # 가상 송신 요청 수이며 실제 네트워크 I/O가 아니다.
        self.mu.mem_write(PACKET,bytes(range(32)))
        # 실제 존재 여부와 가상 호출 receiver를 별도 입력으로 준비한다.
        for owner in range(9):
            self.put(OBJECTS+owner*16,TABLE+owner*8);self.put(TABLE+owner*8+4,CALL+owner*8)

    def put(self,address,value):
        """현재 합성 입력의 DWORD만 기록한다."""
        self.mu.mem_write(address,struct.pack('<I',value))

    def on_instruction(self,mu,address,size,data):
        """송신 순서/인자와 callback 뒤 현재 전역의 재조회만 관찰한다."""
        if CALL<=address<CALL+9*8:
            owner=(mu.reg_read(UC_X86_REG_ECX)-OBJECTS)//16;sp=mu.reg_read(UC_X86_REG_ESP)
            ret,packet=struct.unpack('<II',mu.mem_read(sp,8));assert packet==PACKET
            assert bytes(mu.mem_read(packet,32))==bytes(range(32));self.sent.append(owner);self.externalCalls+=1
            if len(self.sent)==1:
                if self.action==1:self.put(self.broadcast['local'],8)
                if self.action==2 and owner<8:self.put(self.broadcast['connections']+(owner+1)*4,0)
                if self.action==3 and owner<8:mu.mem_write(PLAYERS+(owner+1)*self.player_stride+0x50,b'\xfe')
            mu.reg_write(UC_X86_REG_ESP,sp+8);mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)

    def sample(self,kind,active,connections,local,excluded,action):
        """Player0/상위 플래그/NULL/로컬/지정 제외와 live callback 입력을 실행한다."""
        self.sent=[];self.action=action
        # bit0만 접속 표시이며 다른7비트는 항상1로 입력한다.
        for owner in range(9):
            self.mu.mem_write(PLAYERS+owner*self.player_stride+0x50,bytes((0xfe|((active>>owner)&1),)))
            self.put(self.broadcast['connections']+owner*4,OBJECTS+owner*16 if connections&(1<<owner) else 0)
        self.put(self.broadcast['local'],local);self.put(self.broadcast['server'],1)
        args=(PACKET,) if kind=='A' else (excluded,PACKET)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,0);self.mu.reg_write(UC_X86_REG_EAX,0)
        self.mu.emu_start(self.broadcast['all' if kind=='A' else 'exclude'],STOP,count=4000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        return ','.join(map(str,self.sent)) or '-'


def main():
    """새 고유1356입력만 기록하며 다른 단위 관찰을 재합산하지 않는다."""
    cases=[]  # 명시 접속/NULL/로컬/제외 입력이며 기대값은 계산하지 않는다.
    # 접속 bit0/Player0/NULL/동일 로컬/지정 제외를 조합한다.
    for active,connections,local in itertools.product((0,1,2,0xaa,0x155,0x1fe,0x1ff),(0,1,2,0xa5,0x1fe,0x1ff),(0,1,4,8)):
        cases.append(('A',active,connections,local,0,0))
        for excluded in (0,2,8):cases.append(('E',active,connections,local,excluded,0))
    # 첫 가상 호출이 local/다음 연결/다음 접속 BYTE를 바꾸는 현재 상태 입력이다.
    for kind,action in itertools.product(('A','E'),(1,2,3)):cases.append((kind,0x1fe,0x1fe,1,8,action))
    rows=[];editions={}  # 새 반환과 정확 몸체/외부 경계 근거다.
    # 두 보호 파일은 각각 격리 x86 메모리에서만 실행한다.
    for edition in SPEC:
        oracle=BroadcastOracle(edition)
        for values in cases:rows.append((edition,*values,oracle.sample(*values)))
        editions[edition]=dict(inputs=len(cases),returns=len(cases),assertions=oracle.assertions,peSha=oracle.sha256,
            nativeCalls=dict(oracle.native_calls),externalCalls=oracle.externalCalls,exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.paths})
        print(edition,len(cases),'실제 분배 정상 반환',flush=True)
    assert len(rows)==len(set(row[:-1] for row in rows))==1356
    FIXTURE.write_text('# 실제 PE Player9 송신 분배; 가상+4 송신 장치와 callback 변경은 명시 외부 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',uniqueInputs=len(rows),editions=editions,
        fixtureSha=digest(FIXTURE),toolSha=digest(__file__),
        limitations=['실제 송신 분배를 실행했다. 가상+4 송신 장치 요청/현재상태 변경은 명시 관찰 경계이며 네트워크 I/O가 아니다.',
            'Player 저장소 포인터는 고정했다. 함수 안에서 Player 표 포인터 자체가 교체되는 상황은 검증하지 않았다.',
            '제외 송신의 server assertion 입력은 정상 server1이다. packet 직렬화/수신/게임 전체 동일성은 별도다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
