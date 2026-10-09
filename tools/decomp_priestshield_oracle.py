#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""사제 보호막 전체 생성 wrapper·실제 lookup/finder/소유자 지정의 정상 반환을 대조한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import ROOT,SPECS,TYPES,POOL,LISTS,STACK,STOP,digest
from decomp_priestforcefield_oracle import ForcefieldOracle,CONTROLS,MAP,VTABLE,TARGET,bits
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 실제 wrapper/외부 경계/로컬 조건 전역과 공통 소유자 함수다.
SPEC={
 'originals':dict(entry=0x493d30,create=0x4af530,owner=0x4adf00,new=0x4e4391,load=0x4a8ee0,sound=0x4ab240,
    notice=0x4a9cb0,translate=0x4a89c0,tell=0x4cf960,local=0x540c70,loading=0x5c89b8,clock=0x5c85f0),
 'originalCD':dict(entry=0x40bfb0,create=0x4ab390,owner=0x4aefa0,new=0x4f1650,load=0x437d20,sound=0x453800,
    notice=0x438b50,translate=0,tell=0,local=0x50f6c8,loading=0x5178d4,clock=0x564db8),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 외부 효과의 입력 주소/번호이며 원본 메모리나 OS 주소가 아니다.
NEW=100
POP=STOP+0x400
MEMORY=LISTS+0x1000
SOUND=0x12345678
MESSAGE=b'notice-token\0'
FIXTURE=ROOT/'cpppj/tests/fixtures/priestshield-x86.tsv'
REPORT=ROOT/'cpppj/recovery-priestshield-evidence.json'
KEYS=('scene','owner','allocated','clock','loading','local','change')
# +0/-0·정상 시각·NaN의 실제 double 비트 입력이다.
CLOCKS=(0,0x8000000000000000,0x3ff0000000000000,0x7ff8000000001234)


def scene(case):
    """기대 검색 결과를 만들지 않고 현재 raw/등록 순서 입력만 공급한다."""
    index=case['scene'];x,y=(20.75,21.9) if index%2==0 else (254.9,254.25);owner=case['owner'];nodes=[]
    # 같은 타입/다른 소유자·다른 타입·매장·다른 위치·해시 단계 순서를 구별한다.
    if index:
        for i,sid in enumerate((60,61)):
            typ=166 if index==3 else 167;own=(owner+1)%9 if index==2 else owner
            extra=8 if index==4 else 0;ax=x-1 if index==6 else x
            nodes.append((sid,typ,(0,2,4,6)[(index+i)%4],extra,bits(ax),bits(y),1,1,own,(index+i)%4))
        if index==7:nodes.reverse()
    return (bits(x),bits(y),owner,(0,2,4,6)[index%4],(0,1,8,9)[index%4],168 if index==5 else 167,0,nodes)


def inputs():
    """분기 격자와 생성/소유자/Pop/소리 중 현재 값 변경 입력을 만든다."""
    rows=[]
    # 새 자산의 소유자는 공통 정상 계약인 0~8만 공급한다.
    for index,owner,allocated,clock,loading,match in itertools.product(range(8),(0,1,8),(0,1),range(4),(0,1),(0,1)):
        rows.append(dict(scene=index,owner=owner,allocated=allocated,clock=clock,loading=loading,local=owner if match else 256,change=0))
    # 생성 후 좌표 캡처·Pop/소리 이후 로컬 조건 조회의 시점을 관찰한다.
    for index,owner,allocated,change in itertools.product((0,2,3,4),(1,8),(0,1),(1,2,3,4)):
        rows.append(dict(scene=index,owner=owner,allocated=allocated,clock=0,loading=0,local=owner,change=change))
    return rows


class ShieldOracle(ForcefieldOracle):
    """lookup/owner를 실제 실행하고 생성/Pop·오디오/문구 하위 몸체만 대체한다."""
    def __init__(self,edition):
        """새 내보내기 범위와 SEH의 FS:[0]·가상 호출 표를 준비한다."""
        super().__init__(edition);self.p=SPEC[edition];self.mu.mem_map(0,0x1000);self.returns=0
        self.exports=[ROOT/f'extracted/priestshield/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.stub_calls=collections.Counter()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 원본 불연속 함수 몸체만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 범위 끝 주소는 포함한다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.mem_write(VTABLE+0x74,struct.pack('<I',self.p['owner']));self.mu.mem_write(VTABLE+0x90,struct.pack('<I',POP))

    def ret(self,result=0,purge=0):
        """외부 경계의 ABI 정리만 수행한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,result)
        self.mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',self.mu.mem_read(sp,4))[0]);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def string(self,address):
        """원본이 전달한 ASCII 파일명/번역 key를 읽는다."""
        return bytes(self.mu.mem_read(address,80)).split(b'\0',1)[0].decode('ascii')

    def change(self,stage):
        """외부 효과가 공급한 raw/전역 변경만 적용하며 wrapper의 판단을 계산하지 않는다."""
        if self.case['change']!=stage:return
        if stage==1:
            self.mu.mem_write(self.slot(TARGET)+14,struct.pack('<2I',0x7fc12345,0x80000000));self.mu.mem_write(self.slot(TARGET)+self.o['owner'],bytes(((self.case['owner']+1)%9,)))
        elif stage==2:self.mu.mem_write(self.slot(TARGET)+14,struct.pack('<2I',bits(99.5),bits(98.5)))
        elif stage==3:
            self.mu.mem_write(self.slot(TARGET)+self.o['owner'],b'\x08');self.mu.mem_write(self.p['local'],struct.pack('<I',8));self.mu.mem_write(self.p['loading'],bytes(4));self.mu.mem_write(self.p['clock'],bytes(8))
        elif stage==4:self.mu.mem_write(self.p['clock'],struct.pack('<Q',CLOCKS[3]))

    def on_instruction(self,mu,address,size,data):
        """경계 인자와 실제 owner 진입을 관찰하고 정상 ABI로 반환한다."""
        p=self.p;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==p['owner']:
            value=struct.unpack('<I',mu.mem_read(sp+4,4))[0]
            if ecx!=self.slot(NEW):raise RuntimeError('보호막 소유자 this 오류')
            self.events.append(f'O:{NEW}:{value}');self.change(2)
        if address not in (p['create'],POP,p['new'],p['load'],p['sound'],p['notice'],p['translate'],p['tell']):
            # 부모 ForcefieldOracle의 삭제용 경계는 이번 wrapper의 경계가 아니다.
            from decomp_owner_oracle import OwnerOracle
            OwnerOracle.on_instruction(self,mu,address,size,data);return
        if address==p['create']:
            typ,flags=struct.unpack('<2I',mu.mem_read(sp+4,8));self.events.append(f'C:{typ}:{flags}');marker='C'
            if typ!=scene(self.case)[5] or flags!=2:raise RuntimeError('보호막 생성 인자 오류')
            raw=bytearray(self.stride);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=4;mu.mem_write(self.slot(NEW),bytes(raw));self.change(1);self.ret(self.slot(NEW))
        elif address==POP:
            x,y,flags=struct.unpack('<3I',mu.mem_read(sp+4,12));self.events.append(f'P:{NEW}:{x}:{y}:{flags}');marker='P'
            if ecx!=self.slot(NEW) or flags:raise RuntimeError('보호막 Pop this/flags 오류')
            mu.mem_write(self.slot(NEW)+14,struct.pack('<2I',x,y));mu.mem_write(self.slot(NEW)+11,b'\0');self.change(3);self.ret(purge=12)
        elif address==p['new']:
            size=struct.unpack('<I',mu.mem_read(sp+4,4))[0]
            if size!=0x28:raise RuntimeError('소리 프로세스 확보 크기 오류')
            self.events.append(f'A:{size}:{self.case["allocated"]}');marker='A';self.ret(MEMORY if self.case['allocated'] else 0)
        elif address==p['load']:
            name,first,second=struct.unpack('<3I',mu.mem_read(sp+4,12));file=self.string(name)
            if (file,first,second)!=('priestForceField.wav',0,1):raise RuntimeError('보호막 소리 조회 인자 오류')
            # 파일명만 cdecl 인자다. 아래의 0/1은 다음 SfxProcess 생성자의 대기 인자다.
            self.events.append(f'L:{file}');marker='L';self.change(4);self.ret(SOUND)
        elif address==p['sound']:
            parent,sound,first,second=struct.unpack('<4I',mu.mem_read(sp+4,16))
            if ecx!=MEMORY or (parent,sound,first,second)!=(NEW,SOUND,0,1):raise RuntimeError('보호막 소리 프로세스 인자 오류')
            self.events.append(f'S:{parent}:{sound}:{first}:{second}');marker='S';self.ret(MEMORY,16)
        elif address==p['notice']:
            args=struct.unpack('<6I',mu.mem_read(sp+4,24));name=self.string(args[0])
            if name!='ourPriestImmobile.wav' or args[1:]!=(0,0,0,1,0):raise RuntimeError('로컬 사제 안내 소리 인자 오류')
            self.events.append('N:'+name+':'+':'.join(map(str,args[1:])));marker='N';self.ret()
        elif address==p['translate']:
            buffer,key=struct.unpack('<2I',mu.mem_read(sp+4,8));key=self.string(key)
            if key!='PriestImmobile':raise RuntimeError('사제 안내 번역 key 오류')
            mu.mem_write(buffer,MESSAGE);self.events.append('T:'+key);marker='T';self.ret(buffer)
        else:
            value=struct.unpack('<I',mu.mem_read(sp+4,4))[0];message=self.string(value)
            if message!='notice-token':raise RuntimeError('사제 안내 번역 반환/창 요청 오류')
            self.events.append('D:'+message);marker='D';self.ret()
        self.stub_calls[marker]+=1

    def run(self,case,control):
        """전체 ret·SEH FS·callee 레지스터/x87·raw/전역·외부 사건을 관찰한다."""
        self.case=case;self.events=[];self.setup(scene(case));mu=self.mu;p=self.p
        mu.mem_write(self.slot(NEW),bytes([0xab]*self.stride));mu.mem_write(p['local'],struct.pack('<I',case['local']));mu.mem_write(p['loading'],struct.pack('<I',case['loading']));mu.mem_write(p['clock'],struct.pack('<Q',CLOCKS[case['clock']]))
        # 원본 공통 소유자 지정은 비작업장 genus/일반 모드로 실제 실행한다.
        for name in ('fort','battle','challenge'):mu.mem_write(self.g[name],bytes(4))
        for typ in (167,168):mu.mem_write(TYPES+typ*self.type_stride+0xec,bytes(4))
        mu.mem_write(0,struct.pack('<I',0x12345678));before_map=bytes(mu.mem_read(MAP,self.map_end-MAP))
        self.write_ranges=[(0,4),(self.slot(NEW),self.slot(NEW)+self.stride)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 두 x87 정밀도와 서로 다른 보존 레지스터 값을 입력한다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_EDX,0);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_ECX,self.slot(TARGET));mu.reg_write(UC_X86_REG_ESP,STACK)
        mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.mem_write(STACK,struct.pack('<I',STOP));mu.emu_start(p['entry'],STOP,count=100000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or struct.unpack('<I',mu.mem_read(0,4))[0]!=0x12345678:raise RuntimeError('보호막 전체 정상 반환/ESP/SEH 복구 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()) or mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('보호막 callee/x87 복구 오류')
        if before_map!=bytes(mu.mem_read(MAP,self.map_end-MAP)):raise RuntimeError('wrapper가 하위 Pop 밖의 해시를 수정함')
        self.returns+=1
        return [';'.join(self.events) or '-',bytes(mu.mem_read(self.slot(TARGET),self.stride)).hex(),bytes(mu.mem_read(self.slot(NEW),self.stride)).hex(),
            struct.unpack('<I',mu.mem_read(p['local'],4))[0],struct.unpack('<I',mu.mem_read(p['loading'],4))[0],struct.unpack('<Q',mu.mem_read(p['clock'],8))[0]]


def generate(smoke=False):
    """독립 PE 출력·입력 장면·현재 SHA/실제 진입/대체 수를 기록한다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_priestforcefield_oracle.py',ROOT/'tools/ghidra/priestshield-functions.json',FIXTURE};cases=inputs()[::79] if smoke else inputs()
    # 세 실행 파일을 별도로 두 정밀도에서 정상 반환까지 실행한다.
    for edition in SPECS:
        oracle=ShieldOracle(edition)
        for case in cases:
            observed=[oracle.run(case,c) for c in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError('보호막 생성 두 x87 정밀도 불일치')
            rows.append([edition,*[case[k] for k in KEYS],*observed[0]])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: priestshield {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 실제 보호막 전체 생성 wrapper/lookup/finder/공통 소유자. 자산 생성/Pop·소리/문구 하위 몸체만 대체.\n# edition '+' '.join(KEYS)+' events priest shield localAfter loadingAfter clockAfter\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-10',decompile_date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['전체 생성 wrapper/실제 보호막 lookup·finder/공통 소유자 지정 정상 반환/ABI/x87/SEH','자산 생성·가상 Pop·new/SfxProcess·소리 조회/출력·번역/창은 명시 외부 대체','합성 발자국/해시/raw·소유자 0~8; 실제 보호막 Pop/오디오/전체 사제 postPop/GUI 미완료']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA·전체 입력 순서·반환·lookup/owner/대체 사건 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    # 저장된 원본 출력과 코드/내보내기의 변경을 검출한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if len(rows)!=3*len(inputs()) or report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('보호막 생성 근거 오류')
    # 사건만 집계하며 보호막 존재/생성/알림 결과를 다시 계산하지 않는다.
    for edition,item in report['editions'].items():
        selected=[r for r in rows if r[0]==edition];p=SPEC[edition];events=[e for r in selected for e in r[8].split(';')]
        if len(selected)!=len(inputs()) or item['returns']!=2*len(selected) or item['assertions']:raise RuntimeError('보호막 반환/행 오류')
        for address in (p['entry'],0x4918e0 if edition=='originals' else 0x40bf00):
            if item['native_calls'].get(f'{address:08x}')!=2*len(selected):raise RuntimeError('실제 wrapper/lookup 진입 오류')
        if item['native_calls'].get(f'{p["owner"]:08x}',0)!=2*sum(e.startswith('O:') for e in events):raise RuntimeError('실제 공통 소유자 진입 오류')
        for kind in ('C','P','A','L','S','N','T','D'):
            if item['substitutions'].get(kind,0)!=2*sum(e.startswith(kind+':') for e in events):raise RuntimeError('명시 외부 대체 사건 오류')
        if any(item['native_calls'].get(f'{p[key]:08x}',0) for key in ('create','new','load','sound','notice','translate','tell')):raise RuntimeError('외부 몸체 실행/대체 혼합')
        for row,case in zip(selected,inputs(),strict=True):
            if len(row)!=14 or row[1:8]!=[str(case[k]) for k in KEYS]:raise RuntimeError('보호막 입력/열 오류')
    print(f'priestshield 검증 통과: {len(rows)}개')


if __name__=='__main__':
    # 게임/창 실행 없이 무저장 표본·기록 생성·근거 감사를 선택한다.
    parser=argparse.ArgumentParser();parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
