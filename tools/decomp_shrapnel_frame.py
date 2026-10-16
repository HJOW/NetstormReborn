#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 픽셀 파편의 실제 묶음 생성·두 난수·시간/중력·Draw/픽셀을 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_particle_gradient import Oracle as CallOracle, ROOT, STACK, STOP, digest
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

OBJECT, PARTICLES, BITMAP, SCRATCH, POOL, TLS, CANVAS = (0x12000000 + index * 0x100000 for index in range(7))  # 입력/출력과 CRT TLS를 격리한다.
ENTRIES = {0x460000,0x45fc70,0x4600a0,0x460200,0x4602f0,0x4603e0,0x45fd60,0x45fef0,0x488100,0x487ab0,0x487b20,0x418400,0x418430,0x453040,0x487ba0,0x4e49c0,0x455930,0x4558c0,0x4e64ab,0x4a2230}  # 실제 실행할 생성/시간/난수/그리기 몸체다.
WIDTH, HEIGHT, PITCH = 64,48,68  # 네 바이트 행 padding까지 실제 원문 픽셀에 포함한다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/shrapnel-frame-x86.tsv'  # 의미76BYTE·입자24BYTE·픽셀 전체 원문이다.
REPORT = ROOT / 'cpppj/shrapnel-frame.json'  # 고유 입력/정밀도/외부 경계/해시 근거다.


def bits(value):
    """float 입력만 binary32로 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def semantic(raw):
    """포인터/미사용 바이트를 제외한76BYTE를 원문 그대로 보존한다."""
    return raw[0xc:0x1c]+raw[0x20:0x2c]+raw[0x30:0x4c]+raw[0x54:0x68]


class Oracle(CallOracle):
    """생성/Frame/Draw·CRT/GameRandom·실제 픽셀을 실행하고 할당/TLS/dirty/삭제만 경계로 둔다."""
    def __init__(self):
        """현재 정밀 함수표의 실제 구간과 격리 메모리를 준비한다."""
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image()
        self.mu.mem_map(0x400000,(len(image)+4095)&~4095);self.mu.mem_write(0x400000,image)
        # 원본 힙/장치 주소와 겹치지 않는 공간만 사용한다.
        for address,size in ((OBJECT,0x10000),(PARTICLES,0x10000),(BITMAP,0x10000),(SCRATCH,0x10000),(POOL,0x10000),(TLS,0x10000),(CANVAS,0x10000),(STACK-0x7f000,0x80000),(STOP,0x1000)):
            self.mu.mem_map(address,size)
        self.allowed=[];self.calls=Counter();self.returns=0;self.events=[];self.killed=False
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 함수 밖의 공백/새 코드는 허용하지 않는다.
        for entry in ENTRIES:
            # 함수의 불연속 범위도 그대로 유지한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def word(self,address):
        """현재 격리 메모리의 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def instruction(self,mu,address,size,data):
        """rand는 실제 몸체로 실행하고 TLS 주소만 반환한다. 픽셀도 대체하지 않는다."""
        if address in (0x4e4391,0x4e4354,0x4ea61d,0x499310,0x41bf10):
            esp=mu.reg_read(UC_X86_REG_ESP);ret=self.word(esp);value=0;purge=0
            if address==0x4e4391:
                amount=self.word(esp+4);value=SCRATCH if amount==self.image_size else PARTICLES
                if value==PARTICLES:assert amount%24==0 and 0<amount<=0x10000
                self.events.append(b'N'+struct.pack('<I',amount))
            elif address==0x4e4354:
                assert self.word(esp+4) in (SCRATCH,PARTICLES);self.events.append(b'F'+struct.pack('<I',1 if self.word(esp+4)==SCRATCH else 2))
            elif address==0x4ea61d:value=TLS
            elif address==0x499310:self.events.append(b'D'+bytes(mu.mem_read(esp+4,20)))
            else:
                assert self.word(esp+4)==0;self.events.append(b'K'+bytes(4));self.killed=True;purge=4
            self.calls[f'boundary-{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'허용하지 않은 파편 실행 {address:08x}')
        if address==0x4a2230:
            esp=mu.reg_read(UC_X86_REG_ESP);self.events.append(b'P'+bytes(mu.mem_read(esp+4,12)))
        if address in ENTRIES:self.calls[f'{address:08x}']+=1

    def write(self,mu,access,address,size,value,data):
        """현재 객체/입자/작업 복사본/CRT 상태/게임 난수/픽셀/스택만 쓸 수 있다."""
        regions=((OBJECT,OBJECT+0x68),(PARTICLES,PARTICLES+0x10000),(SCRATCH,SCRATCH+0x10000),(TLS+20,TLS+24),(0x532710,0x532714),(CANVAS,CANVAS+PITCH*HEIGHT),(STACK-0x7f000,STACK+0x1000))
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'허용하지 않은 파편 쓰기 {address:08x}+{size}')

    def particles(self):
        """용량 전체의 원본 색 포인터만 bitmap offset+1로 정규화한다."""
        capacity=self.word(OBJECT+0x24);raw=bytearray(self.mu.mem_read(PARTICLES,capacity*24))
        # 미사용0 토큰도 전체 비교 범위에 포함한다.
        for index in range(capacity):
            offset=index*24+20;pointer=struct.unpack_from('<I',raw,offset)[0]
            if pointer:
                assert BITMAP<=pointer<BITMAP+self.image_size;struct.pack_into('<I',raw,offset,pointer-BITMAP+1)
        return bytes(raw)

    def observe(self):
        """의미 필드/용량 전체/두 난수의 실제 상태를 읽는다."""
        return (semantic(bytes(self.mu.mem_read(OBJECT,0x68))).hex(),self.particles().hex() or '-',str(self.word(0x532710)),str(self.word(TLS+20)))

    def setup(self,variant,width,height,pattern,game_seed,crt_seed):
        """명시 캡처 이미지와 부모/타이머를 준비한다. 생성 결과는 계산하지 않는다."""
        self.variant=variant;mu=self.mu;self.image_size=width*height
        image=bytes(255 if pattern==0 or pattern==1 and (index+index//width)%3==0 else (index*13+7)%255 for index in range(self.image_size))
        mu.mem_write(BITMAP,image);mu.mem_write(PARTICLES,b'\x5a'*0x10000);raw=bytearray(b'\xa5'*0x68)
        struct.pack_into('<3I4f2i',raw,0,(0x50a460,0x50a4b8,0x50a4f0)[variant],1234,50,12.25,13.75,30.5,35.5,0,-25)
        struct.pack_into('<3I2d2f2I',raw,0x24,0,0,0,1,.123456789,123.25,-456.5,0,0)
        struct.pack_into('<I4if',raw,0x50,BITMAP,width,height,width//2,height-1,(6,.24,1.2)[variant]);mu.mem_write(OBJECT,bytes(raw))
        parent=bytearray(50);parent[10]=110;parent[35]=255;struct.pack_into('<2f',parent,14,-1.75 if width==18 else 2.125,1.25 if width==18 else 3.25);mu.mem_write(POOL+2500,bytes(parent))
        mu.mem_write(0x5c8464,struct.pack('<I',POOL));mu.mem_write(0x5c847c,struct.pack('<I',24000));mu.mem_write(0x532710,struct.pack('<I',game_seed));mu.mem_write(TLS+20,struct.pack('<I',crt_seed))
        return image

    def run(self,kind,now=1,scale=(16,11),rate=.3,camera=(.25,-.5),precision=0x027f):
        """Prepare 또는 실제 Frame/Draw/픽셀을 실행해 입력과 전체 후상태를 보존한다."""
        mu=self.mu;state=struct.pack('<d5f',now,*scale,rate,*camera);mu.mem_write(0x55b4d0,state[:8]);mu.mem_write(0x59a924,state[8:16]);mu.mem_write(0x59a934,state[16:])
        mu.mem_write(CANVAS,b'\xc7'*(PITCH*HEIGHT));mu.mem_write(0x5c78ec,struct.pack('<I',PITCH));mu.mem_write(0x5c78f8,struct.pack('<I',CANVAS));mu.mem_write(0x5c7a44,struct.pack('<4i',1,2,WIDTH-1,HEIGHT-2))
        fields,particles,game,crt=self.observe();parent=bytes(mu.mem_read(POOL+2500,50));image=bytes(mu.mem_read(BITMAP,self.image_size));self.events=[];self.killed=False
        self.call(0x460000 if kind==0 else 0x45fd60,OBJECT,b'',0,precision);self.returns+=1
        if kind and not self.killed:self.call(0x45fef0,OBJECT,bytes(8),8,precision);self.returns+=1
        assert bytes(mu.mem_read(BITMAP,self.image_size))==image and bytes(mu.mem_read(POOL+2500,50))==parent
        return (str(kind),str(self.variant),fields,image.hex(),particles,parent.hex(),state.hex(),game,crt,*self.observe(),b''.join(self.events).hex() or '-',bytes(mu.mem_read(CANVAS,PITCH*HEIGHT)).hex())


def generate(oracle,precision):
    """세 실제 가상 묶음 생성과 완료까지의 연속 시간/Draw를 관찰한다."""
    rows=[]
    # 가장자리 제외·투명/불투명·두 난수와18칸의 넓은 속도 저장을 교차한다.
    for variant,(width,height),pattern,game,crt in itertools.product(range(3),((1,1),(3,4),(7,5),(18,11)),range(3),(0,0x12345678),(0,1,0xdeadbeef)):
        oracle.setup(variant,width,height,pattern,game,crt);rows.append(oracle.run(0,precision=precision))
        # 한 조각의 생성 후 상태를 그대로 사용해 수명/퇴행/완료를 방문한다.
        for now in (1,1.025,.99,1.3,2,6,6):
            rows.append(oracle.run(1,now,precision=precision))
            if oracle.killed:break
    return rows


def main():
    """원문 기대값을 저장하거나 재계산 없이 SHA/고유 입력을 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 전체 근거 파일 바이트만 대조한다.
        for path,wanted in facts['sha256'].items():assert digest(ROOT/path)==wanted,path
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({tuple(row[:9]) for row in rows})==len(rows)
        print('파편 SHA/고유 입력 감사:',len(rows));return
    oracles=[Oracle(),Oracle()]
    if args.smoke:
        # 서로 다른 가상 생성 모두에서 실제 CRT/TLS와 픽셀 경로를 확인한다.
        for variant in range(3):
            oracles[0].setup(variant,7,5,2,0,1);oracles[0].run(0);oracles[0].run(1,1.025)
        print('파편 생성/Frame/Draw smoke 정상 반환');return
    outputs=[generate(oracle,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];different=[index for index,pair in enumerate(zip(*outputs)) if pair[0]!=pair[1]];rows=list(dict.fromkeys(outputs[0]));assert len({row[:9] for row in rows})==len(rows)
    FIXTURE.write_text('# kind variant fields76 image particles parent state gameSeed crtSeed fieldsAfter particlesAfter gameAfter crtAfter events pixels68x48\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_shrapnel_frame.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',uniqueInputs=len(rows),observationsPerPrecision=len(outputs[0]),normalReturns=sum(oracle.returns for oracle in oracles),precisionDifferentInputs=different,inputsByKind=dict(Counter(row[0] for row in rows)),nativeCalls=dict(oracles[0].calls),sha256={path:digest(ROOT/path) for path in paths},limitations=['실제 세 묶음 생성/Append/CRT rand/GameRandom/시간/Frame/Draw/픽셀을 실행했다.','new/free·TLS 주소·dirty 큐 소비·Form 삭제만 명시 외부 경계다. 캡처 이미지/부모는 입력이며 실제 생성/Host/캠페인 연결과 구별한다.','입자의 색 주소만 캡처 bitmap offset+1로 정규화한다. 기본53비트 원문과 비교64비트 차이를 보존한다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('파편:',len(rows),'고유 입력, 정밀도 차이',len(different))


if __name__=='__main__':main()
