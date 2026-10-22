#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 분해의 실제 wrapper/생성/등록/준비/반납과 재진입 순서를 관찰한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_dissolve_frame import Oracle as FrameOracle,OBJECT,PARTICLES,BITMAP,SCRATCH,POOL,TLS
from decomp_particle_gradient import ROOT,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

TABLE,NEXT,VTABLE,POP=0x12700000,0x12800000,0x12900000,STOP+0x100  # 공통 표시 목록과 명시 dummy Pop 경계다.
DELETE=STOP+0x200  # 실제 dummy 가상+34가 호출하는 현재 가상+10 경계다.
ENTRIES={0x450510,0x450080,0x44fd80,0x4881d0,0x487fd0,0x423aa0,0x40f1b0,0x487880,0x41bd00,0x4437f0}  # 실제 분해/공통 생성과 scalar 삭제 몸체다.
FIXTURE=ROOT/'cpppj/tests/fixtures/dissolve-lifetime-x86.tsv'  # 필드/입자/두 난수/요청/슬롯 전체 원문이다.
REPORT=ROOT/'cpppj/dissolve-lifetime-20261022.json'  # 현재 원본과 명시 외부 경계를 보존한다.


def semantic(raw):
    """포인터 저장소를 제외한96바이트와 캡처 포인터의 비영 의미4바이트다."""
    pointer=struct.unpack_from('<I',raw,0x50)[0];assert pointer in (0,BITMAP)
    return raw[4:0x2c]+raw[0x30:0x4c]+struct.pack('<I',1 if pointer else 0)+raw[0x54:0x64]+raw[0x6c:0x78]


class Oracle(FrameOracle):
    """실제 wrapper/등록/준비/난수/소멸을 실행하고 세계/캡처/heap/Host는 명시 경계다."""
    def __init__(self):
        """기존 분해 실행기에 목록과 SEH/가상 Pop 입력만 추가한다."""
        super().__init__();self.mu.mem_map(0,0x1000)
        # 전체 정밀 표에서 빠진 실제 VT50eb98+34의10바이트 ret8 몸체를 PE와 직접 확인한다.
        assert bytes(self.mu.mem_read(0x48f0a0,10))==bytes.fromhex('8b016a00ff5010c20800');self.allowed.append((0x48f0a0,0x48f0aa))
        # 원본 주소나 기존 격리 버퍼와 겹치지 않는 목록/가상 표다.
        for address in (TABLE,NEXT,VTABLE):self.mu.mem_map(address,0x1000)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 생성/소멸의 불연속 구간을 그대로 추가한다.
        for entry in ENTRIES:
            # 함수 사이의 공백 실행은 허용하지 않는다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.phase='create';self.capacity=0;self.mutation=0;self.events=[]

    def ret(self,value=0,purge=0):
        """명시 경계의 실제 cdecl/thiscall 반환만 수행한다."""
        mu=self.mu;esp=mu.reg_read(UC_X86_REG_ESP);ret=self.word(esp);mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,ret)

    def instruction(self,mu,address,size,data):
        """요청 순서를 기록하고 외부 callback의 현재 raw/부모 변경을 입력으로 공급한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==DELETE:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;flags=self.word(esp+4);self.events.append(b'X'+struct.pack('<2I',sid,flags));assert sid==60 and flags==0;self.ret(0,4);return
        if address==0x4e4391:
            amount=self.word(esp+4);choices={0:PARTICLES,120:OBJECT,(self.capacity+40)*4:NEXT,288:PARTICLES,12:SCRATCH};assert amount in choices,amount
            self.events.append(b'N'+struct.pack('<I',amount));self.ret(choices[amount]);return
        if address==0x4e4354:
            pointer=self.word(esp+4);labels={TABLE:1,NEXT:1,BITMAP:2,PARTICLES:3,OBJECT:4,SCRATCH:5};assert pointer in labels,hex(pointer)
            label=labels[pointer]
            if label in (2,3,4):
                slot=self.word(OBJECT+0x1c);entry=self.word(self.word(0x595138)+slot*4);assert entry==(0 if label==4 else OBJECT)
            self.events.append(b'F'+struct.pack('<I',label));self.ret();return
        if address==0x4af530:
            typ,flags=struct.unpack('<2I',mu.mem_read(esp+4,8));assert (typ,flags)==(110,2);self.events.append(b'C'+struct.pack('<2I',typ,flags))
            if self.mutation==1:mu.mem_write(POOL+2514,struct.pack('<2f',-7.75,8.25));mu.mem_write(POOL+2535,b'\x80')
            self.ret(POOL+60*50);return
        if address==POP:
            assert mu.reg_read(UC_X86_REG_ECX)==POOL+60*50;x,y,flags=struct.unpack('<3I',mu.mem_read(esp+4,12));assert flags==16;self.events.append(b'P'+struct.pack('<4I',60,x,y,flags));mu.mem_write(POOL+60*50+14,struct.pack('<2I',x,y));self.ret(0,12);return
        if address==0x41bd20:
            typ,parent,form,flags=struct.unpack('<4I',mu.mem_read(esp+4,16));assert (typ,parent,form,flags)==(18,60,0,16) and mu.reg_read(UC_X86_REG_ECX)==OBJECT
            self.events.append(b'A'+struct.pack('<6I',typ,parent,form,flags,self.word(OBJECT+0x1c),self.word(OBJECT+0x20)));mu.mem_write(OBJECT+4,struct.pack('<2I',80,61 if self.mutation==2 else 60));self.ret(OBJECT,16);return
        if address==0x4b1510:
            form,dummy=struct.unpack('<2I',mu.mem_read(esp+4,8));assert (form,dummy)==(80,60);self.events.append(b'D'+struct.pack('<2I',form,dummy));self.ret();return
        if address==0x4a7db0:
            source,*outputs=struct.unpack('<5I',mu.mem_read(esp+4,20));assert source==50;self.events.append(b'I'+struct.pack('<I',source))
            # 캡처 몸체는 기존 캡처 대조와 별도 대조했다. 여기서는 실제 폭/기준점/bitmap 반환 경계다.
            for pointer,value in zip(outputs,(3,4,1,3)):mu.mem_write(pointer,struct.pack('<i',value))
            if self.mutation==3:mu.mem_write(POOL+2535,b'\xff')
            self.ret(BITMAP);return
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        super().instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """원문 목록/SEH 외에는 기존 분해 계산의 제한을 유지한다."""
        if any(low<=address and address+size<=high for low,high in ((TABLE,TABLE+400),(NEXT,NEXT+400),(0,4),(0x595138,0x595140))):return
        super().write(mu,access,address,size,value,data)

    def read(self,mu,access,address,size,value,data):
        """목록/SEH/가상 표의 명시 입력만 프레임 읽기 경계에 추가한다."""
        if any(low<=address and address+size<=high for low,high in ((TABLE,TABLE+400),(NEXT,NEXT+400),(VTABLE,VTABLE+4096),(0,4096))):return
        super().read(mu,access,address,size,value,data)

    def snapshot(self):
        """현재 목록에서 객체 주소만 토큰1로 정규화한다."""
        table,count=self.word(0x595138),self.word(0x59513c);result=[self.word(table+index*4) for index in range(count)]
        return ','.join(str(1 if value==OBJECT else value) for value in result) or '-'

    def notice(self,watched,form,precision):
        """실제 가상+34의 무시 인수/현재 가상 삭제0을 정상 반환까지 관찰한다."""
        self.mu.mem_write(POOL+3000,struct.pack('<I',VTABLE));self.mu.mem_write(VTABLE+0x10,struct.pack('<I',DELETE));self.events=[]
        self.call(0x48f0a0,POOL+3000,struct.pack('<2I',watched,form),8,precision);self.returns+=1;return b''.join(self.events).hex()

    def sample(self,case,precision):
        """실제 생성 wrapper와 scalar 삭제의 전체 상태/순서를 보존한다."""
        orientation,self.capacity,hole,depth,self.mutation,flags,pattern=case;mu=self.mu;self.orientation=orientation;self.image_size=12;self.capture=(3,4,1,3)
        mu.mem_write(OBJECT,b'\xa5'*0x78);mu.mem_write(PARTICLES,b'\x5a'*0x1000);image=bytes((index*13+7)%255 if pattern else 255 for index in range(12));mu.mem_write(BITMAP,image)
        mu.mem_write(0,struct.pack('<I',0x87654321));mu.mem_write(0x5410b8,struct.pack('<I',18));mu.mem_write(0x541210,struct.pack('<I',110));mu.mem_write(0x5c8464,struct.pack('<I',POOL));mu.mem_write(0x5c847c,struct.pack('<I',24000));mu.mem_write(0x595138,struct.pack('<2I',TABLE if self.capacity else 0,self.capacity))
        initial=[0 if index==hole else 100+index for index in range(self.capacity)]
        if initial:mu.mem_write(TABLE,struct.pack('<'+'I'*len(initial),*initial))
        mu.mem_write(0x55b4d0,struct.pack('<d',1.125));mu.mem_write(0x59a924,struct.pack('<2f',16,11));mu.mem_write(0x532710,struct.pack('<I',0x12345678));mu.mem_write(TLS+20,struct.pack('<I',1));mu.mem_write(POOL,bytes(3200));mu.mem_write(POOL+35,b'\xff');mu.mem_write(VTABLE+0x90,struct.pack('<I',POP))
        # 현재 source/원래 dummy/부착 callback이 바꿀 대체 부모를 다른 위치/깊이로 공급한다.
        for sid,typ,z,x,y in ((50,144,depth,2.125,3.25),(60,110,0,9.75,8.5),(61,110,127,-2.75,7.125),(62,145,0,20.5,30.25)):
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typ;raw[35]=z;struct.pack_into('<2f',raw,14,x,y);mu.mem_write(POOL+sid*50,bytes(raw))
        self.events=[];self.phase='create';self.call(0x450510,0,struct.pack('<4I',50,61,62,orientation),0,precision);self.returns+=1
        raw=bytes(mu.mem_read(OBJECT,0x78));before=semantic(raw).hex();particles=self.particles().hex();registered=self.snapshot();events=b''.join(self.events).hex();game,crt=self.word(0x532710),self.word(TLS+20)
        assert raw[0x40:0x48]==b'\xa5'*8 and bytes(mu.mem_read(BITMAP,12))==image and self.word(0)==0x87654321
        self.events=[];self.phase='delete';self.call(0x450080,OBJECT,struct.pack('<I',flags),4,precision,result=OBJECT);self.returns+=1
        after=bytes(mu.mem_read(OBJECT,0x78));assert after[0xc:]==raw[0xc:] and self.word(OBJECT)==0x5019ec and self.word(0)==0x87654321
        return (before,particles,str(game),str(crt),registered,events,semantic(after).hex(),self.snapshot(),b''.join(self.events).hex())


def main():
    """고유 입력의 두 정밀도 원문을 저장하거나 SHA만 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        # 전체 근거 바이트를 재계산 없이 확인한다.
        for path,wanted in {**facts['sha256'],**facts.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')];assert len(rows)==facts['uniqueInputs'] and len({tuple(row[:7]) for row in rows})==len(rows)
        print('분해 수명 SHA/고유 입력 감사:',len(rows));return
    cases=[(orientation,*slots,depth,mutation,flags,pattern) for pattern in (2,0) for orientation,slots,depth,mutation,flags in itertools.product(range(2),((0,-1),(40,20),(1,-1)),(0,127,255),range(4),(0,1))]
    if args.smoke:cases=cases[:1]
    oracles=[Oracle(),Oracle()];rows=[]
    # 다른 정밀도 반환은 고유 입력 수에 합산하지 않는다.
    for case in cases:
        outputs=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1];rows.append((*map(str,case),*outputs[0]))
    notices=[]
    # 서로 다른 watched/form 값이 실제 삭제 flags에 섞이지 않는지 확인한다.
    for watched,form in itertools.product((0,50,65535),(0,80,65535)):
        outputs=[oracle.notice(watched,form,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1];notices.append([watched,form,outputs[0]])
    if args.smoke:print('분해 wrapper/생성/실제 준비/삭제·dummy 통지 smoke 정상 반환');return
    FIXTURE.write_text('# orientation capacity hole depth mutation flags pattern fields100 particles game crt registered createEvents deleted100 unregistered deleteEvents\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_shrapnel_frame.py','tools/decomp_shrapnel_lifetime.py','tools/decomp_dissolve_frame.py','tools/decomp_dissolve_lifetime.py',FIXTURE.relative_to(ROOT).as_posix())
    facts=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),dummyNoticeInputs=notices,normalReturns=sum(oracle.returns for oracle in oracles),precisionDifferentInputs=[],nativeCalls=dict(sum((oracle.calls for oracle in oracles),Counter())),sha256={path:digest(ROOT/path) for path in paths},limitations=['실제 wrapper/분해 생성자/등록/픽셀 묶음/게임 난수 두 번/소멸을 실행한다. 별도9입력은 실제dummy 가상+34의 ret8/가상 삭제0이다. 시계는 생성 중 시작하지 않는다.','new/free·dummy Create/Pop·Host 부착·상호 의존·캡처는 명시 외부 경계다. 실제 캡처480 및 프레임 계산252 자료와 별도다.','원본 미초기화 delta2개와 미사용64/68은a5 입력을 보존한다. bitmap/입자 색 주소/목록 객체 주소만 의미 토큰으로 정규화한다.','원본 wrapper는new120 실패 후 null 초기화를 호출한다. 이 비정상 경로는 정상 관측에 넣지 않았으며 C++ 호스트는 실패를nullptr로 반환한다.'])
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('분해 수명:',len(rows),'고유 입력, 두 정밀도 차이0')


if __name__=='__main__':main()
