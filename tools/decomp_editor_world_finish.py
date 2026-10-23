#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Editor 마지막0046e980과실제iterator/좌표CRT를격리실행해최초관측을고정한다."""
import argparse
from collections import Counter
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import zlib

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ROOT = Path(__file__).resolve().parents[1]  # 보호원문과신규자료의저장소루트다.
PE = ROOT/'originals/Netstorm.exe'  # OS진입없이읽기만하는10.78실행파일이다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 보호PE의현재전체SHA다.
FIXTURE = ROOT/'cpppj/tests/fixtures/editor-world-finish-v2-x86.tsv'  # 삭제경로미도달입력을교정한두번째독립관측이며최초자료를덮어쓰지않는다.
REPORT = ROOT/'cpppj/editor-world-finish-v2-native.json'  # 원문/명시경계/중단범위를구분한다.
POOL = (0x12001000,0x12001000+16000)  # 콜백뒤기준주소변경과캡처form유지를검사하는두Pool이다.
MAP = (0x12010000,0x12040000)  # 현재surface WORD표의두물리저장소다.
SPOT = (0x12070000,0x12090000)  # 현재spot BYTE표의두물리저장소다.
STACK,STOP,VTABLE,VIRTUAL = 0x15008000,0x14000F00,0x14000000,0x14000100  # 별도스택/정상RET/명시가상효과주소다.
POOL_BYTES,MAP_BYTES,SPOT_BYTES = 32*50,256*256*2,256*256  # 전체관측할물리입력범위다.
RANGES = ((0x46E980,0x46EA77),(0x46EA80,0x46EBC2),(0x40EB40,0x40EB72),(0x41D770,0x41D79B),(0x4E49C0,0x4E4A35))  # 실제부모/iterator/정수화/ftol RET까지다.
CONTROLS = (0x27F,0x37F)  # 동일입력을두x87정밀도에서독립반복한다.
REGISTERS = (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)  # 매실행CPU잔여를제거한다.


def digest(path):
    """파일전체바이트SHA를구한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """좌표입력을정확한float32 DWORD로보관한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def signed(value):
    """원본DWORD전체비트를signed로읽는다."""
    return struct.unpack('<i',struct.pack('<I',value&0xFFFFFFFF))[0]


def cases():
    """기대결과를계산하지않고타입/skip/좌표/후보/동적전역의명시입력만만든다."""
    coordinates = ((1,1),(0,1),(0.5,0.5),(15.75,31.5),(255.75,255.75),(256,10),(-0.5,10),(-1,10),
        (10,0),(10,-0.5),(10,-1),(10,256),(float('inf'),1),(float('-inf'),1),(float('nan'),1),(3,4))  # 경계/소수/비정상좌표의원문부분효과다.
    result = [(profile,bits(x),bits(y),neighbors,'none') for profile,(x,y),neighbors in itertools.product(range(6),coordinates,range(3))]
    mutations = ('begin-pool1','begin-map1','begin-coordinate','next1-pool1','next1-island','next1-board0',
        'next1-coordinate','unpop-pool1','unpop-map1','unpop-spots1','unpop-cap3','unpop-cap20','unpop-next-free',
        'release-map1','release-pool1','release-cap3','begin-stop','next1-stop','unpop-stop','release-stop','graph-stop')  # 본문재조회와캐시위치를구별한다.
    result.extend((3,bits(10.75),bits(20.5),0 if mutation.startswith(('unpop-','release-')) else 2,mutation) for mutation in mutations)
    return list(dict.fromkeys(result))


class Oracle:
    """부모/Pooliterator/좌표CRT는실제명령이며Finder/가상/Release/Graph만명시경계다."""
    def __init__(self):
        """PE를읽기전용으로전개하고게임/OS가없는별도주소공간을만든다."""
        if digest(PE)!=PE_SHA:
            raise RuntimeError('Editor후처리보호PE SHA불일치')
        pe=pefile.PE(str(PE));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)  # 보호파서와격리CPU다.
        self.mu.mem_map(0x400000,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
        self.mu.mem_write(0x400000,pe.get_memory_mapped_image())
        self.mu.mem_map(0x12000000,0x100000);self.mu.mem_map(0x14000000,0x1000);self.mu.mem_map(STACK-0x8000,0x10000)
        self.put(VTABLE+0x48,VIRTUAL)
        self.original={start:bytes(self.mu.mem_read(start,end-start)) for start,end in RANGES}  # 전체실행바이트불변대조다.
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)
        self.total=0;self.normal=0;self.stops=Counter();self.boundaries=Counter()  # 최초관측의누계다.

    def put(self,address,value,width=4):
        """명시입력/명시하위효과만정해진폭으로쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def words(self,address,count=1):
        """현재메모리의DWORD를little endian으로읽는다."""
        return struct.unpack('<'+'I'*count,self.mu.mem_read(address,count*4))

    def ret(self,purge=0):
        """선택한외부경계의cdecl/thiscall정리규약만적용한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);target=self.words(esp)[0]  # 원본CALL이놓은복귀주소다.
        self.mu.reg_write(UC_X86_REG_EAX,0);self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def stop(self,cause):
        """명시부분효과를지우지않고실제도달경계에서중단한다."""
        self.cause=cause;self.mu.emu_stop()

    def mutate(self,boundary):
        """원문실행중변경할입력전역/캡처레코드를경계하나에서만쓴다."""
        mutation=self.case[-1]  # 이번명시콜백효과의선택이다.
        if mutation==boundary+'-stop':
            self.stop(boundary);return
        if mutation==boundary+'-pool1':self.put(0x5C8464,POOL[1])
        if mutation==boundary+'-map1':self.put(0x5C84BC,MAP[1])
        if mutation==boundary+'-spots1':self.put(0x5C7C44,SPOT[1])
        if mutation==boundary+'-island':self.put(0x5411D0,42)
        if mutation==boundary+'-board0':self.put(0x531928,0)
        if mutation==boundary+'-coordinate':self.put(self.form+14,bits(0));self.put(self.form+18,bits(2.75))
        if mutation==boundary+'-cap3':self.put(0x5C847C,3)
        if mutation==boundary+'-cap20':self.put(0x5C847C,20)
        if mutation==boundary+'-next-free':self.put(POOL[0]+3*50+11,0,1)

    def write(self,mu,access,address,size,value,data):
        """CPU직접쓰기는지역스택과현재두surface/spot표만허용한다."""
        if STACK-0x400<=address and address+size<=STACK+4:return
        for label,addresses,length in (('S',MAP,MAP_BYTES),('P',SPOT,SPOT_BYTES)):  # 원문직접쓰기전체와정확한물리범위를관측한다.
            for index,base in enumerate(addresses):  # 현재표교체뒤원본이쓴실제대상도구별한다.
                if base<=address and address+size<=base+length:
                    unit=2 if label=='S' else 1  # surface는WORD,spot은BYTE다.
                    if size!=unit or (address-base)%unit:
                        raise RuntimeError('Editor후처리직접쓰기폭/정렬불일치')
                    self.events.append(f'{label}:{index}:{(address-base)//unit}:{value&((1<<(size*8))-1)}');return
        self.stop('surface-range' if any(base-0x2000<=address<base+MAP_BYTES+0x2000 for base in MAP) else 'write-range')

    def instruction(self,mu,address,size,data):
        """하위호출의인자/순서를기록하며상위계산은원본CPU가수행한다."""
        self.instructions+=1;self.total+=1
        if self.instructions>30000:raise RuntimeError('Editor후처리원문명령상한')
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)  # 현재원본호출인자다.
        if address==0x4B16D0:
            args=self.words(esp+4,4);self.finder=ecx;self.form=mu.reg_read(UC_X86_REG_ESI);self.next_calls=0
            self.events.append('B:'+':'.join(str(signed(value)) for value in args));self.boundaries['begin']+=1
            self.neighbors=list(((),(5,),(6,5,7))[self.case[3]])  # 명시Finder결과입력이며원문Finder본체실행을가장하지않는다.
            self.mutate('begin')
            if not self.cause:self.put(self.finder+0x34,self.neighbors[0] if self.neighbors else 0);self.ret(16)
        elif address==0x4B1810:
            current=self.words(ecx+0x34)[0];self.events.append('N:'+str(current));self.boundaries['next']+=1;self.next_calls+=1
            self.mutate('next'+str(self.next_calls))
            if not self.cause:
                if self.neighbors:self.neighbors.pop(0)
                self.put(ecx+0x34,self.neighbors[0] if self.neighbors else 0);self.ret()
        elif address==VIRTUAL:
            self.form=ecx;self.events.append('U:'+str(signed(ecx-POOL[0]))+':'+str(self.words(esp+4)[0]));self.boundaries['unpop']+=1
            self.mutate('unpop')
            if not self.cause:self.ret(4)
        elif address==0x4ABF60:
            self.events.append('R:'+str(signed(self.words(esp+4)[0])));self.boundaries['release']+=1;self.mutate('release')
            if not self.cause:self.ret()
        elif address==0x463110:
            self.events.append('G:'+str(self.words(esp+4)[0]));self.boundaries['graph']+=1;self.mutate('graph')
            if not self.cause:self.ret()
        elif not any(start<=address<end for start,end in RANGES):
            raise RuntimeError('Editor후처리검토실행범위밖: '+hex(address))

    def sample(self,case,control):
        """같은명시raw입력을매번다시준비하고전체저장소/현재전역/trace를관측한다."""
        self.case=case;self.events=[];self.cause='';self.instructions=0;self.form=0;self.finder=0;self.next_calls=0
        profile,x,y,neighbors,mutation=case  # 입력profile·정확한float비트·Finder결과·한하위효과다.
        for address,value in ((0x5C8464,POOL[0]),(0x5C847C,2 if profile==5 else 16),(0x5C84BC,MAP[0]),(0x5C7C44,SPOT[0]),
                (0x5412CC,413 if profile==4 else 157),(0x5411D0,94),(0x531928,256)):self.put(address,value)  # 원문현재전역의명시입력이다.
        for pool_index,base in enumerate(POOL):  # 두실제배열의비변경바이트도전수관측한다.
            raw=bytearray((offset*13+0xA5+pool_index*7)&255 for offset in range(POOL_BYTES))
            for sid in range(32):  # 예약/빈/free/선택타입을명시하며순회결과는계산하지않는다.
                struct.pack_into('<I',raw,sid*50,VTABLE);raw[sid*50+10]=85;raw[sid*50+11]=0 if sid<16 else 1
                struct.pack_into('<II',raw,sid*50+14,x,y)
            targets=() if profile==0 else (1,2,3,4) if profile==2 else (2,3,4,16) if profile==3 else (2,)
            for sid in targets:raw[sid*50+10]=157  # 여러후보/예약SID1을구별하는입력타입이다.
            if profile in (2,3):raw[3*50+11]=1  # 현재후보선택전에다음free를건너뛰는순서를구별한다.
            raw[5*50+10]=94 if pool_index==0 else 42;raw[6*50+10]=42;raw[7*50+10]=94
            self.mu.mem_write(base,bytes(raw))
        for index,base in enumerate(MAP):self.mu.mem_write(base,bytes([0x1D+index,0xA5])*65536)  # 두surface표전체의명시초기패턴이다.
        for index,base in enumerate(SPOT):self.mu.mem_write(base,bytes([0xB3+index])*SPOT_BYTES)  # 두spot표전체의명시초기패턴이다.
        for index,register in enumerate(REGISTERS):self.mu.reg_write(register,0x31415900+index)  # callee-save와CPU잔여초기화다.
        saved={register:self.mu.reg_read(register) for register in (UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)}
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xFFFF)
        self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.emu_start(0x46E980,STOP)
        if self.cause:self.stops[self.cause]+=1
        else:
            if self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or any(self.mu.reg_read(register)!=value for register,value in saved.items()):
                raise RuntimeError('Editor후처리정상RET/스택/callee-save불일치')
            if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:
                raise RuntimeError('Editor후처리x87제어어/TOP불일치')
            self.normal+=1
        if any(bytes(self.mu.mem_read(start,len(expected)))!=expected for start,expected in self.original.items()):
            raise RuntimeError('Editor후처리원문실행바이트변경')
        current=tuple((POOL if address==0x5C8464 else MAP if address==0x5C84BC else SPOT).index(self.words(address)[0]) for address in (0x5C8464,0x5C84BC,0x5C7C44))
        globals_now=(*current,self.words(0x5C847C)[0],self.words(0x5412CC)[0],self.words(0x5411D0)[0],self.words(0x531928)[0])
        pools=':'.join(bytes(self.mu.mem_read(base,POOL_BYTES)).hex() for base in POOL)
        checksums=tuple(zlib.adler32(bytes(self.mu.mem_read(base,length)))&0xFFFFFFFF for addresses,length in ((MAP,MAP_BYTES),(SPOT,SPOT_BYTES)) for base in addresses)
        return '\t'.join((str(control),','.join(map(str,case)),self.cause or '1',','.join(map(str,globals_now)),pools,','.join(map(str,checksums)),';'.join(self.events)))


def main():
    """최초fixture는제품작성전에한번만쓰며이후에는SHA확인또는전수읽기대조만허용한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--dry-run',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))  # 처음고정한자료의전체SHA다.
        for name,expected in report['sha256'].items():  # 기존관측을제품결과에맞춰덮어쓰지않는다.
            if digest(ROOT/name)!=expected:raise RuntimeError('Editor후처리최초근거SHA불일치: '+name)
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    if not args.dry_run and (FIXTURE.exists() or REPORT.exists()):raise RuntimeError('Editor후처리최초fixture/보고서덮어쓰기금지')
    oracle=Oracle();inputs=cases();lines=['# control / profile,xBits,yBits,neighbors,mutation / completed-or-stop / current7 / twoPoolRaw / surface2+spot2Adler32 / nativeEffectTrace']
    for case in inputs:  # 동일명시입력을두제어어로매번새로실행한다.
        first,second=(oracle.sample(case,control) for control in CONTROLS)
        if first.split('\t')[1:]!=second.split('\t')[1:]:raise RuntimeError('Editor후처리두정밀도관측불일치')
        lines.extend((first,second))
    if args.dry_run:
        if '\n'.join(lines)+'\n'!=FIXTURE.read_text(encoding='utf-8'):raise RuntimeError('Editor후처리전수재관측불일치')
        print(json.dumps({'matched':True,'observations':len(inputs)*2,'instructions':oracle.total}));return
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','tools/decomp_editor_world_finish.py','cpppj/tests/fixtures/editor-world-finish-v2-x86.tsv')  # 제품이아닌原문/도구/별도관측만고정한다.
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entry':'0046e980','nativeRanges':[[f'{start:08x}',f'{end-1:08x}'] for start,end in RANGES],
        'uniqueInputs':len(inputs),'observations':len(inputs)*2,'normalReturns':oracle.normal,'explicitStops':dict(oracle.stops),'instructions':oracle.total,'boundaries':dict(oracle.boundaries),
        'scope':'부모전체·실제0040eb40 iterator·0041d770 정수화·004e49c0 ftol. Finder Begin/Next,가상+48,Release004abf60,Graph00463110은명시경계이며하위실제효과성공을주장하지않음. 직접surface/spot쓰기·현재Pool/표/좌표/타입/board와순서·전체두Pool/네표해시·스택/callee-save/x87/실행바이트불변관측.',
        'gameExecuted':False,'osExecuted':False,'productRead':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({name:report[name] for name in ('uniqueInputs','observations','normalReturns','explicitStops','instructions')}))


if __name__=='__main__':
    main()
