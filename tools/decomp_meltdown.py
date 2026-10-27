#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제네레이터 Meltdown의 실제 요청·대상 자격·피해·수신·writer를 독립 실행한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as Base, ROOT, POOL, TYPES, IMAGE, VTABLE, PROCESS, DESTROY, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EAX

FIXTURE = ROOT/'cpppj/tests/fixtures/meltdown-x86.tsv'  # 제품 기대값을 계산하지 않는 새 원문이다.
REPORT = ROOT/'cpppj/meltdown-native-20261028.json'  # 경계와 마지막 디컴파일 PC를 남긴다.
ENTRIES = (0x41c290,0x40b1a0,0x4df230,0x420340,0x4204f0,0x4207a0,0x4205f0,0x40b150,0x40b190,0x4ac200,0x4ac550,0x49a840,0x49a900,0x4adea0,0x422560,0x41cfc0,0x41d0d0,0x41d7a0,0x4e49c0,0x4e4a40,0x4df590)  # 실제 PE 몸체와 공통 기하/패킷 보조 함수의 허용 구간이다.
DAMAGE = STOP+0x800  # 가상98의 명시 외부 효과다.
PACKET = PROCESS+0x800  # 요청/효과의 독립 입력 버퍼다.


def bits(value):
    """float 입력만 원래 DWORD 폭으로 옮긴다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def cases():
    """죽음·genus·보호 프레임·동맹·서버·경계 거리·100개 복사를 교차한다."""
    base=[0,0x200,1,0,0,0,3,3,bits(50),bits(50),bits(48.5),bits(48.5),4,2,1,0,1,3,0,0x030000f8,1,0,1]
    rows=[]
    for command,protocol in itertools.product((0x030000f8,0x830000f8,0x070000f8,0x030000f9,0), (1,0x81)):  # 명시 입력/효과의 원문 순서를 유지한다.
        r=base.copy();r[19]=command;r[20]=protocol;rows.append(r)
    for genus,hp,hard in itertools.product((0,4,0x200,0x4000,0x10000,0x80000000,0x100000,0x80074204),(0,1),(0,0x40)):  # 명시 입력/효과의 원문 순서를 유지한다.
        r=base.copy();r[0]=1;r[1:4]=genus,hp,hard;rows.append(r)
    for genus,dead,ally,boss,distance in itertools.product((4,0x200),(0,1,2,3),(0,1),(0,1),(0,4.5,4.500001,5.5)):  # 명시 입력/효과의 원문 순서를 유지한다.
        r=base.copy();r[0]=2;r[1]=genus;r[4]=dead;r[15:17]=ally,boss;r[10]=bits(48.5+distance);rows.append(r)
    for mutation in (1,2):  # 명시 입력/효과의 원문 순서를 유지한다.
        r=base.copy();r[0]=2;r[18]=mutation;rows.append(r)
    for dead,count,mutation,boss in itertools.product((0,1,2,3),(0,1,3,100),range(4),(0,1)):  # 명시 입력/효과의 원문 순서를 유지한다.
        r=base.copy();r[0]=3;r[4]=dead;r[16:19]=boss,count,mutation;rows.append(r)
    for count,dead,mutation,coordinate in itertools.product((0,1,3,100,0xffffffff),(0,1,2,3),range(4),(.1,254.75)):  # 명시 입력/효과의 원문 순서를 유지한다.
        r=base.copy();r[0]=4;r[4]=dead;r[17:19]=count,mutation;r[10]=bits(coordinate);rows.append(r)
    for count,writing in itertools.product((0,1,3,100,32760,65535,0x80000000),(0,1)):  # 명시 입력/효과의 원문 순서를 유지한다.
        r=base.copy();r[0]=5;r[17]=count;r[22]=writing;rows.append(r)
    return list(dict.fromkeys(map(tuple,rows)))


class Oracle(Base):
    """보호 PE의 명시 몸체만 실행하며 월드 효과/할당/검색은 기록 경계로 둔다."""
    def __init__(self):
        """대상100개의 raw와 실제 프레임 코드를 격리 준비한다."""
        super().__init__();self.mu.mem_map(POOL+0x10000,0xf0000)
        self.allowed=[];ranges(self,ENTRIES);self.instructions=0;self.calls=collections.Counter();self.put(VTABLE+0x98,DAMAGE)

    def instruction(self,mu,address,size,data):
        """원문 계산을 실행하고 선택된 큰 효과의 인자/재진입만 기록한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);args=struct.unpack('<6I',mu.mem_read(sp+4,24));sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;c=self.case
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x4adc60:self.event('hp',sid);self.ret(c[2]);return
        if address==0x4e0620:self.event('assert',args[2]);self.ret();return
        if address==0x4e2dc0:self.events.append('send:'+str(args[0])+':'+bytes(mu.mem_read(args[1],6)).hex());self.ret();return
        if address==0x4ae010:self.event('base',sid,self.get(args[0]));self.ret(0,4);return
        if address==0x4e4391:self.ret(PROCESS);return
        if address==0x4203a0:
            self.event('find',*args[:3]);assert self.get(args[3])==PROCESS and self.get(args[3]+4)==100 and self.get(args[3]+8)==0
            for i in range(c[17]):self.put(PROCESS+i*4,15000+i)  # 명시 입력/효과의 원문 순서를 유지한다.
            self.put(args[3]+8,c[17])
            if c[18]==1:self.put(POOL+2500+14,bits(80.25));self.put(POOL+2500+34,7,1)
            if c[18]==3:self.put(0x52ee81,0x83,1);self.put(0x544c8c,6,2)
            self.ret();return
        if address==0x4e2e30:
            self.events.append('broadcast:'+bytes(mu.mem_read(args[0],220)).hex())
            if c[18]==2:self.put(0x540c70,769);self.put(args[0]+14,99,2)
            self.ret();return
        if address==0x4205f0 and c[0]==3:
            self.events.append(f'receive:{args[0]}:{args[1]}:'+bytes(mu.mem_read(args[2],220)).hex());self.ret();return
        if address==DESTROY:self.event('destroy',sid,args[0]);self.ret(0,4);return
        if address==0x4e4354:self.ret();return
        if address==DAMAGE:
            self.event('damage',sid,*args[:3])
            if c[18]==1:self.put(POOL+3000+14,bits(200))
            if c[18]==2:self.put(POOL+3000+11,2,1)
            if c[0]==4 and c[18]==3:self.put(PACKET+16,1);self.put(PACKET+13,7,1)
            self.ret(0,12);return
        if address==0x470ac0:self.event('ground',*args[:2]);self.ret();return
        if address==0x421f70:self.event('edge',*args[:3]);self.ret();return
        if address==0x45a750:
            self.event('visual',*args[:4]);self.visuals+=1
            if c[18]==1 and self.visuals==1:self.put(PACKET+12,2,1);self.put(PACKET+4,bits(100))
            self.ret();return
        if address==0x4a9cb0:
            self.event('sound',self.string(args[0]),*args[1:])
            if c[18]==2:self.put(PACKET+16,1)
            self.ret();return
        if not any(a<=address and address+size<=b for a,b in self.allowed):raise RuntimeError(f'Meltdown 미허용 실행 {address:08x}')

    def write(self,mu,access,address,size,value,data):
        """직접 쓰기는 원문 스택/SEH와 지정 패킷에 한정한다."""
        if STACK-0x7f000<=address and address+size<=STACK+0x1000 or address==0 and size==4 or PACKET<=address and address+size<=PACKET+220:return
        raise RuntimeError(f'Meltdown 미허용 쓰기 {address:08x}+{size}')

    def sample(self,case,precision):
        """제품 함수 없이 입력을 설치하고 반환·순서·raw·패킷만 관측한다."""
        self.case=case;c=case;self.events=[];self.visuals=0;self.mu.mem_write(TYPES,bytes(0x20000))
        for sid in (50,*range(60,160)):  # 명시 입력/효과의 원문 순서를 유지한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=75 if sid==50 else 70;raw[11]=0 if sid==50 else c[4];raw[34]=2 if sid==50 else c[14];raw[40]=c[5];struct.pack_into('<II',raw,14,c[8],c[9]);self.mu.mem_write(POOL+sid*50,bytes(raw))
        if c[0]==3:self.put(POOL+2500+11,c[4],1)
        for kind,footx,footy,genus in ((75,3,3,0x200),(70,c[6],c[7],c[1])):  # 명시 입력/효과의 원문 순서를 유지한다.
            for off,value in ((0xec,genus),(0x1d4,footx),(0x1d8,footy),(0x124,IMAGE)):self.put(TYPES+500*kind+off,value)  # 명시 입력/효과의 원문 순서를 유지한다.
        self.mu.mem_write(IMAGE,bytes((74,0,1,c[3])))
        for address,value in ((0x5c8464,POOL),(0x59ab20,TYPES),(0x540bc4,c[16]),(0x540c70,513),(0x542ad4,0x030000f8),(0x544c8c,c[21])):self.put(address,value)  # 명시 입력/효과의 원문 순서를 유지한다.
        self.mu.mem_write(0x595200,bytes(81*4));self.put(0x595200+(c[13]*9+c[14])*4,c[15]);self.put(0x52ee80,0x75,1);self.put(0x52ee81,c[20],1);self.put(0x52ee82,0x76,1);self.put(0x52ee83,c[20],1);self.put(0x52ee84,6,2);self.put(0,0x87654321)
        packet=bytearray([255]*220);struct.pack_into('<BBHII BBHI',packet,0,1,0x75,4,c[10],c[11],c[12],c[13],50,c[17])
        for i in range(100):struct.pack_into('<H',packet,20+i*2,60+i)  # 명시 입력/효과의 원문 순서를 유지한다.
        self.mu.mem_write(PACKET,bytes(packet));self.put(PROCESS+0x700,c[19]);op=c[0]
        if op==0:entry,args,callee=0x41c290,(PROCESS+0x700,),True
        elif op==1:entry,args,callee=0x420340,(60,),False
        elif op==2:entry,args,callee=0x4204f0,(*c[10:14],60,50),False
        elif op==3:self.put(PACKET+4,50,2);entry,args,callee=0x4207a0,(123,456,PACKET),False
        elif op==4:entry,args,callee=0x4205f0,(123,456,PACKET),False
        else:entry,args,callee=0x40b150,(c[22],),True
        # 실제 반환 규약·보존 레지스터·스택 표식·SEH·x87를 직접 검사한다.
        from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_EDX,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
        argument=struct.pack('<'+'I'*len(args),*args);target=STOP
        if not callee:
            target=STOP+0x900+op*32
            self.mu.mem_write(target,b'\x83\xc4'+bytes((len(argument),))+b'\xe9'+struct.pack('<i',STOP-(target+8)))
            if (target,target+8) not in self.allowed:self.allowed.append((target,target+8))
        registers={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        self.mu.mem_write(STACK,struct.pack('<I',target)+argument+b'\xc0\xde\xc0\xde')
        for reg,val in {**registers,UC_X86_REG_EAX:0,UC_X86_REG_EDX:0,UC_X86_REG_ECX:PACKET if op==5 else POOL+2500,UC_X86_REG_ESP:STACK,UC_X86_REG_EFLAGS:2,UC_X86_REG_FPCW:precision,UC_X86_REG_FPSW:0,UC_X86_REG_FPTAG:0xffff}.items():self.mu.reg_write(reg,val)  # 명시 입력/효과의 원문 순서를 유지한다.
        self.mu.emu_start(entry,STOP,count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(argument)
        assert bytes(self.mu.mem_read(STACK+4+len(argument),4))==b'\xc0\xde\xc0\xde'
        for reg,val in registers.items():assert self.mu.reg_read(reg)==val  # 명시 입력/효과의 원문 순서를 유지한다.
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800 and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        self.returns+=1;assert self.get(0)==0x87654321
        return str(self.mu.reg_read(UC_X86_REG_EAX) if op==1 else 0),';'.join(self.events) or '-',bytes(self.mu.mem_read(POOL+3000,50)).hex(),bytes(self.mu.mem_read(PACKET,220)).hex()


def main():
    """최초 원문을 동결하며 이미 생성한 자료는 검증 모드에서만 읽는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');opts=parser.parse_args()
    if opts.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in record['sha256'].items():assert digest(ROOT/path)==wanted,path  # 명시 입력/효과의 원문 순서를 유지한다.
        rows=[line.split('\t') for line in FIXTURE.read_text('utf-8').splitlines() if line and not line.startswith('#')]
        assert [tuple(map(int,r[0].split(','))) for r in rows]==cases() and len(rows)==record['uniqueInputs']
        assert all(r[1:5]==r[5:9] for r in rows);print('Meltdown 원문 감사',len(rows));return
    assert not FIXTURE.exists() and not REPORT.exists(),'기존 원문은 덮어쓰지 않는다'
    oracles=[Oracle(),Oracle()];rows=[]
    for case in cases():  # 명시 입력/효과의 원문 순서를 유지한다.
        try:results=[o.sample(case,p) for o,p in zip(oracles,(0x27f,0x37f))]
        except Exception as error:raise RuntimeError(str(case)) from error
        assert results[0]==results[1],case;rows.append((','.join(map(str,case)),*results[0],*results[1]))
    FIXTURE.write_text('# input return53 trace53 raw53 packet53 return64 trace64 raw64 packet64\n'+'\n'.join('\t'.join(r) for r in rows)+'\n','utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_meltdown.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_carrier_falling.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    record=dict(unit=520,host=socket.gethostname(),date='2026-10-28',lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),uniqueInputs=len(rows),normalReturns=sum(o.returns for o in oracles),instructions=sum(o.instructions for o in oracles),nativeCalls=dict(sum((o.calls for o in oracles),collections.Counter())),boundaries='41c290/420340/4204f0/4207a0/4205f0/40b150와 TYPE/중심/거리/Clamp/Snap/패킷 전체. 4203a0 검색·할당/해제·가상피해/삭제·소리/visual/edge·배포/로컬효과는 명시 경계. 검색 본체/실제 게임 전체 완료를 뜻하지 않는다. 실제 OS/창/통신 없음.',sha256={p:digest(ROOT/p) for p in paths})
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(json.dumps({k:record[k] for k in ('uniqueInputs','normalReturns','instructions')}))


if __name__=='__main__':main()
