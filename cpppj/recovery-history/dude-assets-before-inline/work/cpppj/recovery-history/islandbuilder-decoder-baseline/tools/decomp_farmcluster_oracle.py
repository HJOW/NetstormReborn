#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두원본PE의edgeFarm 군집과기존실제단일배치/지역시드/local난수/주변순회를독립실행한다."""
import argparse,collections,csv,itertools,json,struct,zlib
from pathlib import Path
from decomp_fortrandomplacement_oracle import PlacementOracle,case,SPOTS,ROOT,sha,TYPES,POOL,VTABLE,MAP,STOP,STACK,DATA,VIRTUAL,codes
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from decomp_fortobjectload_oracle import PLAN,FINDER_SPECS
from decomp_constructionarrival_oracle import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

# 군집본체/실제placement복귀지점/archipelago전역과기존CD정리의주소다.
SPECS={
 'originals':dict(entry=0x40e560,after=(0x40e709,0x40e794),arch=0x5c8bb0,span=0x542654,record=0x432430,local=0x4bca20),
 'originalCD':dict(entry=0x42cd80,after=(0x42cebb,0x42cf26),arch=0x537184,span=0x537188,record=0x4047c0,local=0x4a59d0)
}
# 실제지면범위와SID는에뮬레이터/fixture의명시입력이며다른모듈과관찰을합산하지않는다.
GROUND_FIRST,GROUND_LAST,GROUND_SID,BORN=60,132,200,10000
FIXTURE=ROOT/'cpppj/tests/fixtures/farmcluster-x86.tsv'
REPORT=ROOT/'cpppj/recovery-farmcluster-evidence.json'
# 수량/지역/seed BYTE/군도/span/지면프레임또는합성랜덤타입/공간허용/owner/프레임난수입력이다.
KEYS=('quantity','region','seed','arch','span','profile','legal','owner','global','record','exists','constructor')


def inputs():
    """군집기대결과계산없이seed/수량/군도/실제지면교체·합성프레임난수와단일배치장벽을교차한다."""
    cases=[]
    for quantity,seed,arch,span,region,profile in itertools.product((1,2),(0,1,255),(0,1),(1,2),(0,2),(0,1)):
        cases.append((quantity,region,seed,arch,span,profile,1,2,0x12345678,1,1))
    # 양수/음수감김의allfail은30000중심시도다. 수량0은최초실패뒤0검사에서끝난다.
    for quantity in (0,1,0xffffffff):cases.append((quantity,2,7,0,1,0,0,2,0,1,1))
    # CD누락지역/record의원본assert뒤반환과patch무진단을별도로관찰한다.
    for record,exists in ((0,1),(1,0),(0,0)):cases.append((1,2,7,0,1,0,0,2,0,record,exists))
    result=[(*input,0) for input in dict.fromkeys(cases)]
    # 실제edgeFarm121생성자를기존완료recipe와직접소비하는두추가입력이다.
    result.extend(((1,0,7,1,2,0,1,2,0x12345678,1,1,1),(2,2,255,0,1,0,1,2,0,1,1,1)))
    return result


class FarmOracle(PlacementOracle):
    """군집만새entry로계수하고기존실제Place와명시legal/가상공간·owner경계를그대로실행한다."""
    def __init__(self,edition):
        """현호스트terrainworld/helper범위와기존완료placement의callee범위를연결한다."""
        self.edition=edition;self.s=FINDER_SPECS[edition];self.p=PLAN[edition]
        from decomp_fortrandomplacement_oracle import SPEC
        self.f=SPEC[edition];self.a=SPECS[edition];self.returns=0;self.cluster_calls=0;self.attempts=0;self.allocations=collections.Counter()
        pe=pefile.PE(str(ROOT/self.s['binary']));image=pe.get_memory_mapped_image();self.mu=Uc(UC_ARCH_X86,UC_MODE_32)
        self.mu.mem_map(pe.OPTIONAL_HEADER.ImageBase,(len(image)+4095)&~4095);self.mu.mem_write(pe.OPTIONAL_HEADER.ImageBase,image)
        # 다른호스트의사라진export초기화에의존하지않고현재PE/동일raw배치를직접구성한다.
        for address,size in ((0,4096),(TYPES,0x20000),(POOL,0x200000),(VTABLE,4096),(MAP,0x40000),(DATA,0x40000),(STOP,4096),(0x30000000,0x40000)):self.mu.mem_map(address,size)
        self.bases=[];cursor=MAP
        for scale in (1,2,4,16):self.bases.append(cursor);cursor+=(256//scale)**2*2
        self.map_end=cursor;self.exports=[];self.allowed=[];self.entries={};self.native=collections.Counter();self.boundaries=collections.Counter();self.selected_ranges={}
        request=json.loads((ROOT/'tools/ghidra/fortrandomplacement-functions.json').read_text(encoding='utf-8'))['editions'][edition]
        finder=ROOT/f'extracted/finder/{edition}/functions.tsv';self.exports.extend([ROOT/f'extracted/finder/{edition}/creation.c',finder])
        with finder.open(encoding='utf-8') as handle:request+= [row['entry'] for row in csv.DictReader(handle,delimiter='\t')]
        wanted=set(int(entry,16) for entry in request);wanted.add(0x41d770 if edition=='originals' else 0x440a50)
        wanted.add(0x4ac200 if edition=='originals' else 0x4abae0)
        wanted.update((0x49aa90,0x4558c0) if edition=='originals' else (0x4444c0,0x48cbd0,0x4446a8))
        if edition=='originalCD':wanted.add(0x4abac0)
        wanted.add(0x462630 if edition=='originals' else 0x416d80)
        full=ROOT/f'extracted/refined/{edition}/functions.tsv';self.exports.extend([ROOT/f'extracted/refined/{edition}/Netstorm.c',full])
        with full.open(encoding='utf-8') as handle:selected=[row for row in csv.DictReader(handle,delimiter='\t') if int(row['entry'],16) in wanted]
        if {int(row['entry'],16) for row in selected}!=wanted:raise RuntimeError('FarmCluster 현호스트완료helper범위누락')
        # 기존완료spec/finder의해당몸체만현재full metadata에서선택한다.
        for row in selected:
            self.entries[int(row['entry'],16)]=row['entry'];self.selected_ranges[row['entry']]=row['ranges']
            for span in row['ranges'].split(';'):
                low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
        for domain in ('terrainworld','farmcluster-helper'):
            paths=[ROOT/f'extracted/{domain}/{edition}/{name}' for name in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as handle:
                for row in csv.DictReader(handle,delimiter='\t'):
                    self.entries[int(row['entry'],16)]=row['entry']
                    # 실제불연속원본범위만허용한다.
                    for span in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in span.split('-'));self.allowed.append((low,high+1))
        self.allowed_bytes=bytearray(0x200000)
        for low,high in self.allowed:self.allowed_bytes[low-0x400000:high-0x400000]=b'\x01'*(high-low)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def instruction(self,mu,address,size,data):
        """실제배치의entry/return 순서와local난수·원본효과를관찰한다. 배치전체를stub하지않는다."""
        if address==self.a['entry']:self.cluster_calls+=1
        if address==self.f['entry']:
            args=self.args(7);self.local_ptr=args[0];self.attempts+=1;self.trace.extend(struct.pack('<7I',self.read(args[0]),*args[1:]))
        if address in self.a['after']:self.trace.extend(struct.pack('<I',mu.reg_read(UC_X86_REG_EAX)))
        if address==self.p['assert_']:
            self.assertions+=1;self.event('A',self.args(3)[2]);self.ret();return
        if address==self.p['new']:
            size=self.args(1)[0]
            if size not in (1200,1024):raise RuntimeError('FarmCluster 임시벡터크기오류')
            self.allocations[f'allocate_{size}']+=1;self.ret(DATA+0x20000 if size==1200 else DATA+0x21000);return
        if address==self.p['free']:
            self.allocations['free']+=1;self.ret();return
        if address==self.p['create']:
            typ,flags=self.args(2);sid=BORN+self.born_count;self.born_count+=1;raw=bytearray((index*37+11)&255 for index in range(self.s['stride']))
            struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=4;raw[6:8]=b'\0\0';mu.mem_write(self.slot(sid),bytes(raw));self.event('C',typ,flags,sid);self.ret(self.slot(sid));return
        if address==self.f['legal']:
            typ=(mu.reg_read(UC_X86_REG_ECX)-TYPES)//self.s['type_stride'];args=self.args(6);self.event('L',typ,*args);self.ret(int(self.input[6]!=0),24);return
        # 기존가상공간/owner효과만완료된실행기를재사용한다.
        if VIRTUAL<=address<VIRTUAL+0x100:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//self.s['stride'];offset=address-VIRTUAL
            if offset==0x48:self.unhash(sid)
            super().instruction(mu,address,size,data)
            if offset in (0x4c,0x90):self.rehash(sid)
            return
        if address==self.f['chunk_get']:self.event('G',*self.args(2))
        if address==self.f['owner_get']:self.event('I',*self.args(1))
        if address==self.f['construct']:self.event('K',(mu.reg_read(UC_X86_REG_ECX)-POOL)//self.s['stride'],mu.mem_read(mu.reg_read(UC_X86_REG_ECX)+10,1)[0])
        if not(0x400000<=address<0x600000 and self.allowed_bytes[address-0x400000]):raise RuntimeError(f'FarmCluster 허용밖native {self.edition} {address:08x} ecx={mu.reg_read(UC_X86_REG_ECX):08x} ret={self.read(mu.reg_read(UC_X86_REG_ESP)):08x}')
        if address in self.entries:self.native[self.entries[address]]+=1

    def event(self,kind,*values):
        """경계인자와순서를원본관찰로저장하며C++와동일한문자열Adler를대조한다."""
        if kind=='G':self.boundaries[kind]+=1;return
        super().event(kind,*values)

    def write(self,mu,access,address,size,value,data):
        """군집/프레임후보의명시임시벡터쓰기와기존raw/스택/SEH/공유난수쓰기만허용한다."""
        if DATA+0x21000<=address and address+size<=DATA+0x21400:return
        super().write(mu,access,address,size,value,data)

    def unhash(self,sid):
        """명시Unpop경계는현재level의WORD체인에서실제SID를제거한다."""
        raw=self.slot(sid);x,y=(int(struct.unpack('<f',self.mu.mem_read(raw+field,4))[0]) for field in (14,18));typ=self.mu.mem_read(raw+10,1)[0]
        level=0 if self.read(TYPES+typ*self.s['type_stride']+0xec)&6 else 1;scale=(1,2,4,16)[level];cell=self.bases[level]+((y//scale)*(256//scale)+x//scale)*2
        current=int.from_bytes(self.mu.mem_read(cell,2),'little');previous=0
        while current:
            after=int.from_bytes(self.mu.mem_read(self.slot(current)+4,2),'little')
            if current==sid:self.put(self.slot(previous)+4 if previous else cell,after,2);self.put(raw+4,0,2);return
            previous=current;current=after

    def rehash(self,sid):
        """명시Pop/Repop경계는현재type/좌표의단일raw hash에같은SID를등록한다."""
        raw=self.slot(sid);x,y=(int(struct.unpack('<f',self.mu.mem_read(raw+field,4))[0]) for field in (14,18));typ=self.mu.mem_read(raw+10,1)[0]
        level=0 if self.read(TYPES+typ*self.s['type_stride']+0xec)&6 else 1;scale=(1,2,4,16)[level];cell=self.bases[level]+((y//scale)*(256//scale)+x//scale)*2
        self.put(raw+4,int.from_bytes(self.mu.mem_read(cell,2),'little'),2);self.put(cell,sid,2)

    def run(self,input,control):
        """실제ground/청크/지역과타입입력만준비하고군집명령을새진입점에서실행한다."""
        self.input=input;quantity,region,seed,arch,span,profile,legal,owner,global_seed,record,exists,constructor=input
        mu=self.mu;s=self.s;f=self.f;p=self.p;g=p['globals'];self.c=case(flags1=0x200010 if not profile else 0x100010,flags2=0x80000,frames=1)
        self.events=[];self.failed=self.born=False;self.born_count=0;self.trace=bytearray();self.attempts=0;self.assertions=0;self.local_ptr=0
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(POOL,bytes(0x200000));mu.mem_write(MAP,bytes(self.map_end-MAP));mu.mem_write(DATA,bytes(0x40000))
        self.put(s['types'],TYPES);self.put(s['pool'],POOL);self.put(s['capacity'],32768);self.put(s['board'],256)
        mu.mem_write(s['grid'],struct.pack('<7I',0,1,256,*self.bases));self.put(f['spot'],SPOTS);self.put(f['islands'],self.bases[0])
        self.put(f['edge'],121);self.put(f['island'],97);self.put(f['override'],0);self.put(f['trace'],0);self.put(g['rng'],global_seed);self.put(f['quarter'],0);self.put(g['priest'],158)
        self.put(self.a['arch'],arch);self.put(self.a['span'],span);self.put(0x5e4794 if self.edition=='originals' else 0x531454,0)
        for offset in (0x48,0x4c,0x74,0x90):self.put(VTABLE+offset,VIRTUAL+offset)
        self.put(f['base_vtable']+0x4c,VIRTUAL+0x4c)
        self.set_type(97,0x10,2,[]);self.set_type(121,self.c['flags1'],0x80000,codes(1));self.put(TYPES+121*s['type_stride'],803)
        self.put(TYPES+121*s['type_stride']+(0x1d0 if self.edition=='originals' else 0x1b0),(0x462630 if self.edition=='originals' else 0x416d80) if constructor else 0)
        self.put((0x50a8e8 if self.edition=='originals' else 0x500b10)+0x4c,VIRTUAL+0x4c)
        # 실패입력도실제ground를공유하지만spot0에서단일body가먼저돌아간다.
        mu.mem_write(SPOTS,bytes((int(bool(legal)),))*65536)
        for x in range(GROUND_FIRST,GROUND_LAST+1):
            for y in range(GROUND_FIRST,GROUND_LAST+1):
                sid=GROUND_SID+(x-GROUND_FIRST)*(GROUND_LAST-GROUND_FIRST+1)+y-GROUND_FIRST;self.node(sid,97,x,y);self.put(self.slot(sid)+(34 if self.edition=='originals' else 32),owner,1)
        self.put(f['chunk'],1);self.put(f['chunk']+4,16);self.put(f['chunk']+8,16)
        # 첫일치지역의seed와다른지역을같은실제청크에입력한다.
        for x in range(16):
            for y in range(16):
                raw=f['chunk']+12+(x*16+y)*12;self.put(raw,region if record else 127,1);mu.mem_write(raw+2,bytes((0,0,0,seed,0,0)))
        self.put(f['owner_list'],100)
        for island in range(100):self.put(f['owner_list']+8+island*32,exists);self.put(f['owner_list']+12+island*32,owner)
        preserved={UC_X86_REG_EBX:0x11223344,UC_X86_REG_ESI:0x22334455,UC_X86_REG_EDI:0x33445566,UC_X86_REG_EBP:0x44556677}
        for register,value in preserved.items():mu.reg_write(register,value)
        for register,value in ((UC_X86_REG_ECX,0),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0)):mu.reg_write(register,value)
        self.put(0,0);mu.mem_write(STACK,struct.pack('<7I',STOP,bits(80.25),bits(80.75),bits(96.625),bits(96.125),region,quantity))
        mu.emu_start(self.a['entry'],STOP,count=30000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or self.read(0) or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or any(mu.reg_read(register)!=value for register,value in preserved.items()):raise RuntimeError('FarmCluster 복귀/ABI/SEH/x87오류')
        self.returns+=1;changed=[]
        # 실제변경된ground와생성슬롯만원본raw에서읽는다.
        for sid in list(range(GROUND_SID,GROUND_SID+(GROUND_LAST-GROUND_FIRST+1)**2))+list(range(BORN,BORN+self.born_count)):
            raw=bytes(mu.mem_read(self.slot(sid),s['stride']))
            if raw[10]!=97:changed.append(str(sid)+':'+raw.hex())
        return [self.attempts,zlib.adler32(self.trace),zlib.adler32(';'.join(self.events).encode()),';'.join(changed) or '-',self.read(g['rng']),self.read(self.local_ptr),self.assertions]


def generate(smoke=False):
    """새군집고유입력/두x87정상복귀·assert분류와기존callee/primitive경계를분리기록한다."""
    cases=inputs();cases=cases[:2] if smoke else cases;rows=[];reports={};files={Path(__file__),FIXTURE,ROOT/'tools/ghidra/farmcluster-helper-functions.json'}
    for edition in SPECS:
        oracle=FarmOracle(edition);normal=asserts=0
        for index,input in enumerate(cases):
            output=oracle.run(input,CONTROLS[0]);repeat=oracle.run(input,CONTROLS[1])
            if output!=repeat:raise RuntimeError(f'FarmCluster 두x87차이 {edition} {input}')
            normal+=int(output[-1]==0);asserts+=int(output[-1]!=0);rows.append([edition,','.join(map(str,input)),*output])
            if index%24==0:print(f'{edition} {index+1}/{len(cases)}',flush=True)
        reports[edition]=dict(cases=len(cases),normal_cases=normal,assert_cases=asserts,returns=oracle.returns,cluster_calls=oracle.cluster_calls,native=dict(oracle.native),primitive=dict(oracle.boundaries),allocations=dict(oracle.allocations),selected_existing_ranges=oracle.selected_ranges);files.update(oracle.exports);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
    if smoke:return
    FIXTURE.write_text('# edition input attempts traceAdler effectsAdler rawChanged globalRandom localRandom assertions\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    for name in ('cpppj/src/o/RawFarmCluster.h','cpppj/src/o/RawFarmCluster.cpp','cpppj/tests/FarmClusterTests.cpp','cpppj/src/o/RawFortRandomPlacement.h','cpppj/src/o/RawFortRandomPlacement.cpp','tools/decomp_fortrandomplacement_oracle.py','tools/decomp_fortobjectload_oracle.py','cpppj/src/o/SquidFactory.h','cpppj/src/o/SquidFactory.cpp','cpppj/src/o/TypeConstructors.inc','cpppj/src/o/DerivedConstructors.inc','tools/decomp_constructionarrival_oracle.py','tools/decomp_damageablepredestroy_oracle.py'):
        if (ROOT/name).exists():files.add(ROOT/name)
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',editions=reports,os_calls=0,new_existing_placement_observations=0,files={path.relative_to(ROOT).as_posix():sha(path) for path in sorted(files)},limits=[
        '군집/seed조회/local난수/실제단일배치·finder·모서리·Retype·HP·프레임은원본명령이다.',
        '완전한legalDrop/factory/가상공간·owner/할당·원본assert는명시primitive경계다. 군집/단일배치전체를stub하지않는다.',
        '실제edgeFarm matchframe/모드1의지면교체와명시합성randframe프로필을구분한다. constructor0 fallback 및실제121Ctor명령/기존Factory consumer를각각관찰한다.',
        '독립군집입력만새관찰이다. 기존placement/Runtime/08/실제121Ctor의기존완료관찰은추가0이며월드초기순서/GUI완주증거가아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'total':len(rows)}),flush=True)


def verify():
    """고유군집입력/정상·assert분리/현재SHA·두x87정상복귀와기존관찰비중복을감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['files'].items():
        if sha(ROOT/name)!=value:raise RuntimeError('FarmCluster SHA불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=report['total'] or len(rows)!=len(inputs())*2 or report['os_calls'] or report['new_existing_placement_observations']:raise RuntimeError('FarmCluster 고유입력/외부계수오류')
    for edition,item in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len({row[1] for row in selected})!=len(selected) or item['returns']!=len(selected)*2 or item['cluster_calls']!=len(selected)*2 or item['normal_cases']!=sum(int(row[-1])==0 for row in selected) or item['assert_cases']!=sum(int(row[-1])!=0 for row in selected):raise RuntimeError('FarmCluster 분류/반환계수오류')
    print(f'FarmCluster {len(rows)}개 현재SHA/고유군집·기존회귀분리/정상·assert감사 통과')


# 부분실행/전수생성/현재출처감사를명시옵션으로만실행한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else generate(args.smoke)
