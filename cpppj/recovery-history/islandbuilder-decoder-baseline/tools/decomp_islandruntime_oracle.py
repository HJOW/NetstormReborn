#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 mutable IslandList의100개기록/현재owner·원래owner/레지던스전파/Player첫지역을 두 PE에서 실행한다."""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_regionpaint_oracle import PaintOracle,ROOT,POOL,CODES,GRID,LISTS,STOP,OWNER,STACK_BASE,STACK,digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 실제 원본100기록/Player/반환사각형은 에뮬레이터의 분리된 입력 저장소다.
ISLANDS,PLAYERS,OUTPUT=0x1b000000,0x1c000000,0x1d000000
BYTES,CONTROLS=3204,(0x027f,0x037f)
FIXTURE=ROOT/'cpppj/tests/fixtures/islandruntime-x86.tsv'
REPORT=ROOT/'cpppj/recovery-islandruntime-evidence.json'
# 두 판본의 실제entry/전역/고정Playerstride와 기존표면 helper 경로다.
SPECS={
 'originals':dict(entries=dict(Reset=0x46edf0,Exists=0x46ee30,Count=0x46ee50,Add=0x46ee60,Delete=0x46ee90,OwnerSet=0x46ef10,GetBounds=0x46f130,SetBounds=0x46f190,Match=0x46f200,TempleGet=0x46f260,TempleSet=0x46f2c0,OwnerGet=0x46f320),
    fort=0x594fb8,special=0x5c85a4,chaff=0x594fa4,revision=0x59a8b0,changed=0x5949d4,residences=0x59a9d8,rescount=0x59a9e0,residence=0x5411bc,
    surfaces=0x542514,players=0x59534c,player_stride=180,player_region=0x78,extent=0x531928,assert_entry=0x4e0620,bounds_line=0x2a4,helpers=(0x40e800,0x40eaf0)),
 'originalCD':dict(entries=dict(Reset=0x4bd450,Exists=0x4bd4c0,Count=0x4bd4f0,Add=0x4bd500,Delete=0x4bd640,OwnerSet=0x4bd670,GetBounds=0x4bd8a0,SetBounds=0x4bd900,Match=0x4bd940,TempleGet=0x4bd9c0,TempleSet=0x4bda40,OwnerGet=0x4bda70),
    fort=0x540a1c,special=0x518904,chaff=0x52e170,revision=0x52039c,changed=OUTPUT+0x100,residences=0x565ae0,rescount=0x565ae8,residence=0x51caa8,
    surfaces=0x5670cc,players=0x50f834,player_stride=172,player_region=0x70,extent=0x52e9a8,assert_entry=0x490040,bounds_line=0x119,helpers=())
}
# 유한/포함0넓이·범위/음수·원본CD의NaN차이를 관찰하는 실제float비트 입력이다.
BOUNDARIES=((0x41200000,0x41a00000,0x42700000,0x428c0000),
 (0,0,0x42700000,0x428c0000),(0xc0000000,0x41a00000,0x42700000,0x428c0000),
 (0x42480000,0x42540000,0x42480000,0x42540000),(0x7fc00001,0x41a00000,0x42700000,0x428c0000),
 (0x41200000,0x7fc00001,0x42700000,0x428c0000),(0x41200000,0x41a00000,0x42700000,0x7fc00001),
 (0x41200000,0x41a00000,0x7fc00001,0x428c0000),(0x41200000,0x41a00000,0x43800000,0x428c0000))
POINTS=((10.,20.),(50.75,52.5),(60.,70.),(61.,71.))


def inputs():
    """기대값을 계산하지 않고 현재32바이트기록/모드/owner조건과 실제callback 뒤 변화를 교차한다."""
    cases=[];base=(3,1,1,2,1,2,1,2,0,0,0,0,9,0,0)
    def add(kind,**values):
        """공통15개 입력의 명시 필드만 바꿔 같은 원본진입점에 전달한다."""
        names=('count','id','exists','owner','original','territory','value','wantterr','special','chaff','fort','revision','changed','bounds','mutation')
        row=list(base)
        for name,value in values.items():row[names.index(name)]=value
        cases.append((kind,tuple(row)))
    for value in (0,165,255):add('Reset',value=value)
    for count in (0,1,3,100,0xffffffff):add('Count',count=count)
    for kind,number,exists in itertools.product(('Exists','OwnerGet','TempleGet','Delete','GetBounds'),(0xffffffff,0,1,2,3,99),(0,1,2)):
        if kind=='GetBounds' and number==0xffffffff:continue
        add(kind,id=number,exists=exists)
    for count,value,territory in itertools.product((0,1,99),(0,1,255,0xffffffff),(0,2,255)):add('Add',count=count,value=value,territory=territory)
    for number,exists,value in itertools.product((0,1,3,99),(0,1),(0,0x12345678,0xffffffff)):add('TempleSet',id=number,exists=exists,value=value)
    for number,exists,bounds in itertools.product((0,1,3,99),(0,1),range(len(BOUNDARIES))):add('SetBounds',id=number,exists=exists,bounds=bounds)
    for owner,original,territory,value,wantterr,special,chaff in itertools.product((0,2),(1,255),(2,255),(1,0xffffffff),(2,0xffffffff),(0,1),(0,1)):
        add('Match',owner=owner,original=original,territory=territory,value=value,wantterr=wantterr,special=special,chaff=chaff)
    for fort,special,value,bounds,mutation in itertools.product((0,1),(0,1),(0,1,8),range(len(BOUNDARIES)),(0,1,2,3,4,6)):
        add('OwnerSet',fort=fort,special=special,value=value,bounds=bounds,mutation=mutation,revision=0xffffffff if mutation else 0)
    for number,exists in itertools.product((0xffffffff,3,99),(0,1)):add('OwnerSet',id=number,exists=exists)
    return list(dict.fromkeys(cases))


class IslandOracle(PaintOracle):
    """actual100기록/owner/모드/bounds/표면helper와 Player쓰기를 실행하며 가상owner/진단만 관찰한다."""
    def __init__(self,edition):
        """현 호스트의 island-runtime12/14 및 필요한 작은 기존helper 범위를 연결한다."""
        super().__init__(edition);self.s=SPECS[edition];self.native=collections.Counter();self.external=collections.Counter();self.returns=0
        for address in (ISLANDS,PLAYERS,OUTPUT):self.mu.mem_map(address,4096)
        paths=[ROOT/f'extracted/island-runtime/{edition}/{name}' for name in ('creation.c','functions.tsv')];self.exports.extend(paths)
        with paths[1].open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):
                # 실제 내보낸 불연속기계어만 실행범위에 포함한다.
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
        if self.s['helpers']:
            full=ROOT/f'extracted/refined/{edition}/functions.tsv';self.exports.append(full)
            with full.open(encoding='utf-8') as stream:rows=[row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16) in self.s['helpers']]
            if len(rows)!=len(self.s['helpers']):raise RuntimeError('IslandRuntime 현재표면/point helper범위누락')
            for row in rows:
                for span in row['ranges'].split(';'):
                    low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
        self.allowed_bytes=bytearray(0x200000)
        for low,high in self.allowed:self.allowed_bytes[low-0x400000:high-0x400000]=b'\x01'*(high-low)
        self.entries=set(self.s['entries'].values());self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        for name,value in (('surfaces',GRID),('residences',LISTS),('players',PLAYERS),('residence',89),('extent',256)):self.put(self.s[name],value)

    def put(self,address,value,width=4):
        """원본폭의 명시입력/경계효과를 little endian으로 기록한다."""
        self.mu.mem_write(address,(int(value)&((1<<(8*width))-1)).to_bytes(width,'little'))

    def read(self,address,width=4):
        """실제BYTE/WORD/DWORD의 현재 값을 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def instruction(self,mu,address,size,data):
        """허용원본 명령과 가상owner/원본assert 입력·callback 뒤 현재 목록/전역변화를 관찰한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);s=self.s;p=self.p
        if address==STOP:mu.emu_stop();return
        if address==s['assert_entry']:
            self.assertions+=1;self.external['assert']+=1;self.events.append('A:B' if self.read(sp+12)==s['bounds_line'] else 'A:E');self.return_from();return
        if address==OWNER:
            raw=mu.reg_read(UC_X86_REG_ECX);sid=(raw-POOL)//p['stride'];owner=self.read(sp+4)
            self.put(raw+p['owner'],owner,1);self.events.append(f'O:{sid}:{owner}');self.external['virtual_owner']+=1
            if sid==60:
                mutation=self.case[14]
                if mutation==1:self.put(LISTS+4,63)
                if mutation==2:self.put(s['rescount'],1)
                if mutation==3:self.put(GRID+(53*256+51)*2,72,2)
                if mutation==4:self.put(s['fort'],1);self.put(s['special'],0)
                if mutation==6:self.put(PLAYERS+self.case[6]*s['player_stride']+s['player_region'],5)
            self.return_from(4);return
        if not(0x400000<=address<0x600000 and self.allowed_bytes[address-0x400000]):raise RuntimeError(f'IslandRuntime 허용밖native {address:08x} {self.kind} {self.case}')
        if address in self.entries:self.native[f'{address:08x}']+=1

    def write(self,mu,access,address,size,value,data):
        """실제record/스택/결과사각형/revision/패치changed/Player첫지역의쓰기만 허용한다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if ISLANDS<=address and address+size<=ISLANDS+BYTES:return
        if OUTPUT<=address and address+size<=OUTPUT+16:return
        if address in (self.s['revision'],self.s['changed']) and size==4:return
        if PLAYERS<=address and address+size<=PLAYERS+9*self.s['player_stride'] and size==4 and (address-PLAYERS)%self.s['player_stride']==self.s['player_region']:return
        raise RuntimeError(f'IslandRuntime 허용밖쓰기 {address:08x}+{size}, {self.kind} {self.case}')

    def run(self,kind,case,control):
        """원본입력만구성하고 전체3204raw/반환/경계out/전역/목록/가상owner출력을 수집한다."""
        self.kind=kind;self.case=case;mu=self.mu;p=self.p;s=self.s;self.events=[];self.assertions=0
        count,number,exists,owner,original,territory,value,wantterr,special,chaff,fort,revision,changed,bounds,mutation=case
        mu.mem_write(ISLANDS,bytes((value&255,))*BYTES);self.put(ISLANDS,count);mu.mem_write(POOL,bytes(0x10000));mu.mem_write(GRID,bytes(0x20000));mu.mem_write(OUTPUT,bytes(16))
        # 실제100기록은Temple/현재owner와BYTE원래owner/지역/네float 및padding입력을 각각 가진다.
        for index in range(100):
            record=ISLANDS+4+index*32;self.put(record,0x11110000+index);self.put(record+4,int(index<count));self.put(record+8,owner)
            self.put(record+12,territory,1);self.put(record+13,original,1)
            for field,bits in enumerate(BOUNDARIES[bounds]):self.put(record+16+field*4,bits)
        if number<100:self.put(ISLANDS+4+number*32+4,exists)
        for name,current in (('fort',fort),('special',special),('chaff',chaff),('revision',revision),('changed',changed),('rescount',4)):self.put(s[name],current)
        for index in range(9):self.put(PLAYERS+index*s['player_stride']+s['player_region'],127)
        # 각레지던스의현재SID/type/float와현재표면의소유자BYTE를별도입력한다.
        for index,(x,y) in enumerate(POINTS):
            sid=60+index;raw=POOL+sid*p['stride'];self.put(raw,CODES+0x2000);self.put(raw+10,89,1);self.put(raw+p['owner'],7,1)
            self.put(raw+14,struct.unpack('<I',struct.pack('<f',x))[0]);self.put(raw+18,struct.unpack('<I',struct.pack('<f',y))[0]);self.put(LISTS+index*4,sid)
            support=70+min(index,2);self.put(POOL+support*p['stride']+p['owner'],(2,3,8)[min(index,2)],1)
            sx,sy=int(x+struct.unpack('<f',struct.pack('<f',.9999))[0]),int(y+struct.unpack('<f',struct.pack('<f',.9999))[0]);self.put(GRID+(sy*256+sx)*2,support,2)
        args=()
        if kind in ('Exists','Delete','OwnerGet','TempleGet'):args=(number,)
        elif kind in ('Add',):args=(value,territory)
        elif kind in ('OwnerSet','TempleSet'):args=(number,value)
        elif kind=='GetBounds':args=(OUTPUT,number)
        elif kind=='SetBounds':args=(number,*BOUNDARIES[bounds])
        elif kind=='Match':args=(number,value,wantterr)
        # 실제레지스터/x87/복귀주소는각독립입력에서초기화한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0)
        mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_ECX,ISLANDS)
        mu.emu_start(s['entries'][kind],STOP,count=300000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4+4*len(args):raise RuntimeError('IslandRuntime 정상복귀/stack오류')
        self.returns+=1
        result=mu.reg_read(UC_X86_REG_EAX) if kind in ('Exists','Count','Add','Match','OwnerGet','TempleGet') else 0
        return [result,bytes(mu.mem_read(ISLANDS,BYTES)).hex(),bytes(mu.mem_read(OUTPUT,16)).hex() if kind=='GetBounds' else '-',self.read(s['revision']),self.read(s['changed']),
            ','.join(str(self.read(PLAYERS+index*s['player_stride']+s['player_region'])) for index in range(9)),';'.join(self.events) or '-',self.read(s['rescount']),
            ','.join(str(self.read(LISTS+index*4)) for index in range(4)),','.join(str(self.read(POOL+(60+index)*p['stride']+p['owner'],1)) for index in range(4)),
            ','.join(str(self.read(s[name])) for name in ('fort','special','chaff')),self.assertions]


def generate(smoke=False):
    """두정밀도 반환을별도횟수로두고정상/진단고유입력과현재PE/범위/실행기SHA를기록한다."""
    cases=inputs();cases=cases[::61] if smoke else cases;rows=[];editions={};files=[Path(__file__),ROOT/'tools/ghidra/island-runtime-functions.json',FIXTURE]
    for edition in SPECS:
        oracle=IslandOracle(edition);normal=asserts=0
        for kind,case in cases:
            output=oracle.run(kind,case,CONTROLS[0]);repeat=oracle.run(kind,case,CONTROLS[1])
            if output!=repeat:raise RuntimeError(f'IslandRuntime 두정밀도차이 {edition} {kind} {case}')
            normal+=int(output[-1]==0);asserts+=int(output[-1]!=0);rows.append([edition,kind,','.join(map(str,case)),*output])
        editions[edition]=dict(cases=len(cases),normal_cases=normal,assert_cases=asserts,returns=oracle.returns,native=dict(oracle.native),external=dict(oracle.external));files.extend(oracle.exports+[ROOT/oracle.p['binary']])
        print(f'{edition}: 정상{normal}/진단{asserts} 고유{len(cases)}개 정상반환',flush=True)
    if smoke:return
    FIXTURE.write_text('# edition kind input result raw bounds revision changed playerRegions effects rescount entries owners modes assertions\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files.extend(ROOT/name for name in ('tools/decomp_regionpaint_oracle.py','cpppj/src/o/RawIslandRuntime.h','cpppj/src/o/RawIslandRuntime.cpp','cpppj/tests/IslandRuntimeTests.cpp') if (ROOT/name).exists())
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',editions=editions,os_calls=0,files={path.relative_to(ROOT).as_posix():digest(path) for path in dict.fromkeys(files)},limits=[
        '실제100개32바이트기록/owner/currentmode/Bounds/표면helper와현재Player첫지역을실행한다.',
        '레지던스가상owner/assert만외부경계이며callback뒤현재목록/표면/mode/Player변경을다시읽는다.',
        '정상과원본assert입력은분리한다. GUI/전체Map생성/실제파생owner·캠페인완주검증이아니다.',
        '물리100기록밖GetBounds/Add의원본메모리접근은정상입력으로계수하지않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows)}))


def verify():
    """SHA/고유입력/두정밀도정상반환과정상·진단입력분리를감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if digest(ROOT/name)!=value:raise RuntimeError('IslandRuntime SHA불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(inputs())*2 or report['os_calls']:raise RuntimeError('IslandRuntime 행/OS계수오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({(row[1],row[2]) for row in selected})!=len(selected) or item['returns']!=len(selected)*2 or item['normal_cases']!=sum(int(row[-1])==0 for row in selected) or item['assert_cases']!=sum(int(row[-1])!=0 for row in selected):raise RuntimeError('IslandRuntime 입력/반환/진단분리오류')
    print(f'IslandRuntime {len(rows)}개 현재SHA/정상·진단/두정밀도반환감사 통과')


# 명시한생성/smoke/현재출처감사만실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:verify()
    else:generate(args.smoke)
