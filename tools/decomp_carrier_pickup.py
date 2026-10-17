#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호PE의일반포획요청/서버변환/기존내용물제거와실제종속finder를독립실행한다."""
import argparse,collections,itertools,json,socket,struct
from decomp_cursor_lifecycle import Oracle as BaseOracle,ROOT,STACK,STOP,TYPES,VTABLE,digest
from decomp_cursor_position import ranges
from decomp_carrier_falling import Oracle as PreviousOracle,POOL
from decomp_nugget_regular import bits
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP

FIXTURE=ROOT/'cpppj/tests/fixtures/carrier-pickup-x86.tsv' # 새독립원문이며기존fixture를덮어쓰지않는다.
REPORT=ROOT/'cpppj/carrier-pickup.json' # 원본반환/현재raw/명령및명시경계다.
ENTRIES=(0x428e00,0x426520,0x4264d0,0x40a9a0,0x4df230,0x4ac200,0x49a840,0x40eb80,0x4b2270,0x4b1b20,0x44daa0) # 전체실행할11몸체다.
DETACH,ATTACH,DESTROY=STOP+0x800,STOP+0x820,STOP+0x840 # 현재가상+48/+50/+10의큰하위효과다.
PACKET,INPUT,THUNK,DISCARD=STOP+0x900,STOP+0x940,STOP+0x980,STOP+0xa00 # cdecl고정호출코드와현재입력이다.
SLOTS=(50,51,52,60,61,70,80,81,82) # 부모/대체부모/기존내용물/대상둘/새내용물/종속셋의전체raw다.


def form_parent_notice():
    """보호PE의ContentForm6 부모통지주소와그빈ret8 원문을직접읽는다."""
    import pefile
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));entry=struct.unpack('<I',pe.get_data(0x112208+0x24,4))[0]
    return dict(vtable=0x512208,entry=entry,code=pe.get_data(entry-0x400000,3).hex())


def cases():
    """현재권한/단독미션/순서/임시dead/다음선행조회/헤더를독립입력한다."""
    base=[0,1,1,60,154,154,0,0,0x8000,61,128,0x100,0,168,168,0x030000b1,1,0,1,0x4f,9,0x1234,6,0,7,1,bits(20.75),bits(21.5),70];rows=[]
    # 요청후보조회는최초권한보다먼저실행되며bool은요청을바꾸지않는다.
    for boss,single,found,boolean,mutation in itertools.product((0,1),(0,1),(0,60),(0,1),(0,1,2)):
        r=base.copy();r[1:4]=boss,single,found;r[25]=boolean;r[17]=mutation;rows.append(r)
    # 원본헤더는bit7만지우며길이를4 이상으로보정하지않는다.
    for protocol,message,length,kind in itertools.product((0,1,0x80,0xff),(0,0x4f,0xff),(0,1,3,4,9,0xffff),(154,158)):
        r=base.copy();r[18:21]=protocol,message,length;r[4]=kind;rows.append(r)
    # 수신의최초free|dead 및현재타입BYTE 관문이다.
    for boss,parent,target,kind in itertools.product((0,1),(0,1,2,3,4,8,12),(0,1,2,3,4,8,12),(154,155)):
        r=base.copy();r[0]=1;r[1]=boss;r[6:8]=parent,target;r[5]=kind;rows.append(r)
    # 반환값/예상체인을주입하지않고효과뒤현재raw와전역을바꾼다.
    for genus,old,oldkind,scene,mutation in itertools.product((0,0x100,0x2000,0x2100,0x8000,0x200000),(0,52),(154,168),range(4),range(20)):
        r=base.copy();r[0]=1;r[8]=genus;r[12:14]=old,oldkind;r[16:18]=scene,mutation;rows.append(r)
    # 기존첫mask1은kind를보관하지만특정타입전역은삭제뒤다시읽는다.
    for old,kind,silent,mutation in itertools.product((0,52),(154,168,169),(154,168),(0,6,7,8,15)):
        r=base.copy();r[0]=2;r[12:15]=old,kind,silent;r[17]=mutation;rows.append(r)
    # 권한진단이현재boss를고친뒤동일한관문을계속검사한다.
    for parent,target,kind in itertools.product(range(4),range(4),(154,155)):
        r=base.copy();r[0]=1;r[1]=0;r[6:8]=parent,target;r[5]=kind;r[17]=3;rows.append(r)
    # 현재입력명령/QA WORD/생성반환SID/contained타입을상수로대체할수없다.
    for command,argument,created,genus,checking,contained in itertools.product((0,1,0x030000b1,0xffffffff),(0,1,0x8000,0xffff),(70,71),(0x100,0x8000),(0,1),(3,6)):
        r=base.copy();r[0]=1;r[15]=command;r[21]=argument;r[28]=created;r[8]=genus;r[23]=checking;r[22]=contained;r[16]=2;rows.append(r)
    # 요청은부모의free/dead를추가관문으로읽지않는다.
    for flags in range(4):
        r=base.copy();r[6]=flags;rows.append(r)
    return list(dict.fromkeys(tuple(r) for r in rows))


class Oracle(BaseOracle):
    """큰월드/장치효과만분리하며서버와내장종속finder는보호PE를그대로실행한다."""
    def __init__(self):
        """격리슬롯/가상경계와cdecl고정호출코드를한번설치한다."""
        super().__init__();self.mu.mem_map(POOL,0x10000);self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter()
        self.put(VTABLE+0x48,DETACH);self.put(VTABLE+0x50,ATTACH);self.put(VTABLE+0x10,DESTROY)
        code=b'\x68'+struct.pack('<I',PACKET)+b'\x6a\x00\xff\x35'+struct.pack('<I',INPUT)+b'\xe8'+struct.pack('<i',0x426520-(THUNK+18))+b'\x83\xc4\x0c\xc3'
        assert len(code)==22;self.mu.mem_write(THUNK,code)
        code=b'\x6a\x32\xe8'+struct.pack('<i',0x4264d0-(DISCARD+7))+b'\x83\xc4\x04\xc3';assert len(code)==11;self.mu.mem_write(DISCARD,code)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def mutate(self,where,sid=0):
        """명시효과만현재전역/raw/호출자소유packet을변경한다."""
        m=self.case[17];target=POOL+self.target*50
        if m==1 and where=='candidate':self.put(0x540bc4,0)
        if m==2 and where=='candidate':self.put(0x594fa4,0);self.put(POOL+60*50+10,155,1)
        if m==3 and where=='assertion':self.put(0x540bc4,1)
        if m==4 and where=='reveal':self.put(PACKET+4,51,2);self.put(0x5429b8,0xffffffff)
        if m==5 and where=='queue':self.put(target+10,155,1);self.put(target+12,0x2222,2)
        if m==6 and where=='find':self.put(0x5412f8,169)
        if m==7 and where=='destroy-old':self.put(0x5412f8,self.case[13])
        if m==8 and where=='after-discard':self.put(target+10,133,1);self.put(target+12,0x3333,2)
        if m==9 and where=='create':self.put(target+11,0x0f,1)
        if m==10 and where=='create':self.put(target+12,0x4444,2)
        if m==11 and where=='detach' and sid==80:self.put(POOL+82*50+4,0,2)
        if m==12 and where=='detach' and sid==80:self.put(POOL+82*50+10,7,1)
        if m==13 and where=='attach':self.put(target+10,70,1)
        if m==14 and where=='create':self.put(0x541088,3)
        if m==15 and where=='destroy-old':self.put(TYPES+self.mu.mem_read(target+10,1)[0]*500+0xec,0x100)
        if m==16 and where=='reveal':self.put(0x540bc4,0)
        if m==17 and where=='queue':self.put(PACKET+4,51,2)
        if m==18 and where=='create':self.put(target+10,70,1)
        if m==19 and where=='send':self.put(0x52ee0b,0xff,1);self.put(0x52ee0a,0x55,1);self.put(0x52ee0c,3,2)

    def instruction(self,mu,address,size,data):
        """실제호출인자와그시점의임시dead를기록하고전체실행범위를감사한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);a=struct.unpack('<5I',mu.mem_read(sp+4,20));this=mu.reg_read(UC_X86_REG_ECX);sid=(this-POOL)//50;c=self.case
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x426900:self.event('P',sid,a[1],a[2]);self.mutate('candidate');self.put(a[0],c[3]);self.put(a[0]+4,c[25]);self.ret(a[0],12);return
        if address==0x4e2dc0:self.packet=bytes(mu.mem_read(a[1],9)).hex();self.events.append('S:'+str(a[0])+':'+self.packet);self.mutate('send');self.ret();return
        if address==0x4e0620:
            assert a[2]==0x20c and self.string(a[1])=='\\Ns\\O\\Carrier.cpp';self.event('I',a[2]);self.mutate('assertion');self.ret();return
        if address==0x4620c0:self.event('V',a[0]);self.target=c[9];self.mutate('reveal');self.ret(c[9]);return
        if address==0x452fe0:self.target=a[1];self.event('Q',*a[:3]);self.mutate('queue');self.ret();return
        if address==0x4ac9f0:self.event('F',sid,*a[:3]);self.mutate('find');self.ret(c[12],12);return
        if address==0x485e40:self.event('B');self.mutate('after-discard');self.ret();return
        if address==0x4afa70:self.event('C',sid,*a,mu.mem_read(POOL+self.target*50+11,1)[0]);self.mutate('create');self.ret(c[28],20);return
        if address==DESTROY:self.event('D',sid,a[0],mu.mem_read(this+11,1)[0],mu.mem_read(this+10,1)[0]);self.mutate('destroy-old' if sid==52 else 'destroy-target',sid);self.ret(0,4);return
        if address==DETACH:self.event('U',sid,a[0]);self.mutate('detach',sid);self.ret(0,4);return
        if address==ATTACH:self.event('A',sid,*a[:2]);self.mutate('attach',sid);self.ret(0,8);return
        if THUNK<=address<THUNK+22 or DISCARD<=address<DISCARD+11:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'포획원본범위밖실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """스택/SEH와대상두슬롯의상태BYTE만실제몸체에서쓸수있다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4 or size==1 and address in (POOL+60*50+11,POOL+61*50+11):return
        raise RuntimeError(f'포획원본범위밖쓰기 {address:08x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호이미지와현재격리자료/스택/호출자범위만허용한다."""
        spans=((0,4),(0x400000,0x600000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(STACK-0x7f000,STACK+0x1000),(STOP,STOP+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in spans):raise RuntimeError(f'포획원본범위밖읽기 {address:08x}+{size}')

    def sample(self,c,precision):
        """원본에입력만제공하고전체raw/현재전역/효과를읽는다."""
        self.case=c;self.events=[];self.packet='-';self.target=60
        # 모든정상가상호출입력은같은실제물리슬롯폭을쓴다.
        for sid in SLOTS:
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=c[4] if sid==60 else c[10] if sid==61 else 133 if sid in (50,51) else 3 if sid==81 else 6;raw[11]=c[6] if sid==50 else c[7] if sid==60 else 8 if sid>=80 else 0;raw[40]=0;struct.pack_into('<II',raw,14,c[26],c[27]);struct.pack_into('<HH',raw,4,0,0);self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.put(POOL+52*50+18,c[13],2)
        self.put(POOL+50*50+6,c[12],2)
        # 포획대상과대체대상은같은명시종속체인을가지며변환몸체는외부경계다.
        chain=((),(80,82),(80,81,82),(81,80,82))[c[16]]
        for sid in (60,61):self.put(POOL+sid*50+6,chain[0] if chain else 0,2);self.put(POOL+sid*50+12,c[21],2)
        for i,sid in enumerate(chain):self.put(POOL+sid*50+4,chain[i+1] if i+1<len(chain) else 0,2)
        self.mu.mem_write(TYPES,bytes(0x20000));self.put(TYPES+c[4]*500+0xec,c[8]);self.put(TYPES+c[10]*500+0xec,c[11]);self.put(TYPES+155*500+0xec,0x2100);self.put(TYPES+133*500+0xec,0x100)
        for a,v in ((0x5c8464,POOL),(0x59ab20,TYPES),(0x540bc4,c[1]),(0x594fa4,c[2]),(0x541088,c[22]),(0x5e4794,c[23]),(0x5412f8,c[14]),(0x5429b8,c[15])):self.put(a,v)
        self.put(0x52ee0a,c[19],1);self.put(0x52ee0b,c[18],1);self.put(0x52ee0c,c[20],2);self.put(INPUT,c[24]);self.mu.mem_write(PACKET,struct.pack('<BB3HB',c[18]&0x7f,c[19],c[20],50,60,c[5]));self.put(0,0x87654321)
        PreviousOracle.invoke(self,(0x428e00,THUNK,DISCARD)[c[0]],b'',precision,False);self.returns+=1;assert self.get(0)==0x87654321
        state=(self.get(0x540bc4),self.get(0x594fa4),self.get(0x5412f8),self.get(0x5429b8),self.get(0x541088),self.mu.mem_read(0x52ee0b,1)[0],self.mu.mem_read(0x52ee0a,1)[0],struct.unpack('<H',self.mu.mem_read(0x52ee0c,2))[0])
        return ';'.join(self.events) or '-',','.join(map(str,state)),','.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in SLOTS),self.packet


def main():
    """두정밀도원문을새로관측하거나현재SHA/입력순서/정상반환을검사한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--verify',action='store_true');args=p.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'));assert report['contentFormParentNotice']==form_parent_notice();rows=[l.split('\t') for l in FIXTURE.read_text('utf-8').splitlines() if l and not l.startswith('#')]
        # 기대값은읽기만하며현재보고서의실제바이트와입력전수를확인한다.
        for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
        assert [r[0] for r in rows]==[','.join(map(str,c)) for c in cases()] and len(rows)==report['uniqueInputs'] and len(rows)*2==report['normalReturns'];assert [i for i,r in enumerate(rows) if r[1:5]!=r[5:9]]==report['precisionDifferentInputs'];print('일반포획원본감사',len(rows),report['normalReturns']);return
    oracles=[Oracle(),Oracle()];rows=[];different=[]
    # C++ 결과를읽지않고각정밀도에서전체원본을독립실행한다.
    for index,c in enumerate(cases()):
        outputs=[o.sample(c,p) for o,p in zip(oracles,(0x27f,0x37f))]
        if outputs[0]!=outputs[1]:different.append(index)
        rows.append((','.join(map(str,c)),*outputs[0],*outputs[1]))
    FIXTURE.write_text('# input effects53 state53 raws53 packet53 effects64 state64 raws64 packet64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originalCD/NETSTORM.EXE','tools/decomp_carrier_pickup.py','tools/decomp_carrier_falling.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','extracted/refined/originals/functions.tsv','cpppj/tests/fixtures/carrier-pickup-x86.tsv')
    report=dict(contentFormParentNotice=form_parent_notice(),typeRecordStride=500,host=socket.gethostname(),date='2026-10-18',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-16'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),precisionDifferentInputs=different,boundaries='428e00/426520/4264d0/40a9a0/4df230/4ac200/49a840/40eb80/4b2270/4b1b20/44daa0 전체. 후보426900/서버송신/권한진단/오벨리스크변환/입력큐/기존내용물조회/보충부모/실제Content생성/가상삭제·Unpop·Attach는명시효과경계다. C++는같은실제종속finder와현재TYPE를쓴다. GP/retN/SEH/canary/x87/실행·읽기·쓰기범위/전체9슬롯/현재전역/패킷/호출순서·인자·임시dead를검사했다. 실제게임창·전체경제·자연완주검증이아니다.',sha256={s:digest(ROOT/s) for s in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({k:report[k] for k in ('uniqueInputs','normalReturns','instructions','nativeCalls','precisionDifferentInputs')},ensure_ascii=False))


if __name__=='__main__':main()
