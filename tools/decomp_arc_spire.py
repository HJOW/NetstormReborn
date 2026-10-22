#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""아크 스파이어의 후처리/Regular/이웃/전체 공격/연쇄 피해를 보호 PE에서 격리 관찰한다."""
import argparse
import collections
import itertools
import json
import socket
import struct
import pefile
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, STACK, STOP, POOL, TYPES, VTABLE, IMAGE
from decomp_cursor_position import ranges
from decomp_particle_gradient import digest
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EAX, UC_X86_REG_EIP

HEAP, SPOTS, OUTPUT = 0x14500000, 0x14600000, IMAGE+0x800  # 격리 Buffer/실제256² 지도/반환 저장소다.
DAMAGE, WRAPPER, FLOAT_RETURN = STOP+0x520, STOP+0x900, STOP+0x980  # 가상 피해·x87 반환을 실제 명령으로 처리하는 구간이다.
ENTRIES = (0x48c080,0x48c6e0,0x48c110,0x48c300,0x48c410,0x40d890,0x4ac550,0x49a840,0x4ac200,0x4adc60,0x41d770,0x41d7a0,0x4ab7c0,0x4e49c0)  # 산술·필터·작은 getter·CRT는 원문을 실행한다.
IDS = (50,60,61,70,71,72)  # 출력으로 보존할 실제 변경 가능 슬롯이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/arc-spire-x86.tsv'  # 기존 기대값과 분리한 새 원문이다.
REPORT = ROOT/'cpppj/arc-spire-20261022.json'  # 실행 범위/호스트/정밀도/해시 근거다.


def bits(value):
    """명시 float 입력을 실제 네 바이트로 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def floating(value):
    """DWORD 입력의 float 비트를 읽는다."""
    return struct.unpack('<f',struct.pack('<I',value))[0]


def cases():
    """계산된 기대값 없이 원본의 각 독립 경계를 교차한다."""
    rows=[]
    for flags,extra,allocate in itertools.product((0,1,2,3,0x50),(0,1,8,9,32),(0,1)):  # 최초 예약 관문/할당 실패다.
        rows.append(('P',(flags,extra,allocate)))
    for event,count,extra,total in itertools.product((0,1,2),(0,1,5,6,7,0x7fffffff,0xffffffff),(0,32),(1,6)):  # signed count 감김과 원래 공통 사슬이다.
        rows.append(('R',(event,count,extra,total,bits(.75))))
    for extra,forward,back,position,capacity,count,found in itertools.product((0,1,32),(0,1),(0,1),(0,1),(0,2),(0,2),(0,1)):  # 첫 이웃 반환과 buffer 가득 참을 분리한다.
        rows.append(('N',(extra,forward,back,position,capacity,count,found)))
    for radius,mask,mode,editor,mutation in itertools.product((0.,1.,4.,6.75,12.),(0,1,2,3),(0,2),(0,1),(0,1,2)):  # 범위 끝 제외·두 축·현재 피해 관문이다.
        rows.append(('A',(bits(radius),bits(10),bits(10),mask,mode,editor,mutation,bits(.75),bits(41.5))))
    for x,y in ((10.25,10.),(10.,10.75),(253.5,253.5)):  # 정확한 정수 이웃과254 clamp를 관찰한다.
        rows.append(('A',(bits(12),bits(x),bits(y),3,2,0,0,bits(.3),bits(41.5))))
    for neighbors,mode,editor,mutation in itertools.product((0,1,2),range(5),(0,1),(0,1,2)):  # 빈/혼합/40개 포화와 체인 생존 재조회다.
        rows.append(('S',(neighbors,mode,editor,mutation,31)))
    for offset in (.000005,.00002,.00005,.0001,.5,.99998):  #502310의 정확한 보정 상수로 칸이 바뀌는 경계다.
        rows.append(('T',(1,2,0,0,31,bits(offset))))
    return list(dict.fromkeys(rows))


def candidates(mode):
    """큰 finder 경계가 공급하는 순서 있는 명시 후보다."""
    return () if mode==0 else (50,60,70,71,72) if mode==1 else (70,71,72) if mode==2 else tuple(range(70,114)) if mode==3 else (70,71,72,73,74,75)


class Oracle(BaseOracle):
    """실제 제어/산술/필터를 실행하고 큰 finder/가상 효과/메모리 장치만 분리한다."""
    def __init__(self):
        """함수표의 불연속 범위와 메모리 읽기/쓰기 가드를 설치한다."""
        super().__init__();self.mu.mem_map(HEAP,0x10000);self.mu.mem_map(SPOTS,0x10000)
        self.allowed=[];ranges(self,ENTRIES);self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
        self.instructions=0  # 실제 실행 명령 수다.
        self.calls=collections.Counter()  # 원문 함수별 진입 수다.
        self.events=[]  # 순서 있는 외부 효과 원문이다.
        # 인수를 데이터에서 읽으므로 앞 호출의 번역 캐시가 즉시값을 재사용하지 않는다.
        code=b''.join(b'\xff\x35'+struct.pack('<I',OUTPUT+0x20+index*4) for index in (2,1,0))
        at=WRAPPER+len(code);code+=b'\xe8'+struct.pack('<i',0x48c6e0-at-5)+b'\xd9\x1d'+struct.pack('<I',OUTPUT)+b'\xc3'
        self.mu.mem_write(WRAPPER,code)
        self.mu.mem_write(FLOAT_RETURN,b'\xd9\x05'+struct.pack('<I',OUTPUT+4)+b'\xc2\x0c\x00')

    def instruction(self,mu,address,size,_):
        """원본 필터/프레임 산술은 대체하지 않고 하위 요청만 관측한다."""
        self.instructions+=1;sp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        args=[self.get(sp+4+index*4) for index in range(4)]
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        if address==0x48c110:self.event('A',args[0])
        if address==0x4e4391:
            if self.kind=='P':self.event('N',args[0])
            result=0 if self.kind=='P' and not self.values[2] else HEAP+self.allocated
            self.allocated+=(args[0]+15)&~15;self.ret(result);return
        if address==0x496e80:self.event('R',args[0],args[1],args[2]);self.ret(ecx,12);return
        if address==0x44bf80:self.event('D',args[0]);self.ret(0,4);return
        if address==0x44b900:
            self.event('C',*args[:3]);self.mu.reg_write(UC_X86_REG_EIP,FLOAT_RETURN);return
        if address==0x4acee0:
            self.event('B',args[0],args[1]);self.put(ecx+36,args[0]);self.ret(0,8);return
        if address==0x4b1fa0:
            self.event('F',*args[:3]);sid=0
            if self.kind=='N':sid=60 if self.values[6] else 0
            elif self.mask&1 and floating(args[0])==int(self.x)+4 and floating(args[1])==int(self.y):sid=60
            elif self.mask&2 and floating(args[0])==int(self.x) and floating(args[1])==int(self.y)+5:sid=61
            self.ret(sid);return
        if address==0x4b16d0:
            self.event('I',*args);self.finder=ecx;self.index=0;self.put(ecx+0x34,self.victims[0] if self.victims else 0);self.ret(ecx,16);return
        if address==0x4b1810:
            self.index+=1;self.put(ecx+0x34,self.victims[self.index] if self.index<len(self.victims) else 0);self.ret();return
        if address==0x472700:
            self.event('L',args[0],args[1])
            if self.mutation==1 and args[1]==70:self.put(POOL+70*50+11,1,1)
            self.ret();return
        if address==DAMAGE:
            sid=(ecx-POOL)//50;self.event('H',sid,*args[:3])
            if self.mutation==2 and sid==70:self.put(POOL+70*50+11,2,1);self.put(POOL+50*50+34,3,1)
            self.ret(0,12);return
        if address==0x4e4354:self.ret();return
        if address==0x4e0620:raise RuntimeError(f'Arc 원본 진단 {args}')
        if WRAPPER<=address<WRAPPER+40 or FLOAT_RETURN<=address<FLOAT_RETURN+9:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'Arc 미검토 명령 {address:x}')

    def write(self,mu,access,address,size,value,_):
        """원문 쓰기를 격리 SEH/힙/부모/반환/스택으로 제한한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,4),(HEAP,HEAP+0x10000),(POOL,POOL+0x10000),(IMAGE,IMAGE+0x1000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'Arc 범위 밖 쓰기 {address:x}+{size}')

    def read(self,mu,access,address,size,value,_):
        """보호 이미지와 명시 입력/스택 밖 읽기를 차단한다."""
        if any(lo<=address and address+size<=hi for lo,hi in ((0,4),(0x400000,0x600000),(HEAP,HEAP+0x10000),(SPOTS,SPOTS+0x10000),(POOL,POOL+0x10000),(TYPES,TYPES+0x20000),(VTABLE,VTABLE+0x1000),(IMAGE,IMAGE+0x1000),(STACK-0x7f000,STACK+0x1000))):return
        raise RuntimeError(f'Arc 범위 밖 읽기 {address:x}+{size}')

    def sample(self,case,precision):
        """현재 입력을 초기화하고 전체 변경 슬롯/후보 buffer/반환을 캡처한다."""
        self.kind,self.values=case;v=self.values;self.events=[];self.allocated=0;self.x=self.y=10.;self.mask=self.mutation=self.mode=self.editor=0;radius=6.;rate=41.5;total=6
        if self.kind=='A':radius=floating(v[0]);self.x=floating(v[1]);self.y=floating(v[2]);self.mask,self.mode,self.editor,self.mutation=v[3:7]
        if self.kind in ('S','T'):self.mode,self.editor,self.mutation=v[1:4]
        self.victims=candidates(self.mode)
        self.mu.mem_write(SPOTS,bytes([1 if self.kind=='A' else 0])*0x10000)
        self.mu.mem_write(HEAP,bytes([0xa5])*0x10000);self.mu.mem_write(IMAGE,bytes(0x1000))
        self.mu.mem_write(TYPES,bytes(0x20000))
        for kind in range(188):self.put(TYPES+kind*500+0xe8,0x10)  # 실제4adc60의 기본 damageable 입력이다.
        for sid in (50,60,61,*range(70,114)):
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=149 if sid<70 else 78;raw[34]=1 if sid<70 else 2
            struct.pack_into('<ff',raw,14,self.x+(4 if sid==60 else 0),self.y+(5 if sid==61 else 0));self.mu.mem_write(POOL+sid*50,bytes(raw))
        if self.mode==1:self.put(POOL+71*50+10,79,1);self.put(TYPES+79*500+0xec,0x120000);self.put(POOL+72*50+40,1,1)
        if self.mode==4:
            self.put(TYPES+78*500+0xe8,0);self.put(POOL+71*50+40,1,1);self.put(POOL+72*50+34,1,1)
            for sid,kind,genus in ((73,91,0x400000),(74,92,0x120000),(75,93,0)):
                self.put(POOL+sid*50+10,kind,1);self.put(TYPES+kind*500+0xec,genus)
        if self.kind=='T':
            self.put(POOL+60*50+14,bits(14+floating(v[5])));self.put(POOL+60*50+18,bits(10+floating(v[5])))
        self.mu.mem_write(0x595200,bytes(81*4))
        for first,second in ((1,1),(2,2),(3,3),(1,3),(3,1)):self.put(0x595200+(first*9+second)*4,1)
        self.put(VTABLE+0x98,DAMAGE);self.put(VTABLE+0xc8,0x48c110)
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x5c7c44,SPOTS);self.put(0x5c85a4,self.editor);self.put(0,0)
        self.put(TYPES+149*500+0x44,bits(radius));self.put(TYPES+149*500+0x48,bits(rate));self.put(TYPES+149*500+0x114,total)
        self.put(OUTPUT+4,bits(.75));receiver=POOL+2500;entry=0;args=b'';purge=0
        if self.kind=='P':
            self.put(receiver+40,v[1],1);entry=0x48c080;args=struct.pack('<I',v[0]);purge=4
        elif self.kind=='R':
            self.put(receiver+40,v[2],1);self.put(TYPES+149*500+0x114,v[3]);entry=WRAPPER
            for index,value in enumerate((v[0],v[1],v[4])):self.put(OUTPUT+0x20+index*4,value)  # wrapper는 현재 인수를 데이터로 읽는다.
        elif self.kind=='N':
            self.put(POOL+60*50+40,v[0],1);self.put(0x595200+10*4,v[1]);self.put(POOL+60*50+34,3,1);self.put(0x595200+12*4,v[1]);self.put(0x595200+28*4,v[2])
            self.put(POOL+60*50+14,bits(14.25 if v[3] else 14));self.put(IMAGE,HEAP);self.put(IMAGE+4,v[4]);self.put(IMAGE+8,v[5])
            entry=0x48c300;args=struct.pack('<4I',IMAGE,14,10,149);purge=16
        elif self.kind=='A':
            self.put(TYPES+149*500+0x48,v[8]);entry=0x48c110;args=struct.pack('<I',v[7]);purge=4
        else:
            self.put(IMAGE,HEAP);self.put(IMAGE+4,20);self.put(IMAGE+8,v[0]);self.put(HEAP,60);self.put(HEAP+4,61)
            self.allocated=0x100;entry=0x48c410;args=struct.pack('<II',IMAGE,v[4]);purge=8
        self.call(entry,receiver,args,purge,precision)
        result=self.get(OUTPUT) if self.kind=='R' else self.mu.reg_read(UC_X86_REG_EAX) if self.kind=='N' else 0
        raws=''.join(bytes(self.mu.mem_read(POOL+sid*50,50)).hex() for sid in IDS)
        buffer=bytes(self.mu.mem_read(HEAP,80)).hex() if self.kind=='N' else '-'
        return (';'.join(self.events) or '-',raws,str(result),str(self.get(IMAGE+8)) if self.kind=='N' else '0',buffer)


def main():
    """두 정밀도의 정상 복귀를 동결하거나 원문/고유 입력/해시만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,wanted in {**report['sha256'],**report.get('implementationSha256',{})}.items():assert digest(ROOT/path)==wanted,path  # 제품 SHA는 최종 검증 버전을 가리킨다.
        rows=[row.split('\t') for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows)==report['uniqueInputs'] and report['normalReturns']==len(rows)*2
        assert [(row[0],row[1]) for row in rows]==[(kind,','.join(map(str,values))) for kind,values in cases()]
        assert all(row[2:7]==row[7:12] for row in rows)
        print('Arc Spire 원문/출처 SHA:',len(rows));return
    oracles=(Oracle(),Oracle());rows=[]
    for case in cases():  # C++ 계산으로 기대값을 만들지 않는다.
        first,second=[oracle.sample(case,precision) for oracle,precision in zip(oracles,(0x27f,0x37f))]
        assert first==second,case
        rows.append((case[0],','.join(map(str,case[1])),*first,*second))
    FIXTURE.write_text('# kind input effects53 raws53 result53 count53 buffer53 effects64 raws64 result64 count64 buffer64\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_arc_spire.py','tools/decomp_cursor_lifecycle.py','tools/decomp_cursor_position.py','tools/decomp_particle_gradient.py','cpppj/tests/fixtures/arc-spire-x86.tsv')
    image=pefile.PE(str(ROOT/paths[0])).get_memory_mapped_image()
    tables={hex(table):{hex(offset):hex(struct.unpack_from('<I',image,table-0x400000+offset)[0]) for offset in (0x10,0x14,0x18,0x20,0x5c,0x78,0x98,0xa4,0xc8)} for table in (0x50e668,0x50e940,0x501ea0)}  # 생성/수명/기본 그림의 실제 진입 주소다.
    report=dict(host=socket.gethostname(),lastFullDecompileHost='HJOW-Athlon',lastFullDecompileDate='2026-10-22',uniqueInputs=len(rows),normalReturns=len(rows)*2,scopes=dict(collections.Counter(row[0] for row in rows)),instructions=sum(oracle.instructions for oracle in oracles),nativeCalls=dict(sum((oracle.calls for oracle in oracles),collections.Counter())),entries=[f'{entry:08x}' for entry in ENTRIES],boundaries='48c080/48c6e0/48c110/48c300/48c410 전체 및 Buffer·작은 TYPE/getter/피격 가능 필터/좌표·지도/CRT는 실제 원문. 힙할당/큰finder/위치lookup/프레임/공통Damageable/가상피해/빔은 명시 경계. SEH·GP·스택·canary·x87·접근범위·변경raw/Buffer를 검사한다.',sha256={path:digest(ROOT/path) for path in paths})
    report['vtables']=tables
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:report[key] for key in ('uniqueInputs','normalReturns','scopes','instructions')}))


if __name__=='__main__':main()
