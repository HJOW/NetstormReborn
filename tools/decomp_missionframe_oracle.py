#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 공통 미션 프레임/예약/정지 기계어를 GUI 없이 독립 실행한다.

사용법: python -X utf8 tools/decomp_missionframe_oracle.py [--verify]
승패·실제 창/Tell·demo·Tutorial1~6·시계 정지 효과는 명시 입력/관찰 경계다.
"""
import argparse
import collections
import hashlib
import itertools
import json
import math
import socket
import struct
import sys
from pathlib import Path

# 보호 PE와 격리 의존성은 읽기 전용이며 결과만 별도 경로에 저장한다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import capstone
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 원본 실제 함수와 가상 입력 영역이다. 다른 함수/OS 실행은 허용하지 않는다.
BINARY = ROOT / 'originals/Netstorm.exe'
SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'
ENTRIES = {'Frame': 0x4c34c0, 'Advance': 0x4c33f0, 'Pause': 0x4c2a40, 'Postpone': 0x4c3170}
RANGES = [(0x4c34c0, 0x4c36a3), (0x4c33f0, 0x4c3466), (0x4c2a40, 0x4c2a67), (0x4c3170, 0x4c3192)]
MISSION, EVENT, STOP, STACK_BASE, STACK = 0x11000000, 0x11001000, 0x20000000, 0x30000000, 0x3000f000
GLOBALS = {'stage': 0x5ca8e8, 'dirty': 0x5ca8ec, 'tutorial': 0x5ca8e4}
# 상태 직렬화는 native 필드 폭/정렬 순서이며 double/float는 비트 그대로 기록한다.
FIELDS = [('stage', 0, 4), ('dirty', 0, 4), ('tutorial', 0, 4), ('demo', 0x68, 4),
    ('start', 0x70, 8), ('next', 0x78, 8), ('delay', 0x80, 4), ('lock', 0x84, 4),
    ('started', 0x88, 4), ('demoend', 0x90, 8), ('timer', 0x98, 8), ('tellat', 0xa8, 8),
    ('owned', 0xb8, 4), ('clear', 0xc8, 4)]
BOUNDARIES = {0x4c36c0: 'O', 0x460de0: 'Q', 0x4c8e90: 'D', 0x445330: 'K', 0x4c79a0: 'G',
    0x4641d0: 'B', 0x4c2a90: 'S', 0x4c3470: 'L', 0x452f90: 'E', 0x460df0: 'P', 0x460e30: 'R'}
TUTORIALS = [0x4c3a20, 0x4c3bb0, 0x4c3f00, 0x4c40f0, 0x4c42b0, 0x4c45e0]
SAVED = {UC_X86_REG_EBX: 0x12345678, UC_X86_REG_ESI: 0x23456789, UC_X86_REG_EDI: 0x3456789a, UC_X86_REG_EBP: 0x456789ab}
FIXTURE = ROOT / 'cpppj/tests/fixtures/missionframe-x86.tsv'
REPORT = ROOT / 'cpppj/missionframe.json'
FOLDER = ROOT / 'extracted/missionframe'


def digest(path):
    """근거 파일의 정확한 내용 SHA-256을 구한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value, width=8):
    """명시 실수 입력을 원래 IEEE 비트로 보존한다."""
    return int.from_bytes(struct.pack('<d' if width == 8 else '<f', value), 'little')


def state_text(state):
    """상태의 실제14필드 비트를 같은 순서의 쉼표 목록으로 기록한다."""
    return ','.join(str(state[key] & ((1 << (width * 8)) - 1)) for key, _, width in FIELDS)


def base_case(op='Frame'):
    """예상 결과를 계산하지 않고 현재 상태와 외부 조회 입력을 준비한다."""
    return dict(op=op, event=0x12345678, now=bits(10.0), depth=0, dialogs=1, knowledge=0, guide=0, blocking=0, mutation=0,
        state=dict(stage=65, dirty=7, tutorial=0, demo=0, start=bits(2.0), next=bits(0.0), delay=bits(0.5, 4),
            lock=0, started=1, demoend=bits(0.0), timer=bits(8.25), tellat=bits(0.0), owned=0, clear=0))


def cases():
    """기한 포함/초과·잠금·정지·7프레임·호출 순서/변경 뒤 재조회 입력을 생성한다."""
    result = []
    # started가0이면 다른 효과가 없으며 미래 기한은 정지 여부를 추가 조회한다.
    for started, lock, future, depth, dialogs in itertools.product((0, 1), (-1, 0, 1, 2, 10), (0, 1), (0, 2), (0, 1)):
        case = base_case();case.update(depth=depth, dialogs=dialogs)
        case['state'].update(started=started, lock=lock, next=bits(10.5 if future else 10.0));result.append(case)
    # signed counter의경계와 세 UI 조회의 short circuit을 독립 입력한다.
    for clear, guide, dialogs, blocking, owned in itertools.product((-1, 0, 5, 6, 7, 0x7fffffff), (0, 1), (0, 1), (0, 1), (0, 3)):
        case = base_case();case.update(depth=2, guide=guide, dialogs=dialogs, blocking=blocking)
        case['state'].update(owned=owned, clear=clear);result.append(case)
    # tellAt은 경과와 같은 시각에는 실행하지 않으며 기한0은 없음이다.
    for deadline, knowledge, dialogs in itertools.product((0.0, math.nextafter(8.0, -math.inf), 8.0, math.nextafter(8.0, math.inf), -1.0), (0, 1), (0, 1)):
        case = base_case();case.update(knowledge=knowledge, dialogs=dialogs);case['state']['tellat']=bits(deadline);result.append(case)
    # demo 기한은 같을 때 실행하며 callback 뒤0을 최종 기록한다.
    for demo, deadline in itertools.product((0, 1, 3), (0.0, 9.5, 10.0, 10.5)):
        case = base_case();case['state'].update(demo=demo, demoend=bits(deadline));result.append(case)
    # tutorial switch는1~6만 분배하며 lock의 음수는0과 다르다.
    for tutorial, demo, lock in itertools.product((0, 1, 2, 3, 4, 5, 6, 7, 0xffffffff), (0, 1), (-1, 0, 1)):
        case = base_case();case['state'].update(tutorial=tutorial, demo=demo, lock=lock);result.append(case)
    # 단계 char 감김과 double현재시각의 float축소 오류를 검출한다.
    for stage, lock, depth, now in itertools.product((65, 127, 255, 0xffffffff), (-1, 0, 1), (0, 2), (1.0, 16777216.25)):
        case=base_case('Advance');case.update(depth=depth, now=bits(now));case['state'].update(stage=stage, lock=lock);result.append(case)
    # 다른 정지 보유자와 미션 소유 표식의 독립 조합이다.
    for depth, owned in itertools.product((0, 1, 4), (0, 1, 7)):
        case=base_case('Pause');case.update(depth=depth);case['state']['owned']=owned;result.append(case)
    # 최초0기한/이미 예약된기한과 고정밀 게임시각의 재지연이다.
    for deadline, now in itertools.product((0.0, 1.0, -1.0), (1.0, 16777216.25)):
        case=base_case('Postpone');case['now']=bits(now);case['state']['next']=bits(deadline);result.append(case)
    # 각 효과의 첫 호출이 공유 상태를 바꾸는 명시 재진입 입력이다.
    for mutation in range(1, 7):
        case=base_case();case['mutation']=mutation
        case['state'].update(lock=1, tutorial=1, tellat=bits(1.0), owned=3, clear=6, demoend=bits(5.0))
        if mutation==6:case['state']['demo']=3
        result.append(case)
    unique = {}  # 같은 명시 입력은 원본 관찰 수에 중복 합산하지 않는다.
    for case in result:unique.setdefault(json.dumps(case, sort_keys=True), case)
    return list(unique.values())


def input_columns(case):
    """실제 원본 실행 전에 입력 열만 고정한다."""
    return [case['op'], case['event'], case['now'], case['depth'], case['dialogs'], case['knowledge'], case['guide'],
        case['blocking'], case['mutation'], state_text(case['state'])]


class Oracle:
    """원본 네 함수만 허용하며 외부 하위는 명시된 ABI 경계로 관찰한다."""
    def __init__(self):
        """보호 바이트를 가상 메모리에 복사하고 허용 명령/쓰기 검사를 등록한다."""
        if digest(BINARY) != SHA:raise RuntimeError('보호 PE SHA-256 불일치')
        pe=pefile.PE(str(BINARY));self.base=pe.OPTIONAL_HEADER.ImageBase;self.data=pe.get_memory_mapped_image()
        self.mu=Uc(UC_ARCH_X86, UC_MODE_32);self.mu.mem_map(self.base,(len(self.data)+4095)&~4095);self.mu.mem_write(self.base,self.data)
        # 입력 객체/사건/반환 표식/스택은 원본 PE와 분리한다.
        for address, size in ((MISSION, 0x2000), (STOP, 0x1000), (STACK_BASE, 0x10000)):self.mu.mem_map(address,size)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.native=collections.Counter();self.boundaries=collections.Counter();self.returns=0

    def put(self, address, value, width=4):
        """명시 입력/외부 효과의 원본 폭을 가상 메모리에 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def get(self, address, width=4):
        """현재 원본 필드/전역 비트를 little endian으로 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def state(self):
        """현재 실제 객체/전역의14필드를 읽으며 기대 상태를 계산하지 않는다."""
        return {key:self.get(GLOBALS.get(key,MISSION+offset),width) for key,offset,width in FIELDS}

    def mutate(self, kind):
        """첫 선택 callback에서 정해진 명시 입력 변화만 수행한다."""
        if kind in self.changed:return
        self.changed.add(kind);mutation=self.case['mutation']
        if mutation==1 and kind=='O':self.put(MISSION+0x84,0);self.put(GLOBALS['tutorial'],2)
        elif mutation==2 and kind=='S':self.put(MISSION+0xb8,1);self.depth=(self.depth+1)&0xffffffff;self.case['dialogs']=0
        elif mutation==3 and kind=='L':self.put(MISSION+0x84,0);self.put(MISSION+0x68,3)
        elif mutation==4 and kind=='D':self.put(MISSION+0x84,2)
        elif mutation==5 and kind=='G':self.put(MISSION+0xb8,0)
        elif mutation==6 and kind=='E':self.put(MISSION+0x90,bits(100.0),8);self.put(MISSION+0x68,0)

    def instruction(self, mu, address, size, data):
        """미허용 native 진입을 차단하고 실제 외부 callback 반환 규약을 보존한다."""
        if address==STOP:return
        operation=next((key for key,value in ENTRIES.items() if value==address),None)
        if operation:self.native[operation]+=1
        kind=BOUNDARIES.get(address)
        if address in TUTORIALS:kind='T'+str(TUTORIALS.index(address)+1)
        if kind is None:
            if not any(low<=address<high for low,high in RANGES):raise RuntimeError(f'미허용 native 실행 {address:08x}')
            return
        self.boundaries[kind]+=1;self.trace.append(kind);self.mutate(kind)
        esp=mu.reg_read(UC_X86_REG_ESP);ret=self.get(esp);cleanup=0;result=0
        if kind=='Q':result=int(bool(self.depth))
        elif kind=='D':result=self.case['dialogs']
        elif kind=='K':result=self.case['knowledge']
        elif kind=='G':result=self.case['guide']
        elif kind=='B':result=self.case['blocking']
        elif kind=='P':self.depth=(self.depth+1)&0xffffffff
        elif kind=='R':self.depth=(self.depth-1)&0xffffffff
        elif kind.startswith('T'):
            number=int(kind[1:]);pointer=self.get(esp+4)
            if pointer!=EVENT or self.get(pointer)!=self.case['event']:raise RuntimeError('Tutorial 현재 사건 포인터 불일치')
            result=0xa000+number;cleanup=4
        if kind in ('O','S','L') and mu.reg_read(UC_X86_REG_ECX)!=MISSION:raise RuntimeError('미션 this 포인터 불일치')
        if kind=='E' and (self.get(esp+8) or self.get(esp+12)):raise RuntimeError('데모명령 실제 인자 불일치')
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);mu.reg_write(UC_X86_REG_EIP,ret)

    def write(self, mu, access, address, size, value, data):
        """실제 몸체의 입력 객체/단계 전역/스택 밖 쓰기를 거부한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x10000:return
        if MISSION<=address and address+size<=MISSION+0x190:return
        if address in GLOBALS.values() and size==4:return
        raise RuntimeError(f'미허용 native 쓰기 {address:08x}+{size}')

    def run(self, case):
        """원본 정상 반환·스택·보존 레지스터·x87를 확인하고 결과를 그대로 관찰한다."""
        self.case=json.loads(json.dumps(case));self.trace=[];self.changed=set();self.depth=case['depth'];mu=self.mu
        mu.mem_write(MISSION,bytes(0x2000))
        # 모든 상태 필드를 native객체/전역의 실제 폭으로 쓴다.
        for key,offset,width in FIELDS:self.put(GLOBALS.get(key,MISSION+offset),case['state'][key],width)
        self.put(0x55b4d0,case['now'],8);self.put(EVENT,case['event']);self.put(0x542928,0x101)
        for reg,value in SAVED.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_ECX,MISSION)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0)
        args=[STOP];after=STACK+4
        if case['op'] in ('Frame','Advance'):args.append(EVENT if case['op']=='Frame' else 0);after+=4
        mu.mem_write(STACK,struct.pack('<'+'I'*len(args),*args));mu.emu_start(ENTRIES[case['op']],STOP,count=5000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=after:raise RuntimeError('정상 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in SAVED.items()):raise RuntimeError('보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=0x027f or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('x87 제어어/정상 스택 오류')
        self.returns+=1;result=mu.reg_read(UC_X86_REG_EAX) if case['op']=='Frame' else 0
        return [state_text(self.state()),self.depth,','.join(self.trace),result]


def generate():
    """C++를 읽거나 실행하지 않고 실제 기계어 관찰과 근거를 저장한다."""
    oracle=Oracle();rows=[]
    for case in cases():rows.append([*input_columns(case),*oracle.run(case)])
    FIXTURE.write_text('# 10.78 공통 미션 프레임 실제 PE 독립 관찰. GUI/OS 실행 없음.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    FOLDER.mkdir(parents=True,exist_ok=True);assembly=[];decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    for low,high in RANGES:
        for item in decoder.disasm(oracle.data[low-oracle.base:high-oracle.base],low):assembly.append(f'{item.address:08x} {item.bytes.hex():<24} {item.mnemonic} {item.op_str}')
    assembly_path=FOLDER/'assembly.txt';assembly_path.write_text('\n'.join(assembly)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),BINARY,FIXTURE,assembly_path,ROOT/'extracted/refined/originals/Netstorm.c']
    report=dict(host=socket.gethostname(),date='2026-10-15',edition='10.78',total=len(rows),returns=oracle.returns,
        native=dict(oracle.native),boundaries=dict(oracle.boundaries),os_calls=0,functions=ENTRIES,
        files={path.relative_to(ROOT).as_posix():digest(path) for path in paths},
        limits=['승패/Tell/list/demo/Tutorial1~6/시계 깊이/UI 조회는 명시 외부 경계이며 실제 게임 실행이 아니다.',
            '프레임 분기/필드/10·7카운터/float지연+double시각/단계char/기한·순서·반환은 실제 원본 바이트다.',
            'PATCH 전용이며 CD 대응·실제 미션 생성/Ready/login·UI 등록/완주/진행 저장은 검증하지 않는다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('미션 공통 프레임 실제 PE',len(rows),'정상 반환',dict(oracle.native),flush=True)


def verify():
    """저장된 원본 근거의 해시/입력 계획/정상 반환 수를 재계산 없이 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for path,sha in report['files'].items():
        if digest(ROOT/path)!=sha:raise RuntimeError('근거 SHA 불일치 '+path)
    plan=cases()
    if len(rows)!=len(plan) or report['returns']!=len(rows) or report['total']!=len(rows) or report['os_calls']:raise RuntimeError('실행/입력 합계 불일치')
    for row,case in zip(rows,plan):
        if len(row)!=14 or row[:10]!=list(map(str,input_columns(case))):raise RuntimeError('원본 입력 계획 불일치')
    if report['native']!=dict(collections.Counter(case['op'] for case in plan)):raise RuntimeError('실제 원본 함수 진입 수 불일치')
    print('미션 공통 프레임 독립 근거 감사 통과',len(rows),flush=True)


def main():
    """요청에 따라 새 기계어 관찰 생성 또는 기존 근거 감사를 수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate()


if __name__=='__main__':main()
