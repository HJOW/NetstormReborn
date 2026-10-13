#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제Rune postPop와신전/altar/player시간조회만보호PE의제한x86으로독립관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_templeritual_oracle import TempleOracle
from decomp_rawfinder_oracle import ROOT,POOL,HEAP,MAP,VTABLE,bits,sha
from decomp_bridgedecay_oracle import TYPES,CAPACITY,FPU_CONTROLS,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

SPEC={
 'originals':dict(main=0x448930,near=0x448a20,time=0x448340,regular=0x496e80,construction=0x443fe0,base=0x4b0d30,suppress=0x594fa4,players=0x59534c,playerStride=180,altar=0x5412e8),
 'originalCD':dict(main=0x484520,time=0x482990,regular=0x48efd0,construction=0x4d06a0,base=0x4ae180,suppress=0x52e170,players=0x50f834,playerStride=172,altar=0x51cbd4),
} # 두판본의실제Rune/시간helper와관찰경계주소다.
PLAYERS=0x11500000 # 원래ownerWORD로조회하는격리player배열이다.
MEMORY=HEAP+0x6000 # 관찰new0x28이반환하는메모리다.
FIXTURE=ROOT/'cpppj/tests/fixtures/runepostpop-x86.tsv' # 고유입력과원본후처리출력이다.
REPORT=ROOT/'cpppj/recovery-runepostpop-evidence.json' # 기존모듈근거와분리한새Rune원본관찰이다.


def inputs():
    """실제flags·후보/시간/확보조건과예약뒤전역/소유자변이입력만만든다."""
    return [(*row,index%3,(index//2)%3) for index,row in enumerate(itertools.product((0,1,2,3),range(10),(0,1),(0,1,8),(0,1),(0,1)))]


class RuneOracle(TempleOracle):
    """기존일반finder제한범위에Rune3/CD3몸체만추가하고외부예약/건설/base는요청으로기록한다."""
    def __init__(self,edition):
        """신규Rune함수표와명시효과주소를같은격리풀에연결한다."""
        super().__init__(edition);self.r=SPEC[edition];self.rune_returns=0;self.mu.mem_map(PLAYERS,0x20000);manifest=ROOT/'tools/ghidra/runepostpop-functions.json';table=ROOT/f'extracted/runepostpop/{edition}/functions.tsv';self.paths.extend((manifest,table,table.with_name('creation.c')))
        with table.open(encoding='utf-8') as stream:
            # 허용범위는내보낸실제몸체만큼만넓힌다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.spec=dict(self.spec,entries={name:self.r[name] for name in self.r if name in ('main','near','time')});self.boundaries={self.p['new']:'new',self.r['regular']:'regular',self.r['construction']:'construction',self.r['base']:'base'}

    def on_instruction(self,mu,address,size,data):
        """Rune분기/조회는실행하고new/localRegular/ConstructionStart/공통postPop요청만현재인자로기록한다."""
        boundary=getattr(self,'boundaries',{}).get(address)
        if not boundary:return super().on_instruction(mu,address,size,data)
        self.stub_calls[boundary]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);a=[self.read(sp+4*i) for i in range(1,4)];purge=0;result=0
        if boundary=='new':
            assert a[0]==40;self.events.append('N:40');result=MEMORY if self.case[2] else 0
            if self.case[-2]==1:self.put(self.r['suppress'],1-self.read(self.r['suppress']))
        elif boundary=='regular':
            assert this==MEMORY and a[:3]==[0x304,8,bits(0.01)];purge=12;result=MEMORY;self.events.append('R:'+','.join(map(str,a)))
            if self.case[-2]==2:self.put(self.slot(8)+(34 if self.patch else 32),2,1)
        elif boundary=='construction':assert a[0]==8;self.events.append('C:'+','.join(map(str,a[:2])))
        elif boundary=='base':assert this==self.slot(8);purge=4;self.events.append('B:8,'+str(a[0]))
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,case,control):
        """현재raw/해시/player입력과실제시간helper/전체postPop효과를원본에독립제공한다."""
        self.case=case;flags,profile,reserve,word,playerFlag,suppress,mutation,extra=case;self.mu.mem_write(POOL,bytes(12*self.stride));self.mu.mem_write(TYPES,bytes(0x40000));self.mu.mem_write(PLAYERS,bytes(0x20000));objects=[(8,166,0,(0,1,8)[extra],bits(40.75),bits(41.25),1,1,0,1)]
        if profile:
            x,y=(80.75,81.25) if profile==7 else (42.75,43.25) if profile==6 else (40.75,41.25);objects.append((6,165,2 if profile==4 else 0,8 if profile==3 else 1 if profile==5 else 0,bits(x),bits(y),3 if profile==6 else 1,3 if profile==6 else 1,0,1))
        if profile!=8:objects.append((7,164,0,0,bits(80.75 if profile==9 else 40.75),bits(81.25 if profile==9 else 41.25),1,1,0,1))
        self.setup(objects);self.put(self.slot(8)+(34 if self.patch else 32),1,1);self.put(self.slot(6)+(34 if self.patch else 32),2 if profile==2 else 1,1);self.put(self.slot(7)+12,word,2);self.put(self.r['players'],PLAYERS);self.put(PLAYERS+word*self.r['playerStride']+0x50,playerFlag);self.put(self.r['suppress'],suppress);self.put(self.p['daisType'] if 'daisType' in self.p else (0x5412ec if self.patch else 0x51cbd8),165);self.put(self.r['altar'],164)
        if not self.patch:self.put(0x546778,0)
        seconds=self.call('time',[],self.slot(8),0,control);near=self.call('near',[],self.slot(8),0,control) if self.patch else None;self.call('main',[flags],self.slot(8),4,control);self.rune_returns+=1
        return [str(seconds),str(near) if near is not None else '-', ';'.join(self.events) or '-', ';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (6,7,8)),str(self.read(self.r['suppress']))]


def generate(smoke=False):
    """두PE/정밀도에서정상반환하는고유입력만새Runefixture와출처SHA로고정한다."""
    cases=inputs();cases=cases[::53] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    # 현재source로각판본의원본body를독립실행하며반복반환은합산하지않는다.
    for edition in SPEC:
        oracle=RuneOracle(edition)
        for index,case in enumerate(cases):
            try:
                result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,','.join(map(str,case)),*result])
        editions[edition]=dict(cases=len(cases),returns=oracle.rune_returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update(oracle.paths);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.write_text('# edition flags,profile,reserve,ownerWord,playerFlag,suppress,mutation,extra seconds near effects slots6,7,8 suppress\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');files.update(ROOT/name for name in ('cpppj/src/o/RawRunePostPop.h','cpppj/src/o/RawRunePostPop.cpp','cpppj/tests/RunePostPopTests.cpp','cpppj/recovery-runepostpop-lineage-evidence.json','tools/decomp_templeritual_oracle.py','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',total=len(rows),editions=editions,files={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(files)},game_runs=0,smoke=smoke,limits=['실제Rune postPop/근처같은owner신전/altar원래ownerWORD/player시간조회몸체는실제PE다.','new/localRegular/ConstructionStart/공통PostPop은명시요청경계이며실제C++소비자는기존Host/Construction/Base를재사용한다.','기존공통/건설2868관찰은새Rune고유입력에합산하지않는다. 실제게임/GUI/완주검사아님.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Rune postPop native 고유 입력:',len(rows))


def verify():
    """현재Rune출처와고유입력/전체실제반환/예약경계를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')];assert len(rows)==report['total'] and report['game_runs']==0
    for name,value in report['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in report['editions'].items():
        assert item['returns']==item['cases']*2 and item['asserts']==0 and item['entries'][f'{SPEC[edition]["main"]:08x}']==item['returns']
        if not report['smoke']:assert {row[2] for row in rows if row[0]==edition}=={'7','10','25'}
    print('Rune postPop 현재SHA/실제반환 감사:',len(rows))


# 명시호출에서만새격리원본관찰을생성하거나현재출처를감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
