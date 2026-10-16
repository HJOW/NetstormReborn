#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 반더 발사/즉시 빔·피해/회피·완료와 두 빔 wrapper의 실제 분기를 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_particle_gradient import Oracle as CallOracle, ROOT, STACK, STOP, digest
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

OBJECT, POOL, TYPES, ALLOC, VTABLE, GUN = (0x12000000 + index * 0x100000 for index in range(6))  # 원본 이미지와 겹치지 않는 독립 입력/경계 주소다.
QUERY, UNPOP, REPOP, NOTIFY = (STOP + offset for offset in (0x100, 0x200, 0x300, 0x400))  # 검토한 외부 가상 효과의 요청 경계다.
ENTRIES = {0x4680f0, 0x467fb0, 0x466e50, 0x466eb0, 0x467330, 0x466f50, 0x41bfb0, 0x41bff0, 0x4ac1e0, 0x49a840, 0x4558c0, 0x455930, 0x472700, 0x472690}  # 대체하지 않고 실행하는 실제 본체다.
SHORT = {0x466ab0:0x466ab4, 0x467f20:0x467f30, 0x467f30:0x467f40, 0x467f70:0x467f74, 0x468130:0x46813c}  # 정밀 표에서 빠진 PE의 실제 RET까지 직접 확인한 getter/Step이다.
KEYS = ('kind', 'frame', 'due', 'dead', 'hp', 'floater', 'seed', 'mutation', 'allocation', 'sound')  # 출력 계산 없이 구성하는 독립 입력 필드다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/vander-fire-x86.tsv'  # 동일 입력의 전체 이미지/슬롯/난수/효과 원문이다.
REPORT = ROOT / 'cpppj/vander-fire.json'  # 관찰 범위와 SHA/반환 근거다.


def sample(**values):
    """일반 HP 표적의 초기 상태만 준비한다. 기대 결과는 계산하지 않는다."""
    row = dict(zip(KEYS, (2, 0, 1, 0, 1, 0, 0, 0, 1, 1))); row.update(values); return row


def inputs():
    """생성/Step/Launch/Frame과 wrapper의 생존·HP·회피·재진입·할당 실패를 방문한다."""
    rows = [sample(kind=0)]
    # 원본 Step의 signed/상위 비트 OR를 그대로 방문한다.
    for frame in (-1, 0, 1, 7, 0x3fff, 0x4000, 0x8000, 0x7fffffff):
        rows.append(sample(kind=1, frame=frame))
    # 원본 실제 구간 난수와 표적 검증의 독립 조합이다.
    for dead, hp, floater, seed, allocation in itertools.product(range(4), (0, 1), (0, 1), (0, 1, 0x12345678, 0xdeadbeef), (0, 1)):
        rows.append(sample(dead=dead, hp=hp, floater=floater, seed=seed, allocation=allocation))
    # due 이전 추적, 죽은 표적 snapshot과 발사/완료 동시 비트를 방문한다.
    for frame, due, dead, hp, floater, seed in itertools.product((0, 1, 0x3fff), (0, 1), range(4), (0, 1), (0, 1), (0, 0x12345678)):
        rows.append(sample(kind=3, frame=frame, due=due, dead=dead, hp=hp, floater=floater, seed=seed))
    # 외부 callback 뒤 현재 필드와 초기 raw 포인터의 다른 재조회를 관찰한다.
    for kind, mutation, floater, seed in itertools.product((2, 3), range(1, 9), (0, 1), (0, 0x12345678)):
        rows.append(sample(kind=kind, mutation=mutation, floater=floater, seed=seed))
    # 명명 wrapper와 NULL을 허용하는 wrapper의 raw주소/0/객체 반환을 구별한다.
    for kind, dead, allocation in itertools.product((4, 5), range(4), (0, 1)):
        for sound in ((1,) if kind == 4 else (0, 1)):
            rows.append(sample(kind=kind, dead=dead, allocation=allocation, sound=sound))
    assert len(rows) == len({tuple(row[key] for key in KEYS) for row in rows})
    return rows


class Oracle(CallOracle):
    """발사/난수/HP 조회/wrapper는 실제 코드이며 공간/빔 생성/피해/Host만 외부 요청이다."""
    def __init__(self):
        """현재 정밀 함수표와 직접 확인한 짧은 getter만 격리 실행한다."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_32); image = pefile.PE(str(ROOT / 'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000, (len(image) + 4095) & ~4095); self.mu.mem_write(0x400000, image)
        # 객체/타입/자산/스택과 SEH를 각각 격리한다.
        for address, size in ((OBJECT,0x10000),(POOL,0x10000),(TYPES,0x20000),(ALLOC,0x10000),(VTABLE,0x10000),(GUN,0x10000),(STACK-0x7f000,0x80000),(STOP,0x1000),(0,0x1000)):
            self.mu.mem_map(address,size)
        self.allowed = list(SHORT.items()); self.calls = Counter(); self.events = []; self.returns = 0
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 함수별 불연속 구간만 허용한다.
        for entry in ENTRIES:
            # 실제 범위의 끝 한 바이트까지 포함한다.
            for part in facts[entry]['ranges'].split(';'):
                low, high = (int(value,16) for value in part.split('-')); self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction); self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def read(self,address):
        """격리 메모리의 little endian DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def slot(self,sid):
        """원본50BYTE stride 주소를 반환한다."""
        return POOL + sid * 50

    def mutate(self,where):
        """선언한 callback에서만 입력을 바꿔 원문 snapshot/재조회를 방문한다."""
        mu = self.mu; mutation = self.case['mutation']
        if where == 'Center' and mutation == 1:
            mu.mem_write(OBJECT+44,struct.pack('<H',61))
        if where == 'Point' and mutation == 2:
            mu.mem_write(OBJECT+8,struct.pack('<I',51)); mu.mem_write(OBJECT+44,struct.pack('<H',61))
        if where == 'Beam' and mutation == 3:
            mu.mem_write(self.slot(60)+11,b'\x02')
        if where == 'Beam' and mutation == 4:
            mu.mem_write(OBJECT+44,struct.pack('<H',61))
        if where == 'Hit' and mutation == 5:
            mu.mem_write(self.slot(60)+11,b'\x02')
        if where == 'Refresh' and mutation == 6:
            mu.mem_write(OBJECT+8,struct.pack('<I',51)); mu.mem_write(OBJECT+44,struct.pack('<H',61)); mu.mem_write(0x55b4d0,struct.pack('<d',100))
        if where == 'Unpop' and mutation == 7:
            mu.mem_write(OBJECT+8,struct.pack('<I',51))
        if where == 'Notify' and mutation == 8:
            mu.mem_write(OBJECT+4,struct.pack('<I',82))

    def instruction(self,mu,address,size,data):
        """현재 요청 인수를 기록하며 원본 분기/난수/메타데이터 계산은 대체하지 않는다."""
        boundaries = {QUERY,UNPOP,REPOP,NOTIFY,0x41bd20,0x4adea0,0x4571b0,0x4ae2d0,0x41bf10,0x4e4391,0x471d10,0x4d4f00,0x4d5000}
        if address in boundaries:
            esp=mu.reg_read(UC_X86_REG_ESP); ret=self.read(esp); this=mu.reg_read(UC_X86_REG_ECX); args=[self.read(esp+4+index*4) for index in range(4)]; value=0; purge=0; where=''
            if address == QUERY:
                sid=(this-POOL)//50;self.events.append(f'G:{sid}:{args[0]}:{args[1]}');value=50
            elif address == 0x41bd20:
                self.events.append('B:'+':'.join(map(str,args)));mu.mem_write(OBJECT+4,struct.pack('<2I',81,50));value=OBJECT;purge=16
            elif address == UNPOP:
                self.events.append(f'U:{(this-POOL)//50}:{args[0]}');purge=4;where='Unpop'
            elif address == REPOP:
                self.events.append(f'P:{(this-POOL)//50}:{args[2]}');purge=12
            elif address == 0x4adea0:
                sid=(this-POOL)//50;self.events.append(f'C:{sid}');mu.mem_write(args[0],struct.pack('<2f',59.5,59.5));value=args[0];purge=4;where='Center'
            elif address == 0x4571b0:
                self.events.append(f'T:{args[1]}:{args[2]}');mu.mem_write(args[0],struct.pack('<2f',52.25,52.75));where='Point'
            elif address == 0x4ae2d0:
                value=GUN
            elif address == NOTIFY:
                self.events.append('M:'+':'.join(map(str,args[:3])));purge=12;where='Notify'
            elif address == 0x41bf10:
                self.events.append(f'K:{self.read(OBJECT+4)}:{args[0]}');purge=4
            elif address == 0x4e4391:
                assert args[0]==104;self.events.append('A:104');value=ALLOC if self.case['allocation'] else 0
            elif address == 0x471d10:
                filename='null' if args[2]==0 else bytes(mu.mem_read(args[2],80)).split(b'\0')[0].decode();self.events.append(f'L:{args[0]}:{args[1]}:{filename}');value=ALLOC;purge=12;where='Beam'
            elif address == 0x4d4f00:
                self.events.append(f'H:{(this-POOL)//50}:{args[0]}');purge=4;where='Hit'
            else:
                self.events.append(f'R:{(this-POOL)//50}');where='Refresh'
            self.calls[f'boundary-{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret);self.mutate(where);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'허용하지 않은 반더 발사 실행 {address:08x}')
        if address in ENTRIES or address in SHORT:
            self.calls[f'{address:08x}']+=1

    def write(self,mu,access,address,size,value,data):
        """객체/명시 네 슬롯/난수/SEH/스택 밖의 쓰기를 거부한다."""
        regions=[(OBJECT,OBJECT+64),(0,4),(0x532710,0x532714),(STACK-0x7f000,STACK+0x1000)]+[(self.slot(sid),self.slot(sid)+50) for sid in (50,51,60,61)]
        if not any(low<=address and address+size<=high for low,high in regions):
            raise RuntimeError(f'허용하지 않은 반더 발사 쓰기 {address:08x}+{size}')

    def sample(self,case,precision):
        """전체 객체/슬롯/난수와 외부 효과 순서를 실제 원문 반환 뒤 읽는다."""
        self.case=case;self.events=[];mu=self.mu;kind=case['kind'];raw=bytearray((index*13+7)&255 for index in range(64))
        struct.pack_into('<3I4f',raw,0,0x50b170,81,50,60,60,59.5,59.5);struct.pack_into('<I',raw,32,0x50b16c);raw[40]=0x20;struct.pack_into('<2H',raw,42,70,60);struct.pack_into('<dI',raw,48,10 if case['due'] else 10.25,0);mu.mem_write(OBJECT,bytes(raw))
        # 현재/대체 부모·표적과 실제 GunForm TYPE를 준비한다.
        for sid,typeid in ((50,148),(51,148),(60,146 if case['floater'] else 158),(61,158),(70,34)):
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);struct.pack_into('<H',raw,6,81);raw[10]=typeid;raw[11]=case['dead'] if sid==60 else 0;raw[12]=0xa5;raw[34]=1;struct.pack_into('<2f',raw,14,50.5 if sid<60 else 60,50.5 if sid<60 else 60);struct.pack_into('<I',raw,36,case['frame']&0xffffffff);mu.mem_write(self.slot(sid),bytes(raw))
        mu.mem_write(VTABLE,struct.pack('<I',QUERY));mu.mem_write(VTABLE+0x48,struct.pack('<I',UNPOP));mu.mem_write(VTABLE+0x90,struct.pack('<I',REPOP));mu.mem_write(GUN,struct.pack('<3I',VTABLE+0x200,70,50));mu.mem_write(VTABLE+0x21c,struct.pack('<I',NOTIFY))
        # HP는 실제 TYPE getter가 읽으며 난수도 원본 전역을 그대로 사용한다.
        for typeid in (146,158):
            mu.mem_write(TYPES+typeid*500,bytes(500));mu.mem_write(TYPES+typeid*500+0xe8,struct.pack('<I',0x10 if case['hp'] else 0))
        for address,value in ((0x5c8464,POOL),(0x59ab20,TYPES),(0x541348,188),(0x5410fc,35),(0x5410f8,34),(0x5412a8,148),(0x5412a0,146),(0x540bbc,800),(0x532710,case['seed'])):
            mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(0x55b4d0,struct.pack('<d',10));mu.mem_write(0,struct.pack('<I',0x87654321));mu.mem_write(ALLOC+0x1000,b'shortZap.wav\0')
        args=struct.pack('<2I',70,60) if kind==0 else struct.pack('<I',case['frame']&0xffffffff) if kind==1 else struct.pack('<2I',50,60) if kind==4 else struct.pack('<3I',50,60,ALLOC+0x1000 if case['sound'] else 0) if kind==5 else b''
        entry=(0x4680f0,0x468130,0x466f50,0x467330,0x472700,0x472690)[kind];self.call(entry,OBJECT,args,len(args) if kind in (0,1) else 0,precision,OBJECT if kind==0 else None);self.returns+=1
        result=mu.reg_read(UC_X86_REG_EAX)
        if kind in (0,3):result='return'
        elif kind in (4,5):result='raw60' if result==self.slot(60) else 'beam' if result==ALLOC else str(result)
        else:result=str(result)
        assert self.read(0)==0x87654321
        return (result,';'.join(self.events) or '-',bytes(mu.mem_read(OBJECT,64)).hex(),b''.join(bytes(mu.mem_read(self.slot(sid),50)) for sid in (50,51,60,61)).hex(),str(self.read(0x532710)),bytes(mu.mem_read(0x55b4d0,8)).hex())


def main():
    """두 정밀도 원문을 보존하거나 재계산 없이 SHA/고유 입력을 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 모든 근거 파일의 원문 바이트를 확인한다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({tuple(row[:len(KEYS)]) for row in rows})==len(rows)
        print('반더 발사 SHA/고유 입력 감사:',len(rows));return
    cases=[sample(kind=kind) for kind in range(6)] if args.smoke else inputs();oracles=[Oracle(),Oracle()];rows=[];different=[]
    # 고유 입력은 두 정밀도를 합산하지 않는다.
    for index,case in enumerate(cases):
        observations=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))]
        if observations[0]!=observations[1]:different.append(index)
        rows.append('\t'.join([*(str(case[key]) for key in KEYS),*observations[0]]))
    if args.smoke:print('반더 발사6종 smoke 정상 반환');return
    FIXTURE.write_text('# '+ ' '.join(KEYS)+' result events fields64 slots200 seedAfter nowAfter\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_vander_fire.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',uniqueInputs=len(cases),normalReturns=sum(oracle.returns for oracle in oracles),precisionDifferentInputs=different,inputsByKind=dict(Counter(str(case['kind']) for case in cases)),nativeCalls=dict(oracles[0].calls),sha256={path:digest(ROOT/path) for path in paths},limitations=['실제 반더 ctor/Step/Frame/Launch·HP/타입 조회·난수·두 빔 wrapper를 실행했다.','공간/중심/코너·빔 생성자/메모리·피해/refresh·Host 부착/조회/완료/삭제는 명시 외부 경계다. 빔 생성/Frame/Draw 자체의 원문 대조는 앞선 별도 자료다.','미초기화 padding·네 raw 슬롯·난수·시각·요청 순서를 보존한다. 실제 캠페인/사용자 자연 완주 증거와 구별한다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('반더 발사:',len(cases),'고유 입력, 정밀도 차이',len(different))


if __name__=='__main__':main()
