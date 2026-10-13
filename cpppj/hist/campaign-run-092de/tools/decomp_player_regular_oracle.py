#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Player Handler와 기존 State28/Notify 및 새Regular2의 같은 Form/Kernel을 관찰한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_player_world_oracle import PlayerOracle,PLAN as PLAYER,ARENA,REGULAR,PROCESS,CONTROLS
from decomp_sid_oracle import POOL
from decomp_owner_oracle import digest
from decomp_oracle import ROOT,STACK,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 원본이반환한float10을fstpfloat32로관찰하는도구명령과저장주소다.
CALLER,RESULT=ARENA+0xc000,ARENA+0xc100
FIXTURE=ROOT/'cpppj/tests/fixtures/player-regular-x86.tsv'
REPORT=ROOT/'cpppj/player-regular.json'
KEYS=('kind','owner','local','template','battle','event','count','reserve','payload','mine','transient','prevent','mission','dot','end','mutation')
PLAN={
 'originals':dict(handle=0x490810,state=0x4bd550,notify=0x4bd540,notifyBody=0x4bd360,log=0x4c2630,compare=0x4e65ed,templates=0x5c89f0,mission=0x594fa0,depth=0x5c89b8,create=0x44d1b0,save=0x44d8d0),
 'originalCD':dict(handle=0x407ef0,state=0x427920,notify=0x427910,notifyBody=0x4275d0,log=0x48ca70,compare=0x4f2980,templates=0x564bd0,mission=0x52e16c,depth=0x5178d4,create=0x44c3c0,save=0x44cc40)}


def sample(**changes):
    """실제handler/기존State범위의명시 입력만준비하고원본출력을계산하지않는다."""
    row=dict(zip(KEYS,(0,1,1,2,1,1,1,1,0x40a00000,1,0,0,1,0,28,0)));row.update(changes);return row


def inputs():
    """현재두owner/count0·1·event1/2/미지정·battle/예약실패와Notify/State진단을교차한다."""
    rows=[]
    # 최초count0은다른사건에도payload를반환하며소유자0도그대로비교한다.
    for index,values in enumerate(itertools.product((0,1,8),(1,8),(2,8),(0,1),(1,2,3),(0,1),(0,1))):
        owner,local,template,battle,event,count,reserve=values;rows.append(sample(owner=owner,local=local,template=template,battle=battle,event=event,count=count,reserve=reserve,payload=(0x40a00000,0x3da3d70a,0x80000000)[index%3]))
    # effect후global변경은실제본문이다음조건/TemplateNotify를재조회하는지확인한다.
    for mutation in range(1,5):rows.append(sample(event=1 if mutation<3 else 2,battle=0 if mutation==2 else 1,mutation=mutation))
    # Notify는name/isMine/순간진단/mode/저장거부를각각구분한다.
    for mine,transient,prevent,mission,dot in itertools.product((0,1),(0,1),(0,1),(0,1),(0,1)):
        rows.append(sample(kind=2,mine=mine,transient=transient,prevent=prevent,mission=mission,dot=dot))
    # shortState는실제DataManager확장진입에서거부하며정상return/assert와합치지않는다.
    for end in (0,1,27,28,32):rows.append(sample(kind=1,end=end))
    return rows


class PlayerRegularOracle(PlayerOracle):
    """14의실제Factory/Player/Regular/Form/Kernel를그대로재사용하고새15몸체만추가한다."""
    def __init__(self,edition):
        """현재별도export4/6+3/3의실제불연속body를추가허용한다."""
        super().__init__(edition);self.r=PLAN[edition];self.hits=collections.Counter();self.external=collections.Counter()
        # 최초PlayerRegular와Statehelper원문을덮지않고별도출처로참조한다.
        for name in ('player-regular','player-regular-state-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8-sig') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.hits[row['entry']]=0
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)));self.allowed.append((CALLER,CALLER+11))

    def on_instruction(self,mu,address,size,data):
        """strcmp/로그/진단/힙예약만관찰경계며실제lookup/길이확인/Notify/Regular부착을실행한다."""
        if not hasattr(self,'r'):return super().on_instruction(mu,address,size,data)
        r=self.r;key=f'{address:08x}'
        if key in self.hits:self.hits[key]+=1
        if address==r['state']:
            owner=(mu.reg_read(UC_X86_REG_ECX)-r['templates'])//20;self.events.append('T:'+str(owner))
        if address==(0x4bd5f6 if self.edition=='originals' else 0x427a3c) and self.regularCase['mutation']==3:
            mu.mem_write(self.p['template'],struct.pack('<I',3))
        if address==r['notify']:
            owner=(mu.reg_read(UC_X86_REG_ECX)-r['templates'])//20;self.events.append('C:'+str(owner))
            if self.regularCase['mutation']==4:mu.mem_write(r['templates']+owner*20+8,bytes(4))
        if address in (r['create'],r['save']) or (address==self.p['new'] and self.read(mu.reg_read(UC_X86_REG_ESP)+4)==60):self.status='boundary';self.boundary=key;self.external[key]+=1;mu.emu_stop();return
        if address not in (r['compare'],r['log'],self.p['assert_']):return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);args=[self.read(sp+4+index*4) for index in range(3)]
        self.external[key]+=1
        if address==r['compare']:
            left=bytes(mu.mem_read(args[0],6)).split(b'\0')[0];right=bytes(mu.mem_read(args[1],6)).split(b'\0')[0];assert left.lower()==right.lower()==b'state';value=0
        elif address==r['log']:
            assert args[0]==29;text=bytes(mu.mem_read(args[1],64)).split(b'\0')[0];self.events.append('L:29:'+text.hex());value=0
            if self.regularCase['mutation']==1 and text==b'SORE LOSER TEST: ':mu.mem_write(self.p['battle'],bytes(4))
            if self.regularCase['mutation']==2 and text==b'SORE LOSER TEST: ':mu.mem_write(self.p['battle'],struct.pack('<I',1))
        else:
            self.events.append('A:'+str(args[2]));self.diagnostics+=1;value=0
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,sp+4);mu.reg_write(UC_X86_REG_EIP,self.read(sp))

    def run_regular(self,case,control):
        """현재metadata/State입력만준비하고원본parent/stream/Template/Regular상태를관찰한다."""
        self.regularCase=case;self.status='return';self.boundary='-';r=self.r;mu=self.mu;self.control=control
        now=struct.unpack('<Q',struct.pack('<d',17.25))[0];super().prepare((0,case['owner'],case['local'],case['template'],1,0,0,case['reserve'],0,now),control)
        self.sid=self.creation_step('Create',152,2)[0];mu.mem_write(POOL+self.sid*self.stride+(34 if self.edition=='originals' else 32),bytes((case['owner'],)));mu.mem_write(self.p['battle'],struct.pack('<I',case['battle']));mu.mem_write(r['mission'],struct.pack('<I',case['mission']));mu.mem_write(r['depth'],struct.pack('<I',0xffffffff))
        mu.mem_write(CALLER,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(CALLER+11)))
        self.events=[];self.diagnostics=0;self.write_ranges.extend(((r['templates'],r['templates']+180),(r['depth'],r['depth']+4)))
        # 각9개현재Template manager/단일State노드/물리data는분석ARENA의독립영역이다.
        for owner in range(9):
            manager=ARENA+0x9000+owner*0x100;stream=ARENA+0xa000+owner*0x40;data=ARENA+0xb000+owner*0x100
            mu.mem_write(r['templates']+owner*20,struct.pack('<5I',manager,0,case['mine'],case['transient'],case['prevent']));mu.mem_write(manager,bytes(0x100));mu.mem_write(manager+4,struct.pack('<II',stream,stream));mu.mem_write(manager+12,b'.map\0' if case['dot'] else b'map.fort\0')
            mu.mem_write(stream,struct.pack('<4I',data,7,case['end'],64)+b'State\0'+bytes(0x3c-22));mu.mem_write(data,bytes((0x70+owner,))+bytes((index*7+owner)&255 for index in range(1,64)))
        if case['kind']==0:
            mu.mem_write(STACK,struct.pack('<4I',CALLER,case['event'],case['count'],case['payload']));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,POOL+self.sid*self.stride);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.emu_start(r['handle'],STOP,count=3000000);returned=str(self.read(RESULT))
        else:
            self.registers(r['templates']+case['template']*20);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);mu.mem_write(STACK,struct.pack('<I',STOP));mu.emu_start(r['state'] if case['kind']==1 else r['notify'],STOP,count=3000000);returned='-'
        if self.status=='return':
            assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+(12 if case['kind']==0 else 0) and mu.reg_read(UC_X86_REG_FPCW)==control and not(mu.reg_read(UC_X86_REG_FPSW)&0x3800) and self.read(0)==0
            self.returns+=1
        else:returned='-'
        # 현재getter뒤owner의재조회입력은본체호출중간에후속helper에서관찰한다.
        regular=','.join(str(self.read(REGULAR+offset,width)) for offset,width in ((16,8),(24,4),(28,4),(32,4))) if any(event.startswith('B:') for event in self.events) else '-'
        form=self.read(REGULAR+4) if regular!='-' else 0;proc=PROCESS[self.edition];kernel=struct.unpack('<'+'I'*proc['kernel_slots'],mu.mem_read(proc['kernel'],4*proc['kernel_slots']))
        current=[]
        # 전체9기록의State현재pos/end/전체28BYTE를직접보존한다.
        for owner in range(9):current.append(','.join((str(self.read(ARENA+0xa000+owner*0x40+4)),str(self.read(ARENA+0xa000+owner*0x40+8)),bytes(mu.mem_read(ARENA+0xb000+owner*0x100,28)).hex())))
        return [self.status,returned,self.boundary,';'.join(self.events) or '-',bytes(mu.mem_read(r['templates'],180)).hex(),';'.join(current),str(self.read(self.p['local'])),str(self.read(self.p['template'])),str(self.read(self.p['battle'])),str(self.read(r['depth'])),regular,bytes(mu.mem_read(POOL+self.sid*self.stride,self.stride)).hex(),str(form),bytes(mu.mem_read(POOL+form*self.stride,self.stride)).hex() if form else '-',','.join(str(index) for index,value in enumerate(kernel) if value) or '-',str(self.read(proc['rotation']) if proc['rotation'] else 0),str(self.diagnostics)]


def generate(smoke):
    """실제parent/helper의정상/assert/미복원경계를구분하고동일두x87결과만보존한다."""
    rows=[];editions={};files={Path(__file__).resolve(),ROOT/'tools/ghidra/player-regular-functions.json',ROOT/'tools/ghidra/player-regular-state-helper-functions.json'}
    for name in ('RawPlayerRegular','RawPlayerTemplateState'):
        for extension in ('h','cpp'):files.add(ROOT/f'cpppj/src/o/{name}.{extension}')
    # 각판본의기존Player/body재사용은새몸체입력수에중복합산하지않는다.
    for edition in PLAN:
        oracle=PlayerRegularOracle(edition);cases=inputs();counts=collections.Counter()
        if smoke:cases=[cases[0],cases[37],cases[57],cases[96],cases[111],cases[115],*cases[288:292],*cases[-12:]]
        for case in cases:
            observed=[oracle.run_regular(case,control) for control in CONTROLS];assert observed[0]==observed[1],(edition,case);row=observed[0];category='boundary' if row[0]=='boundary' else 'assert' if int(row[-1]) else 'normal';counts[category]+=1;rows.append([edition,','.join(str(case[key]) for key in KEYS),*row])
        editions[edition]=dict(unique=len(cases),classification=dict(counts),returns=oracle.returns,actual_body_calls=dict(oracle.hits),external_boundaries=dict(oracle.external));files.update(oracle.exports);files.update((ROOT/edition/'Netstorm.exe' if edition=='originals' else ROOT/'originalCD/NETSTORM.EXE',ROOT/edition/'netstorm.tarc',oracle.shapeFile));print(edition,editions[edition],flush=True)
    if smoke:return
    for name in ('decomp_player_world_oracle','decomp_owner_oracle','decomp_postpop_oracle','decomp_display_oracle','decomp_pop_oracle','decomp_creation_oracle','decomp_sid_oracle','decomp_oracle','decomp_process_oracle','shp','taff'):files.add(ROOT/f'tools/{name}.py')
    FIXTURE.write_text('# edition\t'+','.join(KEYS)+'\tstatus\treturnbits\tboundary\tevents\tTemplate180\tState9\tlocal\ttemplate\tbattle\tdepth\tRegularFields\tParentRaw\tFormSid\tFormRaw\tKernelPids\tRotation\tAsserts\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',date='2026-10-14',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},controls=list(CONTROLS),game_runs=0,os_calls=0,limitations=['실제Handle/getExistingState/Notify/lookup/최소길이helper와기존Factory/Regular/Form/Kernel를실행한다.','State이름ASCII일치비교/로그/진단/heap40은명시경계다.없는State/shortState힙확장/파일저장은미복원진입에서거부하며assert정상반환으로합치지않는다.','CPP전수/같은Host시간0→5→65소비자는후속이다.14Player1680/반복정밀도는새고유수에더하지않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');args=parser.parse_args();generate(args.smoke)
