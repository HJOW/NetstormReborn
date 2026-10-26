#!/usr/bin/env python3
"""원문0047fb90 입력 부모를 격리PE에서 실행하고 전용GUI/World 경계만 명시 장치로 관측한다."""
import argparse,csv,hashlib,io,json,struct,sys
from pathlib import Path

# 산출물은469 staging에만 쓰고 원본/기존fixture는 읽기만 한다.
STAGING=Path(__file__).resolve().parents[1]
ROOT=next(p for p in STAGING.parents if (p/'AGENTS.md').exists())
sys.path.insert(0,str(ROOT/'extracted/oracle-python'));sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle,SCRATCH,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 실제 원문 필드와 명시장치 주소를 분리한다.
PARENT,LIST,MAP,CLOSE,DRAG,VTABLE,IMAGE= [SCRATCH+x for x in (0,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000)]
HELPERS=[0x4239c0,0x4e49c0,0x46d9f0,0x46da20,0x431fc0,0x46ee50,0x4bd300,0x40d900,0x41cc20]
FIXTURE=STAGING/'cpppj/tests/fixtures/editor-island-drag-x86.tsv'
REPORT=STAGING/'cpppj/editor-island-drag-native-20261027.json'
FIELDS=('name','code','x','y','dragIn','canPlace','imageIn','width','height','control','result','flags','dirty','dragOut','source','record','revision','trace','instructions')

def bits(value):
    """장치 경계의 실제 float32비트를 기록한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

class DragOracle(Oracle):
    """부모의 실제 분기/좌표/flag/목록순서와 작은 원문 helper를 실행한다."""
    def __init__(self):
        """원문 기계어를 변경하지 않는 격리메모리와 가상호출 표를 준비한다."""
        super().__init__('originals',[dict(name='Input',originals='0047fb90')],HELPERS)
        self.stubs={};self.mu.mem_map(0,0x1000)
        self.original=[(lo,bytes(self.mu.mem_read(lo,hi-lo))) for lo,hi in self.allowed]
        self.entered={}

    def read(self,address,width=4):
        """실제 DWORD/WORD/BYTE를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def put(self,address,value,width=4):
        """명시 입력 장치의 물리값만 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def args(self,count):
        """실제 thiscall/cdecl 스택 인자를 읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(self.mu.reg_read(UC_X86_REG_ESP)+4,count*4))

    def ret(self,value=0,purge=0):
        """명시장치만 실제 ret폭으로 복귀한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.mu.reg_write(UC_X86_REG_EIP,self.read(sp));self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def event(self,name,*args):
        """관측된 원문 호출순서를 안정된 숫자열로 기록한다."""
        self.trace.append(':'.join(map(str,(name,*args))))

    def on_instruction(self,mu,address,size,data):
        """전용Gump/Bitmap/World 외의 부모·수치·Builder는 스텁하지 않는다."""
        self.instructions+=1
        if address in [0x47fb90,*HELPERS]:self.entered[f'{address:08x}']=self.entered.get(f'{address:08x}',0)+1
        this=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4648e0:
            out=self.args(1)[0];kind={PARENT:'parent',LIST:'list',MAP:'map'}[this]
            bounds={'parent':(0,0,432,412),'list':(0,0,20,300),'map':(100,100,324,324)}[kind]
            mu.mem_write(out,struct.pack('<4i',*bounds));self.event('bounds',kind);self.ret(out,4);return
        if address==0x47d1c0:self.event('image',self.args(1)[0]);self.ret(IMAGE if self.case['imageIn'] else 0,4);return
        if address==0x47d170:self.event('index',self.args(1)[0]);self.ret(3,4);return
        if address==0x4e4391:
            if self.args(1)!=(0xa8,):raise RuntimeError('원문 drag 할당폭')
            self.event('new');self.ret(DRAG);return
        if address==0x47f2a0:self.event('construct');self.ret(DRAG,4);return
        if address==0x47cbd0:self.event('adjust');self.ret();return
        if address==STOP+0x100:self.event('attach',*self.args(2));self.ret(0,16);return
        if address==0x465250:self.event('push');self.ret();return
        if address==0x466480:self.event('killParent' if this==PARENT else 'killDrag');self.ret();return
        if address==STOP+0x104:self.event('update',self.args(1)[0]);self.ret(0,4);return
        if address==0x46de50:
            self.event('can',self.read(this+20),self.args(2)[0]&0xffffff);self.ret(self.case['canPlace'],8);return
        if address==0x46e0d0:
            record=struct.pack('<II',*self.args(2))[:6].hex();self.event('add',record,self.args(3)[2],*struct.unpack('<5I',mu.mem_read(this+4,20)));self.ret(0,12);return
        if address==0x433aa0:self.event('camera',*self.args(2));self.ret();return
        if address==0x46d8a0:self.event('releaseImage');self.ret();return
        if address==0x4e4354:self.event('freeImage');self.ret();return
        if address==0x47d1f0:self.event('remove',self.args(1)[0]);self.ret(0,4);return
        if address==0x4e0620:self.event('assert',self.args(3)[2]);self.ret();return
        return super().on_instruction(mu,address,size,data)

    def sample(self,case,control):
        """새 입력마다 원문 상태/실제 정상복귀/읽기전용CODE를 검사한다."""
        self.case=case;self.trace=[];self.instructions=0;self.mu.mem_write(SCRATCH,bytes(0x10000));self.put(0,0xffffffff)
        self.put(PARENT+4,0xa50300);self.put(PARENT+0xe0,CLOSE);self.put(CLOSE+0x2c,777);self.put(PARENT+0xec,LIST);self.put(PARENT+0xdc,MAP)
        self.put(LIST,VTABLE);self.put(VTABLE+8,STOP+0x104);self.put(VTABLE+0x20,STOP+0x100);self.put(DRAG,VTABLE);self.put(LIST+0x14c,5)
        self.put(PARENT+0xf8,14);self.put(PARENT+0xfc,14);self.put(PARENT+0xd84,DRAG if case['dragIn'] else 0);self.put(PARENT+0xd90,88)
        self.mu.mem_write(PARENT+0xd88,bytes((9,3,0,37,41,43)));self.mu.mem_write(PARENT+0xd00,bytes((i*7+3)&255 for i in range(126)))
        self.put(DRAG+0x3c,bits(case['width']));self.put(DRAG+0x40,bits(case['height']));self.put(0x5949d4,77);self.put(0x540cac,6);self.put(0x568b60,4)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        result=self.call('Input',[case['code'],case['x'],case['y']],PARENT,12)
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:raise RuntimeError('원문x87정상복귀')
        for lo,value in self.original:
            if bytes(self.mu.mem_read(lo,len(value)))!=value:raise RuntimeError('원문CODE변경')
        return dict(result=result,flags=self.read(PARENT+4),dirty=self.read(PARENT+0xd80),dragOut=bool(self.read(PARENT+0xd84)),source=self.read(PARENT+0xd90),record=bytes(self.mu.mem_read(PARENT+0xd88,6)).hex(),revision=self.read(0x5949d4),trace=';'.join(self.trace) or '-',instructions=self.instructions)

def cases():
    """선택·빈이미지·경계·반크기·release·배치거부·성공·목록취소를 분리한다."""
    base=dict(name='drop',code=0x10000,x=150,y=180,dragIn=1,canPlace=1,imageIn=1,width=32.,height=48.)
    result=[base]
    for name,change in [('reject',dict(canPlace=0)),('cancel',dict(x=10,y=10)),('outside',dict(x=80,y=90)),('right',dict(x=324,y=180)),('release',dict(code=0x80010000)),('noDrag',dict(dragIn=0)),('negativeHalf',dict(x=100,y=100,width=64.,height=64.)),('negativeTruncate',dict(x=100,y=100,width=2.,height=2.)),('huge',dict(width=4096.)),('fraction',dict(width=3.5,height=5.25)),('begin',dict(code=0x02000004,x=7,y=99,dragIn=0)),('beginExisting',dict(code=0x02000004,x=7,y=99)),('beginZero',dict(code=0x02000004,x=7,y=0,dragIn=0)),('beginNoImage',dict(code=0x02000004,x=7,y=99,imageIn=0)),('close',dict(code=0x0200002c,x=777)),('closeAlt',dict(code=0x0200002f,x=777)),('closeWrong',dict(code=0x0200002c,x=778)),('dirty',dict(code=0x02000003)),('unknown',dict(code=0x020000f0))]:result.append(dict(base,name=name,**change))
    return result

def main():
    """새fixture만 생성하거나 같은바이트를 검증하며 실패로그는 호출자가 번호별 보존한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if FIXTURE.exists()!=args.verify:raise RuntimeError('새fixture보호관문')
    oracle=DragOracle();rows=[]
    for case in cases():
        for control in (0x27f,0x37f):rows.append(dict(case,control=control,**oracle.sample(case,control)))
    out=io.StringIO(newline='');writer=csv.DictWriter(out,fieldnames=FIELDS,delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerows(rows);payload=out.getvalue().encode('utf-8')
    if args.verify:
        if FIXTURE.read_bytes()!=payload:raise RuntimeError('독립원문재현불일치')
    else:
        FIXTURE.parent.mkdir(parents=True,exist_ok=True);FIXTURE.write_bytes(payload)
        REPORT.write_text(json.dumps(dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon/2026-10-23',runs=len(rows),cases=len(cases()),fixtureSha=hashlib.sha256(payload).hexdigest(),sourceSha=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),peSha=oracle.sha256,entered=oracle.entered,helpers=HELPERS,code=[dict(address=f'{lo:08x}',size=len(value),sha=hashlib.sha256(value).hexdigest()) for lo,value in oracle.original],limits=['전체0047fb90·실제half-size x87/ftol/rect/Builder재설정/중심계산을실행','전용Gump/목록/이미지할당반납·CanPlace결과·큰AddIsland월드효과는명시장치','제품/src/test/기존fixture/원문수정및빌드/게임실행없음']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(runs=len(rows),fixtureSha=hashlib.sha256(payload).hexdigest(),verify=args.verify),ensure_ascii=False))

if __name__=='__main__':main()
