#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 실제 PE의 캠페인 자산 postPop 재정의를 실행한다. 게임·창·OS 진입은 없다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, PLAYERS, POOL, STACK, STOP, STACK_BASE, TARGET, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)

# 실제 PE body·전역·명시 외부 경계 주소다. vtable은 원본 바이트에서 검증한다.
SPEC = {
 'originals': dict(tables=[0x501ea0,0x50b6e8,0x50b7c0,0x50b9b8,0x50b888,0x50b570,0x50f540,0x50e940,0x50da88,0x5196f0,0x519860,0x519918],
    entries=[0x41c150,0x46a520,0x46a600,0x46aca0,0x46a810,0x4694b0,0x49a2b0,0x48eda0,0x4852e0,0x4dce40,0x4dd020,0x4dd1f0],
    allocate=0x4e4391,regular=0x496e80,gun=0x46a1a0,damageable=0x44bf80,base=0x4b0d30,
    type=0x4ac550,flags=0x4ac1e0,genus=0x4ac200,word=0x4ac220,graph=0x4ad3e0,frame=0x467610,
    surface=0x40eaf0,owner=0x4adf00,notify=0x4214a0,blocker=0x46ae80,registration=0x416720,
    at=0x41def0,next=0x4b1810,unpop=0x4afe50,repop=0x41c0d0,remove=0x452fe0,check=0x4ad730,
    pure=0x4ac970,element=0x4a9550,clear=0x446e70,island=0x46f530,exists=0x46ee30,register=0x4db890,
    has=0x4acac0,create=0x4afa70,refresh=0x47d890,region=0x40d980,begin=0x414490,advance=0x40eb40,destroy=0x4af780,
    battle=0x540bc0,boss=0x540bc4,load=0x5c89b8,fort=0x594fb8,checking=0x5e4794,animation=0x5c85d4,
    residence=0x59a9d8,temples=0x5e189c,geysers=0x5950dc,count=0x55a4a4,selected=0x55a2a8,local=0x540c70,
    capacity=0x5c847c,islandtype=0x5411d0,stalagtype=0x5411d4,slottype=0x54126c),
 'originalCD': dict(tables=[0x5022f8,0x5011e8,0x5012b0,0x501120,0x5014f8,0x501440,0x501b90,0x506168,0x500760,0x501d28,0x501de0,0x501e98],
    entries=[0x459ed0,0x436fe0,0x437100,0x436eb0,0x437410,0x4371f0,0x44a4e0,0x4c0be0,0x415d30,0x44ff60,0x450020,0x450210],
    allocate=0x4f1650,regular=0x48efd0,gun=0x434090,damageable=0x4621c0,base=0x4ae180,
    type=0x4abf50,flags=0x4abac0,genus=0x4abae0,word=0x4abb00,graph=0x4adbd0,frame=0x4a25d0,
    surface=None,owner=0x4aefa0,notify=0x448c10,blocker=0x436790,registration=0x4d8670,
    at=0x4eae20,next=0x4eafe0,unpop=0x4ad0b0,repop=0x401c60,remove=0x4a6aa0,check=None,
    pure=0x4ac440,element=0x4383f0,clear=0x43e4e0,island=0x4bdc50,exists=0x4bd4c0,old=0x4bd9c0,register=0x44ebc0,
    has=0x4ac630,create=0x4ac4d0,refresh=0x4773a0,region=0x404090,begin=None,advance=None,destroy=0x4ab7e0,
    battle=0x540a28,boss=0x540a2c,load=0x5178d4,fort=0x540a1c,checking=None,animation=None,
    residence=0x565ae0,temples=0x565af0,geysers=0x5491f8,count=0x520c10,selected=0x51b658,local=0x50f6c8,
    capacity=0x5395f4,islandtype=0x51cabc,stalagtype=0x51cac0,slottype=0x51cb58),
}
# 입력 칸과 독립 기대값 경로다.
KEYS=('kind','flags','extra','battle','load','boss','reserve','animation','word','f1','cost','selected','has','list_count')
FIXTURE=ROOT/'cpppj/tests/fixtures/assetpostpop-x86.tsv'
REPORT=ROOT/'cpppj/recovery-assetpostpop-evidence.json'
# 임시 new 반환·목록·지역 byte의 가상 주소다. 호스트 저장소와 무관하다.
SCRATCH=0x11318000
LIST_BASE=0x11319000
REGION=0x1131f000


def inputs():
    """분기 결과를 계산하지 않고 각 실제 몸체에 입력 격자와 목록 용량/예약 실패를 공급한다."""
    result=[]
    # 12가지 몸체에 flags·abstract/buried·전투·로딩 경계를 교차한다.
    for kind,flags,extra,battle,load in itertools.product(range(12),(0,1,2,7),(0,1,8),(0,1),(0,1)):
        # 일반 client SID를 서버 생성 assert에 보내지 않도록 생성 당시 로딩 조건을 공급한다.
        if kind in (3,4) and flags&1 and not extra&1 and battle and not load: continue
        result.append(dict(kind=kind,flags=flags,extra=extra,battle=battle,load=load,boss=0,reserve=7,
            animation=1,word=25,f1=0x1000,cost=1000,selected=0,has=0,list_count=0))
    # 최초 등록의 개별 확보 실패/목록 가득참/선택 취소/현재 내용물 경계를 추가한다.
    for kind,reserve,count,has,selected in itertools.product((5,6,8,9,10,11),(0,1,2,4),(0,4),(0,1),(0,1)):
        result.append(dict(kind=kind,flags=1,extra=0,battle=1,load=1,boss=0,reserve=reserve,
            animation=1,word=19,f1=0x1000,cost=-123.75,selected=selected,has=has,list_count=count))
    # 실제 boss iterator/가상 삭제 분기를 다른 신전의 buried 여부·지역 byte와 교차한다.
    for kind,extra,load,has in itertools.product((9,10,11),(0,8),(0,1),(0,1)):
        result.append(dict(kind=kind,flags=1,extra=extra,battle=1,load=load,boss=1,reserve=7,
            animation=0,word=7,f1=0x1000,cost=1000,selected=0,has=has,list_count=0))
    # 원본 레지던스의 signed DWORD /2·6단계 경계·CD 하위 byte를 독립 관찰한다.
    for word,animation in itertools.product((0,5,6,11,12,17,18,25,0xffffffff,0xfffffffb,0x80000000,0x7fffffff),(0,1)):
        result.append(dict(kind=6,flags=1,extra=0,battle=1,load=1,boss=0,reserve=7,
            animation=animation,word=word,f1=0x1000,cost=1000,selected=0,has=0,list_count=0))
    # 캐논 타입 비트가 없으면 HP/extra 복사를 하지 않는 분기와 CRT 큰 비용 low WORD를 추가한다.
    for extra,f1 in itertools.product((0,1,8),(0,0x1000)):
        result.append(dict(kind=3,flags=1,extra=extra,battle=1,load=1,boss=0,reserve=7,
            animation=0,word=350,f1=f1,cost=1000,selected=0,has=0,list_count=0))
    for cost in (0.0,500.75,2147483648.0,4294967296.0):
        result.append(dict(kind=8,flags=1,extra=0,battle=1,load=1,boss=0,reserve=7,
            animation=0,word=0,f1=0,cost=cost,selected=0,has=0,list_count=0))
    return result


class AssetOracle(OwnerOracle):
    """새 내보낸 실제 body를 허용하고 가상 효과·새 객체·공간/목록 외부 경계만 관찰한다."""
    def __init__(self,edition):
        """기존 PE 매핑에 원본 FS 체인과 새 함수 범위를 추가한다."""
        super().__init__(edition)
        self.p=SPEC[edition];self.mu.mem_map(0,0x1000)
        self.exports.extend(ROOT/f'extracted/assetpostpop/{edition}/{name}' for name in ('creation.c','functions.tsv'))
        with self.exports[-1].open(encoding='utf-8') as fp:
            # Ghidra가 계산한 불연속 body 범위만 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                for part in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        # 복원 body 분배를 실제 vtable +0x20의 bytes와 독립 대조한다.
        for table,entry in zip(self.p['tables'],self.p['entries']):
            if self.read(table+0x20)!=entry: raise RuntimeError('원본 postPop 가상표 +0x20 불일치')
        self.events=[];self.stub_calls=collections.Counter();self.returns=0

    def read(self,address,width=4):
        """가상 raw DWORD/WORD/BYTE를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_instruction(self,mu,address,size,data):
        """명시 외부 경계에서 인자와 현재 raw를 관찰하고 원본 호출 규약으로 반환한다."""
        p=self.p;esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        args=[self.read(esp+4+4*i) for i in range(7)];ret=self.read(esp);purge=0;value=0
        slot=self.slot(TARGET)
        if address==p['allocate']:
            self.events.append('A:'+str(args[0]));value=SCRATCH if self.case['reserve']&(1<<(self.alloc%3)) else 0;self.alloc+=1
        elif address==p['regular']:
            self.events.append(f'R:{args[1]}:{args[0]}:{args[2]}');purge=12
        elif address==p['gun']:
            self.events.append('P:'+str(args[0]));purge=4
        elif address==p['damageable']:
            self.events.append('D:'+str(args[0]));purge=4
        elif address==p['base']: purge=4
        elif address in (p['type'],p['flags'],p['genus']):
            number=self.read(this+10,1);typ=TYPES+number*self.type_stride
            value=typ if address==p['type'] else self.read(typ+(0xe8 if address==p['flags'] else 0xec))
        elif address==p['frame']:
            self.events.append('F:'+str(args[0]));value=2
        elif address==p['graph']:
            self.events.append('Q:'+str(args[0]));mu.mem_write(this+(30 if self.edition=='originals' else 28),bytes([args[0]&255]));purge=4
        elif address==p['owner']:
            sid=(this-POOL)//self.stride;self.events.append(f'O:{sid}:{args[0]}');mu.mem_write(this+self.o['owner'],bytes([args[0]&255]));purge=4
        elif address==p['surface']: value=60
        elif address==p['notify']:self.events.append('N:'+str(args[0]))
        elif address==p['blocker']:self.events.append('K:'+str(args[0]))
        elif address==p['registration']:self.events.append(f'G:{args[0]}:{args[1]}')
        elif address==p['remove']:self.events.append('M:'+str(args[1]))
        elif address==p['check']:value=1
        elif address==p['pure']: pass
        elif self.edition=='originalCD' and address in (0x440a50,0x440a80): pass
        elif self.edition=='originals' and address==0x414450:
            pointer,capacity,count=(self.read(this+4*i) for i in range(3))
            if count<capacity and args[0] not in [self.read(pointer+4*i) for i in range(count)]:
                mu.mem_write(pointer+4*count,struct.pack('<I',args[0]));mu.mem_write(this+8,struct.pack('<I',count+1))
            purge=4
        elif address==p['at']:
            mu.mem_write(this+(52 if self.edition=='originals' else 52),struct.pack('<I',60));self.iterator=this;self.index=0
            purge=20 if self.edition=='originals' else 16
        elif address==p['next']:
            self.index+=1;mu.mem_write(this+52,struct.pack('<I',61 if self.index==1 else 0))
        elif address==p['unpop']:
            sid=(this-POOL)//self.stride;self.events.append(f'U:{sid}:{args[0]}');purge=4
        elif address==p['repop']:
            sid=(this-POOL)//self.stride;self.events.append(f'V:{sid}:{args[0]}');purge=4
        elif address==p['element']:self.events.append('E:'+str(args[0]))
        elif address==p['clear']:self.events.append('C')
        elif address==p['island']: value=3;purge=8
        elif address==p['exists']: value=1;purge=4
        elif address==p.get('old'): value=0;purge=4
        elif address==p['register']:self.events.append(f'I:{args[1]}:{args[2]}')
        elif address==p['has']:value=self.case['has'];purge=12
        elif address==p['create']:self.events.append('S:'+str(TARGET));purge=20
        elif address==p['refresh']:self.events.append('T')
        elif address==p['region']:value=REGION;purge=8
        elif address==p['destroy']:
            sid=(this-POOL)//self.stride;self.events.append(f'X:{sid}:{args[0]}');mu.mem_write(this+11,bytes([self.read(this+11,1)|2]));purge=4
        else:
            super().on_instruction(mu,address,size,data);return
        self.stub_calls[f'{address:08x}']+=1
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run_body(self,case):
        """입력 상태만 만들고 원본 정상 반환·슬롯·목록·Player 신전 필드를 수집한다."""
        self.case=case;self.events=[];self.alloc=0;p=self.p;patch=self.edition=='originals';slot=self.slot(TARGET)
        self.mu.mem_write(POOL,bytes(self.stride*128));self.mu.mem_write(PLAYERS,bytes(self.player_stride*9))
        raw=bytearray(self.stride);struct.pack_into('<I',raw,0,p['tables'][case['kind']]);raw[10]=70;raw[11]=0
        raw[self.o['owner']]=1;raw[self.o['extra']]=case['extra'];struct.pack_into('<HII',raw,12,0xabcd,0x42480000,0x42500000)
        struct.pack_into('<I' if patch else '<B',raw,36 if patch else 34,case['word'] if patch else case['word']&255);self.mu.mem_write(slot,bytes(raw))
        for sid,number in ((60,94),(61,95)):
            neighbor=bytearray(self.stride);neighbor[10]=number;neighbor[self.o['owner']]=2
            struct.pack_into('<I',neighbor,0,p['tables'][0]);self.mu.mem_write(self.slot(sid),bytes(neighbor))
        if case['boss']:
            self.mu.mem_write(self.slot(61)+10,bytes([70]));self.mu.mem_write(self.slot(61)+self.o['extra'],bytes([case['has']*8]));self.mu.mem_write(REGION,bytes([3]))
        # 원본 allocator 및 iterator의 가상호출 표를 경계에 맞춰 임시 메모리에서 제공한다.
        table=0x11317000;self.mu.mem_write(table,bytes(0x100));self.mu.mem_write(table+0x48,struct.pack('<I',p['unpop']))
        self.mu.mem_write(table+0x10,struct.pack('<I',p['destroy']))
        self.mu.mem_write(table+0x4c,struct.pack('<I',p['repop']));self.mu.mem_write(table+0x74,struct.pack('<I',p['owner']))
        self.mu.mem_write(slot,struct.pack('<I',table))
        for sid in (60,61):self.mu.mem_write(self.slot(sid),struct.pack('<I',table))
        self.mu.mem_write(TYPES+70*self.type_stride,bytes(self.type_stride))
        self.mu.mem_write(TYPES+70*self.type_stride+0xe8,struct.pack('<II',case['f1'],0x200))
        self.mu.mem_write(TYPES+70*self.type_stride+0x74,struct.pack('<I',17))
        self.mu.mem_write(TYPES+70*self.type_stride+0x130,struct.pack('<II',27,39))
        self.mu.mem_write(TYPES+70*self.type_stride+0xc4,struct.pack('<f',case['cost']))
        for name,key in (('battle','battle'),('load','load'),('boss','boss'),('fort','load'),('animation','animation')):
            if p.get(name):self.mu.mem_write(p[name],struct.pack('<I',case[key]))
        if p['checking']:self.mu.mem_write(p['checking'],bytes(4))
        for name,value in (('count',4),('selected',62 if case['selected'] else 0),('local',1),('capacity',128),('islandtype',94),('stalagtype',95),('slottype',133)):
            self.mu.mem_write(p[name],struct.pack('<I',value))
        self.mu.mem_write(self.slot(62)+18,struct.pack('<H',70))
        for i,name in enumerate(('residence','temples','geysers')):
            pointer=LIST_BASE+i*0x100;self.mu.mem_write(pointer,struct.pack('<4I',7,8,9,10));self.mu.mem_write(p[name],struct.pack('<III',pointer,4,case['list_count']))
        if not patch:
            # CD Residence 지면 helper는 body에 내장되어 실제 WORD 지도를 읽는다.
            self.mu.mem_map(0x11400000,0x20000) if not hasattr(self,'mapped_surface') else None;self.mapped_surface=True
            self.mu.mem_write(0x5670cc,struct.pack('<I',0x11400000));self.mu.mem_write(0x11400000,bytes(0x20000))
            self.mu.mem_write(0x11400000+(52*256+50)*2,struct.pack('<H',60))
        self.write_ranges=[(0,4),(POOL,POOL+128*self.stride),(PLAYERS,PLAYERS+9*self.player_stride),(LIST_BASE,LIST_BASE+0x400)]
        self.write_ranges.extend((p[name],p[name]+12) for name in ('residence','temples','geysers'))
        self.write_ranges.append((p['count'],p['count']+4))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,slot)
        self.mu.mem_write(STACK,struct.pack('<II',STOP,case['flags']));self.mu.mem_write(0,struct.pack('<I',0))
        self.mu.emu_start(p['entries'][case['kind']],STOP,count=30000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+8:raise RuntimeError('자산 postPop 반환/스택 오류')
        self.returns+=1
        output=bytearray(self.mu.mem_read(slot,self.stride));struct.pack_into('<I',output,0,p['tables'][case['kind']])
        lists='|'.join(str(self.read(p[name]+8))+':'+','.join(str(self.read(LIST_BASE+i*0x100+4*j)) for j in range(4)) for i,name in enumerate(('residence','temples','geysers')))
        offsets=(0x7c,0x80,0x84,0x88) if patch else (0x74,0x78,0x7c,0x80)
        players=','.join(str(self.read(PLAYERS+self.player_stride+offset)) for offset in offsets)
        neighbors='|'.join(str(self.read(self.slot(sid)+(36 if patch else 34),4 if patch else 1)) for sid in (60,61))
        return [';'.join(self.events),output.hex(),lists,players,self.read(p['count']),neighbors]


def generate(smoke=False):
    """새 입력과 독립 기대값을 보존하고 두 PE/내보내기/도구 SHA를 기록한다."""
    cases=inputs();cases=cases[::73] if smoke else cases;rows=[];editions={};files=[Path(__file__),FIXTURE,ROOT/'tools/ghidra/assetpostpop-functions.json']
    for edition in SPEC:
        oracle=AssetOracle(edition)
        # 정상 반환을 한 입력마다 별도 행으로 기록한다.
        for case in cases:rows.append([edition,*[case[key] for key in KEYS],*oracle.run_body(case)])
        editions[edition]=dict(cases=len(cases),returns=oracle.returns,native_calls=dict(oracle.native_calls),stub_calls=dict(oracle.stub_calls),assertions=oracle.assertions,instructions=oracle.instructions)
        files.extend(oracle.exports);files.append(ROOT/oracle.spec['binary'])
    FIXTURE.write_text('# 두 실제 PE의 자산 postPop 입력/독립 관찰. 외부 경계만 대체하며 게임·OS 실행 없음.\n'+
        '\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8')
    REPORT.write_text(json.dumps(dict(schema=1,total=len(rows),editions=editions,host='VM-W11-CODEX',
        files={path.relative_to(ROOT).as_posix():digest(path) for path in files},
        limitations=['Damageable/common body는 기존 독립 근거를 재사용한다. 여기서는 명시 경계로 관찰한다.','가상효과 뒤 타입/전역 변경·CD 이미 등록된 신전 assert는 후속 경계 입력이 필요하다.'],os_calls=0),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'total':len(rows),'editions':editions},ensure_ascii=False))


def verify():
    """SHA·독립 입력 재현·행 수·정상 반환/원본 assert를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    if len(rows)!=report['total']:raise RuntimeError('fixture 행 수 오류')
    # 증거에 포함한 모든 파일의 현재 bytes를 확인한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha:raise RuntimeError('근거 SHA 변화: '+name)
    # native 결과로 새 입력을 재계산하지 않고 생성 격자/정상 반환 기록만 감사한다.
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=len(inputs()) or item['returns']!=len(selected) or item['assertions']:raise RuntimeError('입력/반환/assert 감사 오류')
        if [[str(case[key]) for key in KEYS] for case in inputs()]!=[row[1:1+len(KEYS)] for row in selected]:raise RuntimeError('독립 입력 격자 불일치')
    print('assetpostpop SHA/입력/정상 반환 감사 통과:',len(rows))


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate(args.smoke)
