#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 비용 표시 생성/래퍼/Idle/삭제의 실제 기계어를 격리 실행해 동결한다."""
import csv
import itertools
import json
import random
import struct
import sys
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, PLAYERS, STACK, STOP, digest, fbits
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

NODE=PLAYERS+0x1000 #원본 이미지/스택과 겹치지 않는 실제0xb8 객체 입력이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/floating-sp-x86.tsv' #C++ 대조 전에 동결할 기계어 관측이다.
REPORT=ROOT/'cpppj/floating-sp.json' #관측 범위와 바이트 해시를 보존한다.


def floatword(word):
    """명시 IEEE754 입력을 십진 반올림 없이 복원한다."""
    return struct.unpack('<f',struct.pack('<I',word))[0]


class Oracle(MemoryOracle):
    """계산 전체를 실행하며 기존 TextGump/할당/실제 가상 효과만 관측 경계로 둔다."""
    def __init__(self):
        """현재 PC 정밀 산출물의 정상 몸체와 원본 소멸 점프만 허용한다."""
        super().__init__()
        self.mu.mem_map(0,0x1000) #SEH의 fs:0을 OS 없이 독립 메모리로 제공한다.
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.allowed=[(0x4d3080,0x4d3085)]
        #생성/Idle/래퍼/삭제와 실제 TextGump 소멸 몸체의 검토 범위다.
        for entry in (0x43db70,0x43dc50,0x43dd20,0x43e620,0x4bc790):
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def snapshot(self):
        """소유 문자열 토큰을 제외한 실제 필드와 패딩 canary를 읽는다."""
        return ','.join(str(self.words(NODE+offset,1)[0]) for offset in (0,0x30,0x4c,0x50,0xa4,0xa8,0xac))+','+bytes(self.mu.mem_read(NODE+0xb0,8)).hex()

    def instruction(self,mu,address,size,data):
        """모든 경계의 호출 순서/현재 필드를 보존하며 Idle의 두 Redraw를 구별한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4e4391:
            assert args[0]==0xb8;self.events.append('N');self.ret(NODE)
        elif address==0x4bb170:
            assert ecx==NODE and args==tuple([0]*4)
            self.put(NODE,0x5144d4);self.events.append('T');self.ret(NODE,16)
        elif address==0x4bb240:
            self.events.append('C');self.ret()
        elif address==0x4a89c0:
            assert self.string(args[1])==b'~3~E~y%d~[IfortGump.3]'
            text='~3~E~y%d~[IfortGump.3]'%struct.unpack('<i',struct.pack('<I',args[2]))[0]
            mu.mem_write(args[0],text.encode()+b'\0');self.ret(args[0])
        elif address==0x4bc800:
            self.events.append('S,'+self.string(args[0]).decode());self.ret(purge=4)
        elif address==0x4bb310:
            assert args[:2]==(0,0);self.events.append('F');self.ret(purge=8)
        elif address==0x465330:
            self.events.append('I,'+str(args[0]));self.ret(purge=4)
        elif address==0x466480:
            self.events.append('K');self.ret()
        elif address==0x464a50:
            assert args[0]==0;self.events.append('R,'+self.snapshot())
            if self.mutate and len(self.events)==1:
                self.put(NODE+0x50,fbits(7.5));self.put(NODE+0xa4,fbits(-1.25));self.put(NODE+0xa8,fbits(3.0))
            self.ret(purge=4)
        elif address==0x466750:
            self.events.append('A,'+','.join(map(str,args)));self.put(NODE+0x4c,args[0]);self.put(NODE+0x50,args[1]);self.ret(purge=16)
        elif address==0x464540:
            self.events.append('O,'+str(args[0]));self.ret(purge=4)
        elif address==0x4756b0:
            assert ecx==NODE+0xa0;self.events.append('C');self.ret()
        elif address==0x464420:
            self.events.append('B');self.ret()
        elif address==0x4e4354:
            assert args[0]==NODE;self.events.append('D');self.ret()
        else:
            if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('미허용 명령 %08x'%address)
            self.instructions+=1

    def run(self,section,values,precision):
        """입력 필드/시계만 준비하고 SEH/스택/비휘발 레지스터/패딩 보존을 확인한다."""
        self.events=[];self.mutate=False
        self.mu.mem_write(NODE-16,b'\xa7'*(0xb8+32));self.put(NODE,0x50688c)
        self.put(0x563e38,0x12345678);self.mu.mem_write(0x55b4d0,struct.pack('<d',3.25))
        args=[];entry=0x43dc50;purge=0
        if section=='idle':
            x,y,v,a,stamp,now,w,h,mutate=values
            #실제 x/y/속도/가속도는 float 비트, 두 시계는 double의 정확한 입력이다.
            for offset,value in ((0x4c,x),(0x50,y),(0xa4,v),(0xa8,a)):self.put(NODE+offset,value)
            self.mu.mem_write(NODE+0xb0,struct.pack('<d',stamp));self.mu.mem_write(0x55b4d0,struct.pack('<d',now))
            self.put(0x5c78f0,w);self.put(0x5c78f4,h);self.mutate=mutate
        elif section=='construct':entry=0x43db70;args=values;purge=8
        elif section=='show':entry=0x43dd20;args=values
        elif section=='delete':entry=0x43e620;args=values;purge=4
        else:raise ValueError(section)
        saved={UC_X86_REG_EBX:0x12345,UC_X86_REG_ESI:0x23456,UC_X86_REG_EDI:0x34567,UC_X86_REG_EBP:0x45678}
        #비휘발 레지스터와 빈 x87 스택은 호출 전후 그대로여야 한다.
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_FPCW,precision);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.mu.reg_write(UC_X86_REG_ECX,NODE);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*[x&0xffffffff for x in args]))
        seh=bytes(self.mu.mem_read(0,16));self.mu.emu_start(entry,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        assert bytes(self.mu.mem_read(NODE-16,16))==bytes(self.mu.mem_read(NODE+0xb8,16))==b'\xa7'*16
        assert bytes(self.mu.mem_read(0,16))==seh
        return [self.snapshot(),';'.join(self.events)]


def verify():
    """동결 원문과 모든 근거 해시를 읽기 전용으로 확인한다."""
    report=json.loads(REPORT.read_text('utf-8'))
    #기존 원문을 새 관측으로 덮어쓰지 않는다.
    for path,want in report['sha256'].items():assert digest(ROOT/path)==want,path
    rows=list(csv.DictReader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))
    assert len(rows)==report['uniqueInputs']
    assert sum(row['state53']!=row['state64'] or row['effects53']!=row['effects64'] for row in rows)==report['precisionDifferentInputs']
    print('Floating SP:',report['uniqueInputs'],'inputs,',report['normalReturns'],'normal returns, precision difference',report['precisionDifferentInputs'])


def observe():
    """경계/특수 float/다양한 시계/가상 재진입을 원본으로 관측하고 C++ 구현 전에 동결한다."""
    oracle=Oracle();cases=[]
    #생성 및 화면/월드 부착 래퍼의 모든 외부 인자는 원문으로 기록한다.
    for cost,accel in itertools.product((0,800,-25,-2147483648,2147483647),(fbits(-2),fbits(0.5))):cases.append(('construct',[cost,accel]))
    for cost,x,y,reference in ((800,320,240,0),(-25,-2,767,0),(0,2147483647,-2147483648,0),(800,320,240,15000)):cases.append(('show',[cost,x,y,reference]))
    for flags in (0,1,0x100,0x101):cases.append(('delete',[flags]))
    #두 좌표의 포함 경계, NaN/무한대, 음의0 및 0/역행 시계를 독립 입력으로 넣는다.
    for x,y,v,stamp,now in itertools.product((-1,0,640,float('nan')),(0,480,481),(-1,0,1),(0.0,3.25),(3.25,3.5)):
        cases.append(('idle',[fbits(x),fbits(y),fbits(v),fbits(-2),stamp,now,640,480,0]))
    randomizer=random.Random(0x43dc50)
    #실제 창 안의 서로 다른 유한 float와 dyadic 시간 입력을 섞는다.
    for _ in range(256):
        cases.append(('idle',[fbits(randomizer.uniform(0,640)),fbits(randomizer.uniform(0,480)),fbits(randomizer.uniform(-4,4)),fbits(randomizer.uniform(-4,4)),3.25,randomizer.randrange(-1024,2048)/1024,640,480,0]))
    for x,y,v in ((float('inf'),1,0),(1,float('inf'),0),(1,1,float('-inf')),(1,1,float('nan')),(-0.0,-0.0,-0.0),(640,480,0),(641,1,0)):
        cases.append(('idle',[fbits(x),fbits(y),fbits(v),fbits(-2),3.25,3.5,640,480,0]))
    cases.append(('idle',[fbits(10),fbits(12),fbits(-3),fbits(-2),3.25,3.5,640,480,1]))
    rows=[]
    #기계어만 두 정밀도에서 실행하며 C++ 기대값을 생성에 사용하지 않는다.
    for section,values in cases:
        a=oracle.run(section,values,0x027f);b=oracle.run(section,values,0x037f)
        rows.append([section,','.join(map(str,values))]+a+b)
    with FIXTURE.open('w',encoding='utf-8',newline='') as fp:
        writer=csv.writer(fp,delimiter='\t',lineterminator='\n');writer.writerow(['section','input','state53','effects53','state64','effects64']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','tools/decomp_floating_sp.py','tools/decomp_priestmenu.py','extracted/refined/originals/functions.tsv',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    report=dict(edition='10.78',entries=['0043db70','0043dc50','0043dd20','0043e620','004d3080','004bc790'],scope='실제 생성/래퍼/Idle/파생 scalar·TextGump 소멸 전체. Text 생성/문자열/측정/기본소멸·할당·가상 재그리기/부착/객체참조·idle등록/Kill은 효과 관측 경계. 원본 창 실행 없음.',uniqueInputs=len(rows),normalReturns=len(rows)*2,instructions=oracle.instructions,precisionDifferentInputs=sum(row[2:4]!=row[4:6] for row in rows),sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');verify()


if __name__=='__main__':
    if '--verify' in sys.argv:verify()
    else:observe()
