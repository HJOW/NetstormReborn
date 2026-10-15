#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 보호 PE의 실제 Form/삭제 writer와 broadcast wrapper를 격리 x86에서 관찰한다."""
import csv
import json
import random
import socket
import struct
from decomp_owner_oracle import OwnerOracle,ROOT,POOL,PLAYERS,STACK,STOP,digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP)

SPEC={  # 직접 읽은 가상 표/코드/현재 등록 헤더와 삭제 표식의 판본별 주소다.
    'originals':dict(depend=0x4ae560,content=0x4aeb40,delete=0x4ac340,wrapper=0x4ac260,
        dependVt=0x512110,contentVt=0x512208,server=0x540bc0,guard=0x5c7a78,capacity=0x5c847c,
        dependHeader=0x52ee52,contentHeader=0x52edc2,deleteHeader=0x52edc6,
        stamp=0x5c845c,token=0x5c8460,buffer=0x5c7c50,send=0x4e2e30,join=0x4ac2d4),
    'originalCD':dict(depend=0x4af750,content=0x4aff30,delete=0x4abc10,wrapper=0x4abb40,
        dependVt=0x505a08,contentVt=0x505b00,server=0x540a28,guard=0x53da08,capacity=0x5395f4,
        dependHeader=0x510100,contentHeader=0x50ff44,deleteHeader=0x50ff50,
        stamp=0x5395d4,token=0x5395d8,buffer=0x56f200,send=0x4c0420,join=0x4abbaf),
}
SID=20000  # 실제 server 경계15000/6000보다 큰 입력 Form SID다.
PARENT=20100  # 실제 가상 부모 getter가 읽는 server 부모다.
OTHER=20001  # 감시 상대 Form은0 또는 이 두 연속 server 슬롯이다.
PACKET=PLAYERS+0x10000  # 격리된 직접 writer의64바이트 출력/잔존 메모리다.
FIXTURE=ROOT/'cpppj/tests/fixtures/form-packet-x86.tsv'  # 새 관찰만 보관하는 독립 fixture다.
REPORT=ROOT/'cpppj/form-packet.json'  # 실행 수/허용 범위/원본 SHA를 기록한다.


class FormPacketOracle(OwnerOracle):
    """실제 writer/헤더/부모 getter만 실행하며 broadcast 장치 요청을 경계로 관찰한다."""
    def __init__(self,edition):
        """현재 PC의 정밀 export 범위와 큰 server SID를 수용하는 격리 풀을 준비한다."""
        super().__init__(edition)
        self.form=SPEC[edition]  # 실제 판본 주소 표다.
        self.paths=[ROOT/f'extracted/form-packet/{edition}/{name}' for name in ('creation.c','functions.tsv')]  # 현재 호스트 정밀 C/몸체 범위다.
        self.allowed=[]  # 다른 단위의 기계어 허용 범위는 이어받지 않는다.
        self.entries=set()  # 실제 진입 수는 새 내보내기만 센다.
        with self.paths[-1].open(encoding='utf-8') as fp:
            # Ghidra의 불연속 함수 몸체 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 각 실제 범위를 하나의 허용 구간으로 등록한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(POOL+0x2000,0x1fe000)
        self.put(self.form['capacity'],24000)  # C++와 같은 논리 용량으로 모든 입력 server SID를 포함한다.
        self.external=0  # broadcast 장치 관찰이며 네트워크 I/O가 아니다.
        self.preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,
            UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}  # 원본 호출 규약의 보존 레지스터 입력이다.

    def put(self,address,value):
        """격리 입력 DWORD 하나를 원본 메모리에 넣는다."""
        self.mu.mem_write(address,struct.pack('<I',value))

    def on_instruction(self,mu,address,size,data):
        """직렬화 이후 broadcast 요청의 포인터만 관찰하며 writer 자체를 대체하지 않는다."""
        if address==self.form['join']:self.written=mu.reg_read(UC_X86_REG_EAX)  # wrapper가 실제로 받은 가상 writer 반환 크기다.
        if address==self.form['send']:
            sp=mu.reg_read(UC_X86_REG_ESP)  # 실제 cdecl 호출 직후 스택이다.
            ret,packet=struct.unpack('<II',mu.mem_read(sp,8))
            assert packet==self.form['buffer']
            self.sent+=1;self.external+=1
            mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        super().on_instruction(mu,address,size,data)

    def sample(self,kind,index):
        """현재 raw 바이트/헤더/상대쌍/flags/표식 조합으로 실제 정상 반환을 관찰한다."""
        rng=random.Random(0x4ae560+index)  # 입력만 만드는 고정 난수이며 기대 패킷은 계산하지 않는다.
        raw=bytearray(rng.randbytes(self.stride))  # 필드 뒤 stale 바이트도 입력의 일부다.
        dependency=kind in 'DW'  # W/V/Z는 실제 wrapper를 통과한다.
        deleting=kind in 'XZ'
        wrapped=kind in 'WVZ'
        struct.pack_into('<I',raw,0,self.form['dependVt' if dependency else 'contentVt'])
        raw[10]=1 if dependency else 6
        struct.pack_into('<H',raw,16,PARENT)
        if dependency:
            struct.pack_into('<HH',raw,18,OTHER if index&1 else 0,OTHER+1 if index&2 else 0)
        # 두 상대의 실제 가상 getter와 server 부모 입력을 준비한다.
        for sid in (OTHER,OTHER+1):
            other=bytearray(self.stride);struct.pack_into('<I',other,0,self.form['dependVt']);struct.pack_into('<H',other,16,PARENT+1)
            self.mu.mem_write(self.slot(sid),bytes(other))
        message=(0x65 if dependency else 0x2d) if index%5==0 else rng.randrange(256)
        protocol=1 if index%5==0 else rng.randrange(256)
        length=(13 if dependency else 18) if index%5==0 else (0,1,3,4,32,65535)[index%6]
        if deleting and index%5==0:message,protocol,length=0x2e,1,8
        headerKey='deleteHeader' if deleting else 'dependHeader' if dependency else 'contentHeader'
        header=self.form[headerKey]  # writer가 실제로 읽는 현재 등록 전역이다.
        if self.edition=='originals':self.mu.mem_write(header,struct.pack('<BBH',message,protocol,length))
        else:self.mu.mem_write(header,struct.pack('<III',message,protocol,length))
        flags=rng.getrandbits(32)  # 삭제는 상위WORD의 실제 마스크를 소비한다.
        argument=rng.getrandbits(32)  # data의 두 번째 인자/삭제의 세 번째 인자는 원본 미사용 입력이다.
        stamp=(0,SID,SID+1,0x10000+SID)[index%4]
        token=rng.getrandbits(32)  # 표식은 낮은4비트로 정규화하지 않는다.
        self.put(self.form['stamp'],stamp);self.put(self.form['token'],token)
        self.put(self.form['server'],1);self.put(self.form['guard'],0)
        self.mu.mem_write(self.slot(SID),bytes(raw))
        initial=rng.randbytes(64)  # 반환 크기 뒤를 포함한 출력 버퍼 입력이다.
        buffer=self.form['buffer'] if wrapped else PACKET
        self.mu.mem_write(buffer,initial);self.sent=0;self.written=None  # 이번 broadcast 수와 실제 writer 반환 관찰값이다.
        self.write_ranges=[(buffer,buffer+64),(self.form['stamp'],self.form['stamp']+4)]
        # 모든 보존 레지스터에0이 아닌 값을 넣어 정상 반환까지 함께 확인한다.
        for register,value in self.preserved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_ECX,self.slot(SID));self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_EDX,0)
        args=(0xffffffff,0xfffffffe if deleting else 0xffffffff,argument,flags) if wrapped else (buffer,argument,flags) if deleting else (buffer,argument)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.reg_write(UC_X86_REG_ESP,STACK)
        entry='wrapper' if wrapped else 'delete' if deleting else 'depend' if dependency else 'content'
        self.mu.emu_start(self.form[entry],STOP,count=6000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4*(len(args)+1)
        assert all(self.mu.reg_read(register)==value for register,value in self.preserved.items())
        assert self.sent==int(wrapped)
        size=self.written if wrapped else self.mu.reg_read(UC_X86_REG_EAX)  # 정적 길이를 대신 계산하지 않고 실제 EAX를 기록한다.
        assert size in (8,13,18)
        assert bytes(self.mu.mem_read(self.slot(SID),self.stride))==bytes(raw)
        return (self.edition,kind,index,raw.hex(),initial.hex(),message,protocol,length,argument,flags,stamp,token,
            size,bytes(self.mu.mem_read(buffer,64)).hex(),struct.unpack('<I',self.mu.mem_read(self.form['stamp'],4))[0],self.sent)


def main():
    """판본당576개의 새로운 고유 입력을 관찰하고 fixture/근거를 UTF-8로 저장한다."""
    rows=[]  # 실제 정상 반환이 끝난 입력/출력만 담는다.
    editions={}  # 판본별 원본/코드 범위/관찰 수의 근거다.
    # 판본마다 별도 x86 메모리와 실제 코드를 사용한다.
    for edition in SPEC:
        oracle=FormPacketOracle(edition)
        # 직접 writer 세 종류와 실제 broadcast wrapper 세 종류를 독립 입력으로 실행한다.
        for kind in 'DCXWVZ':
            # 상대0/각 쌍·서로 다른 헤더·표식 일치/불일치/상위DWORD·임의 필드 바이트를 교차한다.
            for index in range(96):rows.append(oracle.sample(kind,index))
        editions[edition]=dict(inputs=576,returns=576,assertions=oracle.assertions,peSha=oracle.sha256,
            nativeCalls=dict(oracle.native_calls),externalBroadcasts=oracle.external,
            exports={path.relative_to(ROOT).as_posix():digest(path) for path in oracle.paths})
        print(edition,'576 정상 반환',flush=True)
    assert len(rows)==len({row[:3] for row in rows})==1152
    FIXTURE.write_text('# 실제 PE Form/삭제 writer와 wrapper; broadcast 요청만 외부 관찰.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),uniqueInputs=len(rows),editions=editions,
        fixtureSha=digest(FIXTURE),toolSha=digest(__file__),helperSha=digest(ROOT/'tools/decomp_owner_oracle.py'),limitations=[
            '실제 writer/헤더/부모 getter/삭제 표식 및 wrapper를 실행했다. broadcast 장치만 외부 관찰이며 실제 소켓 I/O가 아니다.',
            '정상 server/부모/상대 Form 경로만 실행한다. 원본 assert 경로와 다른 파생 가상 getter/writer는 제외한다.',
            'wrapper 수신자는-1이다. 특정 수신자 조회/송신 분기는 이번 실행 범위가 아니다.',
            '두 논리 풀은24000슬롯이며 주소/확보 바이트 수와 free list 준비 방식은 다르다. writer는 free list를 읽지 않는다. 입력 슬롯과64바이트 버퍼를 직접 대조한다.'
        ]),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':main()
