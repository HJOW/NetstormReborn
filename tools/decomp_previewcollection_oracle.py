#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 커서 미리보기 수집/활성/CRT와 준비된 SP 읽기 전체를 격리 실행한다. 응답 대체는 없다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, STACK, STOP, digest, fbits,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 출력/부모/타입/슬롯/spot/SP 배열은 격리 주소이며 호스트 포인터가 아니다.
DATA,ARRAY,GUMP,TYPE,POOL,SPOTS,SPDATA=0x15000000,0x15000100,0x15001000,0x15001800,0x15002000,0x15010000,0x15020000
CAPACITY,SEED=64,0x44606031
FIXTURE=ROOT/'cpppj/tests/fixtures/previewcollection-x86.tsv'
REPORT=ROOT/'cpppj/recovery-previewcollection-evidence.json'
# 원본 목록/선택/배치/잠금 전역과 패치판 강조/SP 저장소의 실제 주소다.
SPECS={
    'originals':dict(root=0x446060,lock=0x5325c8,count=0x55a1b8,list=0x55a1d8,parent=0x55a278,type=0x55a280,
        legal=0x55a290,x=0x55a300,y=0x55a304,special_a=0x5411d0,special_b=0x5411d4,pool=0x5c8464,spot=0x5c7c44,stride=50,
        offset=0x568af8,copy=0x54d4e0,now=0x540cb8,highlight=0x5318ec,down=0x5c89c4,query=0x595344,owner=0x540c70,
        inited=0x54db24,perm=0x531890,pools=0x594fd0,read=0x40ec50),
    'originalCD':dict(root=0x43da30,lock=0x51b654,count=0x565310,list=0x565268,parent=0x51b624,type=0x51b62c,
        legal=0x51b63c,x=0x565218,y=0x56521c,special_a=0x51cabc,special_b=0x51cac0,pool=0x5395dc,spot=0x52fe48,stride=36),
}
SPECS['original1037']=dict(SPECS['originalCD'])
# 공통 상태 필드의 직렬화 순서다. signed count/순열 값은 입력 DWORD 그대로 저장한다.
FIELDS=('lock','count','flags','depth','parent','parent_depth','legal','x','y','special_a','special_b',
        'offset','copy','now','highlight','down','query','owner','first_pool','first_index','sp')

def node(sid=5,type=1,x=20,y=25):
    """raw 위치와 타입만 지정한다. 다른 필드는 미사용 바이트까지 고정 입력으로 만든다."""
    return dict(sid=sid,type=type,x=fbits(x),y=fbits(y))

def plan():
    """깊이/부분 기록/쌍 제한/배치 칸/패치 피드백의 완성 입력만 만든다."""
    result=[]
    def add(nodes=None,pieces=None,spots=None,limit=64,view=None,**fields):
        """기대 수집 결과 없이 호출/상태/희소 spot/raw 입력을 모은다."""
        nodes=[node()] if nodes is None else nodes;pieces=[n['sid'] for n in nodes] if pieces is None else pieces
        state=dict(lock=0,count=len(pieces),flags=0,depth=7,parent=0,parent_depth=25,legal=1,x=fbits(20),y=fbits(25),
            special_a=82,special_b=94,offset=0,copy=0,now=0x42a40000,highlight=0x12345678,down=0,query=0,owner=0,first_pool=1,first_index=0,sp=fbits(0))
        state.update(fields);result.append(dict(state=state,nodes=nodes,pieces=pieces,spots=spots or [],limit=limit,view=view or [fbits(v) for v in (10,20,30,40)]))
    # 하위 WORD 깊이·부모 -9와 타입별 덮어쓰기 우선순위를 구분한다.
    for flags in (0,4,0x400000,0x400004):
        for parent in (0,1):
            for depth in (0,1,9,0x7fff,0x8000,0xffff,0x10000,0xffffffff): add(flags=flags,parent=parent,depth=depth,parent_depth=depth)
    # 특수 타입은 raw 타입으로 검사하며 선택 타입 플래그와 다른 좌표/기준 깊이를 사용한다.
    for flags in (0,4,0x400000,0x400004):
        for typ in (1,82,94):
            for legal in (0,1,0xffffffff):
                for spot in (0,1,2,3,255): add([node(type=typ)],flags=flags,legal=legal,spots=[(25*256+20,spot)])
    # +0.9999의 float 경계와 CRT 하위 DWORD 주소는 기대 판정 없이 표 입력만 바꾼다.
    for x in (19.9999,20,20.000099182128906,20.00010108947754,20.25,20.9999,21,0,-.9999,float('nan'),float('inf'),4294967296.0):
        for marker in (20,21,0): add([node(type=82)],x=fbits(x),spots=[(25*256+marker,2)])
    for y in (24.9999,25,25.000099182128906,25.00010108947754,25.25,25.9999,26,0,float('nan'),float('inf'),16777216.0):
        for marker in (25,26,0): add([node(type=94)],y=fbits(y),spots=[(marker*256+20,2)])
    # 성공/실패 후보 혼합과 중복 SID의 원래 순서, 마지막 실패 후보의 부분 바이트를 관찰한다.
    mixed=[node(5,82),node(6,1,21,26),node(7,94,22,27),node(8,1,23,28)]
    for pieces in ([5],[5,6],[6,5],[5,6,7,8],[8,7,6,5],[6,6,5,8]):
        for limit in (-1,0,1,2,3,4,5): add(mixed,pieces=pieces,limit=limit,legal=0)
    # 잠금/음수 count는 슬롯/출력/표시 상태에 접근하지 않는다. viewport는 사용하지 않는다.
    for lock in (0,1,0xffffffff):
        for count in (-1,0,1): add(lock=lock,count=count,down=3,query=1,sp=fbits(8000))
    for view in ([fbits(v) for v in (100,100,-100,-100)],[0x7fc00000]*4): add(view=view)
    # 준비된 원본 SP getter가 읽을 값의 float 경계·NaN과 카운터/offset/첫 표 상태다.
    for value in (0,6999.99951171875,7000,7000.00048828125,8000,-8000,float('nan'),float('inf'),-float('inf')):
        for down in (0,1,2,0xffffffff): add(mixed,pieces=[6,8,6],down=down,query=1,sp=fbits(value),owner=8)
    for offset in (0,1,31,32,99,0xffffffff):
        for copy in (offset,offset^1): add(offset=offset,copy=copy,down=2,query=1,sp=fbits(8000))
    for first_pool in (0,1):
        for first_index in (-1,0,1): add(first_pool=first_pool,first_index=first_index,down=3,query=0)
    rng=random.Random(SEED)
    # 희소 spot과 혼합 목록/깊이/제한/준비된 저장소를 고정 시드로 넓힌다.
    for _ in range(100):
        nodes=[node(sid,rng.choice((1,82,94)),rng.uniform(-30,100),rng.uniform(-30,100)) for sid in range(5,13)]
        add(nodes,pieces=[rng.randrange(5,13) for _ in range(rng.randrange(1,9))],limit=rng.randrange(0,17),
            flags=rng.choice((0,4,0x400000,0x400004)),depth=rng.randrange(0x100000000),parent=rng.randrange(2),parent_depth=rng.randrange(0x100000000),
            legal=rng.randrange(2),spots=[(25*256+20,rng.randrange(256))],down=rng.randrange(5),query=rng.randrange(2),owner=rng.randrange(9),
            offset=rng.randrange(100),copy=rng.randrange(100),sp=fbits(rng.choice((0,7000,8000,float('nan')))))
    return result

def raw(n,edition):
    """프레임/깊이/state가 수집에 쓰이지 않는지 확인할 완성 슬롯 입력이다."""
    value=bytearray(((i*29+n['sid']*17)&255 for i in range(SPECS[edition]['stride'])))
    value[10]=n['type'];struct.pack_into('<II',value,14,n['x'],n['y']);return bytes(value)

def encoded(case,edition):
    """호스트 재생용 상태/목록/spot/슬롯과 사용하지 않는 view 인수를 직렬화한다."""
    s=case['state'];config=','.join(str(s[f]&0xffffffff) for f in FIELDS)+','+str(case['limit'])+','+','.join(map(str,case['view']))
    pieces=','.join(map(str,case['pieces'])) or '-';spots=';'.join(f'{i},{b}' for i,b in case['spots']) or '-'
    nodes=';'.join(f'{n["sid"]}={raw(n,edition).hex()}' for n in case['nodes']) or '-';return config,pieces,spots,nodes

class PreviewOracle(ProcessOracle):
    """수집/활성/CRT/SP 읽기 실제 몸체와 부분 출력·ABI·입력 보존을 감사한다."""
    def __init__(self,edition):
        """불연속 허용 범위와 격리 자료만 준비한다. 기존 호출 대체는 사용하지 않는다."""
        super().__init__(edition);self.d=SPECS[edition];self.extra=[ROOT/f'extracted/previewcollection/{edition}/{n}' for n in ('creation.c','functions.tsv')]
        self.allowed=[];self.entry_names={};self.returns=collections.Counter()
        with self.extra[1].open(encoding='utf-8') as fp:
            # 실제 함수의 불연속 범위 사이 코드는 실행하지 않는다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(DATA,0x30000)

    def on_instruction(self,mu,address,size,data):
        """응답 대체/초기화/OS/assert 경계 진입 없이 실제 허용 몸체만 실행한다."""
        if not any(a<=address and address+size<=b for a,b in self.allowed): raise RuntimeError(f'Preview 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,case,control):
        """준비한 전역/슬롯/spot/SP 배열에서 전체 원본 수집기를 정상 반환시킨다."""
        mu,d,s=self.mu,self.d,case['state'];self.entries=collections.Counter();mu.mem_write(DATA,b'\0'*0x30000);mu.mem_write(ARRAY-16,b'\xa5'*(CAPACITY*16+32))
        # 기본 포인터와 선택 타입·부모·목록은 원본 주소의 입력이다.
        for key,value in (('lock',s['lock']),('count',s['count']),('parent',GUMP if s['parent'] else 0),('type',TYPE),('legal',s['legal']),('x',s['x']),('y',s['y']),('special_a',s['special_a']),('special_b',s['special_b']),('pool',POOL),('spot',SPOTS)): self.put(d[key],value)
        self.put(GUMP+0x30,s['parent_depth']);self.put(TYPE+0x104,s['depth']);self.put(TYPE+0xec,s['flags'])
        mu.mem_write(d['list'],b'\0'*80)
        # 목록의 DWORD와 raw 슬롯 전체를 넣는다. 기대 순서/허용 여부는 만들지 않는다.
        for i,sid in enumerate(case['pieces']): self.put(d['list']+i*4,sid)
        for n in case['nodes']: mu.mem_write(POOL+n['sid']*d['stride'],raw(n,self.binary))
        for index,value in case['spots']: self.put(SPOTS+index,value,1)
        readonly=[(POOL,0x1000),(SPOTS,65536),(GUMP,0x100),(TYPE,0x200),(d['list'],80)]
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000),(ARRAY,ARRAY+CAPACITY*16)]
        if self.binary=='originals':
            # 준비된 SP 저장소만 관찰한다. 첫 포인터/순열 sentinel 입력에서는 query가 꺼져 있다.
            for key in ('offset','copy','now','highlight','down','query','owner'): self.put(d[key],s[key])
            self.put(d['inited'],1);mu.mem_write(d['perm'],struct.pack('<32h',s['first_index'],*range(1,32)))
            for k in range(32):
                self.put(d['pools']+k*4,SPDATA+k*36 if k or s['first_pool'] else 0)
                # 잡음은 실제 값 비트 밖에 두어 전체 원본 getter가 스스로 32비트를 모으게 한다.
                for owner in range(9):
                    mask=1<<((s['offset']+k)&31);value=((0xa59c37d1+k*97+owner*17)&~mask)| (mask if s['sp']&(1<<k) else 0)
                    self.put(SPDATA+k*36+owner*4,value)
            readonly.extend([(SPDATA,32*36),(d['pools'],128),(d['perm'],64)])
            self.writes.extend([(d['highlight'],d['highlight']+4),(d['down'],d['down']+4)])
        snapshots=[bytes(mu.mem_read(a,n)) for a,n in readonly]
        # 비영 보존 레지스터와 cdecl/x87/SEH 정상 반환을 확인한다.
        for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX): mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
        mu.mem_write(STACK,struct.pack('<7I',STOP,*case['view'],ARRAY,case['limit']&0xffffffff));mu.emu_start(d['root'],STOP,timeout=30000000,count=5000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or any(mu.reg_read(r)!=v for r,v in REGISTERS.items()): raise RuntimeError('Preview 반환/스택/레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff: raise RuntimeError('Preview x87/SEH 오류')
        count=mu.reg_read(UC_X86_REG_EAX);self.returns[f'{d["root"]:08x}']+=1;output=bytes(mu.mem_read(ARRAY,CAPACITY*16))
        guards=bytes(mu.mem_read(ARRAY-16,16))+bytes(mu.mem_read(ARRAY+CAPACITY*16,16))
        if count>=CAPACITY or count%2 or output[count*16+14:]!=b'\xa5'*(CAPACITY*16-count*16-14) or guards!=b'\xa5'*32: raise RuntimeError('Preview 출력/미사용/가드 오류')
        # 타입/부모/목록/spot/슬롯과 준비된 SP 입력 전체가 보존되어야 한다.
        for (address,size),before in zip(readonly,snapshots,strict=True):
            if bytes(mu.mem_read(address,size))!=before: raise RuntimeError('Preview 읽기 전용 입력 변경')
        state=[self.u32(d['highlight']),self.u32(d['down'])] if self.binary=='originals' else [s['highlight'],s['down']]
        return [*encoded(case,self.binary),str(count),output.hex(),','.join(map(str,state)),json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),guards.hex()]

def generate(smoke):
    """두 x87 정밀도의 관찰이 같은 입력만 저장한다. 기대 목록/상태를 계산하지 않는다."""
    rows=[];editions={};files={ROOT/'tools/decomp_previewcollection_oracle.py',ROOT/'tools/decomp_process_oracle.py',ROOT/'tools/decomp_drawsort_oracle.py',ROOT/'tools/ghidra/previewcollection-functions.json',FIXTURE}
    # 세 독립 PE의 전체 수집 루트와 원본 getter를 실행한다.
    for edition in BINARIES:
        oracle=PreviewOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::29]+indexed[-3:]
        for index,case in indexed:
            observed=[oracle.run(case,c) for c in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Preview x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns));files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 미리보기 수집/활성/CRT와 준비된 SP 읽기 관찰. 응답 대체 0.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),editions=editions,os_calls=0,assertions=0,boundaries=[],
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},limits=['전체 수집/활성/CRT/준비된 SP 읽기 실제 몸체, 응답 대체 0',
            '부분 14바이트 쓰기·두 레이어/원래 보조 깊이·soft 쌍 제한·+0.9999/spot DWORD 주소·판본별 강조 상태 관찰',
            '전체 출력/미사용/가드/읽기 전용 입력/ABI/x87/SEH/허용 실행·쓰기/진입·반환/SHA 감사',
            'SP lazy 초기화/실제 커서 생성·입력/preview sprite 출력/GUI 자동 프레임은 후속']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')

def verify():
    """완성 입력/SHA/전체 폭/부분 칸 이후 보존/실제 진입·반환을 저장 관찰로 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[s.split('\t') for s in FIXTURE.read_text(encoding='utf-8').splitlines() if s and not s.startswith('#')]
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Preview SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['boundaries'] or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Preview 총계 오류')
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];entries=collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Preview 행 수 오류')
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=11 or row[1]!=str(index) or tuple(row[2:6])!=encoded(case,edition): raise RuntimeError('Preview 입력 오류')
            count=int(row[6]);output=bytes.fromhex(row[7])
            if len(output)!=CAPACITY*16 or not 0<=count<CAPACITY or count%2 or output[count*16+14:]!=b'\xa5'*(CAPACITY*16-count*16-14) or row[10]!=(b'\xa5'*32).hex(): raise RuntimeError('Preview 출력 오류')
            counts=json.loads(row[9])
            if counts.get(f'{SPECS[edition]["root"]:08x}')!=1 or len(row[8].split(','))!=2: raise RuntimeError('Preview 루트/상태 오류')
            for address,count in counts.items(): entries[address]+=count*2
        if dict(entries)!=item['entries'] or item['returns']!={f'{SPECS[edition]["root"]:08x}':len(chosen)*2}: raise RuntimeError('Preview 집계 오류')
    print(f'previewcollection 검증 통과: {len(rows)}개, 두 x87 정상 반환 {len(rows)*2}회, 응답/OS/assert 0')

def main():
    """전체 생성/무저장 표본/저장 감사를 선택한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)

if __name__=='__main__': main()
