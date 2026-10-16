#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 비행 Frame/맨 오 워 수명·프레임/표적 해제·통지를 실제10.78 명령으로 관찰한다."""
import csv
import json
import socket
import struct
import sys
from collections import Counter
from datetime import datetime
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, HEAP, PLAYERS, TABLE, IMAGE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

# 원문 객체7개/112BYTE 프로세스/48BYTE 훨리베이스와 실제 TYPE 표의 격리 위치다.
POOL,FIELDS,SUN,TYPES=PLAYERS,HEAP,HEAP+0x1000,TABLE
IDS=(50,51,60,61,62,63,70)
TYPE_IDS=(72,73,80,138,144)


def bits(value):
    """명시 float 입력의 비트만 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class Oracle(BaseOracle):
    """검토한 제어/산술/타이머/좌표는 원문, 큰 가상/검색/의존/장치는 선언된 현재 입력이다."""
    def __init__(self,control=0x027f):
        """범위 표와 직접 검토한 미내보내기 프레임/지연 getter만 허용한다."""
        super().__init__();self.control=control;self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x45c9d0,0x45d630,0x45bf00,0x45b800,0x45b6f0,0x45b760,0x45b7a0,0x41bfb0,0x425e00,0x40e800,0x40e850,0x40ea40,0x40ead0,0x40eab0,0x41cfc0,0x41d0d0,0x45fa80,0x4e49c0,0x4ac550,0x49a840,0x49ace0,0x4adea0,0x4ad730,0x49a900,0x45c440):  # 실제 선택한 작은 직접 helper다.
            for part in functions[address]['ranges'].split(';'):  # 불연속 구간도 전부 보존한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.allowed.extend([(0x45bf50,0x45c01e),(0x45c2e0,0x45c2e7)])
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """원문 CPU 쓰기는 선언한 현재 객체/필드/TYPE stamp/스택만 허용한다."""
        ranges=[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]+[(FIELDS,FIELDS+112),(SUN,SUN+48),(STACK-8192,STACK+256),(0,4)]
        ranges.extend((TYPES+typ*500+0x1f0,TYPES+typ*500+0x1f4) for typ in TYPE_IDS)
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'비행 Frame 선택 밖 쓰기 {address:x}/{size}')

    def now(self,value):
        """효과 뒤 현재 시각 변경 입력이다."""
        self.mu.mem_write(0x55b4d0,struct.pack('<d',value))

    def event(self,value):
        """원문이 실제 선택한 외부 효과 순서만 기록한다."""
        self.events.append(value)

    def instruction(self,mu,address,size,data):
        """큰 하위의 인수/반환/현재 변경만 공급하고 제어 결과는 실제 명령으로 구한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,10);ecx=mu.reg_read(UC_X86_REG_ECX);sid=(ecx-POOL)//50;parent=POOL+50*50
        if address==0x4e0620:self.event('Assert,'+str(args[2]));self.ret();return
        if address==0x41be80:self.event('Robust');self.ret(self.robust,purge=4);return
        if address==0x4ac140:self.event(f'Sid,{args[0]},{args[1]}');self.ret(1 if 0<args[0]<65535 else 0);return
        if address==0x4ad730:self.event('Check,'+str(sid))
        if address==0x4af110:self.event('Count,'+str(args[0]));self.ret(16);return
        if address==0x472690:
            self.event('Launch,'+','.join(map(str,args[:3])))
            if self.mutation==2:self.put(parent+10,138,1);self.put(parent+36,9);self.put(FIELDS+56,51);self.now(200)
            self.ret();return
        if address==0x4a9c30:
            assert args[3:5]==(0,0);self.event('Sound,'+','.join(map(str,args[:3])))
            if self.mutation==11:self.put(parent+36,9);self.put(FIELDS+56,51)
            self.ret();return
        if address==0x4ae780:
            assert args[2]==0;self.event(f'Remove,{args[0]},{args[1]}')
            if self.mutation==3:self.put(FIELDS+12,60,2);self.put(FIELDS+52,71);self.put(FIELDS+8,2,1);self.now(200)
            self.ret();return
        if address==0x4b15f0:
            assert args[1]==FIELDS+48  # 원문은 Form 번호 대신 현재 inner process 포인터를 전달한다.
            self.event(f'Add,{args[0]},{self.words(args[1]+4)[0]}');self.ret();return  # 동일 의존 저장소의 Form 입력으로 정규화한다.
        if address==0x41c010:
            self.event('RefreshSun' if ecx==SUN+32 else 'Refresh')
            if self.mutation==4:self.put(FIELDS+12,60,2);self.put(FIELDS+8,0,1);self.now(200)
            self.ret();return
        if address==0x4d5000:
            self.event('Time,'+str(sid))
            if self.mutation==12:self.put(FIELDS+20,bits(4));self.put(FIELDS+12,61,2);self.put(0x540bc4,0)
            self.ret();return
        if address==0x4d4f00:
            self.event(f'Hit,{sid},{args[0]}')
            if self.mutation==9:self.put(FIELDS+80,7);self.put(parent+36,6);self.now(200)
            self.ret(purge=4);return
        if address==0x45bb80:self.event(f'Scatter,{args[1]},{args[2]}');self.put(args[0],bits(0));self.put(args[0]+4,bits(0));self.ret();return
        if address==0x4afe50:
            self.event(f'Unpop,{sid},{args[0]}')
            if self.mutation==1:self.put(parent+14,bits(71.25));self.put(parent+18,bits(72.5));self.put(FIELDS+56,51);self.now(200)
            self.ret(purge=4);return
        if address==0x4b02d0:self.event('Pop,'+str(sid)+','+','.join(map(str,args[:3])));self.put(ecx+14,args[0]);self.put(ecx+18,args[1]);self.ret(purge=12);return
        if address==0x41c0d0:self.event(f'Repop,{sid},{args[0]}');self.ret(purge=4);return
        if address==0x4af780:self.event(f'Destroy,{sid},{args[0]}');self.ret(purge=4);return
        if address==0x4437f0:self.event('Cell,'+str(sid));self.ret();return
        if address==0x41c100:self.event('Home,'+str(sid)+','+','.join(map(str,args[:2])));self.ret(purge=8);return
        if address==0x45bc30:self.event('Finish');self.ret();return
        if address==0x45c2e0:self.event('Delay')
        if address==0x45ae60:self.query=args[0];self.ret(IMAGE,purge=4);return
        if address==0x46af80:
            self.event(f'May,{self.query},{args[8]}')
            if self.mutation==5:self.put(FIELDS+12,0,2);self.put(FIELDS+10,0,2)
            self.ret(self.may);return
        if address==0x45b840:self.event(f'Closer,{args[0]},{args[1]}');self.ret(self.closer);return
        if address==0x45b9c0:self.event('Capacity,'+str(args[0]));self.ret(3);return
        if address==0x45c4b0:
            self.event('Find')
            if self.mutation==6:self.put(FIELDS+12,60,2)
            if self.mutation==7:self.now(200)
            self.ret();return
        if address==0x4b1a90:
            if args[3]:self.event(f'RemoveSound,{args[0]},{args[1]}');self.ret();return
            self.event(f'Sun,{args[0]},{args[1]}');self.ret(80);return
        if address==0x4ae2d0:self.ret(SUN+32);return
        if address==0x4e2e30:
            pointer=args[0];self.event('Command,'+str(self.words(pointer+4)[0])+','+str(self.words(pointer+12)[0]))
            if self.mutation==10:self.put(FIELDS+12,0,2)
            self.ret();return
        if address==0x40a890:self.mu.mem_write(ecx,bytes(20));self.ret(ecx);return
        if address==0x45fa80 and self.mutation==8:self.put(FIELDS+10,0,2);self.put(FIELDS+12,61,2)
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'비행 Frame 미검토 명령 {address:x}')

    def run(self,case):
        """고유 입력을 설치하고 모든 결과는 실제 정상 반환 뒤 읽는다."""
        kind,values=case;self.events=[];self.mutation=0;self.may=1;self.closer=0;self.robust=1;self.query=0
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x54117c,73);self.put(0x541280,138);self.put(0x541298,144);self.put(0x5410f0,32);self.put(0x54112c,47);self.put(0x540bc0,1);self.put(0x540bc4,1);self.put(0x5e4794,0);self.put(0x55b4a8,7);self.put(0,0x12340000);self.now(100)
        for typ in TYPE_IDS:  # 같은 불변 TYPE와 mutable 소리 stamp 입력이다.
            self.mu.mem_write(TYPES+typ*500,bytes(500));self.put(TYPES+typ*500+0x44,bits(100));self.put(TYPES+typ*500+0x70,400+typ);self.put(TYPES+typ*500+0x1d4,2);self.put(TYPES+typ*500+0x1d8,2)
        for sid in IDS:  # 다른 부모/표적/홈/free/dead/Form의 전체 raw 기록이다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,0x50a388,0,0,0,144 if sid<60 else 72 if sid==61 else 80,1 if sid==62 else 2 if sid==63 else 0,50 if sid==61 else 17,50.25 if sid<60 else 60.25,50.5 if sid<60 else 60.5);raw[34]=1;raw[40]=0;struct.pack_into('<I',raw,36,0);struct.pack_into('<I',raw,42,50);struct.pack_into('<I',raw,46,50);self.mu.mem_write(POOL+sid*50,bytes(raw))
        for sid in (50,51):self.put(POOL+sid*50+6,70,2)  # 실제 Parent getter가 요구하는 부착 head다.
        self.put(POOL+70*50+10,31,1);self.put(POOL+70*50+11,8,1);self.put(POOL+51*50+36,3)
        fields=bytearray(b'\xa5'*112);struct.pack_into('<I',fields,0,0x509f60);fields[8]=0x10;struct.pack_into('<HH',fields,10,61,0);struct.pack_into('<ff',fields,16,.75,.5);struct.pack_into('<dd',fields,32,99.75,0);struct.pack_into('<III',fields,48,0x509f40,70,50);struct.pack_into('<ddIII',fields,64,0,101,0,1,0);struct.pack_into('<dd',fields,96,101,160)
        sun=bytearray(b'\x5a'*48);struct.pack_into('<I',sun,24,8);self.mu.mem_write(SUN,bytes(sun));entry,receiver,args,purge=0x45c9d0,FIELDS+48,[],0
        if kind=='I':
            rain,mode,pattern=values;fields=bytearray([pattern]*112);self.now((0,100.25,-5.25)[mode]);entry=0x45bf00 if rain else 0x45b800;receiver=FIELDS
        elif kind=='A':
            frame,due,target,attack,stamp,self.mutation=values;self.put(POOL+50*50+36,frame);struct.pack_into('<d',fields,96,(99,100,101)[due]);struct.pack_into('<H',fields,12,target);struct.pack_into('<I',fields,80,attack);self.put(TYPES+144*500+0x1f0,stamp);entry=0x45bf50;receiver=FIELDS
        elif kind in ('C','N','D'):
            server,boss,target,home,parentType,homeState,message,info,self.mutation=values;self.put(0x540bc0,server);self.put(0x540bc4,boss);self.put(POOL+50*50+10,parentType,1);self.put(POOL+61*50+11,homeState,1);struct.pack_into('<HH',fields,10,home,target)
            if kind=='C':entry=0x45b6f0;receiver=FIELDS
            if kind=='N':entry=0x45b760;args=[message,777,info];purge=12
            if kind=='D':entry=0x45b7a0;args=[777];purge=4
        else:
            wrapper,server,boss,state,extra,target,place,elapsed,initialized,attack,landing,returnMode,ready,self.may,self.closer,animation,self.mutation=values;entry=0x45d630 if wrapper else 0x45c9d0
            self.put(0x540bc0,server);self.put(0x540bc4,boss);self.put(POOL+50*50+11,state,1);self.put(POOL+50*50+40,extra,1);struct.pack_into('<H',fields,12,target);struct.pack_into('<I',fields,84,initialized);struct.pack_into('<I',fields,80,attack);struct.pack_into('<I',fields,88,landing)
            struct.pack_into('<d',fields,32,100-(0,.025,.25,1,-.25)[elapsed]);struct.pack_into('<d',fields,40,(0,99,101)[returnMode]);struct.pack_into('<d',fields,64,(99,100,101)[ready]);struct.pack_into('<d',fields,96,(99,101)[animation])
            point=((60.25,60.5),(50.25,51.5),(50.5,51.5),(50.75,51.5),(300,300))[place]
            for sid in (60,61,62,63):self.mu.mem_write(POOL+sid*50+14,struct.pack('<ff',*point))  # 각 후보의 기하 입력은 독립된 동일 좌표다.
            if landing==2:self.put(SUN+24,9)
            if returnMode==1 or target==61:self.put(POOL+50*50+10,73,1)
            if self.mutation in (13,14,15):struct.pack_into('<d',fields,104,{13:99,14:100,15:101}[self.mutation])
            if self.mutation==16:self.put(POOL+61*50+12,51,2)
            if self.mutation in (17,18,19):self.put(POOL+70*50+11,{17:12,18:10,19:9}[self.mutation],1)
        self.mu.mem_write(FIELDS,bytes(fields));self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 매 진입의 비휘발 레지스터 입력이다.
        self.mu.reg_write(UC_X86_REG_ECX,receiver);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.emu_start(entry,STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 원문 정상 반환 보존이다.
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control and ((self.mu.reg_read(UC_X86_REG_FPSW)>>11)&7)==0 and self.words(0)[0]==0x12340000
        objects=b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex();globals_image=b''.join(bytes(self.mu.mem_read(a,n)) for a,n in [(0x540bc0,4),(0x540bc4,4),(0x55b4d0,8),(0x55b4a8,4)]).hex();stamps=b''.join(bytes(self.mu.mem_read(TYPES+typ*500+0x1f0,4)) for typ in TYPE_IDS).hex()
        result=str(struct.unpack('<i',struct.pack('<I',self.mu.reg_read(UC_X86_REG_EAX)))[0]) if kind=='A' else '-'
        return [kind,','.join(map(str,values)),bytes(self.mu.mem_read(FIELDS,112)).hex(),objects,globals_image,stamps,bytes(self.mu.mem_read(SUN,48)).hex(),result,';'.join(self.events) or '-']


def cases():
    """초기화/기한/공격/의존/이동과 효과 뒤 변경을 독립 원문 입력으로 만든다."""
    for rain in (0,1):  # 기본/맨 오 워의 서로 다른 초기화다.
        for mode in range(3):  # 0/양수/음수의 현재 시각 입력이다.
            for pattern in (0,0x5a,0xa5):yield 'I',(rain,mode,pattern)  # 초기화가 쓰지 않는 바이트도 대조한다.
    for frame in (0,5,6,7,15):  # 발사6 전후와 마지막 나머지 경계다.
        for due in range(3):  # 다음 애니메이션 기한 전/일치/후다.
            for target in (0,60):  # 실제 표적 존재 여부다.
                for attack in (0,1):  # 실제 공격 상태를 독립 선택한다.
                    for stamp in (0,7):yield 'A',(frame,due,target,attack,stamp,0)  # 같은 타입의 현재 tick 소리를 억제한다.
    for mutation in (2,11):yield 'A',(6,1,60,1,0,mutation)  # 발사/음향 뒤 현재 부모와 프레임이다.
    for kind in ('C','N','D'):  # 해제/통지/부모 사망의 세 실제 진입이다.
        for server in (0,1):  # 통지의 서버 관문이다.
            for boss in (0,1):  # 해제의 별도 boss 진단이다.
                for target in (0,60):  # 표적 없는 자연 반환도 포함한다.
                    for home in (0,61,65535):  # WORD 홈의 두 특별 값과 실제 홈이다.
                        for message in ((0,1,2) if kind=='N' else (0,)):yield kind,(server,boss,target,home,144,0,message,target,0)  # 실제 인수가 없는 진입은 중복 합산하지 않는다.
    for kind in ('C','N'):  # 현재 재진입 쓰기를 해제/통지 양쪽에서 관찰한다.
        for mutation in (3,4):yield kind,(1,1,60,61,144,0,2,17,mutation)  # 틀린info 진단과 효과 뒤 WORD다.
    for typ in (138,144):  # 윈드만 홈에 통지하지 않는 관문이다.
        for state in (1,2,3):yield 'D',(1,1,60,61,typ,state,0,0,0)  # free/dead 홈은 통지하지 않는다.
    base=(0,1,1,0,0,0,0,2,1,0,0,0,2,1,0,1,0)
    for wrapper in (0,1):  # 맨 오 워 wrapper와 직접 공통 진입을 구별한다.
        for server in (0,1):  # 후반 서버 처리를 독립 선택한다.
            for boss in (0,1):  # 사망/수명 삭제 권한이다.
                for state in (0,1,2,3):  # 진입 시 live/free/dead 조합이다.
                    for extra in (0,1,0x20,0x21):
                        row=list(base);row[:5]=[wrapper,server,boss,state,extra];yield 'F',tuple(row)  # abstract/사망과 boss를 분리한다.
    for target in (0,60,61,62,63):  # 표적 없음/적/홈/free/dead 대상이다.
        for place in range(5):  # 먼 곳/일치/근접/반경일치/클램프다.
            for elapsed in (0,1,2,3,4):  # 0/정상/큰/음수 경과다.
                for attack in (0,1):
                    row=list(base);row[5:10]=[target,place,elapsed,1,attack];yield 'F',tuple(row)  # 이동/착륙/비전진 시간을 전수 비교한다.
    for target in (0,60,61):  # 유효 표적/홈과 표적 없는 갱신이다.
        for ready in (0,1,2):  # 검색 기한 전/일치/후다.
            for may in (0,1):  # 실제 MayTarget 외부 입력이다.
                for closer in (0,3,4):
                    row=list(base);row[5]=target;row[12:15]=[ready,may,closer];row[6]=2;yield 'F',tuple(row)  # 자리/거리/갱신 관문이다.
    for initialized in (0,1):  # 최초 초기화 여부다.
        for returnMode in (0,1,2):  # 복귀 타이머 없음/만료/미만료다.
            for landing in (0,1,2):
                row=list(base);row[0]=1;row[5]=61;row[6]=2;row[8]=initialized;row[10]=landing;row[11]=returnMode;yield 'F',tuple(row)  # 최초와 홈 착륙 단계다.
    for mutation in range(1,13):  # 명시 외부 효과마다 현재 읽기를 변경한다.
        row=list(base);row[5]=60;row[6]=2 if mutation==9 else 0;row[9]=1;row[15]=0;row[16]=mutation
        if mutation in (3,4):row[13]=0
        if mutation in (6,7):row[5]=0;row[12]=1
        if mutation==10:row[11]=1
        yield 'F',tuple(row)  # 각 외부 효과 뒤 현재 읽기를 비교한다.
    for server in (0,1):  # 수명 삭제는 서버가 아니라 boss 관문이다.
        for boss in (0,1):
            for mutation in (13,14,15):
                row=list(base);row[0:3]=[1,server,boss];row[16]=mutation;yield 'F',tuple(row)  # 수명 기한 전/일치/후다.
    for mutation in (16,17,18,19):  # 착륙 홈 불일치와 실제 checkForms의 종속 상태다.
        row=list(base);row[5]=61 if mutation==16 else 60;row[6]=2;row[16]=mutation;yield 'F',tuple(row)


def main():
    """원문 자료를 생성하거나 보존 SHA와 고유 입력/정상 반환 수를 감사한다."""
    fixture=ROOT/'cpppj/tests/fixtures/flyer-frame-x86.tsv';report=ROOT/'cpppj/flyer-frame.json'
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for name,value in facts['sha256'].items():assert digest(ROOT/name)==value,name  # 보호 원본과 현재 검증 자료를 확인한다.
        rows=list(csv.reader(fixture.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({(r[0],r[1]) for r in rows});print('Flyer Frame native/SHA:',len(rows));return
    inputs=list(dict.fromkeys(cases()));oracle=Oracle();rows=[oracle.run(case) for case in inputs];alternate=Oracle(0x037f);other=[alternate.run(case) for case in inputs]
    differing=[i for i,(a,b) in enumerate(zip(rows,other)) if a!=b]
    if differing:
        with (ROOT/'extracted/goal-flyer-frame-20261016-extended.tsv').open('w',encoding='utf-8',newline='') as stream:csv.writer(stream,delimiter='\t',lineterminator='\n').writerows(other)
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['kind','input','fields','objects','globals','stamps','sun','result','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_flyer_frame.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawFlyerFrame.h','cpppj/src/o/RawFlyerFrame.cpp','cpppj/tests/FlyerFrameTests.cpp','cpppj/tests/fixtures/flyer-frame-x86.tsv']
    facts={'host':socket.gethostname(),'date':datetime.now().date().isoformat(),'last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'scopes':dict(Counter(r[0] for r in rows)),'precision_replays':[0x027f,0x037f],'normal_returns':len(rows)*2,'extended_differences':len(differing),'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'limits':'실제 공통 제어/이동 산술/타이머/맨 오 워 수명·프레임/홈·표적 제어를 실행한다. 큰 검색/표적 검증/포위/의존/Host·훨리베이스 조회·전송/공간/피해·무기/소리/명령/개발자진단은 선언한 경계다. 중심/offset/거리/클램프/ftol은 실제 명령이다. 전체112BYTE/7객체350BYTE/20BYTE전역/5TYPE stamp/48BYTE Sun/호출/반환/스택/비휘발/SEH/FPU/쓰기guard를 확인한다. 두 x87 재생을 고유 입력에 중복 합산하지 않는다.','sha256':{name:digest(ROOT/name) for name in paths}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Flyer Frame native:',len(rows),facts['scopes'],'extended differences',len(differing))


if __name__=='__main__':main()
