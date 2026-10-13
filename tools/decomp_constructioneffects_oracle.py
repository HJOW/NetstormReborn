#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""건설 밀어내기/섬 실패 정리를 세 PE의 제한 x86으로 대조한다. 게임/OS를 실행하지 않는다.

실제 몸체: 밀어내기/섬 정리 전체, 발자국/좌표/절삭, 일반 finder, 타입 genus, spot 검사.
명시 경계: 경로 타입/종속 삭제, 가상 Unpop/Pop/삭제/알림, 추락/소리, 메시지, assert 보고.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, POOL, STOP, digest
from decomp_constructionclear_oracle import ClearOracle, clear_case, MAP, OTHER, node_text
from decomp_constructionplace_oracle import VTABLE, POP, NOISLAND, ISLAND
from decomp_damageablepredestroy_oracle import CONTROLS, bits
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EAX

# 판본별 루트/경계/전역 주소다. 포인터를 원본 프로세스에서 실행하지 않는다.
SPEC={
 'originals':dict(displace=0x443800,drop=0x46e6c0,path=0x488fe0,clear=0x4b1a90,fall=0x460600,sound=0x4a9d70,
    init=0x40a740,send=0x4e2e30,blocked=0x54db84,server=0x540bc0,spots=0x5c7c44,
    island=0x5411d0,noIsland=0x5412cc,stalag=0x5411d4,stump=0x5411d8,alliance=0x595200),
 'originalCD':dict(displace=0x4d2450,drop=0x48f960,path=0x47d020,clear=0x4eb310,fall=0x454810,sound=0x438c00,
    init=0x40a3d0,send=0x4c0420,blocked=0x52e92c,server=0x540a28,spots=0x52fe48,
    island=0x51cabc,noIsland=0x51cbb8,stalag=0x51cac0,stump=0x51cac4),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 분석 가상 주소와 경로 타입 경계의 응답 토큰, fixture 칸 순서다.
SPOTS,UNPOP,NOTIFY=0x14500000,STOP+0x500,STOP+0x600
PATH_TYPE=0x12345678
KEYS=('type','x','y','footX','footY','player','flags','blocked','server','spot','genus','owner','alliance','profile','mutate')
FIXTURE=ROOT/'cpppj/tests/fixtures/constructioneffects-x86.tsv'
REPORT=ROOT/'cpppj/constructioneffects.json'


def case(**changes):
    """기본 입력을 만든다. 기대 효과는 계산하지 않는다."""
    result=dict(type=100,x=bits(20.5),y=bits(21.25),footX=3,footY=3,player=3,flags=0x1000,blocked=0,
        server=1,spot=2,genus=0x10000,owner=3,alliance=0,profile=1,mutate=0)
    result.update(changes);return result


def inputs(edition):
    """판본 차이·탐색 순서/거리·적대 거부·섬 삭제/보고·현재 자료 재조회를 교차한다."""
    result=[]
    # 이동 후보 genus와 소유자/동맹, spot 비트/위치, 발자국 축소를 교차한다.
    for genus,owner,alliance,spot,foot in itertools.product((0,0x8000,0x10000,0x200000,0x218000),
            (3,4),(0,1),(0,2,4,6,0x12,0x10,7,8,9,10,11,12),((1,1),(3,2),(4,4))):
        result.append(('Displace',case(genus=genus,owner=owner,alliance=alliance,spot=spot,footX=foot[0],footY=foot[1])))
    # 여러 후보/범위 가장자리/빈 finder를 별도 입력한다. 밀어내기 spot 조회는 지도 안에서 수행한다.
    for profile,xy,foot in itertools.product((0,2,3,4),((20.0,21.0),(20.00001,21.00001),(6.0,6.0),(250.0,250.0)),((1,1),(4,2))):
        result.append(('Displace',case(profile=profile,x=bits(xy[0]),y=bits(xy[1]),footX=foot[0],footY=foot[1],spot=2 if 5<xy[0]<250 else 13)))
    # 일부 후보를 옮긴 뒤 거부하는 경우와 자리 없는 적대 후보는 거부하지 않는 경우다.
    for genus,owner,spot in itertools.product((0x10000,0x8000,0x200000),(3,4),(0,2,9)):
        result.append(('Displace',case(profile=2,genus=genus,owner=owner,spot=spot)))
    # 섬 정리: 전파 억제/전체 차단, 추락 억제, 소유자 BYTE와 플레이어 DWORD, 조각 수 보고.
    for profile,flags,blocked,server,player in itertools.product(range(8),(0,0x1000,0x50,0x10,0x40,0x10000000),
            (0,1),(0,1),(3,0x103)):
        result.append(('Drop',case(profile=profile,flags=flags,blocked=blocked,server=server,player=player)))
    # 큰 모서리 거의 올림/지도 자르기·현재 좌표/삭제 뒤 genus 재조회도 독립 관찰한다.
    for xy,mutate,genus in itertools.product(((20.00001,21.00001),(20.00011,21.00011),(1.0,1.0),(255.99998,254.5)),(0,1,2),(0,0x10000)):
        result.append(('Drop',case(profile=3,flags=0,x=bits(xy[0]),y=bits(xy[1]),mutate=mutate,genus=genus)))
    return result


def spots(profile):
    """spot 지도를 입력한다. 7~12는 탐색 위치 한 칸만 열어 순서/판본 거리를 구분한다."""
    positions={7:(20,22),8:(19,22),9:(20,24),10:(21,21),11:(23,21),12:(19,23),13:(1,2)}
    if profile not in positions:return bytes([profile])*65536
    data=bytearray(65536)
    x,y=positions[profile];data[y*256+x]=2;return bytes(data)


def nodes(op,c):
    """독립 후보 입력을 만든다. 일반 finder가 걸러 낼 조건까지 입력에 포함한다."""
    result=[];x,y=(struct.unpack('<f',struct.pack('<I',c[k]))[0] for k in ('x','y'))
    x=min(max(x,1),255);y=min(max(y,1),255)
    if not c['profile']:return result
    if op=='Displace':
        entries=[(OTHER, c['genus'],c['owner'],0,x,y)]
        if c['profile']==2:entries=[(OTHER,0x10000,3,0,x,y),(OTHER+1,c['genus'],c['owner'],0,x,y)]
        if c['profile']==3:entries=[(OTHER,c['genus'],c['owner'],8,x,y)]
        if c['profile']==4:entries=[(OTHER,c['genus'],c['owner'],0,max(x-3,1),max(y-1,1))]
    else:
        # full은 원본 9/1/1 조건을 만족한다. type 96은 카운트 없이 삭제한다.
        full=[NOISLAND]*9+[ISLAND,95,96,OTHER]
        entries=[(OTHER,c['genus'],c['owner'],0,x,y)]
        if c['profile']==2:entries=[(NOISLAND,c['genus'],c['owner'],0,x,y)]
        if c['profile'] in (3,4,5):entries=[(t,c['genus'],4 if c['profile']==4 else c['owner'],8 if c['profile']==5 else 0,x,y) for t in full]
        if c['profile']==6:entries=[(95,c['genus'],c['owner'],0,x,y),(96,c['genus'],c['owner'],0,x,y)]
        if c['profile']==7:entries=[(ISLAND,c['genus'],c['owner'],0,x,y)]
    # 레벨을 순서대로 바꿔 finder의 모든 해시 단계를 사용한다.
    for i,(typ,genus,owner,extra,nx,ny) in enumerate(entries):
        result.append(dict(sid=50+i,type=typ,extra=extra,x=bits(nx),y=bits(ny),owner=owner,frame=0,level=i%4,genus=genus))
    return result


class EffectsOracle(ClearOracle):
    """실제 전체 몸체/탐색/spot/genus를 실행하고 효과와 보고 경계만 기록한다."""
    def __init__(self,edition):
        """새 내보내기의 몸체 범위와 spot/가상 효과 주소를 준비한다."""
        super().__init__(edition);self.r=SPEC[edition];mu=self.mu;mu.mem_map(SPOTS,65536)
        self.exports=[ROOT/f'extracted/constructioneffects/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 포함 끝 주소를 반열린 실행 범위로 바꾼다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 불연속 범위를 각각 허용한다.
                for part in row['ranges'].split(';'):
                    lo,hi=(int(v,16) for v in part.split('-'));self.allowed.append((lo,hi+1))
        mu.mem_write(self.r['spots'],struct.pack('<I',SPOTS))
        mu.mem_write(VTABLE+0x48,struct.pack('<I',UNPOP));mu.mem_write(VTABLE+0xc8,struct.pack('<I',NOTIFY))

    def event(self,letter,values=()):
        """경계 호출을 인자 비트와 함께 기록한다."""
        self.events.append(letter+(':'+':'.join(map(str,values)) if values else ''));self.stub_calls[letter]+=1

    def on_instruction(self,mu,address,size,data):
        """효과만 대체한다. 실제 탐색 범위/후보 조건/보고 분기는 재계산하지 않는다."""
        r=self.r
        sid=self.sid(mu.reg_read(UC_X86_REG_ECX)) if address in (UNPOP,POP,STOP+0x400,NOTIFY) else 0
        if address==r['path']:self.event('R',self.args(1));self.ret(PATH_TYPE);return
        if address==r['clear']:self.event('C',self.args(4));self.ret();return
        if address==UNPOP:self.event('U',[sid,*self.args(1)]);self.ret(purge=4);return
        if address==POP:self.event('P',[sid,*self.args(3)]);self.ret(purge=12);return
        if address==r['fall']:
            current,=self.args(1);self.event('F',[current])
            if self.case['mutate']==1:mu.mem_write(self.slot(current)+14,struct.pack('<2f',1.25,2.5))
            self.ret();return
        if address==r['sound']:
            x,y,name,loop=self.args(4)
            if bytes(mu.mem_read(name,15)).split(b'\0')[0]!=b'islandFall.wav':raise RuntimeError('추락 소리 이름 오류')
            self.event('S',[x,y,loop]);self.ret();return
        if address==STOP+0x400:
            self.event('K',[sid,*self.args(1)])
            if self.case['mutate']==2:
                typ=mu.mem_read(self.slot(sid)+10,1)[0];mu.mem_write(TYPES+typ*self.type_stride+0xec,struct.pack('<I',0x10000))
            self.ret(purge=4);return
        if address==NOTIFY:self.event('N',[sid]);self.ret();return
        if address==r['init']:self.event('I');self.ret(mu.reg_read(UC_X86_REG_ECX));return
        if address==r['send']:
            buffer,=self.args(1);self.event('B',struct.unpack('<4I',mu.mem_read(buffer+4,16)));self.ret();return
        if address==self.spec['assert_report']:
            expression,source,line=self.args(3);text=bytes(mu.mem_read(expression,32)).split(b'\0')[0].decode('ascii')
            self.event('E',[line]);self.reported[text]+=1;self.ret();return
        if address==self.x['begin'] and self.observing:
            self.events.append('A:'+':'.join(map(str,self.args(4))));self.areas+=1
        OwnerOracle.on_instruction(self,mu,address,size,data)

    def run_effect(self,op,c,ns,control):
        """입력 준비 뒤 전체 루트를 정상 반환까지 실행하고 ABI/x87/입력 불변을 확인한다."""
        seed=clear_case(type=c['type'],x=c['x'],y=c['y'],player=c['player'],footX=c['footX'],footY=c['footY'])
        self.prepare(seed,ns);self.case=c;mu=self.mu;r=self.r
        # 섬 관련 현재 전역과 타입 발자국/후보 genus를 공급한다.
        for key,value in [('island',ISLAND),('noIsland',NOISLAND),('stalag',95),('stump',96),('blocked',c['blocked']),('server',c['server'])]:
            mu.mem_write(r[key],struct.pack('<I',value))
        mu.mem_write(TYPES+ISLAND*self.type_stride+self.s['foot'],struct.pack('<2i',c['footX'],c['footY']))
        # 모든 후보 타입의 발자국은 1×1이다. 섬 타입 자체는 요청의 섬 발자국을 유지한다.
        for n in ns:
            record=TYPES+n['type']*self.type_stride
            if n['type']!=ISLAND:mu.mem_write(record+self.s['foot'],struct.pack('<2i',1,1))
            mu.mem_write(record+0xec,struct.pack('<I',n['genus']))
        if self.patch:mu.mem_write(r['alliance'],struct.pack('<81I',*([c['alliance']]*81)))
        mu.mem_write(SPOTS,spots(c['spot']));self.write_ranges=[];self.observing=True
        before=[bytes(mu.mem_read(a,s)) for a,s in ((MAP,self.map_end-MAP),(SPOTS,65536))]
        try:self.invoke(r['displace' if op=='Displace' else 'drop'],[c['x'],c['y'],c['type'],c['player']] if op=='Displace' and self.patch else
            [c['x'],c['y'],c['type']] if op=='Displace' else [c['x'],c['y'],c['flags'],c['player']],control)
        finally:self.observing=False
        if before!=[bytes(mu.mem_read(a,s)) for a,s in ((MAP,self.map_end-MAP),(SPOTS,65536))]:raise RuntimeError('지도 입력 변경')
        return [mu.reg_read(UC_X86_REG_EAX) if op=='Displace' and self.patch else '-', ';'.join(self.events) or '-']


def generate(smoke=False):
    """세 PE와 두 x87 정밀도의 관찰을 기록한다. C++는 저장한 기대값만 재생한다."""
    rows=[];editions={};paths={Path(__file__),FIXTURE,ROOT/'tools/ghidra/constructioneffects-functions.json',
        *[ROOT/f'tools/decomp_{n}_oracle.py' for n in ('owner','priestgeometry','constructionplace','constructionclear','damageablepredestroy')]}
    # 각 PE의 실제 명령을 별도 에뮬레이터에서 실행한다.
    for edition in SPECS:
        oracle=EffectsOracle(edition);oracle.reported=collections.Counter();plan=inputs(edition);counts=collections.Counter();node_count=0
        if smoke:plan=plan[::47]
        # 같은 입력을 두 정밀도로 독립 실행하고 차이가 있으면 저장하지 않는다.
        for op,c in plan:
            ns=nodes(op,c);observed=[oracle.run_effect(op,c,ns,control) for control in CONTROLS]
            if observed[0]!=observed[1]:raise RuntimeError(f'x87 관찰 차이 {edition} {op} {c}')
            nt=';'.join(','.join(str(n[k]) for k in ('sid','type','extra','x','y','owner','frame','level','genus')) for n in ns) or '-'
            rows.append([edition,op,','.join(str(c[k]) for k in KEYS),nt,*observed[0]]);counts[op]+=1;node_count+=len(ns)
        editions[edition]=dict(cases=dict(counts),nodes=node_count,areas=oracle.areas,returns=dict(oracle.returns),
            native_calls=dict(oracle.native_calls),substitutions=dict(oracle.stub_calls),reported=dict(oracle.reported),assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: constructioneffects {len(plan)}개 통과',flush=True)
    if smoke:return
    header=['# 실제 밀어내기/섬 정리 전체·발자국/finder/spot/genus. 이동/삭제/표시/메시지/assert 보고는 명시 경계.',
        '# 판본 연산 입력('+ ' '.join(KEYS)+') 후보(번호,타입,extra,x,y,소유자,프레임,해시단계,genus) 반환 사건']
    FIXTURE.write_text('\n'.join(header)+'\n'+'\n'.join('\t'.join(map(str,r)) for r in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),os_calls=0,
        editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=[
        '실제 루트·발자국/좌표/절삭·일반 finder·spot·현재 genus, 정상 반환 스택/보존 레지스터/x87',
        '경로 타입/contained 삭제·가상 공간 이동/삭제/알림·추락/소리·메시지 초기화/전파·assert 보고는 명시 경계',
        '합성 지도/풀/타입/관계 표, 유한 시작 좌표/발자국, 0..8 밀어내기 관계 인자',
        'fall 좌표 변경/destroy genus 변경은 경계가 입력을 바꾸는 관찰; 원본 게임/GUI/OS 미실행']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'total: {len(rows)}',flush=True)


def verify():
    """기대 효과를 재계산하지 않고 SHA·입력 순서·ABI 반환/경계 사건 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[l.split('\t') for l in FIXTURE.read_text(encoding='utf-8').splitlines() if l and not l.startswith('#')]
    # 근거의 모든 파일이 생성 당시 그대로인지 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('근거 합계 오류')
    # 판본별 입력과 원본 루트/사건의 집계를 맞춘다.
    for edition,item in report['editions'].items():
        chosen=[r for r in rows if r[0]==edition];plan=inputs(edition);counts=collections.Counter();events=[];node_count=0
        if len(chosen)!=len(plan) or item['assertions']:raise RuntimeError('행/원본 보고 몸체 실행 오류')
        # 후보 입력은 생성 계획과 비교하되 결과는 저장된 사건만 센다.
        for row,(op,c) in zip(chosen,plan,strict=True):
            if len(row)!=6 or row[1]!=op or row[2].split(',')!=[str(c[k]) for k in KEYS]:raise RuntimeError('입력 순서 오류')
            ns=nodes(op,c);expected=';'.join(','.join(str(n[k]) for k in ('sid','type','extra','x','y','owner','frame','level','genus')) for n in ns) or '-'
            if row[3]!=expected or row[4] not in (('0','1') if op=='Displace' and edition=='originals' else ('-',)):raise RuntimeError('후보/반환 오류')
            counts[op]+=1;node_count+=len(ns);events.extend(row[5].split(';'))
        if dict(counts)!=item['cases'] or node_count!=item['nodes'] or item['areas']!=2*sum(e.startswith('A:') for e in events):raise RuntimeError('후보/범위/연산 합계 오류')
        # 두 정밀도에서 정상 진입/반환한 루트 수를 검사한다.
        for op,key in [('Displace','displace'),('Drop','drop')]:
            addr=f'{SPEC[edition][key]:08x}'
            if item['native_calls'].get(addr,0)!=2*counts[op] or item['returns'].get(addr,0)!=2*counts[op]:raise RuntimeError('루트 반환 수 오류')
        # 경계의 실제 몸체는 실행하지 않으며 저장 사건과 대체 횟수가 같다.
        for letter in 'RCUPFSKNIBE':
            if item['substitutions'].get(letter,0)!=2*sum(e==letter or e.startswith(letter+':') for e in events):raise RuntimeError('경계 합계 오류: '+letter)
        if sum(item['reported'].values())!=item['substitutions'].get('E',0):raise RuntimeError('보고 합계 오류')
        if any(item['native_calls'].get(f'{SPEC[edition][k]:08x}',0) for k in ('path','clear','fall','sound','init','send')):raise RuntimeError('경계 몸체 실행')
    print(f'constructioneffects 검증 통과: {len(rows)}개')


def main():
    """전체 생성·무저장 표본·저장 근거 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)


if __name__=='__main__':main()
