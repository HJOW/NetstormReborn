# -*- coding: utf-8 -*-
"""447340의 실제 Unpop→현재 TYPE/WORD→Form→Take 순서를 독립 CPU로 관측한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent  # 같은 보호 바이너리와 읽기 전용 내보내기의 루트다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
from decomp_oracle import Oracle, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL,SID,TABLE=0x12000000,50,SCRATCH+0x1000  # 실제50BYTE 객체와 가상48 포인터를 담는 격리 메모리다.


class PickupOracle(Oracle):
    """외부 가상48/내용물 생성/커서 취득만 분리하고447340 몸체를 실행한다."""
    def __init__(self):
        """대상 함수만 허용하며 게임/창/통신은 실행하지 않는다."""
        super().__init__('originals',[dict(name='Pickup',originals='00447340')],[])
        self.mu.mem_map(POOL,0x1000)
        self.trace=[]  # 외부 경계가 받은 실제 인자/순서다.
        self.mutation=0  # Unpop이 현재 TYPE/WORD를 바꾸는 명시 입력이다.
        self.result=0  # 내용물 생성의 원문 폭 반환 입력이다.

    def on_instruction(self,mu,address,size,data):
        """가상/thiscall/cdecl 반환 폭을 원래 호출규약대로 적용한다."""
        if address not in (0x4afe50,0x4afa70,0x447000):
            return super().on_instruction(mu,address,size,data)
        stack=mu.reg_read(UC_X86_REG_ESP)
        ret=struct.unpack('<I',mu.mem_read(stack,4))[0]
        if address==0x4afe50:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50
            flags=struct.unpack('<I',mu.mem_read(stack+4,4))[0]
            self.trace.append(f'U,{sid},{flags}')
            if self.mutation:
                mu.mem_write(POOL+sid*50+10,b'\x78');mu.mem_write(POOL+sid*50+12,b'\x34\x12')
            purge=4
        elif address==0x4afa70:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50
            args=struct.unpack('<IIIII',mu.mem_read(stack+4,20))
            self.trace.append('F,'+','.join(map(str,(sid,*args))))
            mu.reg_write(UC_X86_REG_EAX,self.result);purge=20
        else:
            args=struct.unpack('<II',mu.mem_read(stack+4,8))
            self.trace.append('T,'+','.join(map(str,args)));purge=0
        mu.reg_write(UC_X86_REG_ESP,stack+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def observe(self,flags,kind,word,mutation,result):
        """원문 입력만 설치하고 효과/최종 물리는 실행 뒤 읽는다."""
        self.trace,self.mutation,self.result=[],mutation,result
        self.mu.mem_write(POOL,bytes(0x1000));self.mu.mem_write(POOL+SID*50,struct.pack('<I',TABLE)+bytes(46))
        self.mu.mem_write(TABLE+0x48,struct.pack('<I',0x4afe50));self.mu.mem_write(0x5c8464,struct.pack('<I',POOL))
        self.mu.mem_write(POOL+SID*50+10,bytes([kind]));self.mu.mem_write(POOL+SID*50+12,struct.pack('<H',word))
        self.call('Pickup',[SID,flags])
        raw=bytes(self.mu.mem_read(POOL+SID*50,50))
        return ';'.join(self.trace),f'{raw[10]},{struct.unpack_from("<H",raw,12)[0]}'


def main():
    """새 관측은 신규 파일에만 쓰며 기존 관측은 바이트 일치로 재검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    oracle=PickupOracle();body=bytes(oracle.mu.mem_read(0x447340,0x4c));rows=['# flags type word mutation form trace state(type,word)']
    for case in itertools.product((0,1,0x80,0xffffffff),(82,120,255),(0,65535),(0,1),(0,900)):
        # OR100/unsigned TYPE·WORD/Unpop後再조회/생성0의Take호출도 교차한다.
        trace,state=oracle.observe(*case);rows.append('\t'.join([*(str(v) for v in case),trace,state]))
    content=('\n'.join(rows)+'\n').encode('utf-8');fixture=ROOT/'cpppj/tests/fixtures/cursor-object-pickup-x86.tsv'
    if args.verify:
        assert fixture.read_bytes()==content
    else:
        assert not fixture.exists();fixture.write_bytes(content)
    assert body==bytes(oracle.mu.mem_read(0x447340,len(body)))
    record=dict(host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',rows=len(rows)-1,verifiedExistingFixture=args.verify,originalGameExecuted=False,originalSha256=oracle.sha256,fixtureSha256=hashlib.sha256(content).hexdigest(),code=dict(range='00447340-0044738b',sha256=hashlib.sha256(body).hexdigest(),unchanged=True),replaced=['virtual+48','4afa70 CreateContained','447000 Take'],scope='447340 호출 순서/인자/현재raw 재조회만 관측. 외부 하위/일반입력/게임 전체 실행은 아님')
    (ROOT/'extracted/cpp-pointer-modes-749/native.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(record,ensure_ascii=False))


if __name__=='__main__':
    main()  # 원본 파일과 기존 fixture를 수정하지 않는다.
