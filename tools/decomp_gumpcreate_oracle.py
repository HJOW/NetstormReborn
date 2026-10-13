#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 Gump 기본 생성/등록/루트 초기화를 격리 실행한다. new(0xa0)만 명시 입력 경계다."""
import argparse
import collections
import csv
import itertools
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle,ROOT,BINARIES,CONTROLS,STACK,STOP,digest,fbits,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW)

# 기본 객체/보호 영역의 격리 주소, 등록 표 크기, 재현 가능한 입력 시드와 저장 경로다.
DATA,SIZE,STRIDE,CAPACITY,SEED=0x16000000,160,192,1000,0x4642c01c
TOKENS=tuple(DATA+16+i*STRIDE for i in range(4))
FIXTURE=ROOT/'cpppj/tests/fixtures/gumpcreate-x86.tsv'
REPORT=ROOT/'cpppj/gumpcreate.json'
# 실제 기본/루트 생성 함수, 등록/개수/식별 번호/화면 크기 전역과 new 경계다.
SPECS={
    'originals':dict(Construct=0x4642c0,Root=0x4663d0,table=0x563e38,count=0x565dc8,serial=0x540418,screen=0x5c78f0,new=0x4e4391),
    'originalCD':dict(Construct=0x492da0,Root=0x492b30,table=0x56b748,count=0x535e80,serial=0x535e98,screen=0x516b10,new=0x4f1650),
}
SPECS['original1037']=dict(SPECS['originalCD'])


def rle(values):
    """연속 같은 DWORD만 길이:값으로 줄인다. 기대 등록 위치를 계산하지 않는다."""
    return ';'.join(f'{len(list(group))}:{value}' for value,group in itertools.groupby(values))


def table(profile):
    """빈 표/첫 빈 칸/여러 빈 칸/가득 찬 표의 완성 입력을 만든다."""
    value=[0 if profile==0 else 0x18000000]*CAPACITY
    holes={1:(0,),2:(1,),3:(127,),4:(998,),5:(999,),6:(7,127,999),7:()}
    # 첫/마지막/여러 후보를 명시하며 기대 선택은 원본에서만 관찰한다.
    for index in holes.get(profile,()): value[index]=0
    if profile==6: value[0]=TOKENS[3]
    return value


def plan():
    """플래그/바이트 보존/첫 빈 칸/카운터 감김/루트 할당 실패·현재 전역의 독립 입력이다."""
    result=[]
    # 초기 노드 전체는 생성 전 독립 바이트다. 원본이 덮지 않는 필드도 임의 값으로 남긴다.
    def add(actions,profile=0,count=12,serial=23,screen=(1024,768),pattern=37):
        """연산/전역/전체 표/전체 노드 입력만 복사하고 기대 초기화 바이트는 넣지 않는다."""
        nodes=[bytes((pattern+index*31+offset*17)&255 for offset in range(SIZE)) for index in range(len(TOKENS))]
        result.append(dict(actions=list(actions),table=table(profile),count=count,serial=serial,screen=screen,nodes=nodes))
    flags=(0,1,8,0x10000,0x100000,0x100001,0x200000,0x400000,0xffffffff,0x80000000)
    widths=(fbits(12.75),fbits(-9.5),0,0x80000000,0x7fc00001,0x7f800000)
    # 인접 플래그/미등록/전체 DWORD와 다양한 첫 빈 칸을 교차한다.
    for index,flag in enumerate(flags):
        # 등록 표가 가득 찼어도 count가 정상 범위면 원본은 정상 반환한다.
        for profile in range(8):
            add([('Construct',TOKENS[0],0x81234567+index,widths[index%6],widths[(index+1)%6],flag)],
                profile=profile,count=(0,12,998)[profile%3],serial=(0,0xffffffff,0x7fffffff)[index%3])
    # 동일 raw float가 네 크기 필드에 비트 그대로 복사되는지를 검사한다.
    for index,width in enumerate(widths):
        # 양수/음수 0, NaN/무한대도 생성자에서는 계산하지 않는 DWORD 인자다.
        for height in widths:
            add([('Construct',TOKENS[1],index,width,height,0x100001)],profile=index%8,pattern=index)
    screens=((1024,768),(1920,1080),(0,0),(-1,-2),(16777217,-16777217),(2147483647,-2147483648))
    # 루트 초기화의 표 지우기/할당 실패/카운터 보존과 정수→float 반올림을 관찰한다.
    for profile in range(8):
        # new는 표가 비워진 뒤 호출되며 성공 시에만 기본 생성자를 실행한다.
        for pointer in (0,TOKENS[0]):
            # 입력 화면 크기는 signed DWORD이며 원본 x87 저장을 실행한다.
            for screen in screens:
                add([('Root',pointer)],profile=profile,count=12,serial=0xffffffff,screen=screen)
    # 할당 경계가 현재 전역을 바꾸면 그 뒤의 root/constructor가 새 값을 읽는다.
    for pointer in (0,TOKENS[0]):
        # 개수/식별 번호도 root 함수가 따로 초기화하지 않는다.
        for serial in (0,0xffffffff):
            add([('Root',pointer,997,serial,16777217,-16777217)],profile=7,count=12,serial=23,screen=(640,480))
    # 연속 생성/루트 재초기화/미등록 생성이 현재 등록 표와 카운터를 공유한다.
    for profile in range(8):
        add([('Construct',TOKENS[0],17,fbits(8),fbits(9),1),('Construct',TOKENS[1],18,fbits(10),fbits(11),0x100001),
            ('Construct',TOKENS[2],19,fbits(12),fbits(13),1)],profile=profile,count=12,serial=0xfffffffe)
        add([('Root',TOKENS[0]),('Root',TOKENS[1]),('Construct',TOKENS[2],21,fbits(4),fbits(5),1)],profile=profile,count=12,serial=0xfffffffe)
    # 개수 전역은 signed 비교이고 DWORD 증가다. 음수/넘침도 원본 정상 반환 입력으로 관찰한다.
    for count in (0xffffffff,0x80000000,0x7fffffff):
        add([('Construct',TOKENS[0],0,fbits(1),fbits(2),1)],profile=2,count=count)
    rng=random.Random(SEED)
    # 전체 초기 바이트/임의 float 비트/플래그/식별 번호 조합의 독립 입력을 더한다.
    for index in range(80):
        add([('Construct',TOKENS[index%4],rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32))],
            profile=index%8,count=12,serial=rng.getrandbits(32),pattern=rng.randrange(256))
    return result


def encoded(case):
    """완성 입력 전역/표/노드/연산을 C++가 재생할 열로 기록한다."""
    return [','.join(map(str,(case['count'],case['serial'],*case['screen']))),rle(case['table']),
        ';'.join(f'{token}={raw.hex()}' for token,raw in zip(TOKENS,case['nodes'],strict=True)),
        ';'.join(','.join(map(str,action)) for action in case['actions'])]


class GumpCreateOracle(ProcessOracle):
    """기본/루트 생성의 두 전체 몸체만 실행하며 new는 입력 토큰으로 응답한다."""
    def __init__(self,edition):
        """PE/일반 쓰기 검사기를 준비하고 이번 정확한 함수 구간으로 허용 목록을 교체한다."""
        super().__init__(edition);self.d=SPECS[edition];self.allowed=[];self.entry_names={}
        self.extra=[ROOT/f'extracted/gumpcreate/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        with self.extra[1].open(encoding='utf-8') as fp:
            # Ghidra의 함수별 불연속 범위를 등록하며 사이 주소는 실행하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 마지막 바이트까지 포함한 실제 몸체만 허용한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x10000);self.returns,self.boundaries=collections.Counter(),collections.Counter()

    def globals(self):
        """현재 개수/식별 번호와 signed 화면 크기를 실제 메모리에서 읽는다."""
        return (self.u32(self.d['count']),self.u32(self.d['serial']),*struct.unpack('<2i',self.mu.mem_read(self.d['screen'],8)))

    def on_instruction(self,mu,address,size,data):
        """new 입력 경계 이외에는 원본 명령만 실행하고 실제 함수 진입을 기록한다."""
        if address==self.d['new']:
            esp=mu.reg_read(UC_X86_REG_ESP);request=self.u32(esp+4)
            if request!=SIZE or self.u32(0)==0xffffffff: raise RuntimeError('Gump new 크기/SEH 보호 오류')
            cleared=bytes(mu.mem_read(self.d['table'],CAPACITY*4))==b'\0'*(CAPACITY*4)
            self.allocations.append(','.join(map(str,(request,*self.globals(),int(cleared)))));self.boundaries['new']+=1
            if len(self.action)==6:
                # 입력에 명시한 할당 부수 효과만 쓴다. 기대 루트 크기/식별 번호는 계산하지 않는다.
                for key,value in zip(('count','serial'),self.action[2:4],strict=True): self.put(self.d[key],value)
                mu.mem_write(self.d['screen'],struct.pack('<2i',*self.action[4:6]))
            mu.reg_write(UC_X86_REG_EAX,self.action[1]);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,self.u32(esp));return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'Gump 생성 허용하지 않은 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,case,control):
        """현재 전역을 공유하는 연속 연산의 전체 바이트/등록 표/반환/ABI/x87/SEH를 관찰한다."""
        mu,d=self.mu,self.d;self.entries,self.row_returns=collections.Counter(),collections.Counter()
        self.allocations,self.results=[],[];mu.mem_write(DATA,b'\xa5'*0x1000)
        # 입력 노드는 생성자가 쓰는 정확한 영역 앞뒤에 보호 바이트를 둔다.
        for token,raw in zip(TOKENS,case['nodes'],strict=True): mu.mem_write(token,raw)
        self.put(d['count'],case['count']);self.put(d['serial'],case['serial']);mu.mem_write(d['screen'],struct.pack('<2i',*case['screen']))
        mu.mem_write(d['table']-16,b'\xa5'*16);mu.mem_write(d['table'],struct.pack('<1000I',*case['table']));mu.mem_write(d['table']+CAPACITY*4,b'\xa5'*16)
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000),(d['table'],d['table']+CAPACITY*4),(d['count'],d['count']+4),(d['serial'],d['serial']+4)]
        self.writes.extend((token,token+SIZE) for token in TOKENS)
        # 각 최상위 연산의 현재 입력 전역은 유지하고 호출 프레임/레지스터만 다시 준비한다.
        for self.action in case['actions']:
            op=self.action[0];args=self.action[2:] if op=='Construct' else ();start=d[op]
            # 비영 보존 레지스터를 실제 thiscall/cdecl 호출 규약과 대조한다.
            for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
            # 임시 레지스터와 각 호출의 this/스택을 입력에 맞춘다.
            for reg in (UC_X86_REG_EAX,UC_X86_REG_EDX): mu.reg_write(reg,0)
            mu.reg_write(UC_X86_REG_ECX,self.action[1] if op=='Construct' else 0);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2)
            mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
            mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*(value&0xffffffff for value in args)))
            mu.emu_start(start,STOP,timeout=30000000,count=2000000)
            if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+(16 if op=='Construct' else 0):
                raise RuntimeError('Gump 생성 정상 반환/스택 오류')
            if any(mu.reg_read(reg)!=value for reg,value in REGISTERS.items()): raise RuntimeError('Gump 생성 보존 레지스터 오류')
            if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff:
                raise RuntimeError('Gump 생성 x87/SEH 오류')
            self.row_returns[f'{start:08x}']+=1;self.returns[f'{start:08x}']+=1
            self.results.append(str(mu.reg_read(UC_X86_REG_EAX)) if op=='Construct' else str(self.u32(d['table'])))
        # 미사용 노드 주변/등록 표 경계를 모두 확인한다. 생성자의 전체 출력은 실제 바이트로 남긴다.
        protected=bytearray(mu.mem_read(DATA,0x1000))
        for token in TOKENS: protected[token-DATA:token-DATA+SIZE]=b'\xa5'*SIZE
        if protected!=b'\xa5'*0x1000 or bytes(mu.mem_read(d['table']-16,16))+bytes(mu.mem_read(d['table']+CAPACITY*4,16))!=b'\xa5'*32:
            raise RuntimeError('Gump 생성 보호 바이트 변경')
        if self.globals()[2:]!=tuple(self.action[4:6] if self.action[0]=='Root' and len(self.action)==6 else case['screen']):
            raise RuntimeError('Gump 생성 화면 입력 변경')
        output=';'.join(f'{token}={bytes(mu.mem_read(token,SIZE)).hex()}' for token in TOKENS)
        return [*encoded(case),output,rle(struct.unpack('<1000I',mu.mem_read(d['table'],CAPACITY*4))),','.join(map(str,self.globals())),
            ','.join(self.results),';'.join(self.allocations) or '-',json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),
            json.dumps(dict(self.row_returns),sort_keys=True,separators=(',',':'))]


def generate(smoke):
    """두 정밀도의 실제 관찰이 같을 때만 저장한다. 기대 생성/등록 계산이나 C++ 호출은 하지 않는다."""
    rows,editions=[],{};files={FIXTURE,ROOT/'tools/ghidra/gumpcreate-functions.json'}
    files.update(ROOT/'tools'/name for name in ('decomp_gumpcreate_oracle.py','decomp_drawsort_oracle.py','decomp_process_oracle.py'))
    # 세 PE는 따로 로드해 독립 진입/반환/경계 횟수와 SHA를 남긴다.
    for edition in BINARIES:
        oracle=GumpCreateOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::23]+indexed[-5:]
        # 연속 연산의 전체 관찰을 두 x87 제어 워드에서 비교한다.
        for index,case in indexed:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Gump 생성 x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns),boundaries=dict(oracle.boundaries))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 Gump 기본/루트 생성·등록 관찰. new만 명시 입력 경계.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-12',controls=list(CONTROLS),os_calls=0,assertions=0,boundaries=['new'],
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},
        limits=['기본 생성/첫 빈 칸 등록/루트 초기화 전체 몸체; new(0xa0)만 입력 토큰/현재 전역 변경 응답 경계',
            '전체 0xa0바이트/미작성 +0x28/+0x74/1000개 등록 칸/개수/serial/현재 signed 화면 크기/할당 순서의 실제 관찰',
            '기본 크기 인자는 임의 DWORD를 그대로 복사; 루트는 실제 signed 정수→float 저장',
            '원본 signed 개수 assert에 도달하는 입력/실제 예외 전파/GUI/OS는 제외',
            '전체 정상 반환/보존 레지스터/스택/x87/SEH/허용 실행·쓰기/보호 바이트/SHA/진입·반환·경계 감사']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/완성 입력/행 수/바이트 폭과 실제 기본·루트 진입/반환/new 경계 집계를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 도구/원본/내보내기/fixture의 정확한 바이트를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Gump 생성 SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['boundaries']!=['new'] or report['os_calls'] or report['assertions']:
        raise RuntimeError('Gump 생성 총계/경계 오류')
    if set(report['editions'])!=set(BINARIES): raise RuntimeError('Gump 생성 판본 누락')
    # 판본별 계획/출력 폭/실제 호출 횟수만 감사하며 기대 생성 바이트는 재계산하지 않는다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];entries,returns,boundaries=collections.Counter(),collections.Counter(),collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Gump 생성 행 수 오류')
        # 모든 순서/완성 입력/전체 노드와 등록 표의 기록 폭을 검사한다.
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=13 or row[1]!=str(index) or row[2:6]!=encoded(case): raise RuntimeError('Gump 생성 입력/폭 오류')
            if len(row[6].split(';'))!=len(TOKENS) or any(len(bytes.fromhex(part.split('=')[1]))!=SIZE for part in row[6].split(';')):
                raise RuntimeError('Gump 생성 노드 출력 폭 오류')
            if sum(int(part.split(':')[0]) for part in row[7].split(';'))!=CAPACITY: raise RuntimeError('Gump 생성 등록 표 폭 오류')
            # 저장 관찰은 두 정밀도 실행의 합계다.
            for address,count in json.loads(row[11]).items(): entries[address]+=count*2
            # 최상위 연속 연산의 실제 정상 반환만 집계한다. 루트 내부 기본 생성의 진입은 별도다.
            for address,count in json.loads(row[12]).items(): returns[address]+=count*2
            expected=collections.Counter(f'{SPECS[edition][action[0]]:08x}' for action in case['actions'])
            if dict(expected)!=json.loads(row[12]): raise RuntimeError('Gump 생성 정상 반환 누락')
            if row[10]!='-': boundaries['new']+=len(row[10].split(';'))*2
        if dict(entries)!=item['entries'] or dict(returns)!=item['returns'] or dict(boundaries)!=item['boundaries']: raise RuntimeError('Gump 생성 실행 집계 오류')
    normal=sum(sum(item['returns'].values()) for item in report['editions'].values())
    print(f'gumpcreate 검증 통과: {len(rows)}개, 두 정밀도 최상위 정상 반환 {normal}회, OS/assert 0')


def main():
    """전체 관찰/무저장 표본/저장 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__': main()
