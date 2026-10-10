#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""생산 칸 조각 반환과 contained 생성의 세 실제 PE 명령을 격리 실행한다.

실제 명령: 반환 전체(SEH 정상 경로), contained 생성, 게임 난수, 다리 가중치 추첨, CD 단어 쓰기.
Factory 직접 연산은 타입 6 factory/생성자/빈 postCreate도 실제 명령이며 SID 할당만 경계다.
Return/Create의 명시 경계: factory/파생 Form 생성, 표면 알림, 가상 부착, operator_new, Regular 생성.
게임/OS/예외 처리기는 실행하지 않는다. --verify는 저장 관찰과 SHA/ABI 집계를 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle,ROOT,SPECS,TYPES,POOL,STACK,STOP,digest
from decomp_priestgeometry_oracle import GeometryOracle
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 합성 가상 표/부착 경계/할당 주소와 부모/생성 SID다. 호스트 메모리와 무관하다.
VTABLE,ATTACH,ALLOCATION,PARENT,BORN=0x11010000,STOP+0x400,0x14600000,77,100
# 원본 판본별 명령 진입점과 실제 전역 주소다.
SPEC={
 'originals':dict(entry=0x4468b0,contained=0x4afa70,factory=0x4af530,notify=0x4214a0,allocate=0x4e4391,timer=0x496e80,
    rng=0x4558f0,draw=0x4257c0,quality=30,index=0x52f7e2,sidAllocate=0x4af1d0,constructor=0x4aea40,postCreate=0x4437f0,
    globals=dict(type=0x55a284,bridge=0x5411a0,form=0x541088,keep=0x5318f4,fort=0x594fb8,seed=0x532710,total=0x5453d4)),
 'originalCD':dict(entry=0x43e930,contained=0x4ac4d0,factory=0x4ab390,notify=0x448c10,allocate=0x4f1650,timer=0x48efd0,
    rng=0x48cc10,draw=0x41fc70,quality=26,index=0x565b01,sidAllocate=0x4aae40,constructor=0x4afe00,postCreate=0x4b0470,
    globals=dict(type=0x51b630,bridge=0x51ca8c,form=0x51c978,keep=0x52e918,fort=0x540a1c,seed=0x5308ec,total=0x516618)),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 입력 열은 계산 결과를 포함하지 않는다. 돌연변이는 명시 경계에서만 공급한다.
KEYS=('type','flags','quality','keep','fort','index','seed','argument','attachFlags','rawType',
    'allocation','factoryChange','notifyChange','attachChange','allocateChange')
FIXTURE=ROOT/'cpppj/tests/fixtures/returnpiece-x86.tsv'
REPORT=ROOT/'cpppj/recovery-returnpiece-evidence.json'


def case(**changes):
    """기본 일반 조각 입력에 변경값만 덮어쓴다. 기대 결과는 계산하지 않는다."""
    value=dict(type=100,flags=8,quality=3,keep=0,fort=0,index=0,seed=0,argument=0x12345678,attachFlags=0x10,
        rawType=6,allocation=1,factoryChange=0,notifyChange=0,attachChange=0,allocateChange=0)
    value.update(changes);return {key:value[key]&0xffffffff for key in KEYS}


def inputs():
    """품질 signed 경계·DWORD 플래그·옵션 재조회·난수·생성 인자 폭을 교차한다."""
    rows=[]
    # 품질 감소는 32비트에서 수행하며 생성 후에 BYTE로 좁힌다.
    for flags,quality,keep,index in itertools.product((0,8,0x100,0x108,0x10000,0xffffffff),
            (0,1,2,3,0xff,0x100,0x80000000,0x80000001,0xffffffff),(0,1),(0,1,2)):
        rows.append(('Return',case(flags=flags,quality=quality,keep=keep,index=index)))
    # 생성/부착/할당 뒤 현재 전역 읽기와 알림 뒤 필드 덮어쓰기 순서를 관찰한다.
    for keep,fort,allocation,attach,allocate,raw,notify in itertools.product((0,1),(0,1),(0,1),(0,1),(0,1),(6,82),(0,1)):
        rows.append(('Return',case(keep=keep,fort=fort,allocation=allocation,attachChange=attach,
            allocateChange=allocate,rawType=raw,notifyChange=notify)))
    # 실제 난수/추첨을 다양한 초기 상태로 실행한다. bridge 경로는 옵션/품질/시간을 건너뛴다.
    for seed in (0,1,0xffffffff,0x80000000,*range(17,1100,17)):
        rows.append(('Return',case(type=82,seed=seed,flags=0x100,keep=0,quality=0xffffffff,index=255,fort=1)))
    for factory,raw,keep in itertools.product((0,1),(6,82),(0,1)):
        rows.append(('Return',case(type=82,rawType=raw,factoryChange=factory,keep=keep,notifyChange=1)))
    # 생성 helper를 직접 호출해 flags 기본값과 0x20 우선·WORD/BYTE 절단을 독립 검사한다.
    for flags,attach,typ,raw in itertools.product((0,0x20,0x100,0x10000,0xffffffff),(0,0x10,0x20,0x30,0x10010),
            (100,0x10052),(6,82)):
        rows.append(('Create',case(flags=flags,attachFlags=attach,type=typ,rawType=raw,quality=0x1234ff,notifyChange=1)))
    rows.append(('Create',case(factoryChange=1,rawType=6)))
    # 타입 6의 실제 factory/생성자/빈 postCreate를 실행한다. SID 할당은 별도 검증된 명시 경계다.
    for flags in (0,1,2,3):rows.append(('Factory',case(type=6,flags=flags)))
    return rows


class ReturnOracle(GeometryOracle):
    """명령 실행/쓰기 범위와 경계 ABI를 검사하고 반환/생성 결과만 관찰한다."""
    def __init__(self,edition):
        """읽기 전용 PE와 최소 합성 Form/SEH 메모리를 준비한다."""
        super().__init__(edition);self.d=SPEC[edition];self.mu.mem_map(0,0x1000)
        self.mu.mem_map(VTABLE,0x1000);self.mu.mem_map(ALLOCATION,0x1000);self.mu.mem_map(POOL+0x2000,0x200000-0x2000)
        self.mu.mem_write(VTABLE+0x50,struct.pack('<I',ATTACH))
        self.exports=[ROOT/f'extracted/returnpiece/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.returns=collections.Counter();self.stub_calls=collections.Counter()
        # Ghidra가 계산한 불연속 몸체만 실행을 허용한다.
        with self.exports[1].open(encoding='utf-8') as fp:
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 포함 끝 주소를 반열린 구간으로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.stubs={self.d[key]:(letter,key) for key,letter in (
            ('factory','C'),('notify','N'),('allocate','A'),('timer','T'),('sidAllocate','S'))};self.stubs[ATTACH]=('J','attach')

    def get(self,key):
        """실제 전역 DWORD를 현재 시점에 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(self.d['globals'][key],4))[0]

    def put(self,key,value):
        """입력/명시 효과가 전역을 바꾸며 기계어 쓰기와는 별도로 집계한다."""
        self.mu.mem_write(self.d['globals'][key],struct.pack('<I',value&0xffffffff))

    def args(self,count):
        """현재 스택의 전체 DWORD 인자를 읽는다."""
        return struct.unpack(f'<{count}I',self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def ret(self,result=0,purge=0):
        """명시 경계의 반환값/호출 규약 스택 정리만 공급한다."""
        mu=self.mu;sp=mu.reg_read(UC_X86_REG_ESP);mu.reg_write(UC_X86_REG_EAX,result)
        mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def event(self,letter,*values):
        """전체 DWORD 인자와 raw 슬롯 16진수 관찰을 사건으로 기록한다."""
        self.events.append(letter+':'+':'.join(map(str,values)))

    def raw(self):
        """주변 필드 보존 여부도 비교하도록 생성 슬롯 전체를 관찰한다."""
        return bytes(self.mu.mem_read(self.slot(self.born),self.stride)).hex()

    def stub(self,letter,key):
        """원본이 호출한 효과 인자/순서를 관찰하고 지정된 변동 입력만 공급한다."""
        mu=self.mu;c=self.case;result=0;purge=0
        if key=='sidAllocate':
            self.event(letter,*self.args(1));result=self.born
        elif key=='factory':
            self.event(letter,*self.args(2));result=self.slot(BORN)
            if c['factoryChange']:self.put('bridge',6);self.put('form',7);self.put('type',101)
        elif key=='notify':
            self.event(letter,*self.args(1),self.raw())
            if c['notifyChange']:
                mu.mem_write(self.slot(BORN)+12,b'\x76\x98');mu.mem_write(self.slot(BORN)+20,b'\xff\xff')
                mu.mem_write(self.slot(BORN)+self.d['quality'],b'\xfe')
        elif key=='attach':
            if mu.reg_read(UC_X86_REG_ECX)!=self.slot(BORN):raise RuntimeError('Form 부착 this 오류')
            self.event(letter,BORN,*self.args(2),self.raw());purge=8
            if c['attachChange']:self.put('keep',self.get('keep')^1);self.put('type',102)
        elif key=='allocate':
            self.event(letter,*self.args(1));result=ALLOCATION if c['allocation'] else 0
            if c['allocateChange']:
                self.put('fort',self.get('fort')^1);mu.mem_write(self.d['index'],b'\x02')
        elif key=='timer':
            if mu.reg_read(UC_X86_REG_ECX)!=ALLOCATION:raise RuntimeError('Regular 생성 this 오류')
            self.event(letter,*self.args(3));purge=12;result=ALLOCATION
        else:raise RuntimeError('알 수 없는 조각 반환 경계')
        self.ret(result,purge)

    def on_instruction(self,mu,address,size,data):
        """모든 명시 경계와 실제 내보낸 몸체를 구분해 집계한다."""
        stub=self.stubs.get(address)
        if address==self.d['factory'] and self.operation=='Factory':stub=None
        if stub is None:OwnerOracle.on_instruction(self,mu,address,size,data);return
        letter,key=stub;self.stub(letter,key);self.stub_calls[letter]+=1

    def on_write(self,mu,access,address,size,value,data):
        """스택 외 쓰기는 정확한 Form 필드·SEH 머리·난수/가중치 전역만 허용한다."""
        OwnerOracle.on_write(self,mu,access,address,size,value,data)

    def run(self,operation,case,control):
        """원본 전체 함수의 정상 반환과 보존 레지스터/x87/SEH 복구를 검사한다."""
        self.case=case;self.operation=operation;self.events=[];self.instructions=0;mu=self.mu;self.born=BORN
        if operation=='Factory':
            self.born=(23002 if self.edition=='originals' else 14001) if case['flags']&1 else 5 if case['flags']&2 else (15000 if self.edition=='originals' else 6000)
        # 원본 슬롯 주변 바이트에 서로 다른 값을 두어 덮어쓰기 폭을 관찰한다.
        raw=bytearray((i*37+11)&0xff for i in range(self.stride));struct.pack_into('<I',raw,0,VTABLE)
        raw[10]=case['rawType'];raw[11]=4
        if operation=='Factory':raw=bytearray(self.stride);raw[11]=4
        mu.mem_write(self.slot(self.born),bytes(raw))
        mu.mem_write(TYPES+6*self.type_stride+(0x1d0 if self.edition=='originals' else 0x1b0),struct.pack('<I',self.d['constructor']))
        for key,value in (('type',case['type']),('bridge',82),('form',6),('keep',case['keep']),
                ('fort',case['fort']),('seed',case['seed']),('total',0)):self.put(key,value)
        mu.mem_write(self.d['index'],bytes([case['index']]));mu.mem_write(0,struct.pack('<I',0x11223344))
        self.write_ranges=[(0,4),*[ (self.d['globals'][key],self.d['globals'][key]+4) for key in ('seed','total')],
            *[(self.slot(BORN)+offset,self.slot(BORN)+offset+width) for offset,width in ((12,2),(18,2),(20,2),(self.d['quality'],1))]]
        if operation=='Factory':self.write_ranges.extend((self.slot(self.born)+offset,self.slot(self.born)+offset+width) for offset,width in ((0,4),(10,1)))
        self.prepare_registers(control)
        if operation=='Return':entry=self.d['entry'];args=[case['flags'],PARENT,case['quality']];purge=0
        elif operation=='Create':
            entry=self.d['contained'];args=[case[key] for key in ('type','argument','flags','attachFlags','quality')]
            purge=20;mu.reg_write(UC_X86_REG_ECX,self.slot(PARENT))
        else:entry=self.d['factory'];args=[6,case['flags']];purge=0
        mu.mem_write(STACK,struct.pack(f'<{len(args)+1}I',STOP,*args));mu.emu_start(entry,STOP,count=50000)
        self.check(control,STACK+4+purge);self.returns[f'{entry:08x}']+=1
        if struct.unpack('<I',mu.mem_read(0,4))[0]!=0x11223344:raise RuntimeError('SEH 머리 복구 오류')
        result=mu.reg_read(UC_X86_REG_EAX) if operation=='Create' else 0
        if operation=='Factory':
            pointer=mu.reg_read(UC_X86_REG_EAX)
            if pointer!=self.slot(self.born):raise RuntimeError('contained factory 반환 주소 오류')
            result=self.born
        snapshot=[self.get(key) for key in ('type','bridge','form','keep','fort','seed')]+[mu.mem_read(self.d['index'],1)[0],result]
        return [';'.join(self.events),self.raw(),','.join(map(str,snapshot))]


def generate(smoke=False):
    """세 PE × 두 x87 정밀도에서 같은 관찰만 저장하고 입력 파일 SHA를 기록한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/returnpiece-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','damageablepredestroy')]}
    # 각 판본마다 독립 PE와 메모리를 사용한다.
    for edition in SPECS:
        oracle=ReturnOracle(edition);samples=inputs()
        if smoke:samples=samples[::31]
        # 기대 결과는 x86 실행에서만 생성한다.
        for operation,value in samples:
            results=[oracle.run(operation,value,control) for control in CONTROLS]
            if results[0]!=results[1]:raise RuntimeError(f'조각 반환 x87 차이: {edition} {operation} {value}')
            rows.append([edition,operation,','.join(str(value[key]) for key in KEYS),*results[0]])
        editions[edition]=dict(cases=len(samples),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: returnpiece {len(samples)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# 생산 칸 반환/contained 생성 실제 PE 관찰. 입력: '+' '.join(KEYS)+'\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',decompile_date='2026-10-11',
        controls=list(CONTROLS),os_calls=0,editions=editions,
        files={str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in sorted(paths)},
        scope=['전체 반환/contained/난수/추첨/CD 단어 쓰기는 실제 명령; 정상 SEH 경로만 실행',
            'Return/Create의 factory/파생 Form 생성·표면 알림·가상 부착·operator_new·Regular 생성은 명시 경계',
            'Factory 직접 연산은 타입 6 factory/생성자/빈 postCreate 실제 명령, SID 할당만 명시 경계',
            '원본 assert/게임/OS/예외 처리기 미실행, 유효 부모 77/생성 SID 100·시간 번호 0..2 입력']),
        ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'returnpiece 전체 {len(rows)}개')


def verify():
    """저장 SHA·입력 순서·행 폭·정상 반환/명시 경계 집계를 결과 재계산 없이 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 사용한 실제 파일을 모두 바이트 단위로 검사한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('반환 근거 메타 오류')
    # 판본별로 저장 관찰 사건과 독립 집계를 대조한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];samples=inputs();spec=SPEC[edition]
        if len(selected)!=len(samples) or item['cases']!=len(samples) or item['assertions']:raise RuntimeError('반환 행/assert 오류')
        # 예상 출력은 재계산하지 않고 입력과 열만 검사한다.
        for row,(operation,value) in zip(selected,samples,strict=True):
            if len(row)!=6 or row[1]!=operation or row[2]!=','.join(str(value[key]) for key in KEYS) or len(row[5].split(','))!=8 or len(row[4])!=SPECS[edition]['stride']*2:raise RuntimeError('반환 입력/열 오류')
        for operation,key in (('Return','entry'),('Create','contained'),('Factory','factory')):
            count=2*sum(row[1]==operation for row in selected)
            if item['returns'].get(f'{spec[key]:08x}',0)!=count:raise RuntimeError('반환 ABI 집계 오류')
        if item['native_calls'].get(f'{spec["entry"]:08x}',0)!=item['returns'][f'{spec["entry"]:08x}']:raise RuntimeError('반환 진입 수 오류')
        factoryCount=2*sum(row[1]=='Factory' for row in selected)
        if item['native_calls'].get(f'{spec["contained"]:08x}',0)!=2*len(selected)-factoryCount:raise RuntimeError('contained 진입 수 오류')
        if any(item['native_calls'].get(f'{spec[key]:08x}',0)!=factoryCount for key in ('factory','constructor','postCreate')):raise RuntimeError('contained factory/생성자/postCreate 진입 수 오류')
        events=[event for row in selected for event in row[3].split(';')]
        # 효과 사건 하나는 경계 호출 하나에 해당한다.
        for letter in 'CNJATS':
            if item['substitutions'].get(letter,0)!=2*sum(event.startswith(letter+':') for event in events):raise RuntimeError('반환 경계 수 오류: '+letter)
        if any(item['native_calls'].get(f'{spec[key]:08x}',0) for key in ('notify','allocate','timer','sidAllocate')):raise RuntimeError('반환 몸체/경계 혼합')
        if not all(item['native_calls'].get(f'{spec[key]:08x}',0) for key in ('rng','draw')):raise RuntimeError('실제 난수/추첨 미실행')
    print(f'returnpiece 검증 통과: {len(rows)}개')


def main():
    """전체 생성/무저장 표본/저장 근거 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
