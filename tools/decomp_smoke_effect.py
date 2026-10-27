#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78 냉각 연기의 전체 생성·입자/프레임·색/Draw·소멸을 격리 x86으로 관측한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_fierytail_frame import FieryOracle, OBJECT, PARTICLES, POOL, PALETTE, COLORS, STACK, ROOT
from decomp_owner_oracle import TYPES, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

TABLE,NEXT,SHAPE=0x12500000,0x12501000,0x12502000  # 표시 목록/확장 배열/SHP 경계의 격리 주소다.
ENTRIES=(0x4a8770,0x4a86e0,0x4a8140,0x4a8240,0x4a82b0,0x4a85a0,0x488190,0x487fd0,0x488100,0x487ab0,0x487b40,0x418400,0x418430,0x455930,0x453040,0x487ba0,0x487e50,0x4e49c0,0x487cb0,0x487cd0,0x4437f0,0x49a840,0x4882c0,0x44fd70,0x423aa0,0x40f1b0,0x487880,0x41bd00,0x4e4a40)  # 공통 등록/타이머/난수/색/삭제도 실제 몸체를 실행한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/smoke-effect-x86.tsv'  # 기존 원문을 덮지 않는 새 기대값이다.
REPORT=ROOT/'cpppj/smoke-effect-native-20261027.json'  # 원문 범위·경계/정밀도·SHA 근거다.

def semantic(raw):
    """vtable/호스트 포인터와 미사용 padding을 제외한112BYTE 의미 필드다."""
    return raw[4:0x2c]+raw[0x30:0x4c]+raw[0x50:0x7c]

def case(kind='frame',capacity=4,total=0,first=1,pattern=0,now=1.025,seed=0,remaining=0,due=0,variant=0,warm=1,null=0,hot=(16,-11),depth=20):
    """결과를 계산하지 않고 독립 원문 객체/입자/부모/전역을 준비한다."""
    raw=bytearray(b'\xa5'*128)
    struct.pack_into('<3I4f2i',raw,0,0x511980,81,50,12.25,13.75,30.5,35.5,0,depth-1)
    struct.pack_into('<3I2d2fI',raw,0x24,capacity,total,0 if null else PARTICLES,1,.123456789,123.25,-456.5,first)
    struct.pack_into('<3f3i5f',raw,0x50,due,.25 if variant else 0,-.5 if variant else 0,hot[0],hot[1],remaining,100,-1,9,3,.5)
    particles=bytearray()
    # capacity 밖을 만들지 않으며 사용 수 뒤의 입자/마지막 DWORD도 보존한다.
    for index in range(capacity):
        life=0 if pattern==0 else (100 if index%2==0 else .1) if pattern==1 else (0 if index%2==0 else -1) if pattern==2 else (3.01,2,.001,-.25)[index%4]
        particles+=struct.pack('<5fI',(-7.75 if variant else 25.125)+index,30.375-index,.125,-1.25,life,0xcafe0000+index)
    parent=bytearray(50);struct.pack_into('<I',parent,0,0x501dd0);struct.pack_into('<H',parent,6,81);parent[10]=129;parent[35]=depth&255;parent[40]=variant;struct.pack_into('<ff',parent,14,-7.75 if variant else 20.75,21.9);struct.pack_into('<I',parent,36,3 if variant else 0)  # 패치의 실제 표시 깊이는 raw+35다.
    state=struct.pack('<d5f',now,.75 if variant else 16,1.25 if variant else 11,.3 if variant else 1,.25,-.5)
    palette=struct.pack('<I64I',12,*[(77 if warm else 1000)+i if i<12 else 0xcafe0000+i for i in range(64)])  # 첫 생성 입력은 다른 색으로 채워 실제77..88 쓰기도 확인한다.
    return (kind,bytes(raw),bytes(particles),bytes(parent),state,palette,seed,warm,null,hot[0],hot[1])

def inputs():
    """용량/방출 실패·분열/동적 꼬리·기한 동률·전체시간·현재 카메라와 cold/warm을 방문한다."""
    rows=[]
    # 작은 목록/용량 전체200과 음수 시간도 원문 그대로 대조한다.
    for capacity,first,pattern,now,remaining,variant in itertools.product((0,1,4,12,200),(0,1),(0,1,2,3),(.975,1,1.025,1.3),(-1,0,2),(0,1)):
        rows.append(case(capacity=capacity,total=min(capacity,4),first=first,pattern=pattern,now=now,remaining=remaining,variant=variant,seed=0x12345678 if variant else 0))
    # 가득 찬 목록의 재사용/Append 실패와 due의 같음/양쪽 경계를 따로 넣는다.
    for pattern,due,remaining,seed in itertools.product((0,1,2,3),(.1234567,.123456789,.123457),(-1,0,2),(0,0xdeadbeef)):
        rows.append(case(capacity=4,total=4,pattern=pattern,now=1,remaining=remaining,due=due,seed=seed))
    rows.append(case(null=1))
    # 실제 helper 직접 생성은 공간이 없으면 난수까지 보존한다.
    for capacity,total,pattern,variant in itertools.product((0,1,4),(0,1,4),(0,1,2),(0,1)):
        if total<=capacity:rows.append(case(kind='spawn',capacity=capacity,total=total,pattern=pattern,variant=variant))
    # 포구 signed WORD와 원문128BYTE/200입자·첫/기존 공유 색 표·기본 깊이를 대조한다.
    for kind,warm,hot,depth in itertools.product(('cold','wrapper','ctor'),(0,1),((16,-11),(-32768,32767)),(127,128)):
        rows.append(case(kind=kind,capacity=200,first=0,remaining=8,warm=warm,hot=hot,depth=depth))
    return list({(row[0],semantic(row[1]),*row[2:]):row for row in rows}.values())

class Oracle(FieryOracle):
    """선행 격리 PE/ABI 검사만 재사용하고 연기의 실제 전체 몸체와 출력 경계를 실행한다."""
    def __init__(self):
        """정확한 전체 정밀 범위와 추가 TYPE/SHP/목록/SEH 메모리를 준비한다."""
        super().__init__();self.mu.mem_map(TYPES,0x20000);self.mu.mem_map(TABLE,0x10000);self.mu.mem_map(0,0x1000)
        self.allowed=[]
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in ENTRIES:  # 함수 사이 공백은 실행하지 않는다.
            for segment in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in segment.split('-'));self.allowed.append((low,high+1))
        assert self.number(0x5410c4)==21 and self.number(0x511980+0x18)==0x4a82b0 and self.number(0x511980+0x20)==0x4a85a0
        self.events=[];self.capacity=200;self.returns=Counter()

    def number(self,address,width=4):
        """비정렬 little endian 의미 필드를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def ret(self,value=0,purge=0):
        """선언한 외부 경계만 실제 호출 규약으로 반환한다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP);self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_EIP,self.number(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+purge)

    def instruction(self,mu,address,size,data):
        """new/free·실제 Host 부착·SHP 자료/kill·장치 출력만 경계로 두고 계산은 실행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4e4391:
            amount=self.number(esp+4);choices={128:OBJECT,4800:PARTICLES,8:PALETTE,48:COLORS,160:TABLE,320:NEXT};assert amount in choices,amount
            self.calls['new-'+str(amount)]+=1
            if amount!=128:self.events.append('N:'+str(amount))  # 외부128BYTE 객체 확보는 실제 Host/pending 소유권과 구별한다.
            self.ret(choices[amount]);return
        if address==0x4e4354:
            pointer=self.number(esp+4);label={PARTICLES:'P',OBJECT:'O',TABLE:'R',NEXT:'R',COLORS:'C'}[pointer];self.events.append('F:'+label);self.ret();return
        if address==0x41bd20:
            args=[self.number(esp+4+i*4) for i in range(4)];assert args==[21,50,0,16]
            self.events.append('B:'+':'.join(map(str,args)));mu.mem_write(this+4,struct.pack('<II',81,50));self.ret(OBJECT,16);return
        if address==0x419850:
            self.events.append('H:'+str(self.number(esp+4))+':'+str(self.number(esp+8)));self.ret(SHAPE,8);return
        if address==0x41bf10:
            self.events.append('K:'+str(self.number(esp+4)));self.ret(purge=4);return
        if address in ENTRIES:self.calls[f'{address:08x}']+=1
        super().instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """객체/입자·공유 표/목록·게임 난수/SEH/스택 밖 원문 쓰기를 거부한다."""
        regions=((OBJECT,OBJECT+128),(PARTICLES,PARTICLES+200*24),(PALETTE,PALETTE+8),(COLORS,COLORS+256),(TABLE,TABLE+0x2000),(0,4),(0x595138,0x595140),(0x5c7aa8,0x5c7aac),(0x532710,0x532714),(STACK-0x7f000,STACK+0x1000))
        if not any(low<=address and address+size<=high for low,high in regions):raise RuntimeError(f'연기 원문 쓰기 범위 {address:x}+{size}')

    def sample(self,row,precision):
        """생성 또는 독립 Frame/Spawn→Draw 뒤 의미 필드/입자·난수/목록/효과를 보존한다."""
        kind,raw,particles,parent,state,palette,seed,warm,null,hotx,hoty=row;mu=self.mu
        mu.mem_write(0,bytes(4));mu.mem_write(OBJECT,raw);mu.mem_write(PARTICLES,particles or bytes(4));mu.mem_write(POOL+2500,parent);mu.mem_write(0x5c8464,struct.pack('<I',POOL));mu.mem_write(0x59ab20,struct.pack('<I',TYPES));mu.mem_write(TYPES+129*500,bytes(500));mu.mem_write(0x541348,struct.pack('<I',188));mu.mem_write(0x5e4794,bytes(4))
        mu.mem_write(SHAPE,bytes(64));mu.mem_write(SHAPE+0x20,struct.pack('<hh',hotx,hoty));mu.mem_write(PALETTE,palette[:4]+struct.pack('<I',COLORS));mu.mem_write(COLORS,palette[4:]);mu.mem_write(0x5c7aa8,struct.pack('<I',PALETTE if warm else 0));mu.mem_write(0x595138,bytes(8));mu.mem_write(TABLE,bytes(320));mu.mem_write(NEXT,bytes(320))
        now,sx,sy,rate,cx,cy=struct.unpack('<d5f',state);mu.mem_write(0x55b4d0,struct.pack('<d',now));mu.mem_write(0x59a924,struct.pack('<ff',sx,sy));mu.mem_write(0x59a934,struct.pack('<fff',rate,cx,cy));mu.mem_write(0x532710,struct.pack('<I',seed));self.events=[];self.dirty=[];self.lines=[]
        if kind in ('cold','wrapper','ctor'):
            self.capacity=200
            # 생성자의 기존 배열 포인터는0인 입력이며 다른 padding/미초기화 필드는 보존한다.
            mu.mem_write(OBJECT+0x2c,bytes(4))
            if kind=='ctor':
                args=struct.pack('<3i',50,hotx,hoty)+raw[0x54:0x5c]+raw[0x68:0x7c];self.call(0x4a8140,OBJECT,args,40,precision,result=OBJECT)
            elif kind=='cold':self.call(0x4a86e0,0,struct.pack('<4if',50,hotx,hoty,10,3),0,precision)
            else:self.call(0x4a8770,0,struct.pack('<I',50),0,precision)
            self.returns[kind]+=1
        else:
            self.capacity=struct.unpack_from('<I',raw,0x24)[0]
            if kind=='frame':self.call(0x4a82b0,OBJECT,b'',0,precision)
            else:self.call(0x4a8240,OBJECT,struct.pack('<3f',-7.75,31.125,100),12,precision)
            self.call(0x4a85a0,OBJECT,bytes(8),8,precision);self.returns[kind]+=1;self.returns['draw']+=1
        after=bytes(mu.mem_read(OBJECT,128));assert after[0x4c:0x50]==raw[0x4c:0x50] and after[0x7c:]==raw[0x7c:];assert bytes(mu.mem_read(POOL+2500,50))==parent
        particlesAfter=bytes(mu.mem_read(PARTICLES,self.capacity*24)).hex() or '-';colorAfter=(bytes(mu.mem_read(PALETTE,4))+bytes(mu.mem_read(COLORS,256))).hex();result=[semantic(after).hex(),particlesAfter,colorAfter,str(self.number(0x532710)),b''.join(self.dirty).hex() or '-',b''.join(self.lines).hex() or '-']
        if kind in ('cold','wrapper','ctor'):
            self.call(0x4882c0,OBJECT,struct.pack('<I',1),4,precision);self.returns['destroy']+=1
            table=self.number(0x595138);assert self.number(table)==0
            result.append(semantic(bytes(mu.mem_read(OBJECT,128))).hex())
        else:result.append('-')
        result.append(';'.join(self.events));assert self.number(0)==0;return result

def main():
    """두 정밀도의 원문 결과만 새 자료로 보존하거나 현재 해시를 감사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,expected in report['sha256'].items():assert digest(ROOT/path)==expected,path  # 원본 기대값은 재계산하지 않는다.
        assert sum(bool(line) and not line.startswith('#') for line in FIXTURE.read_text('utf-8').splitlines())==report['uniqueInputs'];print('냉각 연기 SHA 확인',report['uniqueInputs']);return
    selected=inputs()
    if args.smoke:selected=[next(row for row in selected if row[0]==kind) for kind in ('frame','spawn','cold','wrapper','ctor')]
    oracle=Oracle();rows=[];differences=[]
    for index,row in enumerate(selected):  # 실제 원문 반환만 저장하며 C++는 호출하지 않는다.
        try:output=[oracle.sample(row,precision) for precision in (0x27f,0x37f)]
        except Exception as error:raise RuntimeError(f'연기 입력{index} {row[0]}: {error}') from error
        if output[0]!=output[1]:differences.append(dict(index=index,secondary=output[1]))
        values=[row[0],row[1].hex(),row[2].hex() or '-',row[3].hex(),row[4].hex(),row[5].hex(),*map(str,row[6:])];rows.append('\t'.join(values+output[0]))
    if args.smoke:print('연기 전체5종 smoke',len(rows));return
    with FIXTURE.open('x',encoding='utf-8',newline='\n') as stream:stream.write('# 보호10.78 냉각 연기. 힙/실제 부착/SHP 공급·kill·dirty/선 장치는 명시 경계다.\n'+'\n'.join(rows)+'\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_smoke_effect.py','tools/decomp_fierytail_frame.py','tools/decomp_particle_gradient.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',uniqueInputs=len(rows),returns=dict(oracle.returns),precisionDifferences=differences,inputKinds=dict(Counter(row[0] for row in selected)),nativeCalls=dict(oracle.calls),limits='연기 생성/공통 등록/전체 Frame/Spawn/Draw/색/타이머/난수/소멸을 실제로 실행한다. 힙 성공·Host 부착/SHP 공급·kill/dirty/선 장치는 경계다. vtable/포인터와 미사용 padding을 제외한112BYTE 의미 필드 및 입자/팔레트 전체·난수/효과를 대조한다.128BYTE 외부 객체 확보는 실제 Host/pending 소유권과 구별한다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('연기 고유입력',len(rows),'정밀도 차이',len(differences),'반환',json.dumps(dict(oracle.returns)))

if __name__=='__main__':main()
