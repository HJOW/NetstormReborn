#!/usr/bin/env python3
"""실제10.78 섬population/거주구역/중앙객체교체와 공통랜덤배치를 staging에 독립 관측한다."""
import argparse,csv,hashlib,itertools,json,struct,sys
from pathlib import Path

# 보관본 또는 제품 루트를 찾아 같은 fixture를 검증한다. 원문PE와 정밀export는 읽기만 한다.
STAGING=Path(__file__).resolve().parents[1]
ROOT=next(p for p in (STAGING,*STAGING.parents) if (p/'AGENTS.md').exists())
sys.path.insert(0,str(ROOT/'extracted/oracle-python'));sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle,SCRATCH,STACK,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 원본인라인지도/목록과분리된명시장치/풀주소다.opaque주소를실제호스트포인터로쓰지않는다.
WORLD,ISLANDS,BOSS,FORT=0x54c8a0,0x568b60,0x540bc4,0x594fb8
POOL,BUILDER,TABLE,TYPE,POSITION=0x11000000,SCRATCH+0x10000,SCRATCH+0x12000,SCRATCH+0x13000,SCRATCH+0x14000
ENTRIES={'Residence':0x46df80,'Populate':0x46e000,'Replace':0x40e2b0}
HELPERS=[0x40e130,0x40ded0,0x4bca20,0x4bcab0,0x4e49c0,0x40d900,0x432210,0x432180,0x432430,0x4324e0,0x46ee30,0x4adca0]
FIXTURE=STAGING/'cpppj/tests/fixtures/island-population-x86.tsv'
REPORT=STAGING/'cpppj/island-population-native-20261026.json'
SCHEMA=('entry','profile','seed','fort','boss','flip','primary','fallback','hp','at','mutate','control','trace','frame','hpOut','mapOut','calls','instructions')

def bits(value):
    """원문float32호출인자를정수비트로전달한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

def digest(path):
    """입력/새산출물의원문해시를읽기만한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

def rectangle(profile):
    """유효중앙/음수/작은폭/역전의명시경계입력이다."""
    return [(16.,32.,64.,96.),(15.25,21.5,16.1,23.9),(-15.25,-3.5,4.5,9.5),(20.,21.,20.,21.),(20.,21.,19.,19.)][profile]

class PopulationOracle(Oracle):
    """부모/지도/랜덤몸체는PE그대로실행하고큰World효과만명시관측한다."""
    def __init__(self):
        """현재PC함수범위를허용하고원문/모든helper바이트를고정한다."""
        super().__init__('originals',[dict(name=k,originals=f'{v:08x}') for k,v in ENTRIES.items()],HELPERS)
        self.stubs={};self.mu.mem_map(POOL,0x200000);self.original=[(lo,bytes(self.mu.mem_read(lo,hi-lo))) for lo,hi in self.allowed]
        self.visited=set();self.native={}

    def read(self,address,width=4):
        """실제관측DWORD/WORD를littleendian으로읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시장치입력만현재가상메모리에쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def args(self,count):
        """실제CALL스택의전달인자를추측없이읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def ret(self,value=0,purge=0):
        """장치CALL의실제복귀주소와thiscall인자폭만소비한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_EIP,self.read(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def event(self,name,*values):
        """현재하위와실제입력비트를호출순서대로기록한다."""
        self.events.append(':'.join(map(str,(name,*values))))

    def on_instruction(self,mu,address,size,data):
        """부모의분기·반복·PRNG·float계산을대체하지않는다."""
        self.instructions+=1;self.visited.add(address)
        if address in ENTRIES.values() or address in HELPERS:self.native[f'{address:08x}']=self.native.get(f'{address:08x}',0)+1
        if address==0x40db90:
            random,kind,x,y,region,mode,force=self.args(7);self.calls+=1;self.event('P',kind,x,y,region,mode,force,self.read(random))
            self.lastSeed=self.read(random)
            self.ret(15001 if self.calls>=self.case['at'] else 0);return
        if address==0x4c2630:self.event('D',self.args(1)[0]);self.ret();return
        if address==0x4e0620:
            line=self.args(3)[2];self.event('A',line)
            if self.case['flip'] and line in (0xc3,0xd1):self.put(BOSS,1)
            self.ret();return
        if address==0x460d70:
            out=self.args(1)[0];mu.mem_write(out,struct.pack('<d',2.5));self.event('T',*struct.unpack('<II',struct.pack('<d',2.5)));self.ret(out);return
        if address==0x4adea0:
            out=self.args(1)[0];self.event('position',(mu.reg_read(UC_X86_REG_ECX)-POOL)//50);mu.mem_write(out,struct.pack('<2f',55.25,66.5));self.ret(out,4);return
        if address==STOP+0x100:
            self.event('destroy',(mu.reg_read(UC_X86_REG_ECX)-POOL)//50,*self.args(1))
            if self.case['mutate']:self.put(WORLD+12+(2*16+1)*12+2,0xff,1)
            self.ret(0,4);return
        if address==0x4af530:
            self.event('create',*self.args(2));raw=bytearray([0xa5]*50);struct.pack_into('<I',raw,0,TABLE);raw[10]=self.args(2)[0];raw[11]=4;mu.mem_write(POOL+15002*50,bytes(raw));self.ret(POOL+15002*50);return
        if address==0x4ac550:self.event('type',self.read(mu.reg_read(UC_X86_REG_ECX)+10,1));self.ret(TYPE);return
        if address==0x4adc60:self.event('hasHp');self.ret(self.case['hp']);return
        if address==0x49fce0:self.event('maxHp');self.ret(87);return
        if address==0x46f320:self.event('owner',*self.args(1));self.ret(3,4);return
        if address==STOP+0x104:self.event('setOwner',*self.args(1));self.ret(0,4);return
        if address==STOP+0x108:self.event('pop',*self.args(3));self.ret(0,12);return
        if address==0x47d890:self.event('refresh');self.ret();return
        return super().on_instruction(mu,address,size,data)

    def sample(self,case,control):
        """명시지도로매실행을초기화하고정상ret/실제기계어불변을확인한다."""
        self.case,self.events,self.calls,self.instructions,self.lastSeed=case,[],0,0,0
        chunk=bytearray(struct.pack('<III',1,16,16)+(bytes((127,46))+bytes([0xa5])*6+struct.pack('<I',0x12345678))*256)
        for x,y in [(1,2),(2,2),(1,3),(2,3)]:
            at=12+(y*16+x)*12;chunk[at]=3;chunk[at+2:at+8]=bytes((3,1,0x21,case['seed'],0xab,0xcd))
        self.mu.mem_write(WORLD,bytes(chunk));self.mu.mem_write(ISLANDS,bytes(3204));self.put(ISLANDS,4);self.put(ISLANDS+4+3*32+4,1);self.put(ISLANDS+4+3*32+8,3)
        self.mu.mem_write(BUILDER,struct.pack('<6I',WORLD,1,1,15,15,1));self.put(BOSS,case['boss']);self.put(FORT,case['fort'])
        for address,value in [(0x5411bc,89),(0x541174,case['primary']),(0x54126c,case['fallback']),(0x5412d0,158),(0x5c8464,POOL)]:self.put(address,value)
        self.put(TABLE+0x10,STOP+0x100);self.put(TABLE+0x74,STOP+0x104);self.put(TABLE+0x90,STOP+0x108);self.put(TYPE+0x118,0xfffffff9)
        self.mu.mem_write(POOL,bytes(0x200000));self.put(POOL+15001*50,TABLE);self.put(POOL,TABLE)
        args=[3];this=BUILDER;purge=12 if case['entry']=='Residence' else 4
        if case['entry']=='Residence':args=[3,0x11223344,0xabcd]
        if case['entry']=='Replace':args=[*map(bits,rectangle(case['profile'])),3];this=0;purge=0
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.call(case['entry'],args,this,purge)
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('원문x87정상복귀계약')
        for lo,expected in self.original:
            if bytes(self.mu.mem_read(lo,len(expected)))!=expected:raise RuntimeError('원문CODE수정')
        raw=bytes(self.mu.mem_read(POOL+15002*50,50));result=dict(trace=';'.join(self.events) or '-',frame=int.from_bytes(raw[36:40],'little'),hpOut=int.from_bytes(raw[26:30],'little'),mapOut=bytes(self.mu.mem_read(WORLD,3084)).hex(),calls=self.calls,instructions=self.instructions)
        return result

def cases():
    """의미있는현재권한/두모드/중앙영역/seed/실패후대체TYPE입력을분리한다."""
    result=[]
    for entry in ENTRIES:
        for seed,fort,hp,at in itertools.product((0,255),(0,1),(0,1),(1,3)):
            result.append(dict(entry=entry,profile=0,seed=seed,fort=fort,boss=1,flip=0,primary=71,fallback=133,hp=hp,at=at,mutate=0))
    for profile in range(1,5):result.append(dict(result[-1],entry='Replace',profile=profile))
    for entry in ('Residence','Populate'):
        for flip in (0,1):result.append(dict(result[0],entry=entry,boss=0,flip=flip))
    for primary,fallback in [(0,133),(0,0),(0xffffffff,133)]:result.append(dict(result[0],entry='Populate',primary=primary,fallback=fallback,mutate=1))
    return result

def main():
    """새staging자료만생성하며verify에서는기존바이트와다시대조한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if FIXTURE.exists()!=args.verify:raise RuntimeError('새fixture보호:생성/verify선택불일치')
    oracle=PopulationOracle();rows=[]
    for case in cases():
        for control in (0x27f,0x37f):rows.append(dict(case,control=control,**oracle.sample(case,control)))
    import io
    out=io.StringIO(newline='');writer=csv.DictWriter(out,fieldnames=SCHEMA,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows);payload=out.getvalue().encode('utf-8')
    if args.verify:
        if FIXTURE.read_bytes()!=payload:raise RuntimeError('staging원본재현불일치')
    else:FIXTURE.parent.mkdir(parents=True,exist_ok=True);FIXTURE.write_bytes(payload)
    report=dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon/2026-10-23',runs=len(rows),cases=len(cases()),fixtureSha=hashlib.sha256(payload).hexdigest(),sourceSha=digest(Path(__file__)),peSha=oracle.sha256,native=oracle.native,entries=ENTRIES,helpers=HELPERS,code=[dict(address=f'{lo:08x}',bytes=len(value),sha=hashlib.sha256(value).hexdigest()) for lo,value in oracle.original],limits=['원문46df80/46e000/40e130/40e2b0·실제지도getter/setter·40ded0/PRNG/x87실행','기존독립검증40db90의단일객체World효과·실제시간·가상삭제/생성/owner/Pop·현재TYPE/HP는명시장치','제품코드/기존fixture/PE/export수정및게임실행없음'])
    if not args.verify:REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(cases=len(cases()),runs=len(rows),fixtureSha=report['fixtureSha'],verify=args.verify),ensure_ascii=False))

if __name__=='__main__':main()
