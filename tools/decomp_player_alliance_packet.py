#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본00491220 전체와 실제00490ea0을 격리 실행해 동맹 수신의 원문/효과를 동결한다."""
import csv
import itertools
import json
import struct
import sys
from pathlib import Path
from decomp_player_participation import Oracle as ParentOracle, BASE, PLAYERS, ALLIES, STACK, STOP, ROOT, digest
from decomp_priestmenu import Oracle as MemoryOracle
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

PACKET=PLAYERS+0x8000 #실제9바이트 packet과 앞뒤 canary의 독립 입력 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/player-alliance-packet-x86.tsv' #C++ 구현 전에 동결하는 원본 출력이다.
REPORT=ROOT/'cpppj/player-alliance-packet.json' #관측 범위와 원본/도구/fixture SHA를 기록한다.


class Oracle(ParentOracle):
    """기존 Player 메모리/함수 범위를 재사용하고 수신 전체와 broadcast 경계만 더한다."""
    def __init__(self):
        """PE에서 확인한00491220..00491260의 전체 실제 명령을 허용한다."""
        MemoryOracle.__init__(self)
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.allowed=[(0x491220,0x491261)]
        #현재 PC의 전체 정밀 산출물에서 실제 부모/owner getter 범위만 읽는다.
        for address in (0x490ea0,0x48f550):
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """broadcast는 현재 packet 원문만 관측하고 나머지는 실제 부모/기존 Chat 경계를 실행한다."""
        if address==0x4e2e30:
            pointer=self.words(mu.reg_read(UC_X86_REG_ESP)+4,1)[0]
            assert pointer==PACKET
            self.events.append('B,'+bytes(mu.mem_read(pointer,9)).hex())
            self.ret()
        else:
            super().instruction(mu,address,size,data)

    def run(self,values,precision):
        """입력만 준비하고 실제 수신의 전체 Player/방향표/packet/효과와 호출 규약을 읽는다."""
        owner,mask,known,local,meta=values
        inputs=BASE.copy()
        inputs[0]=owner;inputs[2]=0x5a5a5a5a;inputs[6]=known;inputs[7]=local;inputs[8]=meta;inputs[11]=0x155
        self.prepare(inputs)
        packet=bytes((1,0x3d,9,0,owner))+struct.pack('<I',mask)
        guard=b'\xa7'*16
        self.mu.mem_write(PACKET-16,guard+packet+guard)
        saved={UC_X86_REG_EBX:0x123456,UC_X86_REG_ESI:0x234567,UC_X86_REG_EDI:0x345678,UC_X86_REG_EBP:0x456789}
        #비휘발 레지스터는 원본 호출의 서로 다른 입력으로 채워 보존을 확인한다.
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,0x13572468);self.mu.reg_write(UC_X86_REG_EDX,0x24681357)
        self.mu.mem_write(STACK,struct.pack('<IIII',STOP,0x654321,0x765432,PACKET))
        seh=bytes(self.mu.mem_read(0x400000,16))
        self.mu.emu_start(0x491220,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        assert bytes(self.mu.mem_read(PACKET-16,16))==guard and bytes(self.mu.mem_read(PACKET+9,16))==guard
        assert bytes(self.mu.mem_read(0x400000,16))==seh
        return [bytes(self.mu.mem_read(PLAYERS,1620)).hex(),bytes(self.mu.mem_read(ALLIES,324)).hex(),';'.join(self.events) or '-',bytes(self.mu.mem_read(PACKET,9)).hex()]


def verify():
    """동결 관측의 모든 SHA와 입력/두 정밀도 일치를 확인하고 재관측으로 덮어쓰지 않는다."""
    report=json.loads(REPORT.read_text('utf-8'))
    #현재 관측 파일만 검사하며 이전 fixture/보고서는 변경하지 않는다.
    for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
    rows=list(csv.reader(FIXTURE.open(encoding='utf-8',newline=''),delimiter='\t'))[1:]
    assert len(rows)==report['uniqueInputs']==288 and all(row[1:5]==row[5:9] for row in rows)
    print('Player alliance packet: 288 inputs, 576 normal returns, precision difference 0')


def observe():
    """owner/비트마스크/known/로컬/진단 입력의 모든 조합을 두 정밀도로 실제 실행한다."""
    oracle=Oracle();rows=[]
    #순서는 C++ 계산에 의존하지 않는 명시 원본 입력의 직교 조합이다.
    for values in itertools.product(range(9),(0,1,0x100,0xffffffff),(0,1),(1,8),(0,1)):
        a=oracle.run(values,0x027f);b=oracle.run(values,0x037f)
        assert a==b,values
        rows.append([','.join(map(str,values))]+a+b)
    with FIXTURE.open('w',encoding='utf-8',newline='') as fp:
        writer=csv.writer(fp,delimiter='\t',lineterminator='\n')
        writer.writerow(['input','players53','allies53','effects53','packet53','players64','allies64','effects64','packet64']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','tools/decomp_player_alliance_packet.py','tools/decomp_player_participation.py','tools/decomp_priestmenu.py','extracted/refined/originals/functions.tsv',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    report=dict(edition='10.78',entry='00491220',scope='전체 수신/실제00490ea0. Chat/진단/004e2e30 broadcast는 관측 경계. 원본 게임 창 실행 없음.',uniqueInputs=len(rows),normalReturns=len(rows)*2,instructions=oracle.instructions,precisionDifferentInputs=0,sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    verify()


if __name__=='__main__':
    if '--verify' in sys.argv:verify()
    else:observe()
