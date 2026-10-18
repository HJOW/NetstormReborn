#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 Burning의 cold 생성/5픽셀 출력/실제 소멸 순서를 제품 전에 독립 관측한다."""
import argparse,csv,itertools,json,socket,struct,zlib
from decomp_burningprocess_oracle import BurningOracle,ROOT,POOL,STACK,STOP,OBJECT,TABLE,HEAP,PALETTE,TOKEN,OFFSETS,digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW

# 현재 객체/입자/색 표/표면은 모두 격리 메모리다. 실제 파일에는 쓰지 않는다.
PARTICLES,COLORS,PIXELS,IMAGE,ROWS=0x11329000,0x1132b000,0x11350000,0x1132c000,0x1132d000
FIXTURE=ROOT/'cpppj/tests/fixtures/burning-lifetime-draw-x86.tsv' # 기존 연소426/생성256원문과 별도다.
REPORT=ROOT/'cpppj/burning-lifetime-draw.json' # 실몸체와 새 gradient/장치 경계를 구별한다.


def bits(value):
    """float 입력의 비트를 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """수명/source/정수화/카메라/clip과 선택 버퍼/flags/첫 색표를 교차한다."""
    rows=[]
    for point,camera,life,source,clip in itertools.product(((0,0),(1.9,2.9),(15.75,16.25),(-.25,-.75),(31.5,32.5),(63,63)),((0,0),(.5,-.5),(2.25,3.75)),(-1,0,.125,4,8.9999,9,10,float('nan')),(0,1),(0,1)):
        rows.append(('D',(4,1,bits(point[0]),bits(point[1]),bits(life),source,bits(camera[0]),bits(camera[1]),clip,0)))
    rows += [('D',(capacity,total,bits(20.75),bits(22.5),bits(4),0,0,0,0,profile)) for capacity,total,profile in ((0,0,0),(4,0,0),(4,4,1))]
    rows += [('X',values) for values in itertools.product((0,1,0x10,3),(0,1),(0,1),(0,1),(0,1))]
    rows += [('C',values) for values in itertools.product((0,1),(0,50),(0,1,9))]
    return rows


class Oracle(BurningOracle):
    """그림은 실제 메모리에 쓰고 삭제도 실제 공통 등록/base 몸체를 실행한다."""
    def __init__(self,control):
        """현재 정밀 표와 기존 생성자의 격리 allocator를 연결한다."""
        super().__init__('originals');self.control=control;self.kind='';self.initial=False;self.extra=[];self.pixels=[];self.return_count=0
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in (0x423ec0,0x487e50,0x4e49c0,0x423cd0,0x423aa0,0x40f1b0,0x487880,0x41bd00,0x4240f0,0x487cb0,0x4437f0):
            self.entries.add(entry)
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.pixel_write)

    def pixel_write(self,mu,access,address,size,value,data):
        """실제 픽셀 쓰기의 중복/순서도 그대로 기록한다."""
        if self.kind=='D' and PIXELS<=address<PIXELS+4096:
            assert size==1;self.pixels.append(str(address-PIXELS)+':'+str(value&255))

    def on_instruction(self,mu,address,size,data):
        """cold 경로의 큰 색 계산과 실제 외부 장치/반납만 선언 경계다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        if self.kind=='C' and address==self.p['ctor'] and not self.initial:
            self.initial=True;self.write_ranges.append((self.p['palette'],self.p['palette']+4));mu.mem_write(self.p['palette'],struct.pack('<I',0 if self.cold else PALETTE));mu.reg_write(UC_X86_REG_FPCW,self.control)
        if self.kind=='C' and address==0x43ad20:
            values=struct.unpack('<6dId',mu.mem_read(esp+4,60));self.extra.append('G:'+','.join(str(struct.unpack('<Q',struct.pack('<d',v))[0]) for v in values[:6])+':'+str(values[6])+':'+str(struct.unpack('<Q',struct.pack('<d',values[7]))[0]));purge=60;value=mu.reg_read(UC_X86_REG_ECX)
        elif self.kind=='C' and address==0x487d20:
            self.extra.append('Palette');purge=12;value=0
        elif self.kind=='X' and address==0x4e4354:
            pointer=self.read(esp+4);self.extra.append('Free:'+{IMAGE:'image',ROWS:'rows',PARTICLES:'particles',OBJECT:'object'}[pointer]);purge=0;value=0
        elif self.kind=='X' and address==0x4a97e0:
            self.extra.append('Stop:'+str(self.read(esp+4)));purge=0;value=0
        else:super().on_instruction(mu,address,size,data);return
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,self.read(esp))

    def call(self,entry,args):
        """원본 비휘발 레지스터/반환 스택과 x87 균형을 확인한다."""
        saved={UC_X86_REG_EBX:0x13579,UC_X86_REG_ESI:0x24680,UC_X86_REG_EDI:0x56789,UC_X86_REG_EBP:0x67890}
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_ECX,OBJECT);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start(entry,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4*(1+len(args))
        assert all(self.mu.reg_read(register)==value for register,value in saved.items()) and self.mu.reg_read(UC_X86_REG_FPCW)==self.control and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.return_count+=1

    def run_case(self,kind,values):
        """같은 입력을 다시 공급하고 관측한 출력만 직렬화한다."""
        self.kind=kind;self.extra=[];self.pixels=[];self.initial=False
        if kind=='C':
            self.cold,parent,mode=values;result=super().run(dict(parent=parent,mode=mode,registry=1,depth=-11,zeroDepth=-8,fill=90));self.return_count+=1
            assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control
            return '|'.join(result)+'|'+str(bool(self.read(self.p['palette'])))+'|'+';'.join(self.extra)
        self.mu.mem_write(OBJECT,bytes(144));self.mu.mem_write(PARTICLES,bytes(96));self.mu.mem_write(PIXELS,bytes([0xa5])*4096);self.mu.mem_write(0,bytes(4))
        self.write_ranges=[(0,4),(OBJECT,OBJECT+144),(TABLE,TABLE+320),(PIXELS,PIXELS+4096),(self.p['systems'],self.p['systems']+4),(self.p['count'],self.p['count']+4)]
        if kind=='D':
            capacity,total,x,y,life,source,cx,cy,clip,profile=values
            self.mu.mem_write(OBJECT+36,struct.pack('<III',capacity,total,PARTICLES));self.mu.mem_write(PARTICLES,struct.pack('<6I',x,y,0,0,life,OBJECT+0x74 if source else 0));self.mu.mem_write(0x5453d0,struct.pack('<I',PALETTE));self.mu.mem_write(PALETTE,struct.pack('<II',8,COLORS));self.mu.mem_write(COLORS,struct.pack('<9I',*range(31,40)));self.mu.mem_write(0x59a938,struct.pack('<II',cx,cy));self.mu.mem_write(0x5c78ec,struct.pack('<I',64));self.mu.mem_write(0x5c78f8,struct.pack('<I',PIXELS))
            self.mu.mem_write(0x5c7a44,struct.pack('<4i',*((0,0,64,64) if not clip else (10,10,30,30))))
            if profile:
                for index in range(1,4):self.mu.mem_write(PARTICLES+index*24,struct.pack('<6I',bits(20.75+index),bits(22.5-index),0,0,bits(-1 if index==3 else 4),OBJECT+0x74 if index==1 else 0))
            before=bytes(self.mu.mem_read(OBJECT,144))+bytes(self.mu.mem_read(PARTICLES,96));self.call(0x423ec0,(311,233));assert before==bytes(self.mu.mem_read(OBJECT,144))+bytes(self.mu.mem_read(PARTICLES,96));pixels=bytes(self.mu.mem_read(PIXELS,4096))
            return str(zlib.adler32(pixels))+':'+str(sum(pixels))+'|'+';'.join(self.pixels)
        flags,image,rows,particles,sound=values
        self.mu.mem_write(OBJECT,struct.pack('<I',0x503730));self.mu.mem_write(OBJECT+4,struct.pack('<II',83,50));self.mu.mem_write(OBJECT+28,struct.pack('<ii',1,-12));self.mu.mem_write(OBJECT+44,struct.pack('<I',PARTICLES if particles else 0));self.mu.mem_write(OBJECT+80,struct.pack('<I',55 if sound else 0));self.mu.mem_write(OBJECT+88,struct.pack('<I',IMAGE if image else 0));self.mu.mem_write(OBJECT+120,struct.pack('<I',ROWS if rows else 0));self.mu.mem_write(TABLE,struct.pack('<3I',111,OBJECT,222));self.mu.mem_write(self.p['systems'],struct.pack('<II',TABLE,3));self.call(0x4240f0,(flags,));assert self.mu.reg_read(UC_X86_REG_EAX)==OBJECT
        return ','.join(str(self.read(OBJECT+offset)) for offset in (4,8,28,32))+'|'+','.join(str(self.read(TABLE+index*4)) for index in range(3))+'|'+';'.join(self.extra)


def main():
    """두 정밀도 독립 관측 또는 기존 동결 해시를 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert len([row for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')])==report['rows'];print('Burning cold/Draw/소멸 동결 해시 일치');return
    inputs=cases();runs=[];stats=[]
    for control in (0x27f,0x37f):
        oracle=Oracle(control);runs.append([oracle.run_case(kind,values) for kind,values in inputs]);stats.append(dict(control=hex(control),returns=oracle.return_count,instructions=oracle.instructions,assertions=oracle.assertions))
    assert runs[0]==runs[1]
    FIXTURE.write_text('# 실제10.78 cold 생성/5픽셀/삭제. gradient/색표병합·할당/부착/소리는 선언 경계다.\n'+'\n'.join(kind+'\t'+','.join(map(str,values))+'\t'+result for (kind,values),result in zip(inputs,runs[0]))+'\n','utf-8',newline='\n')
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_burningprocess_oracle.py','tools/decomp_burning_lifetime_draw.py',FIXTURE.relative_to(ROOT).as_posix()]
    report=dict(unit=190,date='2026-10-18',host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(inputs),precisionDifferences=0,stats=stats,boundaries='실제 ctor/Draw/색조회/소멸/표시등록/base. gradient/병합·힙/Host부착/소리는 선언 경계이며 실제 game/캡처/Frame/장치/전투 완주 증거가 아니다.',sha256={path:digest(ROOT/path) for path in paths});REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print('Burning',len(inputs),'입력',stats)


if __name__=='__main__':main()
