#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fort 단일 배치·isle 모서리·Retype/HP/프레임·현재 지역/owner의 실제 두 PE를 관찰한다."""
import argparse,collections,csv,itertools,json,struct
from pathlib import Path
from decomp_fortobjectload_oracle import FortOracle,DATA,ROOT,sha,TYPES,POOL,VTABLE,MAP,STOP,STACK,codes,FRAME_CODES,VIRTUAL
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import *

# 고유 fixture/증거와 실제 원본 주소다. 모든 실행 메모리는 에뮬레이터에만 존재한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/fortrandomplacement-x86.tsv'
REPORT=ROOT/'cpppj/recovery-fortrandomplacement-evidence.json'
SPOTS=DATA
LOCAL=DATA+0x39000
KEYS=('type','x','y','flags1','flags2','spot','legal','region','chunk','owner','override','mode','force','frames','seed','terrain','edge','neighbors','mutation','hp','quarter','foot')
SPEC={
 'originals':dict(entry=0x40db90,corner=0x40dab0,retype=0x4b1620,construct=0x4af720,legal=0x49b510,
    chunk=0x54c8a0,chunk_get=0x432180,owner_list=0x568b60,owner_get=0x46f320,spot=0x5c7c44,islands=0x5c84bc,
    edge=0x54123c,island=0x5411dc,override=0x544ce0,trace=0x544ce4,base_vtable=0x501dd0,quarter=0x5ca8f0),
 'originalCD':dict(entry=0x42c3d0,corner=0x42c0d0,retype=0x4ab6b0,construct=0x4ab770,legal=0x445200,
    chunk=0x5484c0,chunk_get=0x404090,owner_list=0x583188,owner_get=0x4bda70,spot=0x52fe48,islands=0x52d590,
    edge=0x51cb28,island=0x51cac8,override=0x5187b8,trace=0x5187bc,base_vtable=0x5058d8,quarter=0x511c90)}


def case(**changes):
    """기대 결과 없이 입력의 원시 DWORD를 구성한다."""
    c=dict(zip(KEYS,(100,bits(20.75),bits(21.9),0x10,0,1,1,3,3,2,0,0,0,1,0x12345678,1,0,0,0,803,0,1)));c.update(changes);return c


def inputs():
    """원본 조기 반환·HP/프레임/소유자·지면 교체와 모든256 모서리 위상을 교차한다."""
    rows=[]
    for spot,legal,chunk,force,override in itertools.product((0,1,0x3f,0x40),(0,1),(3,4,255),(0,1),(0,7)):
        rows.append(('P',case(spot=spot,legal=legal,chunk=chunk,force=force,override=override)))
    for flags,frame,mode,terrain in itertools.product((0,0x10,0x100010,0x200010,0x300010),range(6),(0,1),(0,1,2)):
        rows.append(('P',case(flags1=flags,frames=frame,mode=mode,terrain=terrain)))
    for neighbors in range(256):
        rows.append(('C',case(neighbors=neighbors)));rows.append(('P',case(flags1=0x200010,mode=1,neighbors=neighbors)))
    for x,y,edge,mutation in itertools.product(((20.1,21.1),(20.49,21.49),(20.5,21.5),(20.99,21.99),(1.0,1.0),(254.1,254.1)),(0,1,2),(0,1,2),(0,1,2,3)):
        rows.append(('P',case(x=bits(x),y=bits(y),edge=edge,mutation=mutation,flags1=0x100010,frames=2,terrain=1)))
    for typ,genus,quarter,hp in itertools.product((100,158),(0,0x400000),(0,1),(0,803,70000)):
        rows.append(('P',case(type=typ,flags2=genus,quarter=quarter,hp=hp)))
    for foot in (1,2):rows.append(('R',case(foot=foot)))
    # 같은 전체 입력은 최초 행만 저장한다. 정밀도·재실행을 독립 관찰로 더하지 않는다.
    seen=set();result=[]
    for mode,c in rows:
        key=(mode,*(c[k] for k in KEYS))
        if key not in seen:seen.add(key);result.append((mode,c))
    return result


class PlacementOracle(FortOracle):
    """실제 placement/corner/Retype와 기존 finder/HP/프레임 명령만 허용하는 독립 실행기다."""
    def __init__(self,edition):
        """현재 PE와 고유 export를 연결하고 하위 기존 몸체의 성공한 export를 재사용한다."""
        super().__init__(edition);self.f=SPEC[edition]
        extra=[ROOT/f'extracted/fortrandomplacement/{edition}/{n}' for n in ('creation.c','functions.tsv')];self.exports+=extra
        for table in (extra[1],):
            with table.open(encoding='utf-8-sig') as handle:
                for row in csv.DictReader(handle,delimiter='\t'):
                    self.entries[int(row['entry'],16)]=row['entry']
                    for part in row['ranges'].split(';'):
                        lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))
        self.native=collections.Counter();self.boundaries=collections.Counter()

    def write(self,mu,access,address,size,value,data):
        """실제 body의 쓰기는 raw 슬롯·스택·SEH·동일 global 난수에만 허용한다."""
        ranges=((POOL,POOL+0x200000),(0x30000000,0x30040000),(0,4),(self.p['globals']['rng'],self.p['globals']['rng']+4))
        if any(lo<=address and address+size<=hi for lo,hi in ranges):return
        raise RuntimeError(f'Fort 단일 배치 미허용 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """완전한 legalDrop·factory·가상공간/소유자·메모리 할당만 명시 경계로 둔다."""
        f=self.f;c=self.c;p=self.p;ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==p['assert_']:self.failed=True;self.ret();mu.reg_write(UC_X86_REG_EIP,STOP);return
        if address==f['legal']:
            args=self.args(6);typ=(ecx-TYPES)//self.s['type_stride'];self.event('L',typ,*args)
            if c['mutation']==1:self.put(TYPES+typ*self.s['type_stride']+0xe8,0x200010)
            if c['mutation']==2:self.put(f['override'],7)
            self.ret(c['legal'],24);return
        if address==f['chunk_get']:self.event('G',*self.args(2))
        if address==f['owner_get']:self.event('I',*self.args(1))
        if address==f['construct']:
            self.event('K',(ecx-POOL)//self.s['stride'],mu.mem_read(ecx+10,1)[0])
        if address==p['create']:
            typ,flags=self.args(2);self.born=True;raw=bytearray((i*37+11)&255 for i in range(self.s['stride']))
            struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=4;raw[6:8]=b'\0\0';mu.mem_write(self.slot(100),bytes(raw));self.event('C',typ,flags,100);self.ret(self.slot(100));return
        if address==p['new']:self.ret(DATA+0x20000);return
        if address==p['free']:self.ret();return
        if VIRTUAL<=address<VIRTUAL+0x100:
            sid=(ecx-POOL)//self.s['stride'];offset=address-VIRTUAL
            if offset==0x74:
                owner,=self.args(1);self.event('O',sid,owner);self.put(ecx+(34 if self.edition=='originals' else 32),owner,1)
                if c['mutation']==3:self.put(ecx+10,101,1)
                self.ret(0,4)
            elif offset in (0x48,0x4c):
                flags,=self.args(1);self.event('U' if offset==0x48 else 'R',sid,flags)
                self.put(ecx+11,(mu.mem_read(ecx+11,1)[0]|4) if offset==0x48 else (mu.mem_read(ecx+11,1)[0]&~4),1);self.ret(0,4)
            elif offset==0x90:
                x,y,flags=self.args(3);self.event('P',sid,x,y,flags);self.put(ecx+14,x);self.put(ecx+18,y)
                self.put(ecx+11,mu.mem_read(ecx+11,1)[0]&~4,1);self.ret(0,12)
            else:raise RuntimeError(f'Fort placement 가상 {offset:x}')
            return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'Fort 단일 배치 미허용 실행 {self.edition} {address:08x}')
        if address in self.entries:self.native[self.entries[address]]+=1

    def node(self,sid,typ,x,y,frame=7,level=0):
        """실제 raw/해시에 후보 입력을 등록한다. 출력 결과는 계산하지 않는다."""
        raw=bytearray(self.s['stride']);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;struct.pack_into('<II',raw,14,bits(x),bits(y))
        if self.edition=='originals':struct.pack_into('<I',raw,36,frame)
        else:raw[34]=frame&255
        scale=(1,2,4,16)[level];head=self.bases[level]+((int(y)//scale)*(256//scale)+int(x)//scale)*2
        struct.pack_into('<H',raw,4,struct.unpack('<H',self.mu.mem_read(head,2))[0]);self.mu.mem_write(self.slot(sid),bytes(raw));self.put(head,sid,2)

    def run(self,mode,c,control):
        """정상 반환/명시 원본 assert와 전체 raw·두 난수·현재 전역 효과를 관찰한다."""
        mu=self.mu;s=self.s;f=self.f;p=self.p;g=p['globals'];self.mode,self.c=mode,c;self.failed=self.born=False;self.events=[]
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(POOL,bytes(0x200000));mu.mem_write(MAP,bytes(self.map_end-MAP));mu.mem_write(DATA,bytes(0x40000))
        self.put(s['types'],TYPES);self.put(s['pool'],POOL);self.put(s['capacity'],32768);self.put(s['board'],256)
        mu.mem_write(s['grid'],struct.pack('<7I',0,1,256,*self.bases));self.put(f['spot'],SPOTS);self.put(f['islands'],self.bases[0])
        self.put(f['edge'],121);self.put(f['island'],97);self.put(f['override'],c['override']);self.put(f['trace'],0);self.put(g['rng'],c['seed']);self.put(f['quarter'],c['quarter']);self.put(g['priest'],158)
        self.put(0x5e4794 if self.edition=='originals' else 0x531454,0)
        for offset in (0x48,0x4c,0x74,0x90):self.put(VTABLE+offset,VIRTUAL+offset)
        self.put(f['base_vtable']+0x4c,VIRTUAL+0x4c)
        for typ,flags,genus,frames in ((97,0,2,[]),(121,0x10,2,[]),(101,c['flags1'],c['flags2'],codes(c['frames'])),(c['type'],c['flags1'],c['flags2'],codes(c['frames']))):
            self.set_type(typ,flags,genus,frames);self.put(TYPES+typ*s['type_stride'],c['hp'])
        self.put(TYPES+121*s['type_stride']+(0x1d4 if self.edition=='originals' else 0x1b4),c['foot'])
        x,y=(struct.unpack('<f',struct.pack('<I',c[k]))[0] for k in ('x','y'));ix,iy=int(x),int(y)
        self.put(SPOTS+int(y+0.5)*256+int(x+0.5),c['spot'],1);self.put(LOCAL,0xaabbccdd)
        self.put(f['chunk'],1);self.put(f['chunk']+4,16);self.put(f['chunk']+8,16)
        # 상위 chunk grid는 x 바깥 간격16·y 안쪽이다. 모든 현재 BYTE를 독립 입력으로 채운다.
        for cx in range(16):
            for cy in range(16):self.put(f['chunk']+12+(cx*16+cy)*12,c['chunk'],1)
        self.put(f['owner_list'],20)
        for region in range(20):self.put(f['owner_list']+8+region*32,1);self.put(f['owner_list']+12+region*32,c['owner'])
        if c['terrain']:self.node(50,97,x,y,0xffffffff if c['terrain']==2 else 7)
        offsets=((-1,-1),(0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0))
        # 모든8 이웃 입력을 실제 WORD hash0과 pool의 isle 타입으로 공급한다.
        for index,(dx,dy) in enumerate(offsets):
            if c['neighbors']&(1<<index) and 1<=ix+dx<256 and 1<=iy+dy<256:self.node(60+index,97,ix+dx,iy+dy)
        if c['edge']:self.node(80,121,x,y,7,0 if c['edge']==1 else 1)
        entry=f['entry'];args=(LOCAL,c['type'],c['x'],c['y'],c['region'],c['mode'],c['force']);ecx=0
        if mode=='C':entry=f['corner'];args=(ix,iy)
        if mode=='R':entry=f['retype'];ecx=self.slot(50);args=(121,);self.put(ecx+11,4,1)
        for reg,value in ((UC_X86_REG_EBX,0x11223344),(UC_X86_REG_ESI,0x22334455),(UC_X86_REG_EDI,0x33445566),(UC_X86_REG_EBP,0x44556677),
                (UC_X86_REG_ECX,ecx),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0)):mu.reg_write(reg,value)
        self.put(0,0);mu.mem_write(STACK,struct.pack(f'<{len(args)+1}I',STOP,*args));mu.emu_start(entry,STOP,count=1000000)
        result='A' if self.failed else str(mu.reg_read(UC_X86_REG_EAX)) if mode!='R' else '0'
        if mu.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Fort 배치 종료 미도달')
        if not self.failed:
            if mu.reg_read(UC_X86_REG_ESP)!=STACK+(8 if mode=='R' else 4) or mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.read(0):raise RuntimeError('Fort 배치 정상 반환/x87/SEH 규약 오류')
            for reg,value in ((UC_X86_REG_EBX,0x11223344),(UC_X86_REG_ESI,0x22334455),(UC_X86_REG_EDI,0x33445566),(UC_X86_REG_EBP,0x44556677)):
                if mu.reg_read(reg)!=value:raise RuntimeError('Fort 배치 보존 레지스터 오류')
        raw=bytes(mu.mem_read(self.slot(50),s['stride'])).hex()+';'+(bytes(mu.mem_read(self.slot(100),s['stride'])).hex() if self.born else '-')
        return result,';'.join(self.events) or '-',raw,str(self.read(g['rng'])),str(self.read(LOCAL)),str(self.read(f['override']))


def generate():
    """고유 입력마다 두 x87 재실행이 같은 관찰만 저장한다."""
    rows=[];reports={};files={Path(__file__),FIXTURE,ROOT/'tools/ghidra/fortrandomplacement-functions.json',ROOT/'cpppj/src/o/RawFortRandomPlacement.h',ROOT/'cpppj/src/o/RawFortRandomPlacement.cpp',ROOT/'tools/decomp_fortobjectload_oracle.py'}
    for edition in SPEC:
        oracle=PlacementOracle(edition)
        for mode,c in inputs():
            output=[oracle.run(mode,c,control) for control in CONTROLS];assert output[0]==output[1],(edition,mode,c)
            rows.append('\t'.join((edition,mode,','.join(str(c[k]) for k in KEYS),*output[0])))
        reports[edition]=dict(inputs=len(inputs()),native=dict(oracle.native),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/('originals/Netstorm.exe' if edition=='originals' else 'originalCD/NETSTORM.EXE'))
        print(f'{edition} 단일 배치/모서리/Retype {len(inputs())}개 관찰',flush=True)
    FIXTURE.write_text('# 판본 모드 입력 result effects raw50;raw100 globalRng localRng ownerOverride\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='VM-W11-CODEX',date='2026-10-13',total=len(rows),editions=reports,
        files={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(files)},limits=['실제 단일 body·spot/일반 finder·현재 chunk/owner·모든 모서리·Retype/HP/프레임 실행',
        '완전한 legalDrop/factory/가상 owner/공간 Pop·Unpop·Repop 및 임시 메모리 할당은 명시 경계','일반 CTest는 저장 기대만 사용하며 캠페인/게임/GUI 완주 검증은 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """현재 PE/source/export/fixture SHA와 세 진입·고유 관찰 수를 감사한다."""
    r=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in r['files'].items():assert sha(ROOT/name)==value,name
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==r['total']==2*len(inputs());assert all(len(row)==9 for row in rows)
    for edition,f in SPEC.items():assert all(r['editions'][edition]['native'][f'{f[k]:08x}']>0 for k in ('entry','corner','retype','construct'))
    print(f'Fort 단일 배치 SHA·고유 입력 {len(rows)}개 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');o=parser.parse_args();verify() if o.verify else generate()
