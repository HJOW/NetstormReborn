#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제Dais/Altar 최초Pop·제단삭제전처리·Player SID쌍정리를원본PE의제한x86에서관찰한다."""
import argparse,collections,csv,itertools,json,struct
from pathlib import Path
from decomp_altarsacrifice_oracle import AltarOracle,ROOT,HEAP,VTABLE,STACK,STOP,FPU_CONTROLS,PLAYER_STORAGE,bits,sha
from unicorn.x86_const import *

# 같은판본의실제수명body/마지막공통함수와현재로딩·초기시각·지면/UI경계다.
SPEC={
 'originals':dict(dpp=0x449320,app=0x448ad0,apre=0x44a4d0,dpost=0x449580,apost=0x448b60,post=0x44b9e0,postpop=0x44bf80,regular=0x496e80,loading=0x5c89b8,time=0x5c85f0,surface=0x40eaf0,hasIsland=0x4acdf0,island=0x4acda0,option=0x441090,first=0x8c,second=0x94,home=0x78),
 'originalCD':dict(dpp=0x481360,app=0x4835c0,apre=0x4836b0,dpost=0x481620,apost=0x483660,post=0x461b40,postpop=0x4621c0,regular=0x48efd0,loading=0x5178d4,time=0x564db8,surface=None,hasIsland=0x4acad0,island=0x4acaa0,option=0x4a9400,first=0x84,second=0x8c,home=0x70),
}
SURFACE_STORAGE=HEAP+0x10000 # CD의 직접 표면조회가 읽는256x256 WORD 입력의 전용별도영역이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/daislifecycle-current-x86.tsv' # 각실제입력의요청/raw/Player필드다.
REPORT=ROOT/'cpppj/daislifecycle-current.json' # 마지막전체Damageable경계를분리한현재근거다.

def inputs():
    """새5/5 entry의실제각게이트·현재owner·SID일치/비일치만구성한다."""
    result=[]
    def add(mode,**changes):
        """새입력전역을선언하고기대결과는계산하지않는다."""
        value=dict(mode=mode,flags=0,frame=14,owner=1,local=1,saved=1,suppress=0,initial=0,loading=0,boss=1,reserve=1,profile=0,dead=0,runeExtra=0,first=5,second=5);value.update(changes);result.append(value)
    # Dais최초frame/player/count/로딩reset/boss gates를독립교차한다.
    for flags,frame,owner,suppress,initial,loading,boss in itertools.product((0,1,2,3,7,8),(3,14,64),(0,1,2),(0,1),(0,1),(0,1),(0,1)):add('dpp',flags=flags,frame=frame,owner=owner,suppress=suppress,initial=initial,loading=loading,boss=boss)
    # 저장count세단계와현재표면·경고옵션의서로다른분기다.
    for saved,profile in itertools.product((0,1,2),range(5)):add('dpp',flags=1,saved=saved,profile=profile,boss=0)
    for flags,reserve in itertools.product((0,1,2,3,7),(0,1)):add('app',flags=flags,reserve=reserve)
    for flags,dead,extra,boss in itertools.product((0,8,0x400000),(0,1),(0,1,8,9),(0,1)):add('apre',flags=flags,dead=dead,runeExtra=extra,boss=boss)
    for owner,first,second,flags in itertools.product((0,1,2),(5,77),(5,78),(0,0x400000)):add('dpost',owner=owner,first=first,second=second,flags=flags)
    for flags in (0,8,0x400000,0x48):add('apost',flags=flags)
    return list({json.dumps(case,sort_keys=True):case for case in result}.values())

class LifecycleOracle(AltarOracle):
    """기존제단입력준비/실제helper를재사용하고새5/5 몸체를각독립진입에서실행한다."""
    def __init__(self,edition):
        """현재정밀readOnly범위와필수외부ABI만추가한다."""
        super().__init__(edition);self.l=SPEC[edition];self.lifecycle_returns=0;self.mu.mem_map(SURFACE_STORAGE,0x20000)
        paths=[ROOT/f'extracted/daislifecycle/{edition}/{suffix}' for suffix in ('creation.c','functions.tsv')];self.paths.extend(paths);self.paths.append(ROOT/'tools/ghidra/daislifecycle-functions.json')
        with paths[1].open(encoding='utf-8') as stream:
            # 미선언원본주소는계속실행금지다.
            for row in csv.DictReader(stream,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        for name in ('post','postpop','regular','surface','hasIsland','island','option'):
            if self.l[name]:self.boundaries[self.l[name]]=name

    def on_instruction(self,mu,address,size,data):
        """새생성/현재표면/UI/마지막공통요청만격리하고모든새수명분기는PE명령이다."""
        label=self.boundaries.get(address)
        if not getattr(self,'lifecycle_active',False) or label not in ('post','postpop','regular','surface','hasIsland','island','option','create'):return super().on_instruction(mu,address,size,data)
        if address in self.entries:self.hits[self.entries[address]]+=1
        self.stub_calls[label]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(sp+4*index) for index in range(1,5)];purge=0;result=0
        if label in ('post','postpop'):purge=4;self.events.append(label+':'+str(self.sid_of(this))+','+str(args[0]))
        elif label=='regular':purge=12;assert this==HEAP+0x6000;self.events.append('REG:'+','.join(map(str,args[:3])));result=this
        elif label=='surface':self.events.append('SURFACE:'+','.join(map(str,args[:2])));result=9 if self.life['profile'] else 0
        elif label=='hasIsland':self.events.append('HAS:'+str(self.sid_of(this)));result=self.life['profile']!=1
        elif label=='island':self.events.append('ISLAND:'+str(self.sid_of(this)));result=17 if self.life['profile']==3 else 18
        elif label=='option':assert bytes(mu.mem_read(args[0],80)).split(b'\0')[0]==b'noShowWarnCantUpgrade';self.events.append('OPTION:noShowWarnCantUpgrade');result=self.life['profile']==4
        elif label=='create':assert args[:2]==[164,0];self.events.append('C:164,0,8');result=self.slot(8);self.mu.mem_write(result,bytes(self.stride));self.put(result,VTABLE);self.put(result+10,164,1);self.put(result+11,4,1)
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,life,control):
        """현재raw/Player쌍/초기시각을원본입력으로쓰고효과/최종필드를직접읽는다."""
        # 기존관찰을새입력수나entry호출수에더하지않고초기메모리준비에만사용한다.
        previous_hits=self.hits.copy();previous_stubs=self.stub_calls.copy();self.lifecycle_active=False
        base=dict(mode='count',daisFrame=life['frame'],altarFrame=3,count=0,seed=0,boss=life['boss'],reserve=life['reserve'],captured=0,oldOwner=2,alliance=0,local=life['local'],suppress=life['suppress'],forms=0,extra=0,owner=life['owner']);super().run(base,control);self.hits=previous_hits;self.stub_calls=previous_stubs;self.life=life;self.lifecycle_active=True
        self.put(self.slot(5)+11,2 if life['dead'] else 0,1);self.put(self.l['loading'],life['loading']);self.mu.mem_write(self.l['time'],struct.pack('<d',float(life['initial'])));self.mu.mem_write(SURFACE_STORAGE,bytes(0x20000))
        self.put(self.slot(9),VTABLE);self.put(self.slot(9)+10,166,1);self.put(self.slot(9)+14,bits(40.75));self.put(self.slot(9)+18,bits(41.25));self.put(self.slot(9)+self.o['extra'],life['runeExtra'],1)
        if life['mode']=='apre':
            # 주변 Rune는 같은 현재해시1 버킷의 머리에 입력한다.
            address=self.bases[1]+((41//2)*128+40//2)*2;self.put(self.slot(9)+4,self.read(address,2),2);self.put(address,9,2)
        else:
            # 경고 입력의9는 표면SID이며 해시 후보로 추가하지 않는다.
            self.put(self.slot(9)+10,91,1)
        if not self.patch:self.put(0x5670cc,SURFACE_STORAGE);self.put(SURFACE_STORAGE+(42*256+41)*2,9 if life['profile'] else 0,2)
        for owner in range(9):
            player=PLAYER_STORAGE+owner*self.a['playerStride'];self.put(player+self.a['grantFlag'],life['saved'],1);self.put(player+self.l['home'],17);self.put(player+self.l['first'],life['first']);self.put(player+self.l['first']+4,123);self.put(player+self.l['second'],life['second']);self.put(player+self.l['second']+4,124)
        self.events=[]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(7 if life['mode'] in ('app','apre','apost') else 5));self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.mem_write(0,bytes(4));self.mu.mem_write(STACK,struct.pack('<II',STOP,life['flags']));self.write_ranges=[(HEAP,HEAP+0x10000)];self.mu.emu_start(self.l[life['mode']],STOP,count=4000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8 and self.mu.reg_read(UC_X86_REG_FPCW)==control and not(self.mu.reg_read(UC_X86_REG_FPSW)&0x3800) and self.read(0)==0
        self.lifecycle_returns+=1
        playerfields=[]
        # 현재Player의실제폭BYTE/DWORD를직접읽는다. mirror는독립필드다.
        for owner in range(9):
            player=PLAYER_STORAGE+owner*self.a['playerStride'];playerfields.append(','.join(map(str,(self.read(player+self.a['grantFlag'],1),self.read(player+self.l['first']),self.read(player+self.l['first']+4),self.read(player+self.l['second']),self.read(player+self.l['second']+4)))))
        return [';'.join(self.events) or '-', ';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (5,7,8,9)), ';'.join(playerfields)]

def generate(smoke):
    """새5/5의동일입력을두정밀도로검사하고기존Altar/Dais관찰은합산하지않는다."""
    cases=inputs();cases=cases[::97] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    # 새몸체/검사와전체import경로를현재SHA로연결한다. 기존관찰의입력수는더하지않는다.
    files.update(ROOT/name for name in ('cpppj/src/o/RawDaisLifecycle.h','cpppj/src/o/RawDaisLifecycle.cpp','cpppj/tests/DaisLifecycleTests.cpp','tools/decomp_altarsacrifice_oracle.py','tools/decomp_daisregular_oracle.py','tools/decomp_runepostpop_oracle.py','tools/decomp_templeritual_oracle.py','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    for edition in SPEC:
        oracle=LifecycleOracle(edition)
        for index,case in enumerate(cases):
            try:result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,json.dumps(case,sort_keys=True,separators=(',',':')),*result])
        editions[edition]=dict(returns=oracle.lifecycle_returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update(oracle.paths);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.write_text('# edition inputJSON effects raw5,7,8,9 playerFlag,first,mirror,second,mirror\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),smoke=smoke,game_runs=0,editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},scope='Dais/Altar5/5 상위수명. 가상공간/Factory/소리/UI/지면조회 및 마지막전체Damageable는명시경계. 전체파괴/의식완료주장없음.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('다이스/제단 새 수명관찰:',len(rows))

def verify():
    """현재whole-source SHA와고유입력/두정밀도반환을전수감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==data['total'] and not data['smoke'] and data['game_runs']==0;assert len({(row[0],row[1]) for row in rows})==len(rows)
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in data['editions'].items():assert item['returns']==2*sum(row[0]==edition for row in rows) and item['asserts']==0
    print('현재다이스/제단수명SHA·정상반환감사:',len(rows))

# 명시CLI요청에서만새독립관찰을실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
