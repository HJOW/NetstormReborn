#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 생산 Deck→CFF_TELEPORT 내용물 동기화43df80을 실제 x86으로 독립 관측한다."""
import argparse
import collections
import json
import socket
from pathlib import Path
from decomp_factory_menu import Oracle as MenuOracle, ROOT, POOL, TYPES, TEXT, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

# 실제 전체 동기화/5바이트 Deck 검색/TYPE getter만 실행한다.
ENTRIES=(0x43df80,0x44f4d0,0x49a840)
FIXTURE=ROOT/'cpppj/tests/fixtures/production-sync-x86.tsv' # C++ 작성 전에 동결할 원문이다.
REPORT=ROOT/'cpppj/production-sync.json' # 독립 실행 범위/입력/정상 반환의 출처다.


def inputs():
    """flags/현재생산power/신전/fullDeck/부모 재조회/중복의 명시 입력만 열거한다."""
    rows=[]
    # 단일 내용물의 네 비트와 세 TYPE 프로필/각 생산power를 독립 교차한다.
    for flags in (0,4,8,16,0x100,0x108):
        # kind133만bit4/전체bit4/전체0의 TYPE 입력이다.
        for profile in (0,1,2):
            # 없는 생산과signed0/음수/양수를 구분한다.
            for deck in ('-','128:-1','128:0','128:1'):
                rows.append([f'128:{flags}',deck,profile,0,0,0,'0'])
    # 실제 TYPEbit4의 신전 보존과fullDeck fallback을 교차한다.
    for temple in (0,1,0x80000000):
        # 전체Deck 허용은 신전 조건과 다른 분기다.
        for full in (0,1):
            rows.append(['133:264;128:8;76:4','128:0;72:1',0,temple,full,0,'0'])
    # 실제 입력 순서/중복/부모 변화/기존 조회 충돌을 분리한다.
    for forms,deck,queries in [('-','128:1','0'),('128:8;128:8','128:1','0'),('128:8','76:1;128:1;72:1','0'),('128:4','128:1','0'),('-','128:0;76:-1','0'),('-','128:1;128:1','0,128'),('-','0:1','0'),('-','128:1','128'),('128:264;76:264','72:1','0')]:
        # 두 번째 getter의 다른 부모도 독립 입력으로 공급한다.
        for change in (0,1):rows.append([forms,deck,0,0,0,change,queries])
    # BYTEseen의256회 증가가0으로 감기는 원문 조건을 실행한다.
    for count in (255,256,257):rows.append([';'.join(['128:8']*count),'128:1',0,0,0,0,'128'])
    return list(dict.fromkeys(tuple(row) for row in rows))


def pairs(text):
    """빈 목록과 명시한 두 정수의 쌍만 복원한다."""
    return [] if text=='-' else [tuple(map(int,part.split(':'))) for part in text.split(';')]


class Oracle(MenuOracle):
    """타입/내용물 조건과Deck 검색은 실제 명령이며 생성/삭제/현재getter/조회는 명시 경계다."""
    def __init__(self):
        """보호PE와 같은 정밀 함수 범위만 새 관측에 선택한다."""
        super().__init__();self.allowed=[];self.hits=collections.Counter();self.boundaries=collections.Counter() # 이번 실행 범위와 실제 호출 수다.
        # 함수의 분리된 원문 구간도 모두 포함한다.
        for entry in ENTRIES:
            # 각 구간의 마지막 명령까지 허용한다.
            for part in self.facts[entry]['ranges'].split(';'):
                low,high=(int(word,16) for word in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """실제 분기는 실행하고 명시한 외부 호출의 인자·순서만 기록한다."""
        self.recent.append(hex(address))
        if any(low<=address and address+size<=high for low,high in self.allowed):
            self.instructions+=1
            if address in ENTRIES:self.hits[hex(address)]+=1
            return
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,8);ecx=mu.reg_read(UC_X86_REG_ECX);self.boundaries[hex(address)]+=1
        if address==0x44f760:self.events.append('K');self.ret(TEXT+0xd000)
        elif address==0x48fc60:
            parent=50+(self.change if self.getters else 0);self.getters+=1;self.events.append('T,'+str(parent));self.ret(parent)
        elif address==0x4b2270:
            assert args[0]==50;self.iterator=0;self.put(ecx+8,self.forms[0] if self.forms else 0);self.ret(purge=4)
        elif address==0x4b1b20:
            self.iterator+=1;self.put(ecx+8,self.forms[self.iterator] if self.iterator<len(self.forms) else 0);self.ret()
        elif address==0x446e70:self.events.append('X');self.ret()
        elif address==0x4af780:self.events.append('D,'+str((ecx-POOL)//50)+','+str(args[0]));self.ret(purge=4)
        elif address==0x4acac0:
            value=self.queries[min(self.query,len(self.queries)-1)];self.query+=1;self.events.append('Q,'+str((ecx-POOL)//50)+','+','.join(map(str,args[:3]))+','+str(value));self.ret(value,12)
        elif address==0x4afa70:self.events.append('C,'+str((ecx-POOL)//50)+','+','.join(map(str,args[:5])));self.ret(20000,20)
        elif address==0x4e0620:self.events.append('A,'+self.string(args[0]).hex());self.ret()
        else:raise RuntimeError('생산 동기화 미검토 명령 '+hex(address))

    def run(self,row,control):
        """입력 raw/TYPE/Deck/현재getter만 준비하고 실제 함수의 정상 순서를 관측한다."""
        formText,deckText,profile,temple,full,self.change,queryText=row
        self.prepare([85,1,1,128,0,3,0,800,0,'-','-','-','-',0]);self.events=[];self.forms=[];self.iterator=0;self.getters=0;self.query=0;self.queries=list(map(int,queryText.split(','))) # 이번 입력의 명시 조회 순서다.
        self.put(0x59ab20,TYPES);self.put(0x59534c,TEXT+0x8000);self.put(0x540c70,1);self.put(TEXT+0x8000+180+0x7c,temple);self.put(0x54db14,full)
        # 현재 TYPEbit4 입력을 원문DWORD에 쓴다.
        for kind in range(188):self.put(TYPES+kind*500+0xec,4 if profile==1 or (profile==0 and kind==133) else 0)
        # 같은 종류의 중복도 서로 다른 실제 ContentForm으로 공급한다.
        for index,(kind,flags) in enumerate(pairs(formText)):
            sid=100+index;self.forms.append(sid);self.put(POOL+sid*50,0x512208);self.put(POOL+sid*50+10,6,1);self.put(POOL+sid*50+18,kind,2);self.put(POOL+sid*50+20,flags,2)
        self.mu.mem_write(TEXT+0xd000,bytes(352));deck=pairs(deckText);self.put(TEXT+0xd000+0x14c,len(deck))
        # 순서/중복/signed power를 원래5바이트 자리에 기록한다.
        for index,(kind,power) in enumerate(deck):self.put(TEXT+0xd000+index*5,kind,1);self.put(TEXT+0xd000+index*5+2,power,1)
        _,trace=self.invoke(0x43df80,control=control);return trace


def generate():
    """출력 기대값을 계산하지 않고 실제 원본 호출 순서만 동결한다."""
    oracle=Oracle();rows=[]
    # 두 x87 제어어에서 같은 명시 입력을 새로 공급한다.
    for row in inputs():
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f);assert first==second,row;rows.append([*map(str,row),first])
    FIXTURE.write_text('# 내용물 Deck TYPE프로필 신전 fullDeck 부모변경 조회반환 호출순서\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=[Path(__file__),ROOT/'tools/decomp_factory_menu.py',ROOT/'tools/decomp_priestmenu.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE]
    r=dict(host=socket.gethostname(),date='2026-10-18',unit=174,rows=len(rows),normalReturns=oracle.returns,instructions=oracle.instructions,controls=[0x27f,0x37f],precisionDifferences=0,nativeCalls=dict(oracle.hits),boundaries=dict(oracle.boundaries),sha256={path.relative_to(ROOT).as_posix():digest(path) for path in paths},limits=['43df80 전체/44f4d0 Deck 검색/TYPE getter의 실제 명령이다. 현재 Deck·기술부모/내용물 순회·신전/조회반환/생성·삭제/커서취소/진단은 명시 외부 입력이다.','게임/OS/원본 파일 변경·게임 창 실행은 없다. 전체 생산HUD 그림/다리 공급 주기/미션 완주의 증거는 아니다.'])
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('생산 동기화 원본 관측',len(rows),oracle.returns,oracle.instructions)


def verify():
    """기존 관측의 입력/원문SHA/정상 반환만 검사하며 재생성하지 않는다."""
    r=json.loads(REPORT.read_text('utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
    # 각 독립 입력 출처의 바이트를 확인한다.
    for path,wanted in r['sha256'].items():assert digest(ROOT/path)==wanted,path
    assert len(rows)==r['rows']==len(inputs()) and r['normalReturns']==len(rows)*2 and not r['precisionDifferences']
    # 모든 원문 입력 행을 그대로 보존한다.
    for row,case in zip(rows,inputs()):assert row[:7]==list(map(str,case)) and len(row)==8
    print('생산 동기화 원본/SHA/입력 감사',len(rows))


# 명시한 옵션에서만 새 관측 또는 기존 독립 근거 검사를 실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate()
