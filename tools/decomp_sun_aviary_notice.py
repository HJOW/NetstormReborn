#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""훨리베이스 가상34의 실제45b4a0 전체를 격리 실행해 홈 통지를 관측한다."""
import argparse
import itertools
import json
import socket
import struct

from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, POOL, PROCESS, VTABLE, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

FIXTURE = ROOT/'cpppj/tests/fixtures/sun-aviary-notice-x86.tsv'  # 기존 아비어리 기대값과 별개인 독립 원문이다.
REPORT = ROOT/'cpppj/sun-aviary-notice-native.json'  # 전체 정상 반환과 명시 가상 효과 경계를 구별한다.
UNPOP, REPOP = STOP+0x500, STOP+0x520  # 실제 가상48/4c의 명시 외부 경계다.


def cases():
    """서버·현재 dirty/단계와 효과가 바꾸는 현재 필드를 교차한다."""
    return list(itertools.product((0,1),(0,1,4,255),(1,8,10,0xffffffff),range(4)))


class Oracle(BaseOracle):
    """원본 함수와 실제 정수 포인터 계산을 실행하고 큰 공간/Host 효과만 대체한다."""
    def __init__(self):
        """같은 보호 PE와 허용 함수 범위·격리 입력만 준비한다."""
        super().__init__()
        self.allowed=[]
        ranges(self,(0x45b4a0,))
        self.put(VTABLE+0x48,UNPOP)
        self.put(VTABLE+0x4c,REPOP)
        self.instructions=0

    def instruction(self,mu,address,size,data):
        """효과 인수를 기록하고 명시 재진입 입력만 바꾼다."""
        self.instructions+=1
        sp=mu.reg_read(UC_X86_REG_ESP)
        ecx=mu.reg_read(UC_X86_REG_ECX)
        args=struct.unpack('<4I',mu.mem_read(sp+4,16))
        raw=POOL+50*50  # 실제50바이트 홈 입력의 위치다.
        if address==0x4b1a90:
            assert args==(50,32,0,0)
            self.event('Find',args[0],args[1])
            if self.case[3]==3:
                self.put(0x540bc0,0)
                self.put(0x5410f0,99)
            self.ret(70)
            return
        if address==0x4ae2d0:
            assert ecx==POOL+70*50
            self.event('Resolve',70)
            self.ret(PROCESS+32)
            return
        if address==UNPOP:
            assert ecx==raw and args[0]==0
            self.event('Unpop',50,0)
            self.put(raw+11,4,1)
            if self.case[3]==1:
                self.put(raw+36,99)
                self.put(PROCESS+8,0x10,1)
                self.put(PROCESS+24,5)
                self.put(PROCESS+40,51)
            self.ret(0,4)
            return
        if address==REPOP:
            assert ecx==raw and args[0]==0
            self.event('Repop',50,0,self.get(raw+36))
            self.put(raw+11,0,1)
            if self.case[3]==2:
                self.put(raw+36,77)
                self.put(PROCESS+8,0x40,1)
                self.put(PROCESS+24,9)
            self.ret(0,4)
            return
        if address==0x41c010:
            assert ecx==PROCESS+32
            self.event('Refresh')
            self.ret()
            return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):
            raise RuntimeError(f'홈 통지 범위밖 명령 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """실제 몸체의 raw 프레임/프로세스와 스택 쓰기만 허용한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000:
            return
        if POOL+50*50<=address and address+size<=POOL+51*50:
            return
        if PROCESS<=address and address+size<=PROCESS+48:
            return
        raise RuntimeError(f'홈 통지 범위밖 쓰기 {address:x}+{size}')

    def sample(self,case,precision):
        """원문 입력을 준비하고 전체 두 이미지와 실제 전역/호출 순서를 읽는다."""
        self.case=case
        self.events=[]
        raw=bytearray(b'\xa5'*50)
        struct.pack_into('<I',raw,0,VTABLE)
        raw[10]=72
        raw[11]=0
        struct.pack_into('<I',raw,36,7)
        self.mu.mem_write(POOL+50*50,bytes(raw))
        fields=bytearray(b'\xa5'*48)
        fields[8]=case[1]
        struct.pack_into('<d',fields,16,100.25)
        struct.pack_into('<I',fields,24,case[2])
        struct.pack_into('<III',fields,32,0x509db8,70,50)
        self.mu.mem_write(PROCESS,bytes(fields))
        self.put(0x5c8464,POOL)
        self.put(0x540bc0,case[0])
        self.put(0x5410f0,32)
        self.call(0x45b4a0,POOL+50*50,struct.pack('<II',0x12345678,0xabcdef01),8,precision)
        self.returns+=1
        return (';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+50*50,50)).hex(),bytes(self.mu.mem_read(PROCESS,48)).hex(),f'{self.get(0x540bc0)},{self.get(0x5410f0)}')


def main():
    """최초 독립 관측을 동결하거나 기존 기대값을 보존하며 재생한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--replay',action='store_true')
    args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 역사 기대값을 현재 제품 출력으로 바꾸지 않는다.
        for path,expected in report['sha256'].items():
            assert digest(ROOT/path)==expected,path
        assert report['inputs']==128 and report['normalReturns']==256
        print('훨리베이스 홈 통지 원문 출처 통과')
        return
    if not args.replay:
        assert not FIXTURE.exists() and not REPORT.exists()
    oracles=[Oracle(),Oracle()]  # 두 정밀도는 초기 상태부터 별개 실행한다.
    rows=[]
    # 각 입력의 기대값은 제품과 독립인 보호 x86 실행에서만 읽는다.
    for case in cases():
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
        assert outputs[0]==outputs[1]
        rows.append('\t'.join([','.join(map(str,case)),*outputs[0],*outputs[1]]))
    content='# server,dirty,stage,mutation trace raw fields globals; 두 정밀도\n'+'\n'.join(rows)+'\n'
    if args.replay:
        assert FIXTURE.read_bytes()==content.encode('utf-8')
        print('훨리베이스 홈 통지 기존 원문 재생 일치',len(rows),sum(o.returns for o in oracles))
        return
    FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    names=['originals/Netstorm.exe','tools/decomp_sun_aviary_notice.py','tools/decomp_cursor_lifecycle.py',
        'tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv',
        FIXTURE.relative_to(ROOT).as_posix()]  # 실제 원문/ABI/실행 범위와 이번 독립 결과다.
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',
        inputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),
        scope='10.78 45b4a0 전체 정상 반환. 4b1a90/4ae2d0의 현재 Host 조회, 가상Unpop/Repop와41c010은 명시 효과 경계다. null프로세스 충돌·CD·자연 전투 완료를 포함하지 않는다.',
        sha256={name:digest(ROOT/name) for name in names}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('훨리베이스 홈 통지 독립 원문',len(rows),sum(o.returns for o in oracles))


if __name__=='__main__':
    main()
