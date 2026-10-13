#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""다이스 Progress/Rune304/감김306과 실제 알터 payload를 별도 제한 x86 입력으로 관찰한다."""
import argparse,csv,itertools,json,struct
from pathlib import Path
from decomp_altarsacrifice_oracle import AltarOracle,ROOT,HEAP,VTABLE,STACK,STOP,CALLER,TEMP,FPU_CONTROLS,PLAYER_STORAGE,bits,sha
from unicorn.x86_const import *

# 새 상위 진입점과 장치 요청의 실제 ABI 주소다. 이전 관찰 입력을 다시 합산하지 않는다.
SPEC={
 'originals':dict(progress=0x449f40,rune=0x44a3a0,advance=0x44a200,load=0x4a8ee0,soundCtor=0x4ab240,wall=0x460d70,soundReturns=(0x44a06e,0x44a0ee)),
 'originalCD':dict(progress=0x482a20,rune=0x4846b0,advance=0x482df0,load=0x437d20,soundCtor=0x453800,wall=0x4011b0,soundReturns=(0x482ad0,0x482b3a)),
}
SOUND_MEMORY=(HEAP+0x7000,HEAP+0x7100) # 각 독립 new가 반환하는 격리 음향 프로세스다.
FIXTURE=ROOT/'cpppj/tests/fixtures/daisprogress-x86.tsv' # 새 상위 입력/반환/효과/음향시각/raw 기록이다.
REPORT=ROOT/'cpppj/daisprogress.json' # 현재 출처와 외부 경계를 구분한 독립 근거다.

def inputs():
    """분기 입력을 구성하고 결과는 원본 몸체에서만 얻는다."""
    cases=[]
    def add(mode,**changes):
        """기본 입력에 각 원본 진입점의 현재 전역만 덮어쓴다."""
        value=dict(mode=mode,daisFrame=47,altarFrame=3,count=0,event=24,payload=0.12,seed=0,boss=1,reserve=1,captured=158,oldOwner=2,alliance=0,local=1,suppress=0,forms=0,extra=0,owner=1,priestFrame=0,owned=1,soundMask=3,attachMask=3)
        value.update(changes);cases.append(value)
    # 최초 stage별 Collapse와 마지막 진행의 확보/억제/서버 게이트다.
    for frame,captured,suppress,boss in itertools.product((3,14,25,36,47,58,64,108,119,120,164),(0,158,165),(0,1),(0,1)):
        add('progress',daisFrame=frame,captured=captured,suppress=suppress,boss=boss)
    # 두 음향 할당과 부착의 성공 여부를 각각 독립 입력으로 공급한다.
    for frame,captured,sound,attach in itertools.product((47,108),(0,158,165),range(4),range(4)):
        add('progress',daisFrame=frame,captured=captured,soundMask=sound,attachMask=attach,priestFrame=2)
    # Rune304는 같은 owner의 다이스 조회와 가상삭제48까지 실제 PE에서 실행한다.
    for frame,captured,owned,boss,suppress in itertools.product((3,14,36,47,108),(0,158,165),(0,1),(0,1),(0,1)):
        add('rune',daisFrame=frame,captured=captured,owned=owned,boss=boss,suppress=suppress)
    # 전역169의 감김에서 실제 Finish까지 이어지는 상위306 진입이다.
    for frame,captured,suppress,owner,local in itertools.product((2,44,47,179),(0,158,165),(0,1),(1,2),(1,2)):
        add('advance',daisFrame=frame,captured=captured,suppress=suppress,owner=owner,local=local)
    # 이전 payload9.25와 다른 실제 예약 지연0/.08/.12/1의24 분기를 읽는다.
    for dais,altar,count,payload,seed in itertools.product((3,14,24,64,120),(0,3,6,7,10,13),(0,1,2,3),(0.0,0.08,0.12,1.0),(0,0x12345678)):
        add('handle',daisFrame=dais,altarFrame=altar,count=count,payload=payload,seed=seed)
    # 나머지20..23의 입력 payload 전달도 별도 원본 호출이다.
    for event,altar,payload in itertools.product((20,21,22,23),(0,3,6,7,10,13),(0.0,0.08,0.12,1.0)):
        add('handle',event=event,altarFrame=altar,payload=payload)
    # 자격을 충족한 서버의 stage0..3 이동은 전체 Path 관찰에 따로 맡긴다.
    # 이 도구는 해당 원본 몸체를 효과 없는 경계로 넘기지 않으며 그 입력은 합산하지 않는다.
    cases=[case for case in cases if not(case['mode'] in ('progress','rune') and (case['mode']=='progress' or case['owned']) and case['boss'] and case['captured']==158 and case['daisFrame'] in (3,14,25,36,64,120))]
    return list({json.dumps(case,sort_keys=True):case for case in cases}.values())

class ProgressOracle(AltarOracle):
    """실제 상위 진행/Finish/typed 조회를 실행하고 음향 장치 요청만 분리한다."""
    def __init__(self,edition):
        """기존 허용 원본 범위를 유지하고 새 장치 ABI만 명시한다."""
        super().__init__(edition);self.pg=SPEC[edition];self.progress_returns=0;self.progress_active=False
        self.boundaries.update({self.pg['load']:'soundLoad',self.pg['soundCtor']:'soundCtor',self.pg['wall']:'wallClock'})

    def on_instruction(self,mu,address,size,data):
        """분기 기대값을 만들지 않고 실제 호출 인자와 결과 쓰기를 관찰한다."""
        label=self.boundaries.get(address)
        if not self.progress_active:return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX)
        if label=='new' and ret in self.pg['soundReturns']:
            index=self.pg['soundReturns'].index(ret);assert self.read(sp+4)==40;self.events.append('NS:'+str(index)+',40')
            result=SOUND_MEMORY[index] if self.input['soundMask']&(1<<index) else 0;purge=0
        elif label=='soundLoad':
            args=[self.read(sp+4*index) for index in range(1,4)];assert args[1:]==[0,12]
            name=bytes(mu.mem_read(args[0],80)).split(b'\0')[0].decode('ascii');index=1 if name=='thunderCrack.wav' else 0
            self.events.append('LOAD:'+name+',0,12');result=0x9000+index;purge=0
        elif label=='soundCtor':
            index=SOUND_MEMORY.index(this);args=[self.read(sp+4*item) for item in range(1,5)];assert args==[5,0x9000+index,0,12]
            self.events.append('ATTACH:'+str(index)+','+','.join(map(str,args)));result=this if self.input['attachMask']&(1<<index) else 0;purge=16
        elif label=='wallClock':
            out=self.read(sp+4);mu.mem_write(out,struct.pack('<d',123.25));self.events.append('WALL:123.25');result=out;purge=0
        else:return super().on_instruction(mu,address,size,data)
        if address in self.entries:self.hits[self.entries[address]]+=1
        self.stub_calls[label]+=1;mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,value,control):
        """동일 메모리 입력에서 실제 반환/음향시각/전체 raw/난수/Player를 읽는다."""
        # 기존 count 호출은 입력 메모리 준비에만 쓰고 관찰/경계 수를 되돌린다.
        saved_hits=self.hits.copy();saved_stubs=self.stub_calls.copy();self.progress_active=False
        base=dict(value,mode='count');super().run(base,control);self.hits=saved_hits;self.stub_calls=saved_stubs;self.input=value;self.progress_active=True
        self.table(165,'A'*180);self.put(self.slot(6)+self.o['frame'],value['priestFrame'],self.o['frame_size'])
        for memory in SOUND_MEMORY:self.mu.mem_write(memory,bytes(64))
        # Rune 진입은 실제 owner 조회만 바꾸며 다이스와 같은 좌표를 사용한다.
        self.mu.mem_write(self.slot(9),bytes(self.stride));self.put(self.slot(9),VTABLE);self.put(self.slot(9)+10,166,1);self.put(self.slot(9)+14,bits(40.75));self.put(self.slot(9)+18,bits(41.25));self.put(self.slot(9)+(34 if self.patch else 32),value['owner'] if value['owned'] else 3,1)
        mode=value['mode'];floating=mode in ('rune','advance','handle');args=[0x304,0,bits(value['payload'])] if mode=='rune' else [value['event'],value['count'],bits(value['payload'])] if mode=='handle' else [9] if mode=='progress' else []
        sid=9 if mode=='rune' else 7 if mode=='handle' else 5;entry=self.a['handle'] if mode=='handle' else self.pg[mode]
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(sid));self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.mem_write(0,bytes(4));self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),CALLER if floating else STOP,*args));self.events=[];self.write_ranges=[(HEAP,HEAP+0x10000)];self.mu.emu_start(entry,STOP,count=4000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4 and self.mu.reg_read(UC_X86_REG_FPCW)==control and not(self.mu.reg_read(UC_X86_REG_FPSW)&0x3800) and self.read(0)==0
        self.progress_returns+=1
        return [str(self.read(TEMP)) if floating else '-', ';'.join(self.events) or '-', ';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (5,6,7,9,10)), ';'.join(bytes(self.mu.mem_read(memory+0x20,8)).hex() for memory in SOUND_MEMORY),str(self.read(self.a['random'])),','.join(str(self.read(PLAYER_STORAGE+owner*self.a['playerStride']+self.a['result'])) for owner in range(9))]

def generate(smoke):
    """동일 두 x87 결과와 모든 출처를 새 자료에 기록한다."""
    cases=inputs();cases=cases[::73] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    files.update(ROOT/name for name in ('cpppj/src/o/RawDaisRegular.h','cpppj/src/o/RawDaisRegular.cpp','cpppj/src/o/RawAltarSacrifice.h','cpppj/src/o/RawAltarSacrifice.cpp','tools/decomp_altarsacrifice_oracle.py','tools/decomp_daisregular_oracle.py','tools/decomp_runepostpop_oracle.py','tools/decomp_templeritual_oracle.py','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    for edition in SPEC:
        oracle=ProgressOracle(edition)
        for index,case in enumerate(cases):
            try:result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            rows.append([edition,json.dumps(case,sort_keys=True,separators=(',',':')),*result])
        editions[edition]=dict(returns=oracle.progress_returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update(oracle.paths);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.write_text('# edition inputJSON returnBits effects raw5,6,7,9,10 soundStart0,1 RNG playerResults\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),smoke=smoke,game_runs=0,editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},scope='실제Progress/Rune304/감김306와알터payload. 음향장치/벽시계/가상공간/Factory/마지막전체Damageable는명시경계. Bridge 실제Path와전체같은Host/Pool 의식소비자는아직미완료.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('다이스 상위/알터 새 payload 독립 관찰:',len(rows))

def verify():
    """현재 전체 SHA/고유 입력/두 x87 정상 반환을 모두 확인한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==data['total'] and not data['smoke'] and data['game_runs']==0
    assert len({(row[0],row[1]) for row in rows})==len(rows)
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in data['editions'].items():assert item['returns']==2*sum(row[0]==edition for row in rows) and item['asserts']==0
    print('현재 다이스 상위/알터 payload 전체 SHA·반환 감사:',len(rows))

# 명시 CLI 요청에서만 원본 독립 관찰을 만들거나 보존 근거를 감사한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
