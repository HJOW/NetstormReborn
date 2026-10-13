#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 두 PE의 Player1..8 AI 포인터 순회와 첫 미복원 하위 요청 경계를 관찰한다."""
import argparse
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import ROOT,STOP,STACK,STACK_BASE,digest,pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 새 Player 전체 9기록의 독립 주소이며 가이저/AI 힙을 만들지 않는다.
PLAYERS=0x11c00000
# x87 반복은 독립 입력으로 다시 세지 않는다. 이 정수 몸체의 상태 불변도 확인한다.
CONTROLS=(0x027f,0x037f)
# 현재 원본 포인터·stride·AI 필드·첫 미복원 요청 주소다. CD는 직접 inlined 분기다.
PLAN={
    'originals':dict(binary='originals/Netstorm.exe',entry=0x416720,players=0x59534c,stride=0xb4,ai=0xb0,boundary=0x4153c0),
    'originalCD':dict(binary='originalCD/NETSTORM.EXE',entry=0x4d8670,players=0x50f834,stride=0xac,ai=0xa8,boundary=0x4d86a0)}
# 이번 좁은 등록 관찰은 기존 가이저 채집/AssetPostPop 완료 자료와 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/geyser-ai-registration-x86.tsv'
REPORT=ROOT/'cpppj/geyser-ai-registration.json'


def inputs():
    """SID/state/Player0 무시와 각 현재 nonnull AI 위치를 명시 입력으로 교차한다."""
    cases=[]
    # null AI가 원본 SID/state를 검사하지 않고 자연 반환하는 범위를 만든다.
    for sid,state,ignored in itertools.product((0,1,50,122,32768,65535,0xffffffff),(0,1,2,0xffffffff),(0,0x12345678,0xffffffff)):
        cases.append((sid,state,ignored,0,0,0))
    # nonnull 하위가 필요한 입력은 완료/정상/assert 수에 합산하지 않는 명시 거부다.
    for owner,sid,state,pointer in itertools.product(range(1,9),(0,50,0xffffffff),(0,1,0xffffffff),(1,0x12345678,0xffffffff)):
        cases.append((sid,state,0xffffffff,owner,pointer,0))
    # 여러 현재 포인터가 있으면 첫 번호에서 하위 요청을 중단한다.
    cases.extend(((50,1,123,3,0x12345678,1),(50,0,123,8,0xffffffff,1)))
    return cases


def player_bytes(spec,case):
    """전체 padding을 A5로 보존하고 현재 AI 필드만 입력 DWORD로 쓴다."""
    sid,state,ignored,owner,pointer,multiple=case
    raw=bytearray(b'\xa5'*(9*spec['stride']))
    # 모든 실제 owner의 현재 AI 포인터를 별도로 초기화한다.
    for index in range(9):struct.pack_into('<I',raw,index*spec['stride']+spec['ai'],0)
    struct.pack_into('<I',raw,spec['ai'],ignored)
    if owner:
        struct.pack_into('<I',raw,owner*spec['stride']+spec['ai'],pointer)
        if multiple:
            # 뒤 번호의 값은 첫 요청 전에는 읽지 않아야 한다. owner8 입력은 앞 owner2도 공급한다.
            other=7 if owner==3 else 2
            struct.pack_into('<I',raw,other*spec['stride']+spec['ai'],0xfedcba98)
    return bytes(raw)


class RegistrationOracle:
    """새로 내보낸 등록 몸체만 실행하며 현재 Player AI 필드의 읽기 순서를 기록한다."""
    def __init__(self,edition):
        """원본 PE/범위와 읽기 전용 Player 전체기록 및 스택을 독립 메모리에 준비한다."""
        self.edition=edition;self.p=PLAN[edition]
        pe=pefile.PE(str(ROOT/self.p['binary']));self.base=pe.OPTIONAL_HEADER.ImageBase;blob=pe.get_memory_mapped_image()
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);self.mu.mem_map(self.base,(len(blob)+4095)&~4095);self.mu.mem_write(self.base,blob)
        # 호스트/OS 호출이나 실제 게임 프로세스를 실행하지 않는다.
        for address,length in ((PLAYERS,0x1000),(STOP,0x1000),(STACK_BASE,0x40000)):self.mu.mem_map(address,length)
        self.exports=[ROOT/f'extracted/geyser-ai-registration/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[]
        with self.exports[1].open(encoding='utf-8-sig') as stream:
            # callback은 의미 감사용이며 실제 등록 entry의 몸체만 실행 허용한다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16)!=self.p['entry']:continue
                for item in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in item.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.code);self.mu.hook_add(UC_HOOK_MEM_READ,self.read);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.returns=0;self.rejections=0

    def read(self,mu,access,address,size,value,data):
        """실제 실행이 읽는 현재 Player AI DWORD만 순회 번호로 기록한다."""
        relative=address-PLAYERS-self.p['ai']
        if size==4 and relative>=0 and relative%self.p['stride']==0 and relative//self.p['stride']<9:
            self.visits.append(relative//self.p['stride'])

    def write(self,mu,access,address,size,value,data):
        """원본 등록이 Player/게임 전역을 쓰지 않는 null 경로에서 스택 쓰기만 허용한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        raise AssertionError(f'등록의 예상 밖 쓰기 {self.edition}:{address:08x}+{size}')

    def code(self,mu,address,size,data):
        """첫 nonnull 하위 요청을 실행 전에 명시 중단하고 원본 현재 인자를 보존한다."""
        if address==self.p['boundary']:
            pointer=mu.reg_read(UC_X86_REG_ECX if self.edition=='originals' else UC_X86_REG_EDI)
            sp=mu.reg_read(UC_X86_REG_ESP)
            if self.edition=='originals':sid,state=struct.unpack('<II',mu.mem_read(sp+4,8))
            else:sid,state=self.case[:2]
            self.request=(self.visits[-1],pointer,sid,state);mu.emu_stop();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise AssertionError(f'등록 허용 범위 밖 실행 {self.edition}:{address:08x}')

    def run(self,case,control):
        """원본 정상 반환 또는 첫 거부 경계와 전체 Player 바이트 불변을 관찰한다."""
        self.case=case;self.visits=[];self.request=None;mu=self.mu;raw=player_bytes(self.p,case)
        mu.mem_write(PLAYERS,raw);mu.mem_write(self.p['players'],struct.pack('<I',PLAYERS))
        # 이전 호출의 비휘발성 레지스터나 x87 상태를 입력에 섞지 않는다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<III',STOP,*case[:2]));mu.emu_start(self.p['entry'],STOP,count=5000)
        after=bytes(mu.mem_read(PLAYERS,len(raw)));assert after==raw
        assert mu.reg_read(UC_X86_REG_FPCW)==control and mu.reg_read(UC_X86_REG_FPSW)==0
        if self.request:self.rejections+=1;kind='boundary';request=','.join(str(value) for value in self.request)
        else:
            assert mu.reg_read(UC_X86_REG_ESP)==STACK+4
            self.returns+=1;kind='normal';request='-'
        return [kind,','.join(str(value) for value in self.visits),request,str(zlib.adler32(after)&0xffffffff)]


def generate(smoke):
    """두 x87 관찰의 동일성을 확인하고 정상과 명시 거부 입력을 별도 기록한다."""
    rows=[];editions={};paths={Path(__file__).resolve(),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/ghidra/geyser-ai-registration-functions.json',
        ROOT/'cpppj/src/o/RawGeyserAiRegistration.h',ROOT/'cpppj/src/o/RawGeyserAiRegistration.cpp',ROOT/'cpppj/tests/GeyserAiRegistrationTests.cpp'}
    # 각 판본의 실제 stride와 실제 기계어를 사용한다.
    for edition in PLAN:
        oracle=RegistrationOracle(edition);cases=inputs()
        if smoke:cases=cases[:3]+cases[84:87]+cases[-2:]
        normal=rejected=0
        # 반복은 같은 입력의 반환 상태 검사이며 고유 입력에 합산하지 않는다.
        for case in cases:
            results=[oracle.run(case,control) for control in CONTROLS];assert results[0]==results[1],(edition,case)
            normal+=results[0][0]=='normal';rejected+=results[0][0]=='boundary';rows.append([edition,','.join(str(value) for value in case),*results[0]])
        editions[edition]=dict(unique=len(cases),normal_unique=normal,unrestored_boundary_unique=rejected,assert_unique=0,normal_returns=oracle.returns,boundary_stops=oracle.rejections)
        paths.update(oracle.exports);paths.add(ROOT/oracle.p['binary']);print(f'{edition}: 정상{normal}/미복원하위거부{rejected}',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition\tsid,state,ignoredPlayer0,owner,pointer,multiple\tclassification\tactualAiReadOrder\tfirstRequest\tfullPlayerAdler\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');paths.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-14',total=len(rows),normal_total=sum(item['normal_unique'] for item in editions.values()),
        unrestored_boundary_total=sum(item['unrestored_boundary_unique'] for item in editions.values()),assert_total=0,controls=list(CONTROLS),editions=editions,
        game_runs=0,os_calls=0,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},limitations=[
        '원본 null AI 등록 순회만 정상 완료 범위다. nonnull 첫 요청은 원본 하위 실행 전 중단하고 C++도 명시 거부한다.',
        '미복원 AI 하위 거부 입력은 정상/assert 또는 전체 AI·전체 PostPop 완료 수에 합산하지 않는다.',
        'Player 저장소는 유효한 nonnull 원본 base와 9개 실제 기록이라는 입력 계약이다. base 주소 감김/해방 저장소는 관찰 범위 밖이다.',
        'PATCH callback4153c0은 의미 감사용으로만 내보냈다. CD callback은 등록 부모 안에 inlined되어 있다.',
        '미션 상위 순서의 정적 감사는 이 작은 PE 호출의 실제 미션/GUI 실행 근거가 아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """관찰 출처 전체와 완료/거부 분류 및 fixture의 현재 바이트를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 원본/내보내기/도구/생산소스/fixture를 모두 바이트 단위로 확인한다.
    for path,sha in report['files'].items():assert digest(ROOT/path)==sha,path
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total']==604 and report['normal_total']==168 and report['unrestored_boundary_total']==436
    assert sum(row[2]=='normal' for row in rows)==168 and sum(row[2]=='boundary' for row in rows)==436
    print('가이저 AI 등록 정상168/하위명시거부436 및 전체 출처 SHA 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)
