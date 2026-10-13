#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 PE의 Fort 객체/contained/상위 청크 순회·HP·P/P 난수를 실제 명령으로 실행한다."""
import argparse,collections,csv,itertools,json,struct
from pathlib import Path
from decomp_constructionarrival_oracle import ArrivalOracle,SPECS as FINDER_SPECS,TYPES,POOL,VTABLE,MAP,STOP,STACK,ROOT,sha
from decomp_damageablepredestroy_oracle import CONTROLS,bits
from unicorn.x86_const import *

# 새 고유 자료 주소와 가상 효과 진입점이다. 원본 주소/호스트 프로세스와 별개다.
DATA=0x15000000
HEAP=DATA+0x20000
STREAM=DATA
BYTES=DATA+0x1000
FRAME_CODES=DATA+0x10000
VIRTUAL=0x20000800
KEYS=('stored','type','position','flags1','flags2','version','defaultOwner','owner','special','mission','fort','blocked',
      'quarter','seed','frames','terrain','mutation','x','y','contents','frame','qa','qb','bridgeFrame','legacy','factory','chaffMode','slot')
PLAN={
 'originals':dict(object=0x4bdc60,contents=0x4bd130,chunk=0x4be160,load=0x4be610,create=0x4af530,content=0x4afa70,
    notify=0x4214a0,new=0x4e4391,free=0x4e4354,assert_=0x4e0620,prepare=0x4bd800,lookup=0x44ceb0,format=0x4a89c0,
    region=0x432180,allowed=0x46f200,finish=0x40e1a0,template=0x5c89f0,
    globals=dict(version=0x5c89c0,conversion=0x5c89bc,depth=0x5c89b8,scope=0x5c89b0,bridge=0x5411a0,island=0x5411d0,
       large=0x5411d4,noIsland=0x5412cc,ownerSaved=0x542644,ownerCleared=0x542648,keep=0x542640,special=0x5c85a4,
       mission=0x594fa0,fort=0x594fb8,blocked=0x54db14,quarter=0x5ca8f0,priest=0x5412d0,altar=0x5412e8,dais=0x5412ec,
       rng=0x532710,chaffMode=0x594fa4,width=0x54c8a4,height=0x54c8a8)),
 'originalCD':dict(object=0x428320,contents=0x426dd0,chunk=0x4288a0,load=0x428d50,create=0x4ab390,content=0x4ac4d0,
    notify=0x448c10,new=0x4f1650,free=0x4f1a50,assert_=0x490040,prepare=0x427e00,lookup=0x44c4e0,format=0x420560,
    region=0x404090,allowed=0x4bd940,finish=0x42ca70,release=0x426830,template=0x564bd0,
    globals=dict(version=0x5178dc,conversion=0x5178d8,depth=0x5178d4,scope=0x5178bc,bridge=0x51ca8c,island=0x51cabc,
       large=0x51cac0,noIsland=0x51cbb8,ownerSaved=0x5178c8,ownerCleared=0x5178cc,keep=0x5178c4,special=0x518904,
       mission=0x52e16c,fort=0x540a1c,blocked=0x52e8a8,quarter=0x511c90,priest=0x51cbbc,altar=0x51cbd4,dais=0x51cbd8,
       rng=0x5308ec,chaffMode=0x52e170,width=0x5484c4,height=0x5484c8))}
FIXTURE=ROOT/'cpppj/tests/fixtures/fortobjectload-x86.tsv'
REPORT=ROOT/'cpppj/fortobjectload.json'


def case(**changes):
    """기대 계산 없이 원본에 공급할 입력만 만든다."""
    values=dict(zip(KEYS,(100,100,0xe3,0,0,2,1,2,0,1,0,0,0,0,0,0,0,16,32,0,23,129,0xfffd,17,3,4,1,1)))
    values.update(changes);return values


def codes(profile):
    """고정 P/P·fallback·256 후보 경계의 프레임 입력이다."""
    return [[],[(80,80,0,0),(80,80,1,255),(65,65,0,0)],[(80,65,0,0),(80,65,1,0)],
            [(80,67,0,0),(80,68,0,0)],[(65,65,0,0)],[(80,80,i&255,i&255) for i in range(300)]][profile]


def packet(c):
    """저장 포맷 입력을 만들며 native 효과/owner/HP 결과는 계산하지 않는다."""
    result=bytearray((c['position'],c['stored']))
    if not c['type']:return bytes(result+b'\0')
    if c['type']==97:return bytes(result+b'\x55')
    if c['flags1']&0x20:result.append(c['frame'])
    if c['flags1']&0x1000:result.append(c['qa'])
    if c['flags1']&0x2000:result+=struct.pack('<H',c['qb'])
    if c['flags2']&4:result.append(c['bridgeFrame'])
    if c['version']==0:result.append(c['legacy'])
    if c['version']>1 and c['flags2']&0x4000:result.append(c['factory'])
    if c['version']>0 and c['flags2']&0x5d77cf00:result.append(c['owner'])
    if c['flags1']&0x20000:
        # 첫 항목 local flags가 뒤 형제에도 남고 재귀 ContentForm의 현재 부모를 조회한다.
        profiles=[b'\0',bytes((2,110,9,111,0xfd,0xff,0x20)),bytes((1,112,1,111,0xfd,0xff,0x20)),bytes((1,0))]
        result+=profiles[c['contents']]
    return bytes(result)


def inputs():
    """owner 정책/저장 필드/HP·프레임·후처리·상위 선택을 교차한 고유 입력이다."""
    result=[]
    for version,genus,owner,mode in itertools.product(range(3),(0,4,0x200,0x4000,0x10002000),(0,1,8,9,255),(0,1,2)):
        result.append(('O',case(version=version,flags2=genus,owner=owner,special=mode==2,mission=mode==1,stored=253 if genus==4 else 100,type=82 if genus==4 else 100)))
    for flags,frames,terrain,mutation in itertools.product((0x10,0x1030,0x3030,0x200010,0x100010,0x300010),range(6),range(3),(0,1)):
        result.append(('O',case(flags1=flags,frames=frames,terrain=terrain,mutation=mutation,seed=0xabc123)))
    for genus,fort,blocked,mode,count in itertools.product((0,0x400000,0x40000),(0,1),(0,1),(0,1,2),(0,1,2)):
        result.append(('O',case(flags2=genus,fort=fort,blocked=blocked,special=mode==2,mission=mode==1,qa=count)))
    for profile,flags,seed in itertools.product(range(4),(0x20000,0x23030),(0,0x12345678)):
        result.append(('O',case(flags1=flags,contents=profile,seed=seed)))
    result.extend([('O',case(type=0,stored=0)),('O',case(type=97,stored=97)),('O',case(type=157,stored=157)),
                   ('O',case(type=158,stored=158,flags1=0x10,quarter=1)),('O',case(flags1=0x20000,contents=1,mutation=3))])
    result.extend([('O',case(stored=77,type=100)),('O',case(stored=100,type=0)),('O',case(stored=97,type=100))])
    for slot,chaff,fort,pattern in itertools.product((1,2,8),(0,1),(0,1),(0,1,2)):
        result.append(('L',case(slot=slot,defaultOwner=slot,chaffMode=chaff,fort=fort,qa=pattern,flags2=0x40000)))
    return result


class FortOracle(ArrivalOracle):
    """기존 PE/일반 finder 격리 메모리를 재사용하며 새 실제 Fort 몸체만 실행한다."""
    def __init__(self,edition):
        """두 판본 코드/전역·저장 스트림과 명시 효과 진입점을 준비한다."""
        super().__init__(edition);self.p=PLAN[edition];self.mu.mem_map(DATA,0x40000);self.mu.mem_map(0,0x1000)
        self.exports=[ROOT/f'extracted/fortobjectload/{edition}/{n}' for n in ('creation.c','functions.tsv')]
        self.allowed=[];self.entries={}
        with self.exports[1].open(encoding='utf-8-sig') as handle:
            # export의 분리된 범위를 모두 허용하고 실제 진입 수를 기록한다.
            for row in csv.DictReader(handle,delimiter='\t'):
                self.entries[int(row['entry'],16)]=row['entry']
                for part in row['ranges'].split(';'):
                    lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))
        if edition=='originalCD':
            # 성공한 본체 export를 재사용하고 새로 확인한 세 CD helper만 더한다.
            extra=[ROOT/f'extracted/fortobjectload-helper/{edition}/{n}' for n in ('creation.c','functions.tsv')];self.exports+=extra
            with extra[1].open(encoding='utf-8-sig') as handle:
                for row in csv.DictReader(handle,delimiter='\t'):
                    self.entries[int(row['entry'],16)]=row['entry']
                    for part in row['ranges'].split(';'):
                        lo,hi=(int(x,16) for x in part.split('-'));self.allowed.append((lo,hi+1))
        self.boundaries=collections.Counter();self.native=collections.Counter()

    def write(self,mu,access,address,size,value,data):
        """실제 저장 필드·stream cursor·난수/로딩 전역·스택/후보 배열 밖의 쓰기를 거부한다."""
        if address==0 and size==4:return
        ranges=[(0x30000000,0x30040000),(STREAM,STREAM+0x400),(HEAP,HEAP+0x400),(POOL+100*self.s['stride'],POOL+240*self.s['stride'])]
        ranges += [(a,a+4) for name,a in self.p['globals'].items() if name in ('rng','depth','scope','version','conversion')]
        if any(lo<=address and address+size<=hi for lo,hi in ranges):return
        raise RuntimeError(f'Fort 미허용 쓰기 {address:08x}/{size}')

    def ret(self,value=0,purge=0):
        """명시 경계의 원래 cdecl/thiscall 반환 폭을 유지한다."""
        self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.return_stub(purge)

    def args(self,count):
        """스택 인자를 DWORD 원시 비트로 읽는다."""
        return struct.unpack(f'<{count}I',self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def event(self,kind,*values):
        """실제 효과 순서/인자를 기대 문자열로 저장한다."""
        self.events.append(':'.join(map(str,(kind,*values))));self.boundaries[kind]+=1

    def instruction(self,mu,address,size,data):
        """실제 type/HP/frame/random/Fort 분기는 실행하며 아래 가상/입출력 효과만 대체한다."""
        p=self.p;c=self.c;g=p['globals'];ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==p['assert_']:raise RuntimeError(f'Fort 원본 assert {self.edition} {self.mode} {c}')
        if address==p['create']:
            typ,flags=self.args(2);sid=100+self.born;self.born+=1
            raw=bytearray((i*37+11)&255 for i in range(self.s['stride']));struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[11]=4
            raw[6:8]=b'\0\0';mu.mem_write(self.slot(sid),bytes(raw));self.event('C',typ,flags,sid);self.ret(self.slot(sid));return
        if address==p['content']:
            typ,quantity,flags,attach,quality=self.args(5);parent=ecx;sid=200+self.children;self.children+=1
            raw=bytearray(self.s['stride']);struct.pack_into('<I',raw,0,VTABLE);raw[10]=6;raw[11]=8;mu.mem_write(self.slot(sid),bytes(raw))
            self.event('I',(parent-POOL)//self.s['stride'],typ,quantity,flags,attach,quality,sid);self.ret(sid,20);return
        if address==p['notify']:
            self.event('N',*self.args(1));self.ret();return
        if address==p['new']:self.ret(HEAP);return
        if address==p['free']:self.ret();return
        if VIRTUAL<=address<VIRTUAL+0x100:
            sid=(ecx-POOL)//self.s['stride'];offset=address-VIRTUAL
            if offset==0x74:
                owner,=self.args(1);self.event('O',sid,owner);self.put(ecx+(34 if self.edition=='originals' else 32),owner,1)
                if c['mutation']==1:self.put(ecx+10,101,1)
                if c['mutation']==2:self.put(g['blocked'],1)
                self.ret(0,4)
            elif offset==0x84:self.event('S',sid,*self.args(1));self.ret(0,4)
            elif offset==0x80:self.event('Q',sid);self.ret(c['qa'])
            elif offset==0x90:
                x,y,flags=self.args(3);self.event('P',sid,x,y,flags)
                if c['mutation']==3:self.put(TYPES+mu.mem_read(ecx+10,1)[0]*self.s['type_stride']+0xe8,0)
                self.ret(0,12)
            elif offset==0x10:self.event('D',sid,*self.args(1));self.ret(0,4)
            else:raise RuntimeError(f'가상 slot {offset:x}')
            return
        if address==p['prepare']:self.ret();return
        if address==p['format']:
            buffer,_,territory=self.args(3);mu.mem_write(buffer,f'Terr{territory:02d}\0'.encode());self.ret(buffer);return
        if address==p['lookup']:
            name,_=self.args(2);text=bytes(mu.mem_read(name,8)).split(b'\0')[0].decode();self.ret(self.sections[text],8);return
        if address==p['region']:
            x,y=self.args(2);self.last_xy=(x,y);self.event('G',x,y);self.put(DATA+0x30000,(x+y*2)%3,1);self.ret(DATA+0x30000,8);return
        if address==p['allowed']:
            region,owner,territory=self.args(3);self.event('A',region,owner,territory);accepted=(region+territory+c['qa'])%2==0;self.ret(accepted,12);return
        if address==p['finish']:self.event('F',*self.args(1));self.ret();return
        if address==p.get('release'):self.put(g['conversion'],0);self.ret();return
        if address in (p['object'],p['chunk']):pass
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'Fort 미허용 실행 {self.edition} {address:08x}')
        if address in self.entries:self.native[self.entries[address]]+=1

    def read(self,address):
        """관찰용 DWORD 읽기다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def set_type(self,typ,flags1,flags2,frames):
        """원본 type 구조체와 프레임 코드 입력만 공급한다."""
        a=TYPES+typ*self.s['type_stride'];self.put(a,803);self.put(a+0xe8,flags1);self.put(a+0xec,flags2)
        self.put(a+0x114,len(frames));self.put(a+0x124,FRAME_CODES+typ*0x500)
        self.mu.mem_write(FRAME_CODES+typ*0x500,b''.join(bytes(frame) for frame in frames))
        self.put(a+(0x1d4 if self.edition=='originals' else 0x1b4),1);self.put(a+(0x1d8 if self.edition=='originals' else 0x1b8),1)

    def run(self,mode,c,control):
        """두 FPU 정밀도로 실제 object 또는 전체 template 진입/정상 반환과 상태를 관찰한다."""
        self.mode,self.c=mode,c;self.events=[];self.born=self.children=0;mu=self.mu;p=self.p;g=p['globals'];s=self.s
        mu.mem_write(TYPES,bytes(0x20000));mu.mem_write(POOL,bytes(0x200000));mu.mem_write(MAP,bytes(self.map_end-MAP));mu.mem_write(DATA,bytes(0x40000))
        self.put(s['types'],TYPES);self.put(s['pool'],POOL);self.put(s['capacity'],32768);self.put(s['board'],256)
        mu.mem_write(s['grid'],struct.pack('<7I',0,1,256,*self.bases))
        for off in (0x10,0x74,0x80,0x84,0x90):self.put(VTABLE+off,VIRTUAL+off)
        values=dict(version=c['version'],conversion=0,depth=3,scope=0,bridge=82,island=97,large=98,noIsland=157,ownerSaved=0x5d77cf00,
            ownerCleared=0x10002000,keep=0x40480002,priest=158,altar=164,dais=165,rng=c['seed'],width=2,height=2)
        for key in ('special','mission','fort','blocked','quarter','chaffMode'):values[key]=c[key]
        for key,value in values.items():self.put(g[key],value)
        self.put(0x5e4794,0)
        self.set_type(c['type'],c['flags1'],c['flags2'],codes(c['frames']));self.set_type(101,c['flags1'],c['flags2'],codes(c['frames']))
        self.set_type(164,0x10,0,[]);self.set_type(110,0x1000,0,[]);self.set_type(111,0x2000,0,[]);self.set_type(112,0x20000,0,[])
        self.put(TYPES+110*s['type_stride']+0x100,0x110);self.put(TYPES+112*s['type_stride']+0x100,0x20)
        data=packet(c);self.sections={}
        if mode=='O':
            if c['stored']<246 and c['stored']!=c['type']:
                # 원본 변환표 입력만 공급한다. 변환 후 island/빈 타입 판정은 실제 객체 body가 실행한다.
                conversion=DATA+0x400;mu.mem_write(conversion,bytes(range(255)));self.put(conversion+c['stored'],c['type'],1);self.put(g['conversion'],conversion)
            mu.mem_write(BYTES,data);mu.mem_write(STREAM,struct.pack('<4I',BYTES,0,len(data),len(data)));entry=p['object'];args=(STREAM,c['x'],c['y'],c['defaultOwner']);ecx=0
            if c['terrain']:
                sid=50;a=self.slot(sid);self.put(a,VTABLE);self.put(a+10,97,1);self.put(a+11,0,1)
                self.put(a+14,bits(c['x']+(c['position']>>4)));self.put(a+18,bits(c['y']+(c['position']&15)))
                self.put(a+(36 if self.edition=='originals' else 34),0xffffffff if c['terrain']==2 else 7,4 if self.edition=='originals' else 1)
                self.set_type(97,0,2,[]);self.put(self.bases[0]+((c['y']+(c['position']&15))*256+c['x']+(c['position']>>4))*2,sid,2)
        else:
            # 모든2×2 청크 자료를 공급하되 Terr는 정책이 선택한 개수만 실제 소비한다.
            for index,name in enumerate([f'Terr{i:02d}' for i in range(20)]+['Chaff']):
                body=bytes((2,))+b''.join(bytes((99,1,0))+bytes((0x21,c['stored'],c['owner'])) for _ in range(4)) if index in (0,1,20) else b'\0'
                address=BYTES+index*0x100;stream=STREAM+index*16;mu.mem_write(address,body);mu.mem_write(stream,struct.pack('<4I',address,0,len(body),len(body)));self.sections[name]=stream
            ecx=p['template']+c['slot']*20;self.put(ecx,0x1111);entry=p['load'];args=(0,20)
        for reg,value in ((UC_X86_REG_EBX,0x11223344),(UC_X86_REG_ESI,0x22334455),(UC_X86_REG_EDI,0x33445566),(UC_X86_REG_EBP,0x44556677),
                (UC_X86_REG_ECX,ecx),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,control),(UC_X86_REG_FPSW,0)):mu.reg_write(reg,value)
        self.put(0,0);mu.mem_write(STACK,struct.pack(f'<{len(args)+1}I',STOP,*args));mu.emu_start(entry,STOP,count=1000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+(12 if mode=='L' else 4):raise RuntimeError('Fort 정상 반환 규약 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.read(0):raise RuntimeError('Fort x87/SEH 규약 오류')
        for reg,value in ((UC_X86_REG_EBX,0x11223344),(UC_X86_REG_ESI,0x22334455),(UC_X86_REG_EDI,0x33445566),(UC_X86_REG_EBP,0x44556677)):
            if mu.reg_read(reg)!=value:raise RuntimeError('Fort 보존 레지스터 오류')
        raws=';'.join(bytes(mu.mem_read(self.slot(100+i),s['stride'])).hex() for i in range(self.born)) or '-'
        state=','.join(str(self.read(g[key])) for key in ('version','depth','scope','blocked'))
        return data.hex(),';'.join(self.events) or '-',raws,str(self.read(g['rng'])),state


def generate():
    """고유 input마다 실제 두 정밀도 관찰이 같은 경우만 저장하며 재실행 수를 새 입력에 더하지 않는다."""
    rows=[];reports={};files={Path(__file__),FIXTURE,ROOT/'tools/ghidra/fortobjectload-functions.json',ROOT/'tools/ghidra/fortobjectload-helper-functions.json',
        ROOT/'cpppj/src/o/RawFortObjectLoad.h',ROOT/'cpppj/src/o/RawFortObjectLoad.cpp',ROOT/'tools/decomp_constructionarrival_oracle.py',ROOT/'tools/decomp_damageablepredestroy_oracle.py'}
    for edition in PLAN:
        oracle=FortOracle(edition)
        for mode,c in inputs():
            values=[oracle.run(mode,c,control) for control in CONTROLS];assert values[0]==values[1],(edition,mode,c)
            rows.append('\t'.join((edition,mode,','.join(str(int(c[k])) for k in KEYS),*values[0])))
        reports[edition]=dict(inputs=len(inputs()),native=dict(oracle.native),boundaries=dict(oracle.boundaries));files.update(oracle.exports);files.add(ROOT/FINDER_SPECS[edition]['binary'])
        print(f'{edition} Fort 객체/contained/전체 순회 {len(inputs())}개 정상 반환',flush=True)
    FIXTURE.write_text('# 판본 모드 입력 저장bytes effects raw rng version,depth,scope,blocked\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(host='VM-W11-CODEX',date='2026-10-13',total=len(rows),editions=reports,
        files={f.relative_to(ROOT).as_posix():sha(f) for f in sorted(files)},limits=['실제 Fort 객체/내용물/청크/Terr→Chaff/HP/일반 finder/P/P 난수 실행',
        'factory·가상 owner/state/pop/destroy/count·contained 생성·지역 정책·데이터 관리/후처리는 명시 경계','실제 게임/GUI 및 캠페인 완주 검사가 아님']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """현재 보호 PE/새 코드/export/fixture와 실제 입력·정상 범위를 감사한다."""
    r=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in r['files'].items():assert sha(ROOT/name)==value,name
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==r['total']==2*len(inputs());assert all(len(row)==8 for row in rows)
    for edition,p in PLAN.items():assert r['editions'][edition]['native'][f"{p['object']:08x}"] and r['editions'][edition]['native'][f"{p['load']:08x}"]
    print(f'Fort 현재 SHA/실제 body/저장 필드/입력 {len(rows)}개 감사 통과')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');o=parser.parse_args();verify() if o.verify else generate()
