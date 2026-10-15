#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 보호 PE의 초기 시야 선택·중심 변환·실제 ftol을 격리 x86에서 실행한다."""
import csv
import itertools
import json
import socket
import struct
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, POOL, PLAYERS, CAPACITY, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

SPEC = {  # 정밀 내보내기로 확인한 함수/전역의 판본별 실제 주소다.
    'originals': dict(entry=0x4340d0, commit=0x497960, capacity=0x5c847c, local=0x540c70, fort=0x594fb8, battle=0x594fbc,
        challenge=0x594fc8, target=0x541208, depth=0x5c89b8, requests=0x59a8b0, revision=0x5c85dc, camera=0x59a8bc, width=0x5c78f0, height=0x5c78f4),
    'originalCD': dict(entry=0x488f50, commit=0x4582e0, capacity=0x5395f4, local=0x50f6c8, fort=0x540a1c, battle=0x540a20,
        challenge=0x540a24, target=0x51caf4, depth=0x5178d4, requests=0x52039c, revision=0x518954, camera=0x5203a8, width=0x516b10, height=0x516b14),
}
CAMERA = PLAYERS+0x8000  # 실제 화면 장치 대신 사용하는 명시 카메라 상태 입력이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/camera-home-x86.tsv'  # 새 초기 시야 고유 입력의 실제 결과다.
REPORT = ROOT/'cpppj/camera-home.json'  # 원본 PE/정밀 몸체/도구/fixture SHA를 기록한다.
TYPE_DATA = ((70,0x200,3,6),(71,0x210000,1,1),(72,0x200000,3,2),(73,0x10000,6,4),(74,0,1,1))  # 사제·신전·다른 walker·Challenge의 명시 TYPE 입력이다.


def bits(value):
    """명시 float 입력을 원본 DWORD로 저장한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def scene(variant):
    """기대 판단 없이 raw 슬롯 입력만 만든다. 각 항목은 sid/type/state/owner/x/y다."""
    priest = (50,71,0,1,bits(94.25),bits(108.8125))  # 기본 내 사제의 실제 좌표 형식이다.
    table = {  # SID 순서·상태·genus·소유자 차이로 서로 독립인 장면을 준비한다.
        0: [], 1: [priest], 2: [priest,(90,70,0,1,bits(211),bits(201))],
        3: [(6,71,0,1,bits(17.125),bits(18.25)),(90,71,0,1,bits(193.625),bits(199.75))],
        4: [priest,(90,71,2,1,bits(190),bits(191))],
        5: [priest,(90,71,1,1,bits(190),bits(191))],
        6: [priest,(90,71,4,1,bits(190.125),bits(191.25))],
        7: [(50,71,0,2,bits(94),bits(108))],
        8: [(50,73,0,1,bits(94),bits(108)),(90,70,0,1,bits(112),bits(117))],
        9: [priest,(90,72,0,8,bits(193.625),bits(199.75))],
        10: [(50,71,0,1,0x7fc00000,0x7f800000)],
        11: [(50,71,0,1,bits(-0.125),bits(256.125))],
        12: [(50,72,0,1,bits(32.5),bits(48.25))],
        13: [(6,74,0,2,bits(11),bits(12)),(90,74,0,1,bits(180.25),bits(170.75))],
        14: [(50,70,0,1,bits(94),bits(108))],
    }
    return table[variant]


class CameraOracle(OwnerOracle):
    """거대한 Renderer commit만 관찰 경계로 두고 카메라/타입/Carrier/ftol은 실제 명령을 실행한다."""
    def __init__(self, edition):
        """같은 격리 PE와 현재 정밀 함수 범위만 연결한다."""
        super().__init__(edition)
        self.camera = SPEC[edition]  # 이 판본의 실제 주소다.
        self.paths = [ROOT/f'extracted/camera-home/{edition}/{name}' for name in ('creation.c','functions.tsv')]  # 현재 호스트의 정확 함수 범위 근거다.
        self.allowed,self.entries = [],set()  # 초기 시야 호출 사슬에 있는 실제 몸체만 허용한다.
        with self.paths[-1].open(encoding='utf-8') as fp:
            # 내보낸 각 몸체의 불연속 코드 구간을 그대로 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 분리된 각 함수 구간만 허용하며 구간 사이의 코드는 실행하지 않는다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'))
                    self.allowed.append((low,high+1))
        self.mu.mem_map(0,4096)
        self.write_ranges=[(0,4),(CAMERA+0x50,CAMERA+0x58)]  # 실제 Carrier의 SEH 머리/카메라 pending 원점 쓰기만 허용한다.
        # 원본이 변경하는 세 전역 DWORD만 쓰기 범위에 추가한다.
        for key in ('depth','requests','revision'):
            self.write_ranges.append((self.camera[key],self.camera[key]+4))
        self.put(self.camera['capacity'],CAPACITY)
        self.put(self.camera['camera'],CAMERA)
        # 실제 getter에 들어갈 같은 타입별 genus와 판본별 발자국 위치를 설정한다.
        for number,genus,x,y in TYPE_DATA:
            self.put(TYPES+number*self.type_stride+0xec,genus)
            self.put(TYPES+number*self.type_stride+(0x1d4 if edition=='originals' else 0x1b4),x)
            self.put(TYPES+number*self.type_stride+(0x1d8 if edition=='originals' else 0x1b8),y)

    def put(self,address,value):
        """입력 DWORD를 원본 little endian으로 기록한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def word(self,address):
        """현재 격리 메모리의 원본 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def on_instruction(self,mu,address,size,data):
        """실제 commit 직전의 원점/억제/카운터를 관찰하고 이 장치 경계만 반환한다."""
        if address==self.camera['commit']:
            assert mu.reg_read(UC_X86_REG_ECX)==CAMERA
            x,y=struct.unpack('<ii',mu.mem_read(CAMERA+0x50,8))  # 실제 x87/ftol 및 화면 절반 차감 결과다.
            self.applied.append(','.join(map(str,(x,y,self.word(self.camera['depth']),self.word(self.camera['requests']),self.word(self.camera['revision'])))))
            sp=mu.reg_read(UC_X86_REG_ESP)  # 실제 thiscall ret0의 반환 프레임이다.
            ret=self.word(sp)
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,ret)
            return
        super().on_instruction(mu,address,size,data)

    def sample(self,variant,fort,battle,challenge,local,width,height,depth,requests,revision):
        """장면과 서로 독립인 모드/로컬 DWORD/화면/카운터 입력을 실제 원본에 공급한다."""
        self.applied=[]  # 이 원본 호출의 실제 Renderer 요청 순서다.
        # 원본 반복자의 시작 번호2부터 빈 슬롯을 free로 초기화한다.
        for sid in range(2,CAPACITY):
            raw=bytearray(self.stride);raw[11]=1
            self.mu.mem_write(POOL+sid*self.stride,bytes(raw))
        # 장면의 번호/상태/소유자/좌표 DWORD를 물리 위치에 직접 쓴다.
        for sid,typ,state,owner,x,y in scene(variant):
            raw=bytearray(self.stride);raw[10]=typ;raw[11]=state;raw[self.o['owner']]=owner
            struct.pack_into('<II',raw,14,x,y)
            self.mu.mem_write(POOL+sid*self.stride,bytes(raw))
        # 원본 모드와 화면 크기/카운터에 독립 입력을 그대로 공급한다.
        for key,value in dict(fort=fort,battle=battle,challenge=challenge,local=local,width=width,height=height,depth=depth,requests=requests,revision=revision,target=74).items():
            self.put(self.camera[key],value)
        self.put(CAMERA+0x48,4097);self.put(CAMERA+0x4c,2819)
        self.put(CAMERA+0x50,0x12345678);self.put(CAMERA+0x54,0x23456789)
        self.put(0,0)
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f)
        top=self.mu.reg_read(UC_X86_REG_FPSW)&0x3800  # 실제 ftol 호출 전 x87 스택 깊이다.
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.emu_start(self.camera['entry'],STOP,count=20000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==top and self.mu.reg_read(UC_X86_REG_FPCW)==0x027f and self.word(0)==0
        return '|'.join(self.applied) or '-',self.word(self.camera['depth']),self.word(self.camera['requests']),self.word(self.camera['revision'])


def main():
    """정의된 두 모드의 고유 입력과 실제 정상 반환만 별도 기록한다."""
    cases=[]  # 원본 선택식을 사용하지 않는 독립 입력 표다.
    # 일반 모드의 장면/플레이어/화면/갱신 전역을 독립적으로 조합한다.
    for variant,mode,local,size,counters in itertools.product(range(13),((0,0),(1,0),(0,1)),(0,1,2,8,257),((640,480),(801,601),(1024,768)),((0,0,0),(3,0xfffffffe,0xffffffff))):
        cases.append((variant,*mode,0,local,*size,*counters))
    # 실제 좌표가 정의된 Challenge 발견/빈 목록 경로만 입력한다.
    for variant,size,counters in itertools.product((13,14),((640,480),(801,601),(1024,768)),((0,0,0),(3,0xfffffffe,0xffffffff))):
        cases.append((variant,0,0,1,1,*size,*counters))
    rows,editions=[],{}  # 실제 출력과 판본별 실행/근거 집계다.
    # 각 판본의 고유 입력은 별도 격리 PE에서 실행한다.
    for edition in SPEC:
        oracle=CameraOracle(edition)  # 판본마다 별도의 PE/x87/메모리 공간이다.
        # C++에도 같은 물리 장면을 공급하도록 입력과 실제 관찰을 함께 저장한다.
        for values in cases:
            raw=';'.join(','.join(map(str,item)) for item in scene(values[0])) or '-'  # C++에도 동일 물리 입력을 직접 공급한다.
            rows.append((edition,values[0],raw,*values[1:],*oracle.sample(*values)))
        editions[edition]=dict(inputs=len(cases),returns=len(cases),assertions=oracle.assertions,peSha=oracle.sha256,
            nativeCalls=dict(oracle.native_calls),exports={p.relative_to(ROOT).as_posix():digest(p) for p in oracle.paths})
        print(edition,len(cases),'초기 카메라 실제 정상 반환',flush=True)
    assert len(rows)==len(set(row[:-4] for row in rows))
    FIXTURE.write_text('# 실제 초기 시야/타입/Carrier 억제/ftol. 거대한 Renderer commit만 명시 관찰 경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),uniqueInputs=len(rows),editions=editions,fixtureSha=digest(FIXTURE),
        toolSha=digest(__file__),supportSha=digest(ROOT/'tools/decomp_owner_oracle.py'),
        limitations=['현재 pool/TYPE 입력은 합성이며 실제 미션 화면 전체 동일성을 증명하지 않는다.', 'Challenge의 좌표 미초기화 경로와 억제 깊이 UINT_MAX 감김은 실행하지 않았다.', '실제 Renderer commit 내부/직접 Home의 표시 프로세스는 별도다. x87 제어워드는0x027f다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':
    main()
