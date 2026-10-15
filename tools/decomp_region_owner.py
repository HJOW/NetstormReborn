#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 실제 영역 소유자/float snap/CRT를 외부 callback 없이 격리 실행한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
import sys
from pathlib import Path

# 보호 바이너리는 읽기만 하며 합성 입력/보고는 별도 경로에 쓴다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# VM의 원본과 겹치지 않는 Pool/표면 지도/반환 표식/스택이다.
POOL, SURFACES, STOP, STACK_BASE, STACK = 0x11000000, 0x11100000, 0x20000000, 0x30000000, 0x3000f000
FIXTURE = ROOT / 'cpppj/tests/fixtures/region-owner-x86.tsv'
REPORT = ROOT / 'cpppj/region-owner.json'
SPEC = {
    'originals': dict(binary='originals/Netstorm.exe', stride=50, owner=34, integer=0x46f710, floating=0x46f770,
        pool=0x5c8464, surfaces=0x5c84bc, terrain=0x5411dc,
        ranges=[(0x46f710,0x46f766),(0x46f770,0x46f79b),(0x41d7a0,0x41d7d7),(0x4e49c0,0x4e4a35)]),
    'originalCD': dict(binary='originalCD/NETSTORM.EXE', stride=36, owner=32, integer=0x4bddb0, floating=0x4bde10,
        pool=0x5395dc, surfaces=0x52d590, terrain=0x51cac8,
        ranges=[(0x4bddb0,0x4bde0e),(0x4bde10,0x4bde3b),(0x440a80,0x440abb),(0x4f161c,0x4f1643)])}
SAVED = {UC_X86_REG_EBX:0x12345678, UC_X86_REG_ESI:0x23456789, UC_X86_REG_EDI:0x3456789a, UC_X86_REG_EBP:0x456789ab}


def digest(path):
    """근거 파일의 현재 바이트를 SHA-256으로 고정한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value):
    """float32 입력을 십진수 손실 없이 저장한다."""
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases():
    """좌표 경계·현재 타입·WORD0/비영·소유자BYTE·dead/void 상태의 독립 입력을 만든다."""
    result = []
    # 정수 getter는 모든 안/밖 경계와 일치/불일치 타입을 직접 검사한다.
    for x, y, sid, terrain, owner, state in itertools.product((-1,0,255,256),(-1,0,255,256),(0,5),(97,98),(0,7,255),(0,6)):
        result.append(['Integer',x & 0xffffffff,y & 0xffffffff,sid,terrain,97,owner,state])
    # snap 직전/경계/큰 값/비유한도 실제 CRT로 실행한다. 기대 좌표를 Python으로 계산하지 않는다.
    values = [bits(x) for x in (-1.0,-0.99999,-0.00002,0.0,0.00002,0.99999,1.0,254.0,255.0,255.00002,256.0,16777216.0,4294967296.0)]
    values += [0x3f7fffff,0x3f800001,0x7fc00001,0x7f800000,0xff800000]
    for x, y, sid, terrain, owner in itertools.product(values,(bits(0.0),bits(1.5),bits(255.0)),(0,5),(97,98),(7,255)):
        result.append(['Float',x,y,sid,terrain,97,owner,6])
    return result


class Oracle:
    """네 실제 함수만 실행하고 스택 밖 native 쓰기/외부 함수 진입을 거부한다."""
    def __init__(self, edition):
        """보호 PE와 합성 입력 공간을 분리하여 초기화한다."""
        self.spec=SPEC[edition]  # 현재 판본의 실제 진입 주소/필드 폭/허용 범위다.
        pe=pefile.PE(str(ROOT/self.spec['binary']));self.base=pe.OPTIONAL_HEADER.ImageBase;self.data=pe.get_memory_mapped_image()  # 원본 이미지 기준 주소와 읽기 전용 바이트다.
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(self.data)+4095)&~4095);self.mu.mem_write(self.base,self.data)  # 실제 PE 몸체를 실행할 독립 VM이다.
        # 서로 겹치지 않는 가상 영역만 확보한다.
        for address,size in ((POOL,0x10000),(SURFACES,0x20000),(STOP,0x1000),(STACK_BASE,0x10000)):self.mu.mem_map(address,size)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.instructions=0;self.returns=0  # 실제 명령 실행 수와 정상 최상위 반환 수를 각각 센다.

    def put(self,address,value):
        """명시 입력 DWORD를 가상 메모리에만 쓴다."""
        self.mu.mem_write(address,struct.pack('<I',value & 0xffffffff))

    def instruction(self,mu,address,size,data):
        """CRT를 포함해 지정한 실제 함수 범위만 허용한다."""
        if address==STOP:return
        if not any(low<=address and address+size<=high for low,high in self.spec['ranges']):raise RuntimeError(f'미허용 native 진입 {address:08x}')
        self.instructions+=1

    def write(self,mu,access,address,size,value,data):
        """몸체는 인자/CRT의 스택 임시값만 쓸 수 있다."""
        if not (STACK_BASE<=address and address+size<=STACK_BASE+0x10000):raise RuntimeError(f'미허용 native 쓰기 {address:08x}+{size}')

    def run(self,case,control):
        """정상 반환·callee 정리·비휘발 레지스터·x87 스택/제어어를 확인한다."""
        op,x,y,sid,terrain,actual,owner,state=case;s=self.spec;mu=self.mu
        self.put(s['pool'],POOL);self.put(s['surfaces'],SURFACES);self.put(s['terrain'],terrain)
        # 모든 칸에 같은 명시 SID를 입력해 원본이 어느 경계 칸을 고르든 실제 getter 결과를 관찰한다.
        mu.mem_write(SURFACES,struct.pack('<H',sid)*65536);raw=bytearray(s['stride']);raw[10]=actual;raw[11]=state;raw[s['owner']]=owner
        mu.mem_write(POOL+5*s['stride'],bytes(raw))
        # 호출 규약에서 보존해야 할 레지스터의 명시 입력을 고정한다.
        for reg,value in SAVED.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_ECX,0x55555555)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<III',STOP,x,y));mu.emu_start(s['integer'] if op=='Integer' else s['floating'],STOP,count=2000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+12:raise RuntimeError('native 정상 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in SAVED.items()):raise RuntimeError('native 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('native x87 제어어/스택 오류')
        self.returns+=1;return mu.reg_read(UC_X86_REG_EAX)


def generate():
    """C++를 읽거나 실행하지 않고 실제 PE 관찰을 두 x87 모드에서 저장한다."""
    rows=[];counts={};instructions={}
    # 판본별 VM을 따로 만들어 전역/슬롯 폭이 섞이지 않게 한다.
    for edition in SPEC:
        oracle=Oracle(edition)
        # 같은 고유 입력을 두 x87 제어어에서 실행해 실제 결과 차이를 확인한다.
        for case in cases():
            observed=[oracle.run(case,control) for control in (0x027f,0x037f)]
            if observed[0]!=observed[1]:raise RuntimeError('x87 모드별 소유자 결과 차이')
            rows.append([edition,*case,observed[0]])
        counts[edition]=oracle.returns;instructions[edition]=oracle.instructions
    FIXTURE.write_text('# 실제 두 PE 영역소유자/float snap/CRT 정상반환. 외부 경계/게임 실행 없음.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),FIXTURE,*[ROOT/s['binary'] for s in SPEC.values()]]
    report={'host':socket.gethostname(),'date':'2026-10-15','unique_inputs':len(rows),'returns':counts,'instructions':instructions,
            'boundaries':[],'os_calls':0,'spec':SPEC,'files':{p.relative_to(ROOT).as_posix():digest(p) for p in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'inputs':len(rows),'returns':counts},indent=2))


def verify():
    """기존 근거 바이트/입력/집계를 재실행 없이 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 저장한 모든 원본/입력/생성기 바이트의 현재 해시를 대조한다.
    for path,sha in report['files'].items():
        if digest(ROOT/path)!=sha:raise RuntimeError('근거 SHA 불일치 '+path)
    plan=[[edition,*case] for edition in SPEC for case in cases()]
    if len(rows)!=len(plan) or report['unique_inputs']!=len(rows) or report['os_calls']:raise RuntimeError('근거 입력 집계 불일치')
    # 실제 실행 전에 확정한 명시 입력 순서와 fixture 입력을 비교한다.
    for row,case in zip(rows,plan):
        if row[:-1]!=list(map(str,case)):raise RuntimeError('근거 입력 계획 불일치')
    if any(value!=2*len(cases()) for value in report['returns'].values()):raise RuntimeError('정상반환 집계 불일치')
    print('영역소유자 독립 근거 감사 통과',len(rows))


def main():
    """새 원본 관찰 또는 기존 자료 감사 중 명시한 작업만 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()


if __name__=='__main__':main()
