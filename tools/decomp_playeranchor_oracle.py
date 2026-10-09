#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 PE의 Player 후보 수집/거리 선택/그래프/contained 몸체를 실제로 실행한다.

CRT 임시 160바이트 할당/반납만 명시 대체하며 게임/OS를 실행하지 않는다.
"""
import argparse
import csv
import itertools
import json
import struct
import zlib
from pathlib import Path
from decomp_owner_oracle import (OwnerOracle,ROOT,SPECS,TYPES,PLAYERS,LISTS,POOL,STACK,STOP,digest,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,
    UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS)
from decomp_damageablepredestroy_oracle import CONTROLS
from unicorn.x86_const import UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 임시 배열·surface 지도·별도 추가 목록/직접 거리 선택용 배열은 격리 주소다.
HEAP,MAP,EXTRAS,DIRECT=0x11400000,0x11500000,0x11319000,0x11319800
# 실제 entry/전역/CRT 대체 진입 주소다. 후보·거리·그래프·종속 조회는 대체하지 않는다.
SPEC={
 'originals':dict(entry=0x48fdb0,nearest=0x48fd30,at=0x462b80,of=0x462bc0,allocate=0x4e4391,free=0x4e4354,
    ready=0x540414,invalid=0x540410,ignore=0x54db14,map=0x542514,extras=0x55a70c,count=0x55a714,
    contained=0x541088,boss=0x540bc4,checking=0x5e4794,fence=0x5411c0,archer=0x541288),
 'originalCD':dict(entry=0x406f80,nearest=0x406ef0,at=0x45b670,of=0x45b6f0,allocate=0x4f1650,free=0x407c50,
    ready=0x5207f8,invalid=0x5207f4,ignore=0x52e8a8,map=0x5670cc,extras=0x5670e0,count=0x5670e8,
    contained=0x51c978,boss=0x540a2c),
}
SPEC['original1037']=dict(SPEC['originalCD'])
# 새 기대값과 근거는 기존 fixture/도구와 분리한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/playeranchor-x86.tsv'
REPORT=ROOT/'cpppj/recovery-playeranchor-evidence.json'
KEYS=('kind','owner','type','ready','invalid','ignore','genus','mapGraph','mapFilled','x','y','workshops','additional','nodes','sid')


def bits(value):
    """좌표 입력만 float32 비트값으로 변환한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def encode(nodes):
    """입력 슬롯을 SID/type/owner/graph/x/y/head/next/kind/mask 순서로 직렬화한다."""
    return ';'.join(','.join(map(str,node)) for node in nodes)


def inputs():
    """목록·종속·그래프·소유자·거리 경계를 교차하며 기대 반환은 계산하지 않는다."""
    result=[]
    # 전체 collector의 초과/중복 목록과 별도 추가 목록 진입 조건을 교차한다.
    for index,values in enumerate(itertools.product((0,1,2,8,40,44),(0,1,8),(0,2,4,0x4000),(0,1),(0,1,2),(0,1,2))):
        count,extra,genus,ignore,links,mode=values;owner=(0,1,2,8)[index%4];nodes=[]
        # surface/비표면 작업장·소유자 불일치·그래프 불일치를 섞은 raw 입력을 만든다.
        for n in range(66):
            sid=50+n;typ=82 if n%3 else 83;current=owner if n%5 else (owner+1)%9
            graph=7 if n%3 else 3;head=119 if links==1 or (links==2 and n%2) else 118 if links==2 else 0
            nodes.append([sid,typ,current,graph,bits(20+n%7/4),bits(21+n%5/4),head,0,0,0])
        nodes += [[116,82,owner,(7,7,254)[mode],bits(20),bits(21),0,0,0,0],
            [118,5,0,0,0,0,0,120,0,16],[120,6,0,0,0,0,0,0,82,0],[119,6,0,0,0,0,0,0,82,16]]
        workshops=list(range(50,50+count));additional=list(range(106,106+extra))
        if count>2 and index%5==0:workshops[2]=workshops[0]
        if extra>1 and index%7==0:additional[1]=50
        result.append(dict(kind='L',owner=owner,type=82,ready=int(index%13!=0),invalid=(254,0,7)[index%3],ignore=ignore,
            genus=genus,mapGraph=(7,7,254)[mode],mapFilled=int(mode!=1),x=bits(20.25),y=bits(21.5),
            workshops=','.join(map(str,workshops)),additional=','.join(map(str,additional)),nodes=encode(nodes),sid=50))
    # 직접 그래프 helper의 활성/표면·비표면/SID 0·지도 밖·절삭 경계를 관찰한다.
    for ready,filled,graph,coordinate in itertools.product((0,1),(0,1),(0,7,254),((-0.5,21),(20.9999,21.5),(255.9,255.9),(-1,21),(256,21))):
        nodes=[[50,82,1,graph,bits(20.5),bits(21.5),0,0,0,0],[51,83,1,3,bits(coordinate[0]),bits(coordinate[1]),0,0,0,0],
            [116,82,1,graph,bits(20),bits(21),0,0,0,0]]
        base=dict(owner=1,type=82,ready=ready,invalid=254,ignore=0,genus=2,mapGraph=graph,mapFilled=filled,
            x=bits(coordinate[0]),y=bits(coordinate[1]),workshops='',additional='',nodes=encode(nodes),sid=0)
        result.append(dict(base,kind='G'))
        # surface/비표면/번호 0의 서로 다른 그래프 경로를 같은 장면에서 실행한다.
        for sid in (0,50,51):result.append(dict(base,kind='S',sid=sid))
    # 거리 선택의 순서/단정도 최소값·9999 경계·소유자 필터/NaN을 독립 실행한다.
    for index,(position,count,owner) in enumerate(itertools.product((0,1,2,3,4),(0,1,2,5),(0,1,2))):
        coordinates=[(20,21),(20.00001,21),(21,22),(10019,21),(float('nan'),21)]
        nodes=[]
        # 같은 거리와 경계 거리, 소유자가 다른 후보를 목록 순서에 넣는다.
        for n in range(5):
            x,y=coordinates[(position+n)%5];nodes.append([50+n,82,n%3,7,bits(x),bits(y),0,0,0,0])
        result.append(dict(kind='N',owner=owner,type=82,ready=1,invalid=254,ignore=0,genus=2,mapGraph=7,mapFilled=1,
            x=bits(20),y=bits(21),workshops=','.join(str(50+n) for n in range(count)),additional='',nodes=encode(nodes),sid=0))
    # 같은 sqrt(5) 거리의 float 최소값 저장을 실제 원본에 관찰시킨다. 기대 SID는 계산하지 않는다.
    for x,y in ((21,23),(18,20)):
        nodes=[[sid,82,1,7,bits(x),bits(y),0,0,0,0] for sid in (50,51)]
        result.append(dict(kind='N',owner=1,type=82,ready=1,invalid=254,ignore=0,genus=2,mapGraph=7,mapFilled=1,
            x=bits(20),y=bits(21),workshops='50,51',additional='',nodes=encode(nodes),sid=0))
    return result


class PlayerAnchorOracle(OwnerOracle):
    """허용한 함수 전체를 실행하고 CRT 메모리 경계만 기록 대체한다."""
    def __init__(self,edition):
        """현재 호스트의 새 내보내기와 가상 FS/힙/지도를 준비한다."""
        super().__init__(edition);self.s=SPEC[edition];self.mu.mem_map(0,0x1000);self.mu.mem_map(HEAP,0x1000);self.mu.mem_map(MAP,0x20000)
        self.exports=[ROOT/f'extracted/playeranchor/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries=set();self.substitutions={'allocate':0,'free':0}
        with self.exports[1].open(encoding='utf-8') as fp:
            # 함수 전체의 실제 불연속 코드 범위를 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 함수마다 모든 분리 범위를 실행 목록에 더한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def on_write(self,mu,access,address,size,value,data):
        """힙 DWORD 쓰기에서 실제 후보 개수와 순서를 관찰한다."""
        if HEAP<=address<HEAP+160 and size==4:self.filled=max(self.filled,(address-HEAP)//4+1)
        super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """160바이트 확보/반납만 명시 대체하며 코드 몸체는 원본을 실행한다."""
        s=self.s
        if address in (s['allocate'],s['free']):
            sp=mu.reg_read(UC_X86_REG_ESP)
            if address==s['allocate']:
                if struct.unpack('<I',mu.mem_read(sp+4,4))[0]!=160:raise RuntimeError('힙 요청 크기 오류')
                self.substitutions['allocate']+=1;result=HEAP
            else:
                pointer=struct.unpack('<I',mu.mem_read(sp+4,4))[0] if self.edition=='originals' else struct.unpack('<I',mu.mem_read(mu.reg_read(UC_X86_REG_ECX),4))[0]
                if pointer!=HEAP:raise RuntimeError('힙 반납 주소 오류')
                self.substitutions['free']+=1;result=0
            mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(sp,4))[0]);mu.reg_write(UC_X86_REG_ESP,sp+4);return
        super().on_instruction(mu,address,size,data)

    def run(self,case,control):
        """실제 반환·임시 후보 목록·전체 raw/지도·FS/x87/ABI를 관찰한다."""
        mu,s=self.mu,self.s;self.filled=0;self.instructions=0;mu.mem_write(0,bytes(4));mu.mem_write(POOL,bytes(128*self.stride));mu.mem_write(HEAP,b'\xcd'*160)
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(TYPES+82*self.type_stride+0xe8,struct.pack('<II',0x800,case['genus']))
        mu.mem_write(TYPES+83*self.type_stride+0xe8,struct.pack('<II',0,0));mu.mem_write(s['map'],struct.pack('<I',MAP));mu.mem_write(MAP,struct.pack('<H',116 if case['mapFilled'] else 0)*65536)
        # 입력 노드의 raw 필드만 넣으며 종속 슬롯의 kind/mask는 좌표와 겹친다.
        for entry in case['nodes'].split(';'):
            sid,typ,owner,graph,x,y,head,nxt,kind,mask=map(int,entry.split(','));raw=bytearray(self.stride)
            raw[10]=typ;raw[self.o['owner']]=owner;raw[30 if self.edition=='originals' else 28]=graph
            raw[14:22]=struct.pack('<II',x,y);raw[4:8]=struct.pack('<HH',nxt,head)
            if typ in (5,6):raw[18:22]=struct.pack('<HH',kind,mask)
            mu.mem_write(self.slot(sid),bytes(raw))
        workshops=[int(v) for v in case['workshops'].split(',') if v];additional=[int(v) for v in case['additional'].split(',') if v]
        mu.mem_write(LISTS,struct.pack('<64I',*(workshops+[0]*(64-len(workshops)))));mu.mem_write(PLAYERS+case['owner']*self.player_stride,struct.pack('<III',LISTS,64,len(workshops)))
        mu.mem_write(EXTRAS,struct.pack('<64I',*(additional+[0]*(64-len(additional)))));mu.mem_write(s['extras'],struct.pack('<I',EXTRAS));mu.mem_write(s['count'],struct.pack('<I',len(additional)))
        # 원본이 읽는 현재 전역을 각 실행 전에 다시 넣는다.
        for name,value in [('ready',case['ready']),('invalid',case['invalid']),('ignore',case['ignore']),('contained',6),('boss',1)]:mu.mem_write(s[name],struct.pack('<I',value))
        if self.edition=='originals':
            # SID 0의 assert는 별도 C++ 보호 검사에서 다룬다.
            for name in ('checking','fence','archer'):mu.mem_write(s[name],bytes(4))
        self.write_ranges=[(0,4),(HEAP,HEAP+160)]
        preserved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x34567890,UC_X86_REG_EBP:0x456789ab}
        # 보존 레지스터를 식별 가능한 값으로 시작한다.
        for reg,value in preserved.items():mu.reg_write(reg,value)
        # 임시 레지스터는 이전 입력의 결과가 남지 않게 지운다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX):mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        if case['kind']=='L':entry,args=s['entry'],[case['owner'],case['type'],case['x'],case['y']]
        elif case['kind']=='G':entry,args=s['at'],[case['x'],case['y']]
        elif case['kind']=='S':entry,args=s['of'],[case['sid']]
        else:
            mu.mem_write(DIRECT,struct.pack('<III',LISTS,64,len(workshops)));entry,args=s['nearest'],[case['x'],case['y'],DIRECT,case['owner']]
        before=(zlib.adler32(mu.mem_read(POOL,128*self.stride)),zlib.adler32(mu.mem_read(MAP,0x20000)))
        mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));mu.emu_start(entry,STOP,count=100000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Player 정상 반환/스택 오류')
        if any(mu.reg_read(reg)!=value for reg,value in preserved.items()):raise RuntimeError('Player 보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPCW)!=control or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.mem_read(0,4)!=bytes(4):raise RuntimeError('Player FS/x87 오류')
        raw,map_=zlib.adler32(mu.mem_read(POOL,128*self.stride)),zlib.adler32(mu.mem_read(MAP,0x20000))
        if before!=(raw,map_):raise RuntimeError('Player 읽기 자료 변경')
        candidates=struct.unpack('<'+'I'*self.filled,mu.mem_read(HEAP,self.filled*4)) if self.filled else ()
        return [mu.reg_read(UC_X86_REG_EAX),','.join(map(str,candidates)),raw,map_]


def generate(smoke=False):
    """실제 원본의 두 정밀도 관찰이 일치한 새 fixture/근거를 만든다."""
    rows=[];editions={};paths={Path(__file__),ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py',ROOT/'tools/ghidra/playeranchor-functions.json',FIXTURE}
    # 세 판본마다 별도 허용 범위와 실제 PE를 실행한다.
    for edition in SPECS:
        oracle=PlayerAnchorOracle(edition);cases=inputs()
        if smoke:cases=cases[::max(1,len(cases)//30)]
        # 같은 입력의 두 x87 정밀도 결과를 비교한다.
        for case in cases:
            values=[oracle.run(case,control) for control in CONTROLS]
            if values[0]!=values[1]:raise RuntimeError(f'Player x87 차이: {edition} {case}')
            rows.append([edition,*[case[key] for key in KEYS],*values[0]])
        editions[edition]=dict(cases=len(cases),native_calls=dict(oracle.native_calls),substitutions=oracle.substitutions,assertions=oracle.assertions)
        paths.update(oracle.exports);paths.add(ROOT/oracle.spec['binary']);print(f'{edition}: Player {len(cases)}개 통과',flush=True)
    if smoke:return
    FIXTURE.write_text('# Player/거리/그래프/contained 실제 정상 반환. 임시 CRT 메모리만 대체; 게임/OS 미실행.\n# edition '+
        ' '.join(KEYS)+' result candidates rawAdler mapAdler\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-09',controls=list(CONTROLS),os_calls=0,editions=editions,
        files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(paths)},limits=['Player/거리/그래프/contained 몸체 전체 정상 반환; CRT 160바이트 확보/반납만 대체',
            '정상 확보 입력; 호스트 CRT 실패/예외 전달과 원본 밖 SID/타입/비유한 그래프 좌표는 C++ 진단',
            '거리 중간값 double/최소값 float; 두 x87 정밀도가 같은 관찰 입력, 임의 80비트 경계 전체 동등성 아님',
            '전체 MayPlace/주변 권한/최종 관계/게임/GUI/OS 검증 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """SHA/행/실제 collector·거리·그래프 진입과 메모리 대체를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 원본·도구·내보내기·fixture 바이트가 기록과 같은지 확인한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError(f'SHA 불일치: {name}')
    if len(rows)!=report['total'] or report['controls']!=list(CONTROLS) or report['os_calls']:raise RuntimeError('Player 근거 오류')
    # 각 판본의 정상 진입과 할당/반납 균형을 검사한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition];s=SPEC[edition]
        if len(selected)!=item['cases'] or item['assertions'] or item['substitutions']['allocate']!=item['substitutions']['free']:raise RuntimeError('Player 행/assert/힙 오류')
        if item['native_calls'].get(f'{s["entry"]:08x}',0)!=2*sum(row[1]=='L' for row in selected):raise RuntimeError('실제 Player 진입 누락')
        # 단독 관찰뿐 아니라 collector가 호출한 실제 하위 몸체도 포함해 센다.
        for key,kind in [('nearest','N'),('at','G'),('of','S')]:
            if item['native_calls'].get(f'{s[key]:08x}',0)<2*sum(row[1]==kind for row in selected):raise RuntimeError('실제 하위 helper 누락')
    print(f'playeranchor 검증 통과: {len(rows)}개')


def main():
    """전체 관찰/표본/저장 감사 중 지정한 작업을 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)


if __name__=='__main__':main()
