#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""알터20~24/해방/다이스삭제 전처리와 실제 희생완료 몸체를 두 보호PE의 제한x86에서 관찰한다."""
import argparse,collections,csv,itertools,json,struct
from pathlib import Path
from decomp_daisregular_oracle import DaisOracle,ROOT,HEAP,VTABLE,STACK,STOP,CALLER,TEMP,PROCESS,FPU_CONTROLS,bits,sha
from decomp_bridgedecay_oracle import TYPES
from unicorn.x86_const import *

# 새 몸체와 명시 효과의 실제 주소다. 판단/프레임/난수/typed/contained 순회는 PE에서 실행한다.
SPEC={
 'originals':dict(handle=0x44a5e0,escape=0x4486d0,query=0x448bb0,release=0x448bd0,free=0x448500,pre=0x44afd0,count=0x44a480,contained=0x4b26f0,finish=0x449b30,damagepre=0x44b4b0,removeanimation=0x4605d0,effect=0x429010,globalSound=0x4a9cb0,random=0x532710,refresh=0x495060,stopvoice=0x452fe0,local=0x540c70,players=0x59534c,playerStride=0xb4,result=0xa0,grantFlag=0x71,suppress=0x594fa4,voice=0x542a70,form=0x4ae2d0,processType=0x41bff0,localSpecial=0x44fa00,localType=0x492a30,foreignType=0x48f380,localSp=0x40eec0,chat=0x4cf960,format=0x4a89c0,menu=(0x44fc00,0x4beb30,0x4bfac0),priestExists=0x4922e0,status=0x448fd0),
 'originalCD':dict(handle=0x483800,escape=0x483040,query=0x483c50,release=None,free=0x482ec0,pre=0x4821e0,count=0x483670,contained=0x4eb260,finish=0x482620,damagepre=0x4615e0,removeanimation=0x4547d0,effect=0x4e5c10,globalSound=0x438b50,random=0x5308ec,refresh=0x40f0e0,stopvoice=0x4a6aa0,local=0x50f6c8,players=0x50f834,playerStride=0xac,result=0x98,grantFlag=0x69,suppress=0x52e170,voice=0x512bf8,form=0x4af4e0,processType=0x48f8b0,localSpecial=0x45ef80,localType=0x40e210,foreignType=None,localSp=0x414650,chat=0x4c3dc0,format=0x420560,menu=(0x4825f0,),priestExists=0x40da20,status=0x483e70),
}
DESTROY,UNPOP,REPOP,OWNER,POP=tuple(HEAP+0xf200+index*16 for index in range(5)) # 명시 파생 가상 요청 진입점이다.
PLAYER_STORAGE=HEAP+0x2000 # 원본Player 전역은배열주소포인터이며이격리입력의9개기록을가리킨다.
FIXTURE=ROOT/'cpppj/tests/fixtures/altarsacrifice-x86.tsv' # 독립 PE 입력/반환/효과/raw/RNG/player 기록이다.
REPORT=ROOT/'cpppj/recovery-altarsacrifice-evidence.json' # 새 상위 관찰과 기존 helper 재사용 범위를 구분한다.

def inputs():
    """순서/분기 입력만 교차하며 반환/필드/난수 결과를 계산하지 않는다."""
    cases=[]
    def add(mode,**changes):
        """행의 입력전역을 명시하고 중복은 뒤에서 제거한다."""
        value=dict(mode=mode,daisFrame=14,altarFrame=3,count=0,seed=0,boss=1,reserve=1,captured=158,oldOwner=2,alliance=0,local=1,suppress=0,forms=1,extra=0,owner=1);value.update(changes);cases.append(value)
    # 알터 방향 O 구간의 앞/뒤 감김과 다른방향, payload9.25 기본반환이다.
    for event,frame,count,seed,boss in itertools.product((20,21,22,23,24),(0,3,4,6,7,10,13),(0,1,2,3,4),(0,1,0x12345678),(0,1)):
        add('handle',event=event,altarFrame=frame,count=count,seed=seed,boss=boss)
    # 다이스K 최종 가상삭제/확보실패/동맹 자동해방의 판본차이를 분리한다.
    for event,frame,captured,alliance,reserve in itertools.product((21,24),(0,14),(0,158),(0,1),(0,1)):add('handle',event=event,daisFrame=frame,captured=captured,alliance=alliance,reserve=reserve)
    # 직접 하위 실행은 입력 동일성이 아닌 함수 진입 범위가 별도다.
    for frame,count,seed in itertools.product((0,14,24,64,120),(0,1,2,3,4),(0,0x12345678)):add('escape',daisFrame=frame,count=count,seed=seed)
    for frame in (0,14,24,64,120):add('count',daisFrame=frame)
    for captured,owner,local in itertools.product((0,158),(1,2),(1,2)):add('free',captured=captured,owner=owner,local=local)
    for captured,owner in itertools.product((0,158),(1,2)):add('release',captured=captured,owner=owner)
    # 복수24일 때 매번 실제Finish를 실행한다. extra9 분기는 공통몸체만 부른다.
    for forms,extra,captured,suppress,owner,local in itertools.product((0,1,2),(0,1,8,9),(0,158,165),(0,1),(1,2),(1,2)):
        add('pre',forms=forms,extra=extra,captured=captured,suppress=suppress,owner=owner,local=local)
    return list({json.dumps(case,sort_keys=True):case for case in cases}.values())

class AltarOracle(DaisOracle):
    """실제 알터·Dais Finish·프레임 진행·범위 난수와 일반/종속/form walker를 실행한다."""
    def __init__(self,edition):
        """현재 readOnly8/7과 같은PC의 기존 완료 helper 범위만 추가한다."""
        super().__init__(edition);self.a=SPEC[edition];self.returns=0
        for domain in ('altarsacrifice','assetregular','frameadvance','process'):
            paths=[ROOT/f'extracted/{domain}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.paths.extend(paths)
            with paths[1].open(encoding='utf-8') as stream:
                # 선언된 각 원본 함수의 실제 불연속 범위만 읽기 실행 허용한다.
                for row in csv.DictReader(stream,delimiter='\t'):
                    self.entries[int(row['entry'],16)]=row['entry']
                    for span in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in span.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.paths.append(ROOT/'tools/ghidra/altarsacrifice-functions.json');self.spec=dict(self.spec,entries=dict(self.spec['entries'],**{name:value for name,value in self.a.items() if name in ('handle','escape','query','release','free','pre','count','contained','finish') and value}))
        self.boundaries=dict(self.boundaries,**{})
        for name in ('damagepre','removeanimation','effect','globalSound','refresh','stopvoice','form','processType','localSpecial','localType','foreignType','localSp','chat','format','priestExists','status'):
            if self.a[name]:self.boundaries[self.a[name]]=name
        for address in self.a['menu']:self.boundaries[address]='menu'
        if self.patch:self.boundaries[0x494ea0]='localType' # 실제 Grant의무인수UI 초기화thunk다. 획득타입은호출한Grant인수에서관찰한다.
        if self.patch:self.boundaries.update({0x48f550:'playerIndex',0x4cef50:'playerPrefix'})
        else:
            # CD의실제Get/Set SP는x87반환을포함해실행하고 마지막UI갱신만관찰한다.
            self.boundaries.pop(self.a['localSp']);self.boundaries[0x4ee9d0]='spDisplay';table=ROOT/'extracted/refined/originalCD/functions.tsv';self.paths.extend((table,table.with_name('Netstorm.c')))
            with table.open(encoding='utf-8') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    if int(row['entry'],16) not in (0x414610,0x414650):continue
                    self.entries[int(row['entry'],16)]=row['entry']
                    for span in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in span.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\1'*(high+1-low)
        self.boundaries.update({DESTROY:'destroy',UNPOP:'unpop',REPOP:'repop',OWNER:'owner',POP:'pop',self.p['create']:'create'})
        for offset,address in ((0x10,DESTROY),(0x48,UNPOP),(0x4c,REPOP),(0x74,OWNER),(0x90,POP)):self.put(VTABLE+offset,address)

    def on_write(self,mu,access,address,size,value,data):
        """새몸체가직접쓰는 raw/플레이어/RNG/스택외쓰기를거부한다."""
        if self.slot(5)<=address and address+size<=self.slot(13)+self.stride:return
        if address==(0x55a420 if self.patch else 0x52e2e0) and size==4:return
        if not self.patch and address==0x5120d4 and size==4:return
        if address==self.a['random'] and size==4:return
        return super().on_write(mu,access,address,size,value,data)

    def on_instruction(self,mu,address,size,data):
        """현재가상/UI/장치 요청만격리하고분기·조회·Finish·RNG는원본명령을실행한다."""
        if address==(0x448260 if self.patch else 0x482560):self.grant_type=self.read(mu.reg_read(UC_X86_REG_ESP)+4)
        label=self.boundaries.get(address)
        if not label:return super().on_instruction(mu,address,size,data)
        if address in self.entries:self.hits[self.entries[address]]+=1
        self.stub_calls[label]+=1;sp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(sp);this=mu.reg_read(UC_X86_REG_ECX);args=[self.read(sp+4*index) for index in range(1,8)];purge=0;result=0
        if label=='frame':sid=self.sid_of(this);purge=8;assert args[1]==0;self.events.append(f'F:{sid},{args[0]},0');self.put(this+self.o['frame'],args[0],self.o['frame_size'])
        elif label=='new':assert args[0]==40;self.events.append('N:40');result=HEAP+0x6000 if self.input['reserve'] else 0
        elif label=='shared':assert this==HEAP+0x6000;purge=12;result=this;self.events.append('S:'+','.join(map(str,args[:3])))
        elif label=='sound':name=bytes(mu.mem_read(args[2],80)).split(b'\0')[0].decode('ascii');self.events.append('A:'+','.join(map(str,args[:2]))+','+name+','+str(args[3]))
        elif label=='globalSound':name=bytes(mu.mem_read(args[0],80)).split(b'\0')[0].decode('ascii');self.events.append('G:'+name+','+','.join(map(str,args[1:6])))
        elif label in ('destroy','unpop','repop','owner'):
            sid=self.sid_of(this);purge=4;self.events.append(label+':'+str(sid)+','+str(args[0]))
            if label=='owner':self.put(this+(34 if self.patch else 32),args[0],1)
        elif label=='pop':sid=self.sid_of(this);purge=12;self.events.append('P:'+str(sid)+','+','.join(map(str,args[:3])));self.put(this+14,args[0]);self.put(this+18,args[1])
        elif label=='create':assert args[:2]==[158,0];result=self.slot(8);self.mu.mem_write(result,bytes(self.stride));self.put(result,VTABLE);self.put(result+10,158,1);self.put(result+11,4,1);self.events.append('C:158,0,8')
        elif label=='effect':purge=8;self.events.append('E:'+str(self.sid_of(this))+','+','.join(map(str,args[:2])))
        elif label=='removeanimation':self.events.append('X:'+str(args[0]))
        elif label=='damagepre':purge=4;self.events.append('B:'+str(self.sid_of(this))+','+str(args[0]))
        elif label=='refresh':self.events.append('R:'+str(args[0]))
        elif label=='stopvoice':self.events.append('V:'+','.join(map(str,args[:3])))
        elif label=='form':purge=0;sid=self.sid_of(this);result=PROCESS+(sid-12)*64;self.put(result+0x18,24);self.events.append('Q:'+str(sid))
        elif label=='processType':result=61
        elif label=='localSpecial':self.events.append('L:'+str(args[0]))
        elif label=='localType':self.events.append('T:'+str(args[0] if not self.patch else self.grant_type))
        elif label=='foreignType':self.events.append('H:'+','.join(map(str,args[:2])))
        elif label=='localSp':self.events.append('SP:'+str(args[0]))
        elif label=='chat':self.events.append('CHAT:'+bytes(mu.mem_read(args[0],100)).split(b'\0')[0].decode('ascii'))
        elif label=='format':name=bytes(mu.mem_read(args[1],100)).split(b'\0')[0].decode('ascii');self.mu.mem_write(args[0],name.encode()+b'\0');result=args[0]
        elif label=='menu':self.events.append('UI:'+hex(address))
        elif label=='priestExists':result=1
        elif label=='status':result=3
        elif label=='playerIndex':result=(this-PLAYER_STORAGE)//self.a['playerStride']
        elif label=='playerPrefix':mu.mem_write(args[0],b'P\0')
        elif label=='spDisplay':self.events.append('SP:'+str(self.read(0x5120d4)))
        elif label=='find':assert args[0]==604;purge=4;result=0
        elif label=='kill':raise RuntimeError('새알터입력의미선언604삭제')
        else:return super().on_instruction(mu,address,size,data)
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_ESP,sp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def run(self,value,control):
        """독립 원본 입력을 구성하고 실제 caller 반환/raw/RNG/player를 읽는다."""
        super().run(('can',3,'A',0,0,1,1,1,0x210000,1,0),control);self.input=value
        # 기존 입력 준비가 Temple용 가상표를 갱신하므로 새 알터효과의 주소를 다시 공급한다.
        for offset,address in ((0x10,DESTROY),(0x48,UNPOP),(0x4c,REPOP),(0x74,OWNER),(0x90,POP)):self.put(VTABLE+offset,address)
        self.table(165,'K'*8+'L'*8+'M'*8+'N'*8+'O'*8+'P'*8+'A'*132);self.table(164,'A'*3+'O'*4+'M'*3+'L'*3+'P'*3)
        # 실제Frame consumer의현재1칸SHP와일치하는해시단계입력이다. 해시는이미1단계버킷에등록돼있다.
        for sid in (5,6,7):self.put(self.slot(sid)+(33 if self.patch else 31),1,1)
        self.put(self.slot(5)+self.o['frame'],value['daisFrame'],self.o['frame_size']);self.put(self.slot(7)+self.o['frame'],value['altarFrame'],self.o['frame_size']);self.put(self.slot(5)+(34 if self.patch else 32),value['owner'],1);self.put(self.slot(7)+12,value['oldOwner'],2);self.put(self.slot(5)+self.o['extra'],value['extra'],1);self.put(self.a['local'],value['local']);self.put(self.a['suppress'],value['suppress']);self.put(self.a['voice'],99);self.put(self.p['boss'],value['boss']);self.put(self.a['random'],value['seed']);self.put(self.d['alliances']+(value['oldOwner']+value['owner']*9)*4,value['alliance'])
        self.put(self.slot(5)+6,10 if value['captured'] else 0,2);self.put(self.slot(10)+18,value['captured'],2);self.put(self.slot(10)+20,36,2);self.put(self.slot(10)+12,value['oldOwner'],2);self.put(self.slot(7)+6,12 if value['forms'] else 0,2)
        # Form체인의 actual walker는 다음링크를먼저읽는다. FromForm만명시호스트요청이다.
        for sid in (12,13):self.mu.mem_write(self.slot(sid),bytes(self.stride));self.put(self.slot(sid)+10,61,1);self.put(self.slot(sid)+4,13 if sid==12 and value['forms']==2 else 0,2)
        self.put(self.a['players'],PLAYER_STORAGE)
        if not self.patch:self.put(0x5120d4,bits(125.5));self.put(0x518904,0)
        for owner in range(9):self.mu.mem_write(PLAYER_STORAGE+owner*self.a['playerStride'],bytes(self.a['playerStride']));self.put(PLAYER_STORAGE+owner*self.a['playerStride']+self.a['grantFlag'],1,1)
        mode=value['mode'];floating=mode=='handle';args=[value.get('event',24),value['count'],bits(9.25)] if floating else [value['count']] if mode=='escape' else [0x400000] if mode=='pre' else [];sid=7 if mode in ('handle','count','release') else 5
        if mode=='release' and not self.patch:return None
        # 반환과 x87/SEH/스택을 실제 호출 경계에서 확인한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(sid));self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.mem_write(0,bytes(4));self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),CALLER if floating else STOP,*args));self.events=[];self.write_ranges=[(HEAP,HEAP+0x10000)];self.mu.emu_start(self.a[mode],STOP,count=4000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4 and self.mu.reg_read(UC_X86_REG_FPCW)==control and not(self.mu.reg_read(UC_X86_REG_FPSW)&0x3800) and self.read(0)==0
        self.returns+=1
        return [str(self.read(TEMP) if floating else self.mu.reg_read(UC_X86_REG_EAX)) if mode in ('handle','count') else '-', ';'.join(self.events) or '-', ';'.join(bytes(self.mu.mem_read(self.slot(sid),self.stride)).hex() for sid in (5,6,7,8,10)),str(self.read(self.a['random'])),','.join(str(self.read(PLAYER_STORAGE+owner*self.a['playerStride']+self.a['result'])) for owner in range(9))]

def generate(smoke=False):
    """동일 입력의 두FPU 결과를 대조하고 재사용 모듈의 기존관찰은 합산하지 않는다."""
    cases=inputs();cases=cases[::101] if smoke else cases;rows=[];editions={};files={Path(__file__),FIXTURE}
    # 실행한 새상위body/검사/전체import근거를누락없이현재SHA로연결한다. 미수행Lifecycle는별도보고서대상이다.
    files.update(ROOT/name for name in ('cpppj/src/o/RawAltarSacrifice.h','cpppj/src/o/RawAltarSacrifice.cpp','cpppj/src/o/RawDaisRegular.h','cpppj/src/o/RawDaisRegular.cpp','cpppj/tests/AltarSacrificeTests.cpp','cpppj/tests/DaisRegularTests.cpp','tools/decomp_daisregular_oracle.py','tools/decomp_runepostpop_oracle.py','tools/decomp_templeritual_oracle.py','tools/decomp_rawfinder_oracle.py','tools/decomp_bridgeeffects_oracle.py','tools/decomp_bridgedecay_oracle.py'))
    for edition in SPEC:
        oracle=AltarOracle(edition)
        for index,case in enumerate(cases):
            try:result=oracle.run(case,FPU_CONTROLS[0]);assert oracle.run(case,FPU_CONTROLS[1])==result
            except Exception as error:raise RuntimeError(f'{edition} #{index} {case}: {error}') from error
            if result is not None:rows.append([edition,json.dumps(case,sort_keys=True,separators=(',',':')),*result])
        editions[edition]=dict(returns=oracle.returns,entries=dict(oracle.hits),boundaries=dict(oracle.stub_calls),asserts=oracle.assertions);files.update(oracle.paths);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    FIXTURE.write_text('# edition inputJSON returnBits effects raw5,6,7,8,10 RNG playerResults\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n');REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',decompile_date='2026-10-14',total=len(rows),smoke=smoke,editions=editions,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},game_runs=0,scope='실제알터/해방/Dais전처리/Finish 상위 몸체. Frame/가상공간/공통Damageable/음향/UI/FromForm·Type효과 경계. 실제같은Host/Pool C++소비자는후속.'),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('알터/다이스 새 독립 원본 관찰:',len(rows))

def verify():
    """같은원본입력·두정밀도반환·현재source/fixture/import SHA를전부감사한다."""
    data=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==data['total'] and not data['smoke'] and data['game_runs']==0
    assert len({(row[0],row[1]) for row in rows})==len(rows)
    for name,value in data['files'].items():assert sha(ROOT/name)==value,name
    for edition,item in data['editions'].items():assert item['returns']==2*sum(row[0]==edition for row in rows) and item['asserts']==0
    print('알터 현재전체SHA/독립원본반환감사:',len(rows))

# 명시 요청으로만 독립 PE관찰을실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
