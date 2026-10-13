#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Ascend 후보/반경 삭제/다른 소유자 섬 조회를 두 PE에서 실행한다. 게임/OS/창을 열지 않는다."""
import argparse
import collections
import csv
import itertools
import hashlib
import json
import struct
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle,ROOT,POOL,HEAP,MAP,STACK_BASE,VTABLE,bits,sha
from decomp_bridgedecay_oracle import TYPES,STACK,STOP,FPU_CONTROLS
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 실제 함수·외부 경계와 공유 IslandList/타입 전역 주소다.
SPEC={
 'originals':dict(entries=dict(Candidate=0x4db680,Tick=0x4db8e0,Count=0x46f480),island=0x46f530,
    allocate=0x4e4391,free=0x4e4354,random=0x532710,noisland=0x5412cc,islands=0x568b60),
 'originalCD':dict(entries=dict(Candidate=0x44e230,Tick=0x44ee30,Count=0x4bdba0),island=0x4bdc50,
    allocate=0x4f1650,free=0x4f1a50,random=0x5308ec,noisland=0x51cbb8,islands=0x583188)
}
# 분리된 후보 버퍼와 가상 삭제 진입점이다. 원본 파일을 수정하지 않는다.
BUFFER,DESTROY=HEAP+0xe000,STOP+0x340
SPOTS=0x11400000 # 네 단계 해시와 겹치지 않는 별도 spot 입력 지도다.
FIXTURE=ROOT/'cpppj/tests/fixtures/vortexascend-x86.tsv'
REPORT=ROOT/'cpppj/recovery-vortexascend-evidence.json'
PREVIOUS_REPORT=ROOT/'extracted/vortexascend-before-cd-repair-evidence.json' # CD 경계 교정 전 전체 출처 기록이다.
PREVIOUS_FIXTURE=ROOT/'extracted/vortexascend-before-cd-repair-x86.tsv' # 같은 교정 전 패치 행의 바이트 출처다.


def inputs():
    """후보의 실제 비트 조건과 반경/20개 제한/난수·현재 owner/섬 목록 입력만 만든다."""
    cases=[]
    # genus 제외 비트·현재 섬/owner·ground/flags1와 현재 표면0/noIsland 경계를 교차한다.
    for genus,flags,spot,support,island,owner in itertools.product((0,0x2,0x100,0x10000,0x200000,0x128b302),
        (0,0x200000),(0,2),(0,94,157),(9,10),(1,2,0)):
        cases.append(('Candidate',(genus,flags,spot,support,island,owner,1,0)))
    # 현재 콜백 후 type/owner/spot 변경은 후속 원본 재조회를 요구한다.
    for mutation in (1,2,3):cases.append(('Candidate',(0,0x200000,0,0,9,2,1,mutation)))
    # count는 signed DWORD이며32초과를64로 바꾼다. 최대20개 앞 후보의 선택과 빈 최종 반경이다.
    for count,size,rng,owner in itertools.product((0,1,2,32,33,64,0xffffffff,0x80000000),(0,1,19,20,21,40),(0,1,0xffffffff),(1,2)):
        cases.append(('Tick',(count,size,rng,owner,1,0)))
    # null new는 빈 후보에만 안전하게 정상 반환한다. 후보를 쓰는 원본 NULL 충돌 입력은 별도 진단 대상으로 둔다.
    for count in (0,33,0xffffffff):cases.append(('Tick',(count,0,0,1,0,0)))
    # allocation 외부 경계 뒤 owner/현재 후보의 상태 변경도 원본 명령이 다시 읽는다.
    for mutation in (1,2):cases.append(('Tick',(33,21,0x12345678,1,1,mutation)))
    # 존재/nonzero owner/기준 owner를 가진 raw32바이트 섬 기록이다.
    for owner,variant in itertools.product((0,1,2,8,0xffffffff),range(4)):cases.append(('Count',(owner,variant)))
    return list(dict.fromkeys(cases))


class AscendOracle(FinderOracle):
    """actual 후보/일반 finder/난수/목록 조건을 실행하고 섬 좌표 조회·new/free·가상 삭제만 관찰한다."""
    def __init__(self,edition):
        """최신 정밀 함수와 기존 actual 일반 finder를 연결한다."""
        super().__init__(edition);self.p=SPEC[edition];self.entries=set();self.returns=0
        self.mu.mem_map(SPOTS,65536);self.put(self.g['spots'],SPOTS)
        self.spec=dict(self.spec,entries=self.p['entries']);self.stubs={}
        for name in ('vortexascend','assetregular'):
            paths=[ROOT/f'extracted/{name}/{edition}/{suffix}' for suffix in ('creation.c','functions.tsv')]
            self.body_paths.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:
                # Ghidra가 내보낸 함수 몸체 바깥 명령은 실행하지 않는다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.entries.add(int(row['entry'],16))
                    for span in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in span.split('-'))
                        self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        # CD의 작은 Exists helper는 같은 호스트의 최신 전체 정밀 표에서 읽는다. Ghidra 작업을 추가하지 않는다.
        if edition=='originalCD':
            path=ROOT/'extracted/refined/originalCD/functions.tsv';self.body_paths.append(path)
            with path.open(encoding='utf-8') as stream:
                required={0x4bd480,0x407c40,0x407c50,0x407c91}
                rows=[row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16) in required]
            if len(rows)!=len(required):raise RuntimeError('최신 CD Exists/벡터 소멸자 helper 범위 누락')
            for row in rows:
                self.entries.add(int(row['entry'],16))
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'))
                    self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        self.mu.mem_write(VTABLE+0x10,struct.pack('<I',DESTROY))

    def read(self,address,width=4):
        """raw DWORD/WORD/BYTE를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시 입력/효과의 원본 폭만 쓴다."""
        self.mu.mem_write(address,int(value).to_bytes(width,'little'))

    def on_write(self,mu,access,address,size,value,data):
        """몸체의 스택/FS·후보버퍼·공유난수 쓰기만 허용한다. raw/hash/spot의 실제 명령 쓰기는 금지다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if address==0 and size==4:return
        if BUFFER<=address and address+size<=BUFFER+80:return
        if self.p['random']<=address and address+size<=self.p['random']+4:return
        raise RuntimeError(f'Ascend 허용 밖 쓰기 {address:08x}+{size}')

    def on_instruction(self,mu,address,size,data):
        """외부 섬조회/new/free/가상삭제의 현재 인자를 관찰하며 실제 후보조건은 대체하지 않는다."""
        if address in self.entries:self.native_calls[f'{address:08x}']+=1
        p=self.p
        if address not in (p['island'],p['allocate'],p['free'],DESTROY):return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX)
        args=[self.read(sp+4+i*4) for i in range(4)];purge=0;value=0
        if address==p['island']:
            self.events.append(f'I:{args[0]}:{args[1]}');purge=8;value=self.island
            if self.kind=='Candidate':
                mutation=self.case[7]
                if mutation==1:self.put(self.slot(60)+(34 if self.stride==50 else 32),1,1)
                if mutation==2:self.put(TYPES+89*self.type_stride+0xe8,0)
                if mutation==3:self.put(SPOTS+52*256+50,2,1)
        elif address==p['allocate']:
            if args[0]!=80:raise RuntimeError('Ascend 후보 버퍼 크기 오류')
            self.events.append('N:80');value=BUFFER if self.case[4] else 0
            if self.case[5]==1:self.put(self.slot(50)+(34 if self.stride==50 else 32),2,1)
            if self.case[5]==2:self.put(self.slot(60)+(40 if self.stride==50 else 35),8,1)
        elif address==p['free']:self.events.append('F:'+str(int(args[0]==BUFFER)))
        elif address==DESTROY:
            self.events.append(f'D:{self.sid_of(this)}:{args[0]}');purge=4;self.put(this+11,self.read(this+11,1)|2,1)
        self.stub_calls[f'{address:08x}']+=1
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,kind,case,control):
        """입력 장면·실제 반환/raw/난수/효과 순서를 수집한다."""
        self.kind=kind;self.case=case;p=self.p;ownerOffset=34 if self.stride==50 else 32
        self.events=[];self.island=9
        self.mu.mem_write(SPOTS,bytes(65536));self.mu.mem_write(POOL,bytes(128*self.stride))
        self.put(p['noisland'],157);self.put(p['random'],0)
        for typ in (89,94,124,157):
            self.mu.mem_write(TYPES+typ*self.type_stride,bytes(self.type_stride))
            self.put(TYPES+typ*self.type_stride+self.foot_offset,1);self.put(TYPES+typ*self.type_stride+self.foot_offset+4,1)
        if kind=='Candidate':
            genus,flags,spot,support,self.island,owner,player,mutation=case
            self.setup([(60,89,0,0,bits(50.75),bits(52.5),1,1,0,1)])
            self.put(self.slot(60)+ownerOffset,owner,1);self.put(TYPES+89*self.type_stride+0xe8,flags);self.put(TYPES+89*self.type_stride+0xec,genus)
            self.put(SPOTS+52*256+50,spot,1)
            self.put(self.bases[0]+(52*256+50)*2,127 if support else 0,2)
            self.put(self.slot(127)+10,support,1)
            result=self.call('Candidate',(60,9,player),0,0,control)
        elif kind=='Tick':
            count,size,rng,owner,allocate,mutation=case
            objects=[(50,124,0,0,bits(50.75),bits(52.5),1,1,0,1)]
            # 같은 버킷의 최대40개 후보는 원본 Next가 이미 보관한 next 순서대로 읽어야 한다.
            objects += [(60+i,89,0,0,bits(50.75),bits(52.5),1,1,0,1) for i in range(size)]
            self.setup(list(reversed(objects)))
            self.put(self.slot(50)+ownerOffset,owner,1)
            for i in range(size):self.put(self.slot(60+i)+ownerOffset,2,1)
            self.put(TYPES+89*self.type_stride+0xe8,0x200000);self.put(p['random'],rng)
            result=self.call('Tick',(count,),self.slot(50),4,control)
        else:
            owner,variant=case
            self.put(p['islands'],6)
            for index in range(6):
                exists=int((index+variant)%3!=0);other=(0,1,2,8,0xffffffff,1)[(index+variant)%6]
                self.put(p['islands']+index*32+8,exists);self.put(p['islands']+index*32+12,other)
            result=self.call('Count',(owner,),p['islands'],4,control)
        self.returns+=1
        return [result,self.read(p['random']),';'.join(self.events) or '-',
            ','.join(str(self.read(self.slot(sid)+11,1)) for sid in range(60,100)),self.read(self.slot(50)+ownerOffset,1)]


def generate(smoke=False,edition_only=None):
    """고유 입력과 두 x87 정밀도/정상 반환·현재 출처 SHA를 별도 근거로 기록한다."""
    cases=inputs();cases=cases[::61] if smoke else cases;rows=[];editions={};reused={}
    files=[Path(__file__),ROOT/'tools/ghidra/vortexascend-functions.json',FIXTURE,
        ROOT/'cpppj/src/o/RawVortexAscend.h',ROOT/'cpppj/src/o/RawVortexAscend.cpp',ROOT/'cpppj/tests/VortexAscendTests.cpp']
    for edition in SPEC:
        if edition_only and edition!=edition_only and not smoke:
            old=json.loads(REPORT.read_text(encoding='utf-8'))
            if sha(FIXTURE)!=old['files'][FIXTURE.relative_to(ROOT).as_posix()]:raise RuntimeError('재사용 전 fixture SHA 불일치')
            old_rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
            chosen=[row for row in old_rows if row[0]==edition]
            if [(row[1],row[2]) for row in chosen]!=[(kind,','.join(map(str,values))) for kind,values in cases]:raise RuntimeError('재사용 고유 입력 불일치')
            binary=ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE')
            if sha(binary)!=old['files'][binary.relative_to(ROOT).as_posix()]:raise RuntimeError('재사용 PE SHA 불일치')
            oracle=AscendOracle(edition)
            rows.extend(chosen);editions[edition]=old['editions'][edition]
            reused[edition]=dict(report_sha256=sha(REPORT),fixture_sha256=sha(FIXTURE),cases=len(chosen),
                reason='해당 PE/입력/관찰 경계는 변경되지 않았다. CD vector 소멸자→실제free만 재실행한다.')
            files.extend(oracle.body_paths+[binary]);print(f'{edition}: SHA/고유입력 확인 후 {len(chosen)}개 기존 관찰 보존',flush=True)
            continue
        if edition_only and edition!=edition_only and smoke:continue
        oracle=AscendOracle(edition)
        for kind,values in cases:
            output=oracle.run(kind,values,FPU_CONTROLS[0]);check=oracle.run(kind,values,FPU_CONTROLS[1])
            if output!=check:raise RuntimeError(f'Ascend 정밀도 차이 {edition} {kind} {values}')
            rows.append([edition,kind,','.join(map(str,values)),*output])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,assertions=oracle.assertions,
            entries=dict(oracle.native_calls),external=dict(oracle.stub_calls))
        files.extend(oracle.body_paths+[ROOT/oracle.spec['binary']]);print(f'{edition}: {len(cases)}개 정상 반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition kind input result rng events deletedStates templeOwner\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files += [ROOT/'tools/decomp_rawfinder_oracle.py',ROOT/'tools/decomp_bridgeeffects_oracle.py',ROOT/'tools/decomp_bridgedecay_oracle.py']
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',editions=editions,os_calls=0,native_reuse=reused,
        files={path.relative_to(ROOT).as_posix():sha(path) for path in dict.fromkeys(files)},limits=[
            'AscendTick/Candidate/CountForeignOwnedIslands·실제 일반 finder/CRT·공유 난수는 실제 PE 명령이다.',
            '섬 좌표 조회·new80/free·vtable+10 가상 삭제만 명시 관찰 경계다.',
            '게임/GUI/실제 피해/생산/전체 Ascend 송신·종료 사슬 검증이 아니다.',
            'new 실패 후 후보 쓰기는 원본 NULL 충돌이며 정상 입력으로 계수하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows),'cases':len(cases)}))


def verify():
    """SHA/고유 입력/두 정밀도 반환·실제 finder 및 원본 assert를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('SHA 불일치: '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(inputs())*2 or report['os_calls']:raise RuntimeError('Ascend 행 수/OS 경계 오류')
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition]
        if item['returns']!=len(chosen)*2 or item['assertions'] or len({(row[1],row[2]) for row in chosen})!=len(chosen):raise RuntimeError('Ascend 반환/입력/assert 오류')
    # 패치 재사용은 이전 행과 도구/PE/함수 범위의 연결을 매 감사마다 확인한다.
    for edition,reuse in report.get('native_reuse',{}).items():
        chosen=['\t'.join(row) for row in rows if row[0]==edition]
        digest=hashlib.sha256(('\n'.join(chosen)+'\n').encode('utf-8')).hexdigest()
        tool=Path(__file__).relative_to(ROOT).as_posix()
        if len(chosen)!=reuse['cases'] or digest!=reuse['rows_sha256'] or reuse['previous_tool_sha256']!=reuse['previous_sources'][tool] or reuse['current_tool_sha256']!=report['files'][tool]:
            raise RuntimeError('Ascend 재사용 행/이전 실행기 출처 연결 오류')
        for name,value in reuse['preserved_sources'].items():
            if reuse['previous_sources'][name]!=value or sha(ROOT/name)!=value:raise RuntimeError('Ascend 재사용 PE/함수 범위/부모 실행기 변경: '+name)
    print(f'Ascend {len(rows)}개 SHA/정상 반환 감사 통과')


def link_reuse():
    """재사용한 패치 행/원본·범위/부모도구/이전 실행기 SHA를 이전 기록에 연결한다. native를 새로 실행하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));old=json.loads(PREVIOUS_REPORT.read_text(encoding='utf-8'))
    reuse=report['native_reuse']['originals']
    if sha(PREVIOUS_REPORT)!=reuse['report_sha256'] or sha(PREVIOUS_FIXTURE)!=reuse['fixture_sha256']:raise RuntimeError('재사용 이전 기록 SHA 불일치')
    before=[line for line in PREVIOUS_FIXTURE.read_text(encoding='utf-8').splitlines() if line.startswith('originals\t')]
    current=[line for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line.startswith('originals\t')]
    if current!=before or len(current)!=748:raise RuntimeError('재사용 패치 행 변경')
    preserved={name:value for name,value in old['files'].items() if '/originals/' in name or name=='originals/Netstorm.exe' or
        name in ('tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py')}
    for name,value in preserved.items():
        if sha(ROOT/name)!=value:raise RuntimeError('재사용 패치 PE/범위/부모 실행기 변경: '+name)
    tool=Path(__file__).relative_to(ROOT).as_posix()
    reuse.update(rows_sha256=hashlib.sha256(('\n'.join(current)+'\n').encode('utf-8')).hexdigest(),
        previous_tool_sha256=old['files'][tool],current_tool_sha256=sha(Path(__file__)),
        preserved_sources=preserved,previous_sources=old['files'],
        change_audit='native SPEC 변경은 originalCD.free 00407c50→004f1a50이며 추가 actual 허용 범위도 CD의 Exists/벡터 소멸자뿐이다. 입력·originals SPEC·originals on_instruction·일반 finder·반환 수집은 그대로다. 선택 생성/출처 연결은 워크플로 변경이다. C++ 반경의 0.99999f 교정은 현재 패치748개 전수재생으로 검증했다.')
    report['files'][tool]=sha(Path(__file__))
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('패치748개 행 SHA/PE/범위·이전도구 출처 연결 완료. 신규 native 관찰0.')


# 명시 실행만 새 fixture를 생성하거나 기존 출처를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');parser.add_argument('--edition',choices=list(SPEC));parser.add_argument('--link-reuse',action='store_true')
    args=parser.parse_args()
    if args.link_reuse:link_reuse()
    elif args.verify:verify()
    else:generate(args.smoke,args.edition)
