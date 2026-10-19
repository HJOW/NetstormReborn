#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 섬 정리 caller의 실제 좌표 변환·헤더·20BYTE 방송 구간을 격리 관측한다."""
import argparse
import itertools
import json
import socket
import struct

from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)

FIXTURE=ROOT/'cpppj/tests/fixtures/drop-island-packet-x86.tsv'  # 제품 구현 전 별도 동결하는 원본 패킷이다.
REPORT=ROOT/'cpppj/drop-island-packet-native.json'  # 부분 구간 완료와 전체 함수 반환을 구별한다.


def cases():
    """좌표 절삭/음수/낮은DWORD 감김과 모든 payload·기존 스택 바이트를 교차한다."""
    points=((0.,0.),(135.,108.),(135.5,108.25),(-1.25,-2.5),(1/65536,255.9999),(-0.,0.),(32768.,-32768.))
    return [(struct.unpack('<I',struct.pack('<f',x))[0],struct.unpack('<I',struct.pack('<f',y))[0],flags,owner,seed)
            for (x,y),flags,owner,seed in itertools.product(points,(0,8,0x1000,0xffffffff),(0,1,8,0xffffffff),(0,0xa5,0xff))]


class Oracle(BaseOracle):
    """실제46e6e7..46e732와40a740/4df230/_ftol만 실행한다."""
    def __init__(self):
        """같은 보호 PE와 격리 스택을 사용하며 명령/메타데이터를 수정하지 않는다."""
        super().__init__()
        self.allowed=[]
        ranges(self,(0x40a740,0x4df230,0x4e49c0))
        self.allowed.append((0x46e6e7,0x46e732))
        assert bytes(self.mu.mem_read(0x52edde,4))==bytes((0x35,1,20,0))
        self.instructions=0

    def instruction(self,mu,address,size,data):
        """원격 송신만 인수 관측 경계이며 실제 작성/좌표 변환은 PE가 수행한다."""
        self.instructions+=1
        if address==0x4e2e30:
            pointer=self.get(mu.reg_read(UC_X86_REG_ESP)+4)
            self.packet=bytes(mu.mem_read(pointer,20))
            self.ret()
            return
        if address==0x46e732:
            self.ret()  # 검토한 caller 구간의 완료이며46e6c0 전체 함수의 정상 반환은 아니다.
            return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):
            raise RuntimeError(f'섬 정리 패킷 미검토 주소 {address:x}')

    def write(self,mu,access,address,size,value,data):
        """caller 스택/패킷 밖으로 실행 중 쓰지 못하게 한다."""
        if not STACK-0x1000<=address or address+size>STACK+0x1000:
            raise RuntimeError(f'섬 정리 패킷 스택 밖 쓰기 {address:x}')

    def sample(self,case,precision):
        """caller의 원래 입력 레지스터/스택으로 실행하고 전체 packet과 canary를 읽는다."""
        x,y,flags,owner,seed=case
        self.mu.mem_write(STACK-0x1000,b'\xcc'*0x2000)
        self.put(STACK,STOP)
        self.put(STACK+0x74,x)
        self.put(STACK+0x78,y)
        self.put(STACK+0x80,owner)
        self.mu.mem_write(STACK+0x1c,bytes((seed,))*20)
        self.packet=None
        registers={UC_X86_REG_EAX:0,UC_X86_REG_EBX:flags,UC_X86_REG_ECX:0,UC_X86_REG_EDX:0,
            UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab,
            UC_X86_REG_ESP:STACK,UC_X86_REG_EFLAGS:2,UC_X86_REG_FPCW:precision,UC_X86_REG_FPSW:0,UC_X86_REG_FPTAG:0xffff}
        # 구간의 실제 EBX=flags 입력을 ABI sentinel로 덮어쓰지 않는다.
        for register,value in registers.items():
            self.mu.reg_write(register,value)
        self.mu.emu_start(0x46e6e7,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert self.mu.reg_read(UC_X86_REG_EBX)==flags and self.mu.reg_read(UC_X86_REG_ESI)==0x23456789
        assert self.mu.reg_read(UC_X86_REG_EDI)==0x3456789a and self.mu.reg_read(UC_X86_REG_EBP)==0x456789ab
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        assert bytes(self.mu.mem_read(STACK+0x30,4))==b'\xcc'*4 and self.packet is not None
        return self.packet


def main():
    """새 독립 기대값을 동결하거나 보존된 관측을 그대로 재생한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    parser.add_argument('--replay',action='store_true')
    args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 과거 관측이나 기대값을 다시 쓰지 않는다.
        for name,expected in report['sha256'].items():
            assert digest(ROOT/name)==expected,name
        assert report['inputs']==336 and report['segmentCompletions']==672 and report['normalReturns']==0
        print('섬 정리 방송 원본 패킷 출처 통과')
        return
    if not args.replay:
        assert not FIXTURE.exists() and not REPORT.exists()
    oracles=[Oracle(),Oracle()]
    rows=[]
    # 매 기대값은 기존C++ 결과를 읽지 않고 보호 x86 구간에서 생성한다.
    for case in cases():
        packets=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
        assert packets[0]==packets[1]
        wire=struct.unpack_from('<II',packets[0],4)  # 기존RawConstructionEffects가 방송 경계로 넘기는 실제16.16 DWORD다.
        rows.append('\t'.join([','.join(map(str,case)),*(str(value) for value in wire),*(packet.hex() for packet in packets)]))
    content='# xbits,ybits,flags,owner,seed wireX wireY packet53 packet64\n'+'\n'.join(rows)+'\n'
    if args.replay:
        assert FIXTURE.read_bytes()==content.encode('utf-8')
        print('섬 정리 방송 원본 구간 재생 일치',len(rows),len(rows)*2)
        return
    FIXTURE.write_text(content,encoding='utf-8',newline='\n')
    names=['originals/Netstorm.exe','tools/decomp_drop_island_packet.py','tools/decomp_cursor_lifecycle.py',
        'tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv',FIXTURE.relative_to(ROOT).as_posix()]
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),date='2026-10-19',lastDecompile='VM-W11-Codex / 2026-10-19',
        inputs=len(rows),segmentCompletions=len(rows)*2,normalReturns=0,instructions=sum(o.instructions for o in oracles),
        scope='46e6e7..46e732 실제 caller/40a740/4df230/4e49c0, 전송4e2e30은 인수 경계다. server/flags 관문은 구간 앞이므로 기존constructioneffects 전체 fixture에 맡긴다. CD·전체46e6c0 반환·자연 전투 완료로 확대하지 않는다.',
        sha256={name:digest(ROOT/name) for name in names}),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('섬 정리 방송 독립 원문',len(rows),len(rows)*2)


if __name__=='__main__':
    main()
