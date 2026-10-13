#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE 미션 승패/사제 상태/edge latch를 GUI 없이 격리 실행한다.

실제 조건/목록/최대HP/gate/잠금은 PE 몸체다. 서식·영역소유/UI/스크립트만 명시 경계다.
python -X utf8 tools/decomp_missionoutcome_oracle.py [--verify]
"""
import argparse
import collections
import copy
import csv
import hashlib
import itertools
import json
import random
import struct
import sys
from pathlib import Path

# 원본/출력 경로와 분석 전용 Python 라이브러리 경로다.
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'extracted/oracle-python'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 가상 입력 메모리는 실제 호스트 주소/게임 객체와 무관하다.
POOL, TYPES, PLAYERS, LIST, MISSION, VTABLE = 0x11000000, 0x11100000, 0x11200000, 0x11300000, 0x11400000, 0x11500000
STOP, CALLBACK, NAME, STACK_BASE, STACK = 0x20000000, 0x20000100, 0x20000200, 0x30000000, 0x3003f000
CAPACITY = 128
FIXTURE = ROOT / 'cpppj/tests/fixtures/missionoutcome-x86.tsv'
REPORT = ROOT / 'cpppj/missionoutcome.json'
SPEC = {
    'originals': dict(binary='originals/Netstorm.exe', stride=50, type_stride=500, player_stride=180, temple=124,
        check=0x4c36c0, edge=0x4c3940, dead=0x4922e0, captured=0x492340, lookup=0x4921f0, hp=0x4adcb0,
        types=0x59ab20, pool=0x5c8464, players=0x59534c, roster=0x5954c4, count=0x5954cc, status=0x595428,
        local=0x540c70, allies=0x595200, blocked=0x565dd4, format=0x4a89c0, region=0x46f770,
        section=0x482de0, close=0x466710, assert_report=0x4e0620, owner=34),
    'originalCD': dict(binary='originalCD/NETSTORM.EXE', stride=36, type_stride=468, player_stride=172, temple=116,
        check=0x410380, edge=0x410a90, dead=0x40da20, captured=0x40da70, lookup=0x40d940, hp=0x4aed30,
        types=0x51c960, pool=0x5395dc, players=0x50f834, roster=0x549188, count=0x549190, status=0x5119a0,
        local=0x50f6c8, allies=0x50f6e0, blocked=0x535e8c, format=0x420560, region=0x4bde10,
        section=0x480de0, close=0x492c20, owner=32),
}
# 함수 전후 보존해야 하는 비휘발 레지스터다.
SAVED = {UC_X86_REG_EBX:0x12345678, UC_X86_REG_ESI:0x23456789, UC_X86_REG_EDI:0x3456789a, UC_X86_REG_EBP:0x456789ab}


def digest(path):
    """근거 파일의 내용 SHA-256을 반환한다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def text_values(values):
    """DWORD 입력/관찰을 같은 순서의 쉼표 목록으로 저장한다."""
    return ','.join(map(str, values))


def base_case():
    """기대 결과 없이 두 활성 플레이어와 명시 raw 객체 입력을 만든다."""
    allies = [0] * 81
    # 자기 동맹도 원본 표에 명시 입력한다.
    for player in range(9):
        allies[player * 9 + player] = 1
    return dict(op='Check', local=1, demo=0, blocked=0, found=1, mutation=0, region=1, argument=1,
        flags=[0,34,34,0,0,0,0,0,0], temples=[0,50,52,0,0,0,0,0,0], statuses=[0]*9,
        allies=allies, latches=[0]*43, roster=[51,53],
        nodes=[(50,100,0,1,49,100,0,0),(51,100,4,1,100,100,1095237632,1101135872),
               (52,100,0,2,100,100,0,0),(53,100,0,2,100,100,1102053376,1109393408)])


def cases():
    """쿼리·latch·gate·HP경계·팀/목록·효과 뒤 재조회 입력만 생성한다."""
    rows = []
    # inactive, captured, dead 및 상태의 비정상 다른 값을 별도로 둔다.
    for op, player, flags, status in itertools.product(('Dead','Captured'), (0,1,8,9,0xffffffff), (0,2,32,128), (0,1,2,3)):
        case = base_case();case.update(op=op, argument=player)
        case['flags'][1 if player>8 else player] = flags
        case['statuses'][1 if player>8 else player] = status
        rows.append(case)
    # 한 사건의 조건과 최초 DWORD 잠금/gate/섹션 유무를 교차한다.
    for condition, latch, blocked, found, demo in itertools.product((0,1),(0,5),(0,2),(0,1),(0,1)):
        case = base_case();case.update(op='Event',argument=condition,blocked=blocked,found=found,demo=demo)
        case['latches'][40] = latch;rows.append(case)
    # HP가 절반과 같을 때, free/void/dead bit의 독립 조건, 사제 owner 상태를 검사한다.
    for hp, state, status in itertools.product((-1,0,49,50,51,32767), (0,1,2,4), (0,1,2)):
        case = base_case();node = list(case['nodes'][0]);node[2]=state;node[4]=hp;case['nodes'][0]=tuple(node)
        case['statuses'][2]=status;rows.append(case)
    # 비대칭/상호동맹과 현재 로컬 플레이어, 섹션이 없는 경우/막힌 경우도 실제 전체 몸체로 관찰한다.
    for local, a, b, blocked, found, demo in itertools.product((1,2),(0,7),(0,9),(0,1),(0,1),(0,1)):
        case = base_case();case.update(local=local,blocked=blocked,found=found,demo=demo)
        case['allies'][11]=a;case['allies'][19]=b;rows.append(case)
    # 콜백이 바꾼 현재 입력은 native 몸체가 다시 조회한다.
    for mutation in range(1,5):
        case=base_case();case['mutation']=mutation;rows.append(case)
    # 고정된 입력 seed로 여덟 플레이어/정확한 활성 mask/다양한 초기 잠금을 함께 준비한다.
    rng = random.Random(0x4c36c0)
    for _ in range(32):
        case=base_case();case['flags']=[rng.choice((0,1,2,32,34,255)) for _ in range(9)]
        case['statuses']=[rng.randrange(4) for _ in range(9)]
        case['allies']=[rng.choice((0,1,0xffffffff)) for _ in range(81)]
        case['latches']=[rng.choice((0,0,0,3)) for _ in range(43)]
        case['local']=rng.randrange(1,9);case['region']=rng.randrange(9);rows.append(case)
    return rows


def inputs(case, edition):
    """호스트 float 계산을 섞지 않고 raw 노드의 각 명시 입력을 저장한다."""
    return [edition,case['op'],case['local'],case['demo'],case['blocked'],case['found'],case['mutation'],case['region'],case['argument'],
        text_values(case['flags']),text_values(case['temples']),text_values(case['statuses']),text_values(case['allies']),
        text_values(case['latches']),text_values(case['roster']),';'.join(text_values(node) for node in case['nodes'])]


class OutcomeOracle:
    """읽기 전용 PE의 내보낸 함수와 명시된 외부 경계만 실행한다."""
    def __init__(self, edition):
        """새 VM과 함수/쓰기 허용 범위를 만들고 원본 파일은 변경하지 않는다."""
        self.edition=edition;self.s=SPEC[edition];p=pefile.PE(str(ROOT/self.s['binary']))
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);mapped=p.get_memory_mapped_image()
        self.mu.mem_map(p.OPTIONAL_HEADER.ImageBase,(len(mapped)+4095)&~4095);self.mu.mem_write(p.OPTIONAL_HEADER.ImageBase,mapped)
        # 합성 입력 공간은 서로 겹치지 않는다.
        for address,size in ((POOL,0x10000),(TYPES,0x20000),(PLAYERS,0x1000),(LIST,0x1000),(MISSION,0x1000),(VTABLE,0x1000),(STOP,0x1000),(STACK_BASE,0x40000)):
            self.mu.mem_map(address,size)
        self.facts=[];self.allowed=[];self.native=collections.Counter();self.boundaries=collections.Counter();self.returns=0
        self.exports=[ROOT/f'extracted/missionoutcome/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.exports[1].open(encoding='utf-8') as fp:
            # 불연속 실제 함수 범위를 각각 보존한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.facts.append(row)
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def get(self,address,width=4):
        """원본 little endian 전역/필드를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시 입력/외부 효과만 가상 메모리에 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def string(self,address):
        """원본 문자열의 유한 NUL 종료를 읽는다."""
        value=bytes(self.mu.mem_read(address,256));return value.split(b'\0')[0].decode('ascii')

    def snapshot(self):
        """현재 상태와 43개 잠금을 효과 순서와 함께 기록한다."""
        s=self.s
        flags=[self.get(PLAYERS+i*s['player_stride']+80,1) for i in range(9)]
        temples=[self.get(PLAYERS+i*s['player_stride']+s['temple']) for i in range(9)]
        statuses=[self.get(s['status']+i*4) for i in range(9)]
        latches=[self.get(MISSION+0xe0+i*4) for i in range(43)]
        return '/'.join((text_values([self.get(s['local']),self.get(MISSION+104),self.get(s['blocked'])]),
            text_values(flags),text_values(temples),text_values(statuses),text_values(latches)))

    def mutate(self,kind):
        """정해 둔 외부 효과 첫 호출만 현재 공유 입력을 바꾼다. 기대값은 계산하지 않는다."""
        if kind in self.mutated:return
        self.mutated.add(kind);s=self.s;m=self.case['mutation']
        if kind=='section' and m==1:
            self.put(s['local'],2);self.put(PLAYERS+2*s['player_stride']+80,0,1)
            self.put(s['status']+4,2);self.put(PLAYERS+s['player_stride']+s['temple'],0)
        elif kind=='close' and m==2:
            self.put(s['blocked'],1);self.put(MISSION+104,1);self.put(s['status']+4,1)
        elif kind=='stop' and m==3:
            self.put(MISSION+0xe4,7);self.put(s['status']+8,2)
        elif kind=='region' and m==4:
            self.put(s['local'],2)

    def instruction(self,mu,address,size,_):
        """OS/IAT/미내보내기 실행을 차단하고 명시 경계의 ABI만 대체한다."""
        s=self.s;self.instructions+=1
        if address==STOP:return
        # 실제 몸체 진입을 separately 집계한다.
        for name in ('check','edge','dead','captured','lookup','hp'):
            if address==s[name]:self.native[name]+=1
        kind=next((key for key in ('format','region','section','close') if address==s[key]),None)
        if address==CALLBACK:kind='stop'
        if address==s.get('assert_report'):kind='assert'
        if kind is None:
            if not any(low<=address<high for low,high in self.allowed):raise RuntimeError(f'미허용 native 실행 {address:08x}')
            return
        self.boundaries[kind]+=1;esp=mu.reg_read(UC_X86_REG_ESP);ret,a,b,c=struct.unpack('<4I',mu.mem_read(esp,16));cleanup=0
        if kind=='format':
            pattern=self.string(b);value=(pattern%c).encode('ascii')+b'\0';mu.mem_write(a,value);mu.reg_write(UC_X86_REG_EAX,a)
        elif kind=='section':
            self.effects.append('section:'+self.string(a)+'#'+self.snapshot());self.mutate(kind)
            mu.reg_write(UC_X86_REG_EAX,self.case['found']);cleanup=4
        elif kind=='region':
            self.effects.append(f'region:{a},{b}#'+self.snapshot());self.mutate(kind)
            mu.reg_write(UC_X86_REG_EAX,self.case['region']);cleanup=8
        elif kind=='assert':
            self.effects.append('assert:'+str(self.case['argument'])+'#'+self.snapshot())
        else:
            self.effects.append(kind+'#'+self.snapshot());self.mutate(kind)
        mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup);mu.reg_write(UC_X86_REG_EIP,ret)

    def write(self,mu,access,address,size,value,_):
        """실제 함수의 쓰기는 스택/미션 잠금 영역만 허용한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if MISSION+0xe0<=address and address+size<=MISSION+0x18c:return
        raise RuntimeError(f'미허용 native 쓰기 {address:08x}/{size}')

    def run(self,case):
        """각 사례를 독립 입력 상태에서 정상 반환까지 실행한다."""
        s=self.s;mu=self.mu;self.case=case;self.effects=[];self.mutated=set();self.instructions=0
        # 전역 포인터/배열은 실제 원본 함수가 읽는 주소에 배치한다.
        for key,value in (('pool',POOL),('types',TYPES),('players',PLAYERS),('roster',LIST),('count',len(case['roster'])),('local',case['local']),('blocked',case['blocked'])):
            self.put(s[key],value)
        mu.mem_write(POOL,bytes(CAPACITY*s['stride']));mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(PLAYERS,bytes(0x1000));mu.mem_write(MISSION,bytes(0x1000))
        self.put(MISSION,VTABLE);self.put(VTABLE+40,CALLBACK);self.put(MISSION+104,case['demo'])
        # 테스트 Type는 최대 HP만 입력한다. 전체 최대 HP 몸체는 실제로 실행한다.
        for sid,typ,state,owner,hp,maximum,x,y in case['nodes']:
            base=POOL+sid*s['stride'];self.put(base+10,typ,1);self.put(base+11,state,1);self.put(base+s['owner'],owner,1)
            self.put(base+26,hp,4 if self.edition=='originals' else 2);self.put(base+14,x);self.put(base+18,y)
            self.put(TYPES+typ*s['type_stride'],maximum);self.put(TYPES+typ*s['type_stride']+232,16)
        # 활성 플래그/신전/사제 상태/동맹/잠금은 입력 순서를 그대로 보존한다.
        for i in range(9):
            self.put(PLAYERS+i*s['player_stride']+80,case['flags'][i],1);self.put(PLAYERS+i*s['player_stride']+s['temple'],case['temples'][i])
            self.put(s['status']+i*4,case['statuses'][i])
        for i,value in enumerate(case['allies']):self.put(s['allies']+i*4,value)
        for i,value in enumerate(case['latches']):self.put(MISSION+0xe0+i*4,value)
        for i,value in enumerate(case['roster']):self.put(LIST+i*4,value)
        mu.mem_write(NAME,b'BadTeamDead\0')
        # debug HP 모드는 이번 합성 입력에서 꺼져 있다. 원본 readonly ROM과 필드 몸체는 보존한다.
        if self.edition=='originals':
            self.put(0x5e4794,0);self.put(0x5ca8f0,0)
        else:self.put(0x511c90,0)
        for reg,value in SAVED.items():mu.reg_write(reg,value)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ECX,MISSION);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
        args=[STOP];entry=s['check'];after=STACK+4
        if case['op'] in ('Dead','Captured'):
            args.append(case['argument']);entry=s['dead' if case['op']=='Dead' else 'captured']
        elif case['op']=='Event':
            args.extend((MISSION+0x180,case['argument'],NAME));entry=s['edge'];after=STACK+16
        mu.mem_write(STACK,struct.pack(f'<{len(args)}I',*args));mu.emu_start(entry,STOP,count=100000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=after:raise RuntimeError('미션 승패 정상 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in SAVED.items()):raise RuntimeError('미션 승패 보존 레지스터 오류')
        self.returns+=1;result=0 if case['op']=='Check' else mu.reg_read(UC_X86_REG_EAX)&255
        return [self.snapshot(),'|'.join(self.effects),result]


def generate(smoke=False):
    """두 실제 PE 출력과 정상 반환/실행 사실을 저장한다. C++는 읽거나 호출하지 않는다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/ghidra/missionoutcome-functions.json',FIXTURE}
    for edition in SPEC:
        oracle=OutcomeOracle(edition);plan=cases()[::37] if smoke else cases()
        for case in plan:rows.append([*inputs(case,edition),*oracle.run(case)])
        editions[edition]=dict(cases=len(plan),returns=oracle.returns,native=dict(oracle.native),boundaries=dict(oracle.boundaries),functions=oracle.facts)
        paths.update(oracle.exports);paths.add(ROOT/oracle.s['binary']);print(edition,len(plan),'정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# 두 실제 PE의 미션 조건·edge latch 독립 관찰. 게임/GUI/OS 미실행.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='VM-W11-CODEX',date='2026-10-13',os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},
        limits=['서식·영역 소유자·스크립트/UI·패치 범위 assert는 명시 외부 경계다. 전체 게임 실행 대조가 아니다.',
                '승패 조건/사제 현재 목록/ownerState/최대HP/gate/잠금·정상 반환·보존 레지스터·허용 쓰기는 실제 PE다.',
                '합성 callback mutation이 후속 재조회 및 latch 최종 쓰기를 검사한다. 실제 GUI/공간/전체 월드 연결은 별도다.']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('total',len(rows),flush=True)


def verify():
    """기대값을 재계산하지 않고 저장 근거/실행 수/입력 계획을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    for path,sha in report['files'].items():
        if digest(ROOT/path)!=sha:raise RuntimeError('근거 SHA 불일치: '+path)
    if report['total']!=len(rows) or report['os_calls']:raise RuntimeError('미션 근거 합계 오류')
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];plan=cases()
        if len(chosen)!=len(plan) or item['returns']!=len(plan) or item['cases']!=len(plan):raise RuntimeError('미션 실행 수 오류')
        for row,case in zip(chosen,plan):
            if len(row)!=19 or row[:16]!=list(map(str,inputs(case,edition))):raise RuntimeError('미션 명시 입력 계획 오류')
        if item['native']['check']!=sum(case['op']=='Check' for case in plan):raise RuntimeError('미션 native 루트 수 오류')
    print('미션 승패 독립 근거 감사 통과:',len(rows),flush=True)


def main():
    """생성/짧은 실제 PE smoke/저장 감사 중 요청한 동작 하나를 수행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
