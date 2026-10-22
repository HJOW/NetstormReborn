#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dissolve의 실제 픽셀 준비/초기화/시계/프레임/Draw를 보호 PE에서 격리 실행한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_shrapnel_frame import Oracle as BaseOracle, OBJECT, PARTICLES, BITMAP, SCRATCH, POOL, TLS, CANVAS, WIDTH, HEIGHT, PITCH, ENTRIES as BASE_ENTRIES
from decomp_particle_gradient import ROOT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

ENTRIES={0x4500a0,0x450460,0x44fde0,0x44fe40,0x44ffc0,0x4558f0,0x4239f0}  # 나머지 난수/타이머/픽셀/입자 버퍼는 기존 실제 helper다.
FIXTURE=ROOT/'cpppj/tests/fixtures/dissolve-frame-x86.tsv'  # 기존 파편 원문과 분리한84바이트 의미 필드다.
REPORT=ROOT/'cpppj/dissolve-frame-20261022.json'  # 현재 호스트/원문/정밀도/실행 범위 근거다.


def semantic(raw):
    """포인터와 미사용64/68을 제외한 의미84바이트를 그대로 남긴다."""
    return raw[0xc:0x1c]+raw[0x20:0x2c]+raw[0x30:0x4c]+raw[0x54:0x64]+raw[0x6c:0x78]


class Oracle(BaseOracle):
    """실제500a0/50460/Frame/Draw와 두 게임 난수 helper·픽셀을 실행한다."""
    def __init__(self):
        """상위의 격리 메모리에 정확한 새 함수 구간/읽기 가드만 추가한다."""
        super().__init__()
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in ENTRIES:  # 원문 함수 밖 실행을 허용하지 않는다.
            for part in facts[entry]['ranges'].split(';'):  # 불연속 범위도 보존한다.
                lo,hi=(int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)

    def instruction(self,mu,address,size,data):
        """캡처만 명시 입력으로 공급하며 원본 입자/시간 산술은 대체하지 않는다."""
        if address==0x4e4391 and self.word(mu.reg_read(UC_X86_REG_ESP)+4)==0:
            esp=mu.reg_read(UC_X86_REG_ESP);self.events.append(b'N'+bytes(4));target=self.word(esp)
            mu.reg_write(UC_X86_REG_EAX,PARTICLES);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,target);return
        if address==0x4a7db0:
            esp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<5I',mu.mem_read(esp+4,20));assert args[0]==50
            for pointer,value in zip(args[1:],self.capture):mu.mem_write(pointer,struct.pack('<i',value))  # 실제 캡처의 치수/기준점 경계다.
            target=self.word(esp);mu.reg_write(UC_X86_REG_EAX,BITMAP);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,target);return
        if address==0x4e0620:raise RuntimeError(f'Dissolve 원본 진단 {self.word(mu.reg_read(UC_X86_REG_ESP)+12):x}')
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        super().instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """120바이트 객체/입자/난수/픽셀과 실제 스택 밖 쓰기를 차단한다."""
        regions=((OBJECT,OBJECT+0x78),(PARTICLES,PARTICLES+0x10000),(SCRATCH,SCRATCH+0x10000),(TLS+20,TLS+24),(0x532710,0x532714),(CANVAS,CANVAS+PITCH*HEIGHT),(STACK-0x7f000,STACK+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in regions):raise RuntimeError(f'Dissolve 범위 밖 쓰기 {address:x}+{size}')

    def read(self,mu,access,address,size,value,data):
        """보호 이미지/명시 bitmap·부모·입자·장치·스택만 읽게 한다."""
        regions=((0x400000,0x600000),(OBJECT,OBJECT+0x78),(PARTICLES,PARTICLES+0x10000),(BITMAP,BITMAP+0x10000),(SCRATCH,SCRATCH+0x10000),(POOL,POOL+0x10000),(TLS,TLS+0x10000),(CANVAS,CANVAS+0x10000),(STACK-0x7f000,STACK+0x1000))
        if not any(lo<=address and address+size<=hi for lo,hi in regions):raise RuntimeError(f'Dissolve 범위 밖 읽기 {address:x}+{size}')

    def setup(self,width,height,pattern,seed,orientation):
        """캡처/더미/폴과 초기 의미 필드는 독립 명시 입력으로 공급한다."""
        image=super().setup(0,width,height,pattern,seed,1);self.orientation=orientation;self.capture=(width,height,width//2,height-1)
        self.mu.mem_write(OBJECT+0x68,bytes(16));self.mu.mem_write(OBJECT,struct.pack('<I',0x508c08))
        for sid,x,y in ((61,0.,0.),(62,9.5,13.25)):  # 두 원본 폴은 별도 부모 슬롯이다.
            raw=bytearray(50);struct.pack_into('<2f',raw,14,x,y);self.mu.mem_write(POOL+sid*50,bytes(raw))
        return image

    def observe(self):
        """전체 의미/입자와 같은 게임 난수 상태를 읽는다."""
        return (semantic(bytes(self.mu.mem_read(OBJECT,0x78))).hex(),self.particles().hex() or '-',str(self.word(0x532710)))

    def run(self,kind,now=1,precision=0x27f):
        """준비/캡처 뒤 초기화 또는 실제 Frame/Draw를 정상 복귀까지 관찰한다."""
        mu=self.mu;state=struct.pack('<d5f',now,16,11,.3,.25,-.5);mu.mem_write(0x55b4d0,state[:8]);mu.mem_write(0x59a924,state[8:16]);mu.mem_write(0x59a934,state[16:])
        mu.mem_write(CANVAS,b'\xc7'*(PITCH*HEIGHT));mu.mem_write(0x5c78ec,struct.pack('<I',PITCH));mu.mem_write(0x5c78f8,struct.pack('<I',CANVAS));mu.mem_write(0x5c7a44,struct.pack('<4i',1,2,WIDTH-1,HEIGHT-2))
        fields,particles,game=self.observe();parents=bytes(mu.mem_read(POOL+2500,50))+bytes(mu.mem_read(POOL+61*50,100));image=bytes(mu.mem_read(BITMAP,self.image_size));self.events=[];self.killed=False
        entry=0x4500a0 if kind==0 else 0x450460 if kind==1 else 0x44fe40
        args=struct.pack('<5i',29,31,61,62,self.orientation) if kind==0 else struct.pack('<4i',50,61,62,self.orientation) if kind==1 else b''
        self.call(entry,OBJECT,args,len(args),precision);self.returns+=1
        if kind==2 and not self.killed:self.call(0x44ffc0,OBJECT,bytes(8),8,precision);self.returns+=1
        assert bytes(mu.mem_read(BITMAP,self.image_size))==image
        return (str(kind),str(self.orientation),fields,image.hex(),particles,parents.hex(),state.hex(),game,*self.observe(),b''.join(self.events).hex() or '-',bytes(mu.mem_read(CANVAS,PITCH*HEIGHT)).hex())


def generate(oracle,precision):
    """가로/세로·캡처 준비와 최초 delta0·이동/퇴행/소진을 관찰한다."""
    rows=[]
    for (width,height),pattern,orientation,seed in itertools.product(((1,1),(3,4),(7,5),(18,11)),range(3),(0,1),(0,0x12345678)):
        oracle.setup(width,height,pattern,seed,orientation);rows.append(oracle.run(0,precision=precision))  # 원점 인자를 받는 독립 준비다.
        oracle.setup(width,height,pattern,seed,orientation);rows.append(oracle.run(1,precision=precision))  # 실제 부모/캡처 기준점을 적용한다.
        for now in (1.1,1.125,1.09,1.4,3,10,10):  # 최초 Frame의 시간 경과는0으로 처리한다.
            rows.append(oracle.run(2,now,precision))
            if oracle.killed:break
    return list(dict.fromkeys(rows))


def main():
    """원문53/64비트 관측을 분리해 저장하거나 SHA만 재확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 고정된 보호 원문/제품/검사의 출처다.
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows)==facts['uniqueInputs'] and len({tuple(row[:8]) for row in rows})==len(rows)
        print('Dissolve frame SHA/고유 입력:',len(rows));return
    oracles=[Oracle(),Oracle()];outputs=[generate(oracle,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
    first,second=outputs;assert len(first)==len(second)
    # 동일 입력의 다른 정밀도 결과는 합치지 않고 별도 원문 파일에 남긴다.
    FIXTURE.write_text('# kind orientation fields84 image particles parents150 state game fieldsAfter84 particlesAfter gameAfter events pixels68x48\n'+'\n'.join('\t'.join(row) for row in first)+'\n',encoding='utf-8',newline='\n')
    other=FIXTURE.with_name('dissolve-frame-x86-p64.tsv');other.write_text('# kind orientation fields84 image particles parents150 state game fieldsAfter84 particlesAfter gameAfter events pixels68x48\n'+'\n'.join('\t'.join(row) for row in second)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_dissolve_frame.py','tools/decomp_shrapnel_frame.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix(),other.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(first),normalReturns=sum(oracle.returns for oracle in oracles),precisionDifferentRows=[index for index,pair in enumerate(zip(first,second)) if pair[0]!=pair[1]],nativeCalls=dict(sum((oracle.calls for oracle in oracles),Counter())),sha256={path:digest(ROOT/path) for path in paths},limitations=['4500a0/450460/44fde0/44fe40/44ffc0와 입자 버퍼·두 게임 난수·시계·dirty 계산·실제 픽셀 출력은 원문이다.','캡처 이미지·큰 메모리 확보/반납·장치 dirty 소비·Form 삭제는 명시 경계다. 색 주소만 bitmap offset+1로 정규화한다.','사용하지 않는64/68은 의미 비교에서 제외한다. 53/64비트의 실제 저장값 차이는 그대로 보존한다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Dissolve:',len(first),'입력/정밀도 차이',len(facts['precisionDifferentRows']))


if __name__=='__main__':main()
