#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ColorSort 생성 부모43b430/기본색띠 부모43b300의 실제 호출순서와 현재 전역을 먼저 고정한다."""
import argparse
from collections import Counter
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_FPCW

PRIMARY, SEARCH, GRADIENT, COUNT, LOADED = 0x555d88,0x555dd8,0x555d78,0x53199c,0x555e08  # 실제 부모가 재조회하는 현재 슬롯이다.
BLOCKS = (POOL+0x1000,POOL+0x6000,POOL+0xc000)  # 서로 분리된 실제0x3800/0x5008/0x5020 장치 블록이다.
TOKENS = {0:0,11:BLOCKS[0],22:BLOCKS[1],33:BLOCKS[2]}  # 하위효과 입력의 포인터만 명시한다.
FIXTURE,REPORT = ROOT/'cpppj/tests/fixtures/colorsort-startup-x86.tsv',ROOT/'cpppj/colorsort-startup-native.json'  # 제품전에 고정하는 새 부모 자료다.


def cases():
    """NULL 할당/옛 슬롯/진단 및 하위 현재값 변경과 중단을 독립 입력한다."""
    rows = [('startup',loaded,old,old,old,profile,'none') for loaded,old,profile in itertools.product((0,1),(0,11),range(8))]
    rows += [('gradient',1,11,22,old,profile,'none') for old,profile in itertools.product((0,33),(0,4))]
    for mutation in ('assert-sort22','assert-stop','free1-sort33','free1-search0','free1-stop','free2-sort22','alloc1-source','alloc1-stop','sort-current33','sort-return0','sort-stop','search-sort22','search-count91','search-stop','qsort-count91','qsort-search33','qsort-stop','alloc3-count7','alloc3-stop','gradient-sort22','gradient-return0','gradient-stop'):
        rows.append(('startup',0 if mutation.startswith('assert') else 1,11,22,33,0,mutation))
    return rows


class Oracle(MemoryOracle):
    """두 부모는 PE가 실행하고 하위 생성/정렬/할당은 명시 장치이며 빈 성공을 제품에 제공하지 않는다."""
    def __init__(self):
        super().__init__()
        self.mu.mem_map(0,0x1000)  # 원문 SEH 연결 DWORD만 격리 물리로 제공한다.
        self.total=0

    def event(self,kind,text):
        """실제 인자를 먼저 남기고 종류별 호출 위치를 반환한다."""
        self.events.append(text);self.calls[kind]+=1
        return kind+str(self.calls[kind])

    def change(self,boundary):
        """하위 복귀 직전의 실제 현재값 변경만 제공한다."""
        if self.mutation==boundary+'-stop':
            self.cause=boundary;self.mu.emu_stop()
        elif self.mutation==boundary+'-sort22':self.put(PRIMARY,TOKENS[22])
        elif self.mutation==boundary+'-sort33':self.put(PRIMARY,TOKENS[33])
        elif self.mutation==boundary+'-current33':self.put(PRIMARY,TOKENS[33])
        elif self.mutation==boundary+'-search0':self.put(SEARCH,0)
        elif self.mutation==boundary+'-search33':self.put(SEARCH,TOKENS[33])
        elif self.mutation==boundary+'-count91':self.put(COUNT,91)
        elif self.mutation==boundary+'-count7':self.put(COUNT,7)
        elif self.mutation==boundary+'-source':self.mu.mem_write(TEXT,b'changed\0')

    def instruction(self,mu,address,size,data):
        """원문 직접store와 인자 캡처/재조회는 그대로 실행한다."""
        self.total+=1;esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,16)
        if address==0x4e0620:
            assert args[2]==0x1c3 and self.string(args[0])==b'paletteLoaded'
            self.event('assert','A:451');self.change('assert')
            if not self.cause:self.ret()
            return
        if address==0x4e4354:
            boundary=self.event('free','F:'+str(args[0]));self.change(boundary)
            if not self.cause:self.ret()
            return
        if address==0x4e4391:
            amount=args[0];index={0x3800:0,0x5008:1,0x5020:2}[amount]
            result=0 if self.profile&(1<<index) else BLOCKS[index]
            boundary=self.event('alloc','N:'+str(amount)+':'+str(result));self.change(boundary)
            if not self.cause:self.ret(result)
            return
        if address==0x439de0:
            target=mu.reg_read(UC_X86_REG_ECX);self.event('sort','C:'+str(target)+':'+str(args[0])+':'+bytes(mu.mem_read(args[0],8)).hex())
            mu.mem_write(target,b'SORT');self.change('sort')
            if not self.cause:self.ret(0 if self.mutation=='sort-return0' else target,4)
            return
        if address==0x43a8e0:
            target=mu.reg_read(UC_X86_REG_ECX);self.event('search','S:'+str(target)+':'+str(self.words(PRIMARY,1)[0]))
            mu.mem_write(target,b'BASE');self.change('search')
            if not self.cause:self.ret()
            return
        if address==0x4e6010:
            assert args[1:]==args[1:] and args[1:4]==(256,80,0x43a1e0)
            self.event('qsort','Q:'+','.join(map(str,args[:4])));self.change('qsort')
            if not self.cause:self.ret()
            return
        if address==0x43ad20:
            target=mu.reg_read(UC_X86_REG_ECX);raw=bytes(mu.mem_read(esp+4,60))
            self.event('gradient','G:'+str(target)+':'+str(self.words(PRIMARY,1)[0])+':'+raw.hex())
            mu.mem_write(target,b'GRAD');self.change('gradient')
            if not self.cause:self.ret(0 if self.mutation=='gradient-return0' else target,60)
            return
        if not (0x43b300<=address<0x43b539):
            raise RuntimeError('ColorSort 부모 허용밖 '+hex(address))

    def sample(self,case,control):
        """현재 전역·하위 물리·source를 새로 명시하고 정상/중단을 구별한다."""
        kind,loaded,primary,search,gradient,self.profile,self.mutation=case
        self.calls,self.events,self.cause=Counter(),[],''
        for address,value in ((LOADED,loaded),(PRIMARY,TOKENS[primary]),(SEARCH,TOKENS[search]),(GRADIENT,TOKENS[gradient]),(COUNT,0xffffffff)):
            self.put(address,value)
        for address in BLOCKS:self.mu.mem_write(address,b'\xa5'*0x5020)
        self.mu.mem_write(TEXT,b'original\0');self.mu.mem_write(STACK-0x200,b'\xcd'*0x220);self.put(STACK,STOP);self.put(STACK+4,TEXT)
        self.put(0,0);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.emu_start(0x43b430 if kind=='startup' else 0x43b300,STOP,count=10000)
        values=','.join(map(str,(self.words(PRIMARY,1)[0],self.words(SEARCH,1)[0],self.words(GRADIENT,1)[0],self.words(COUNT,1)[0],self.words(LOADED,1)[0])))
        raw=':'.join(bytes(self.mu.mem_read(address,4)).hex() for address in BLOCKS)+':'+bytes(self.mu.mem_read(TEXT,8)).hex()
        return '\t'.join((str(control),','.join(map(str,case)),self.cause or 'return',values,raw,';'.join(self.events)))


def main():
    """먼저 원문만 고정하고 이후에는 같은 최초 SHA를 감사한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,value in report['sha256'].items():
            if digest(ROOT/name)!=value:raise RuntimeError('ColorSort 부모 최초 SHA '+name)
        print('ColorSort 부모 최초보존',report['observations']);return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('ColorSort 부모 최초 자료 덮어쓰기 금지')
    oracle=Oracle();rows=[];statuses=Counter()
    for case in cases():
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('ColorSort 부모 정밀도 차이')
        rows.extend((first,second));statuses[first.split('\t')[2]]+=2
    FIXTURE.write_text('# control / kind,loaded,primary,search,gradient,allocationFailureProfile,mutation / completion / globals5 / threePrefixesAndSource / trace\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_colorsort_startup.py','cpppj/tests/fixtures/colorsort-startup-x86.tsv')
    report={'status':'단위385후속·제품전최초고정','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','observations':len(rows),'completions':dict(statuses),'instructions':oracle.total,'precisionDifferences':0,'gameExecuted':False,'guiExecuted':False,
            'scope':'실제43b430/43b300 부모만. 하위439de0/43a8e0/qsort/43ad20는명시생성장치이며전체ColorSort물리완료를주장하지않음.진단뒤계속·free뒤무조건0·생성반환값·현재count캡처·부분중단.',
            'sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('observations','completions','instructions')}))


if __name__=='__main__':main()
