#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 보호 PE의 실제 가이저 채집·난수·수량WORD·isle 조회를 독립 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_reward_oracle import RewardOracle, ROOT, ARENA, POOL, TYPES, VTABLE, STACK, STOP, bits, digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# packet/출력 이미지와 관찰 경계는 모두 에뮬레이터 내부 주소다.
PACKET=ARENA+0x1f000
MAP=ARENA+0x40000
DESTROY,POP=0x13000800,0x13000810
FIXTURE=ROOT/'cpppj/tests/fixtures/geysermined-x86.tsv'
REPORT=ROOT/'cpppj/geysermined.json'
SPEC={
 'originals':dict(entry=0x4266c0,sound=0x4a9d70,format=0x4a89c0,create=0x4af530,rng=0x532710,boss=0x540bc4,quantity=0x4ac220,map=0x5c84bc,nugget=0x5412c0,isle=0x5411dc,empty=0x5411a8),
 'originalCD':dict(entry=0x4e41e0,sound=0x438c00,format=0x420560,create=0x4ab390,rng=0x5308ec,boss=0x540a2c,quantity=0x4abb00,map=0x52d590,nugget=0x51cbac,isle=0x51cac8,empty=0x51ca94),
}
# 음향 뒤 현재 좌표를 바꾸는 입력이다. cell은 독립 입력 표면의 놓을 위치이며 기대 출력을 계산하지 않는다.
COORDINATES={4:(-0.25,255.75,0,255),5:(256.0,41.25,40,41),6:(float('nan'),41.25,0,41),7:(4294967296.0,0.0,0,0),8:(-4294967296.0,0.0,0,0),9:(float('inf'),0.0,0,0)}


def inputs():
    """WORD 경계·분수/음수cost·free/권한·현재표면과 효과 뒤 변경을 교차한다."""
    base=list(itertools.product((0,199,200,201,400,2000,65535),(bits(0),bits(200),bits(199.5),bits(-1),bits(70000)),(0,0x12345678),(0,1),(0,1),(0,83,97),(0,)))
    # callback 뒤 수량/cost/boss/좌표·삭제 뒤 표면 변경을 현재 값으로 다시 읽는 입력이다.
    base.extend(itertools.product((200,400),(bits(200),bits(float('nan')),bits(float('inf'))),(0,0xffffffff),(0,1),(0,),(0,83,97),(1,2,3)))
    base.extend(itertools.product((200,),(bits(200),),(0,0xffffffff),(0,),(1,),(0,83,97),COORDINATES))
    # 원본 음향 뒤 타입이 Bridge로 바뀐 경우 setter 기록 뒤 checking/buried assert의 판본 차이를 검사한다.
    base.extend(itertools.product((200,400),(bits(0),bits(200)),(0,0xffffffff),(0,),(0,1),(0,),(10,11,12,13)))
    return list(dict.fromkeys(base))


class MinedOracle(RewardOracle):
    """PE 로더만 재사용하고 실제 실행/쓰기 범위는 채집 handler의 고유 목록으로 제한한다."""
    def __init__(self,edition):
        """최신 정밀 함수 범위와 명시 음향/가상 삭제/생성/Pop 경계를 준비한다."""
        super().__init__(edition);self.p=SPEC[edition];self.allowed=[];self.entry_names={};self.mu.mem_map(MAP,0x20000);self.returns=0
        self.stub_addresses={0x4e0620} if self.patch else set()
        self.manifest=ROOT/'tools/ghidra/geysermined-functions.json';self.table=ROOT/f'extracted/geysermined/{edition}/functions.tsv';self.source=ROOT/f'extracted/geysermined/{edition}/creation.c'
        entries={int(e,16) for e in json.loads(self.manifest.read_text(encoding='utf-8'))['editions'][edition]}
        with self.table.open(encoding='utf-8') as stream:
            # 내보낸 실제 불연속 몸체만 허용한다. 전체 PE나 외부 OS 함수는 실행하지 않는다.
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) not in entries:continue
                self.entry_names[int(row['entry'],16)]=f"{int(row['entry'],16):08x}"
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.writes=[(ARENA,ARENA+0x60000),(STACK-0xf000,STACK+0x1000),(self.p['rng'],self.p['rng']+4)]
        self.boundaries={self.p['sound']:'sound',self.p['format']:'format',self.p['create']:'create',DESTROY:'destroy',POP:'pop'}

    def on_instruction(self,mu,address,size,data):
        """난수/차감/WORD/isle 판단은 실제 실행하며 외부효과만 현재 인자로 기록한다."""
        boundary=getattr(self,'boundaries',{}).get(address)
        if address==0x4e0620 and self.patch:
            self.events.append('A');return super().on_instruction(mu,address,size,data)
        if not boundary:return super().on_instruction(mu,address,size,data)
        self.stub_hits[boundary]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.u32(sp);this=mu.reg_read(UC_X86_REG_ECX);a=[self.u32(sp+4*i) for i in range(1,5)];result=0;purge=0
        if boundary=='format':
            self.choice=a[2];text=f'geyserMined{self.choice}.wav'.encode()+b'\0';mu.mem_write(a[0],text);result=a[0]
        elif boundary=='sound':
            self.events.append('S:'+','.join(map(str,(a[0],a[1],self.choice,a[3]))));assert a[3]==0
            # 입력 callback 효과만 공급하고 handler의 재조회/분기는 원본이 수행한다.
            if self.mutation==1:
                self.put(self.slot+12,400,2);self.put(self.p['nugget'],155);self.put(TYPES+155*self.s['type_stride']+0xc4,bits(199.5));self.put(self.p['boss'],1)
            elif self.mutation==2:
                self.put(self.slot+14,bits(42.25));self.put(self.slot+18,bits(43.75));self.put(self.slot+12,200,2)
            elif self.mutation in COORDINATES:
                x,y,_,_=COORDINATES[self.mutation];self.put(self.slot+14,bits(x));self.put(self.slot+18,bits(y));self.put(self.slot+12,200,2)
            elif self.mutation>=10:
                self.put(self.slot+10,82,1);self.put(self.slot+40 if self.patch else self.slot+35,8 if self.mutation>=12 else 0,1)
                if self.patch:self.put(0x5e4794,int(self.mutation in (11,12)))
        elif boundary=='destroy':
            assert this==self.slot and a[0]==0x1000;purge=4;self.events.append('D:'+str(a[0]));self.put(this+11,self.mu.mem_read(this+11,1)[0]|2,1)
            if self.mutation==3:
                x,y=(int(struct.unpack('<f',mu.mem_read(this+o,4))[0]) for o in (14,18));cell=MAP+(y*256+x)*2
                self.put(cell,0 if self.surface==97 else 8,2);self.put(POOL+8*self.s['stride']+10,97,1)
        elif boundary=='create':
            assert a[:2]==[84,0];self.events.append('C:'+','.join(map(str,a[:2])));result=POOL+7*self.s['stride'];self.put(result,VTABLE);self.put(result+10,a[0],1);self.put(result+11,4,1)
        elif boundary=='pop':
            assert this==POOL+7*self.s['stride'] and a[2]==0;purge=12;self.events.append('P:'+','.join(map(str,a[:3])));self.put(this+14,a[0]);self.put(this+18,a[1]);self.put(this+11,0,1)
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case,control=0x027f):
        """packet의+4 SID와 현재raw 입력만 공급하고 실제 출력을 읽는다."""
        quantity,cost,seed,free,boss,surface,mutation=case;self.mutation=mutation;self.surface=surface;self.events=[];self.slot=POOL+6*self.s['stride'];self.mu.mem_write(ARENA,bytes(0x60000))
        self.put(self.s['pool_ptr'],POOL);self.put(self.s['type_ptr'],TYPES);self.put(self.p['map'],MAP);self.put(self.p['rng'],seed);self.put(self.p['boss'],boss);self.put(self.p['nugget'],154);self.put(self.p['isle'],97);self.put(self.p['empty'],84)
        self.put(0x5411a0 if self.patch else 0x51ca8c,82)
        if self.patch:self.put(0x5e4794,0)
        self.put(PACKET+4,6,2);self.put(self.slot,VTABLE);self.put(self.slot+10,122,1);self.put(self.slot+11,free,1);self.put(self.slot+12,quantity,2);self.put(self.slot+14,bits(40.75));self.put(self.slot+18,bits(41.25));self.put(TYPES+154*self.s['type_stride']+0xc4,cost)
        self.put(POOL+8*self.s['stride']+10,surface,1);self.put(MAP+(41*256+40)*2,8 if surface else 0,2);self.put(VTABLE+0x10,DESTROY);self.put(VTABLE+0x90,POP)
        if mutation in COORDINATES:
            _,_,x,y=COORDINATES[mutation];self.put(MAP+(y*256+x)*2,8 if surface else 0,2)
        self.call(self.p['entry'],[0,0,PACKET],control);self.returns+=1
        raw=bytes(self.mu.mem_read(self.slot,self.s['stride']));born=bytes(self.mu.mem_read(POOL+7*self.s['stride'],self.s['stride']))
        return [self.u32(self.p['rng']),';'.join(self.events) or '-',raw.hex(),born.hex()]


def generate(smoke=False):
    """두 정밀도 결과가 같은 고유입력만 기록하고 보호PE/현재소스/정밀범위SHA를 고정한다."""
    cases=inputs();cases=cases[::137] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    for edition in SPEC:
        oracle=MinedOracle(edition)
        # 두 실제 PE는 같은 입력을 독립적으로 수행한다.
        for index,case in enumerate(cases):
            try:
                result=oracle.run(case);assert oracle.run(case,0x037f)==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,','.join(map(str,case)),*result])
        assertion_cases=sum('A' in row[3].split(';') for row in rows if row[0]==edition)
        editions[edition]=dict(cases=len(cases),normal_cases=len(cases)-assertion_cases,assertion_cases=assertion_cases,returns=oracle.returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_hits),asserts=oracle.asserts);files.update((oracle.manifest,oracle.table,oracle.source,ROOT/f'{edition}/{"Netstorm.exe" if edition=="originals" else "NETSTORM.EXE"}'))
    FIXTURE.write_text('# edition quantity,cost,seed,free,boss,surface,mutation rng events raw born\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.update(ROOT/name for name in ('cpppj/src/o/RawGeyserMined.h','cpppj/src/o/RawGeyserMined.cpp','cpppj/tests/GeyserMinedTests.cpp','tools/decomp_reward_oracle.py','cpppj/src/o/SpStore.h','cpppj/src/o/SpStore.cpp','cpppj/src/o/RawPacketDispatch.h','cpppj/src/o/RawPacketDispatch.cpp','cpppj/src/o/RawPacketLengths.h'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',total=len(rows),normal_total=sum(e['normal_cases'] for e in editions.values()),assertion_total=sum(e['assertion_cases'] for e in editions.values()),editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files) if p.exists()},game_runs=0,limits=['음향/서식정리·가상삭제/생성/Pop과 assert 보고는 명시 관찰 경계다. 패치 assert 입력8개는 기록 뒤 C++예외로 대조하며 정상 입력에 더하지 않는다. 실제 상위Carrier도착/채집예약/packet송신은 미연결이다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('가이저 채집 native 입력:',len(rows))


def verify():
    """현재입력/출처SHA·실제handler/난수/수량setter/isle 판단·두정밀도반환을 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[r.split('\t') for r in FIXTURE.read_text(encoding='utf-8').splitlines() if r and not r.startswith('#')]
    assert report['total']==len(rows)==2*len(inputs()) and report['game_runs']==0
    assert report['normal_total']==2304 and report['assertion_total']==8
    for name,expected in report['files'].items():assert digest(ROOT/name)==expected,name
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];assert [r[1] for r in chosen]==[','.join(map(str,c)) for c in inputs()];assert item['returns']==2*len(chosen)
        assert item['entries'][f'{SPEC[edition]["entry"]:08x}']==item['returns'];assert set(item['boundaries'])<= {'format','sound','destroy','create','pop'}
        assert item['assertion_cases']==sum('A' in row[3].split(';') for row in chosen) and item['asserts']==2*item['assertion_cases']
        assert item['normal_cases']+item['assertion_cases']==len(chosen)
    print('가이저 채집 SHA/실제몸체/반환 감사:',len(rows))


# 명시 호출에서만 native 관찰 생성 또는 저장 근거 감사를 수행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
