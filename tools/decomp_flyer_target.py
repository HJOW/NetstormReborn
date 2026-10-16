#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""비행 표적 선택·의존 경쟁·포위 위치의 실제10.78 명령과 전체 결과를 관찰한다."""
import csv
import json
import socket
import struct
import sys
from collections import Counter
from datetime import datetime
from decomp_local_mission_transport import Oracle as BaseOracle,ROOT,HEAP,PLAYERS,TABLE,STACK,STOP,digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPTAG,UC_X86_REG_FPSW

# 실제 풀과 분리한112BYTE 프로세스·현재 TYPE·프로세스 조회 입력의 위치다.
POOL,FIELDS,TYPES,PROCESSES=PLAYERS,HEAP,TABLE,HEAP+0x10000
IDS=tuple(range(50,54))+tuple(range(60,64))+tuple(range(70,74))+tuple(range(80,85))+tuple(range(90,95))
TYPE_IDS=(1,6,9,31,63,73,80,81,82,83,144)


def bits(value):
    """명시 float 입력의 비트를 만들며 제품 결과는 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class Oracle(BaseOracle):
    """선택·산술·의존 finder/상대 getter는 실제 명령이며 큰 공간·Host·명령은 입력 경계다."""
    def __init__(self,control=0x027f):
        """현재 전체 정련 함수 범위와 직접 검토한 짧은 getter/필터만 실행한다."""
        super().__init__();self.control=control;self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            funcs={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        entries=(0x45c4b0,0x45b840,0x45b9c0,0x45ba10,0x45bb80,0x453720,0x4b22d0,0x4b1b80,0x4b1bd0,0x4b2390,0x4ae510,0x41bfb0,0x49a840,0x49ace0,0x4adea0,0x49a900,0x4ad6e0,0x4ac200,0x41cfc0,0x41d0d0,0x45c440,0x45fa80,0x4e49c0,0x41def0,0x41d770,0x41d7a0,0x40e800,0x40e880,0x40e8f0,0x40ead0,0x4ad730)
        for entry in entries:  # 검토한 실제 함수의 불연속 범위를 전부 보존한다.
            for part in funcs[entry]['ranges'].split(';'):
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.allowed.extend(((0x453740,0x453748),(0x4ad010,0x4ad015),(0x45c2e0,0x45c2e7)))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """원문 CPU는 선언한 원문 필드·객체·스택만 쓰도록 제한한다."""
        ranges=[(FIELDS,FIELDS+112),(STACK-8192,STACK+256),(0,4)]+[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]
        if not any(lo<=address and address+size<=hi for lo,hi in ranges):raise RuntimeError(f'비행 표적 미선언 쓰기 {address:x}/{size}')

    def event(self,value):
        """실제 원문이 선택한 큰 경계의 인수/순서만 기록한다."""
        self.events.append(value)

    def now(self,value):
        """효과 뒤 현재 시각의 명시 변경 입력이다."""
        self.mu.mem_write(0x55b4d0,struct.pack('<d',value))

    def mutation(self,where):
        """독립 외부 효과가 현재 입력 저장소를 바꾸는 경우를 공급한다."""
        if where=='Offset' and self.mut==1:self.put(FIELDS+56,51);self.put(POOL+50*50+14,bits(71.25));self.put(POOL+50*50+18,bits(72.5))
        if where=='May' and self.mut==2:self.put(FIELDS+10,0,2);self.put(FIELDS+56,51);self.put(POOL+50*50+10,73,1);self.put(TYPES+80*500+0xec,0x120000)
        if where=='Begin' and self.mut==3:self.put(0x54117c,144)
        if where=='Next' and self.mut==4:self.now(200);self.put(POOL+62*50+14,bits(49.25))
        if where=='Process' and self.mut==5:self.put(POOL+51*50+14,bits(49.75))
        if where=='Watch' and self.mut==6:self.put(POOL+82*50+4,0,2);self.put(POOL+71*50+16,52,2);self.put(PROCESSES+71*16+8,52)
        if where=='Command' and self.mut==7:self.put(FIELDS+12,0,2);self.put(FIELDS+56,51);self.now(200)
        if where=='Delay' and self.mut==8:self.now(200);self.mu.mem_write(FIELDS+40,struct.pack('<d',777))
        if where=='Check' and self.mut==9:self.put(POOL+50*50+42,17);self.put(POOL+50*50+46,18)

    def instruction(self,mu,address,size,data):
        """검색 후보·기존Host·명령만 대체하며 실제 제어와 수치 결과를 대신 계산하지 않는다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,10);sid=(ecx-POOL)//50
        if address==0x4e0620:self.event('Assert,'+str(args[2]));self.ret();return
        if address==0x4ad730:self.event('Check,'+str(sid));self.mutation('Check')
        if address==0x45fa80:self.event('Offset,'+str(sid));self.mutation('Offset')
        if address==0x4b16d0:
            self.finder=ecx;self.index=0;self.event('Begin,'+','.join(str(struct.unpack('<i',struct.pack('<I',v))[0]) for v in args[:4])+','+str(self.words(ecx+56)[0]));self.mutation('Begin');self.put(ecx+52,self.candidates[0] if self.candidates else 0);self.ret(purge=16);return
        if address==0x4b1810:
            self.event('Next');self.mutation('Next');self.index+=1;self.put(ecx+52,self.candidates[self.index] if self.index<len(self.candidates) else 0);self.ret();return
        if address==0x46af80:
            self.event('May,'+str(self.words(FIELDS+56)[0])+','+str(args[8]));self.mutation('May');self.ret(1 if self.may & (1<<(args[8]-60)) else 0);return
        if address==0x4d4d00:self.ret(0);return
        if address==0x4ae510:self.event('Watch,'+str(sid));self.mutation('Watch')
        if address==0x4ae2d0:self.event('Process,'+str(sid));self.mutation('Process');self.ret(PROCESSES+sid*16);return
        if address==0x45c2e0:self.event('Delay');self.mutation('Delay')
        if address==0x40a890:self.mu.mem_write(ecx,bytes(20));self.ret(ecx);return
        if address==0x4e2e30:self.event('Command,'+str(self.words(args[0]+4)[0])+','+str(self.words(args[0]+12)[0]));self.mutation('Command');self.ret();return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'비행 표적 미검토 명령 {address:x}')

    def run(self,case):
        """명시 raw 입력을 설치하고 원본 정상 반환 뒤 전체 바이트/반환/효과를 읽는다."""
        kind,values=case;self.events=[];self.mut=0;self.may=15;self.candidates=[60,61,62,63];self.index=0
        for addr,value in ((0x5c8464,POOL),(0x5c847c,128),(0x59ab20,TYPES),(0x541348,188),(0x540bc0,1),(0x540bc4,1),(0x5e4794,0),(0x594fc8,0),(0x541074,1),(0x54116c,63),(0x54117c,73),(0,0x12340000)):self.put(addr,value)  # 현재 전역 입력이며 판정 결과가 아니다.
        self.now(100)
        for typ in TYPE_IDS:  # 현재 TYPE의 실제 조회 필드만 공급한다.
            self.mu.mem_write(TYPES+typ*500,bytes(500));self.put(TYPES+typ*500+0x44,bits(30));self.put(TYPES+typ*500+0xec,0x100000 if typ in (73,144) else 0);self.put(TYPES+typ*500+0x1d4,2);self.put(TYPES+typ*500+0x1d8,2)
        for sid in IDS:  # 모든 객체를 같은50BYTE 이미지로 준비한다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBB',raw,0,0x50a388,0,0,0,144,0);struct.pack_into('<ff',raw,14,50.25,50.5);raw[34]=1;raw[40]=0;struct.pack_into('<I',raw,36,0);struct.pack_into('<II',raw,42,0,0);self.mu.mem_write(POOL+sid*50,bytes(raw))
        for index,sid in enumerate(range(50,54)):  # 각 프로세스와 동일 부모 연결이다.
            self.put(POOL+sid*50+6,70+index,2);self.put(POOL+sid*50+14,bits((50.25,55.25,70.25,58.25)[index]));self.put(POOL+sid*50+10,80 if sid==53 else 144,1)
        for index,sid in enumerate(range(60,64)):  # 서로 다른 후보 TYPE/위치로 독립 필터를 선택한다.
            self.put(POOL+sid*50+10,80+index,1);self.put(POOL+sid*50+14,bits((60.25,55.25,52.25,80.25)[index]));self.put(POOL+sid*50+18,bits(50.5))
        for index,sid in enumerate(range(70,74)):  # 현재 Form과 inner 프로세스의 같은 부모 입력이다.
            self.put(POOL+sid*50,0x512098);self.put(POOL+sid*50+10,31,1);self.put(POOL+sid*50+11,8,1);self.put(POOL+sid*50+16,50+index,2);self.mu.mem_write(PROCESSES+sid*16,struct.pack('<III',0, sid,50+index))
        chain=[84,82,80,81,83]
        for index,sid in enumerate(range(80,85)):  # 원본 감시 쌍은 상대 Form의 부모가 process Form이다.
            self.put(POOL+sid*50,0x512110);self.put(POOL+sid*50+10,1 if sid!=84 else 6,1);self.put(POOL+sid*50+11,8,1);self.put(POOL+sid*50+18,90+index,2);self.put(POOL+(90+index)*50,0x512110);self.put(POOL+(90+index)*50+10,1,1);self.put(POOL+(90+index)*50+11,8,1);self.put(POOL+(90+index)*50+16,70+index if index<4 else 0,2)
        fields=bytearray(b'\xa5'*112);struct.pack_into('<I',fields,0,0x509f60);fields[8]=0x10;struct.pack_into('<HH',fields,10,61,0);struct.pack_into('<d',fields,40,0);struct.pack_into('<III',fields,48,0x509f40,70,50)
        if kind=='F':
            scene,origin,rangebits,parentType,home,self.may,flags,foot,server,checking,returnmode,self.mut=values
            self.put(0x540bc0,server);self.put(0x5e4794,checking);self.put(POOL+50*50+10,parentType,1);self.put(TYPES+parentType*500+0x44,rangebits);struct.pack_into('<H',fields,10,home);struct.pack_into('<d',fields,40,(0,99,101)[returnmode]);self.put(TYPES+80*500+0xec,flags);self.put(TYPES+80*500+0x1d4,foot);self.put(TYPES+80*500+0x1d8,foot)
            anchors=((0,0),(48,49),(0,49),(-2,-3),(17000001,17000003));self.put(POOL+50*50+42,anchors[origin][0]);self.put(POOL+50*50+46,anchors[origin][1])
            if scene==1:self.candidates.reverse()
            if scene==2:self.candidates=[]
            if scene==3:self.candidates=[60,60,62]
            if scene==4:self.put(POOL+61*50+14,bits(60.25))
            if scene==5:self.candidates=[60];self.put(POOL+60*50+14,bits(79.25))
            if scene==6:self.candidates=[61]
            if scene==7:self.candidates=[63]
            if scene==8:self.candidates=[62,60,61]
            entry,args,receiver=0x45c4b0,[],FIELDS
        else:
            target,flyer,footx,footy,tstate,fstate,scene,checking,self.mut=values;self.put(TYPES+80*500+0x1d4,footx);self.put(TYPES+80*500+0x1d8,footy);self.put(POOL+60*50+11,tstate,1);self.put(POOL+50*50+11,fstate,1);self.put(0x5e4794,checking)
            if scene==1:chain.reverse()
            if scene==2:chain=[]
            if scene==3:chain.remove(80)
            if scene==4:chain=[80]
            if scene==5:self.put(POOL+80*50+18,0,2)
            if scene==6:self.put(POOL+71*50+10,9,1)
            if scene==7:self.put(POOL+71*50+10,63,1)
            if scene==8:self.put(POOL+51*50+10,80,1)
            if scene==9:self.put(POOL+51*50+11,2,1)
            if scene==10:self.put(POOL+51*50+14,bits(50.25))
            if scene==11:self.put(POOL+51*50+14,bits(50.250003814697266))
            if scene==12:self.put(0x54116c,31)
            if scene==13:self.put(POOL+51*50+6,0,2)
            if scene==14:self.put(POOL+50*50+10,73,1);self.put(POOL+52*50+10,73,1)
            entry={'C':0x45b840,'P':0x45b9c0,'I':0x45ba10,'S':0x45bb80}[kind];args=[target,flyer] if kind!='P' else [target];receiver=0
            if kind=='S':args=[FIELDS+16,target,flyer]
        self.put(POOL+60*50+6,chain[0] if chain else 0,2)
        for index,sid in enumerate(chain):self.put(POOL+sid*50+4,chain[index+1] if index+1<len(chain) else 0,2)  # 별도 기대 순서 없이 명시 링크를 설치한다.
        self.mu.mem_write(FIELDS,bytes(fields));self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 정상 반환 비휘발 입력이다.
        self.mu.reg_write(UC_X86_REG_ECX,receiver);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.emu_start(entry,STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 실제 반환의 보존 규약이다.
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control and ((self.mu.reg_read(UC_X86_REG_FPSW)>>11)&7)==0 and self.words(0)[0]==0x12340000
        result=str(struct.unpack('<i',struct.pack('<I',self.mu.reg_read(UC_X86_REG_EAX)))[0]) if kind in ('C','P','I') else '-'
        objects=b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS);globals_image=b''.join(bytes(self.mu.mem_read(addr,size)) for addr,size in ((0x540bc0,4),(0x5e4794,4),(0x55b4d0,8),(0x54117c,4),(0x54116c,4),(0x541074,4)))
        return [kind,','.join(map(str,values)),bytes(self.mu.mem_read(FIELDS,112)).hex(),objects.hex(),globals_image.hex(),result,';'.join(self.events) or '-']


def cases():
    """각 실제 분기에 필요한 독립 입력만 생성하며 기대 결과를 계산하지 않는다."""
    for kind in ('C','P','I','S'):  # 네 실제 cdecl 진입을 분리한다.
        base=(60,50,2,2,0,0,0,0,0)
        variants=[]
        for target in (0,60,61):  # 빈 대상과 의존 없는 대상이다.
            for flyer in ((50,) if kind=='P' else (0,50,51,52,53)):
                for footx,footy in ((1,1),(1,2),(2,1),(2,2),(3,3)):
                    for tstate in (0,1,2,3):variants.append((target,flyer,footx,footy,tstate,0,0,0,0))
        if kind!='P':
            for scene in range(15):  # 실제 체인 순서·누락·타입·거리·상한을 독립 선택한다.
                for checking in (0,1):
                    row=list(base);row[6:8]=[scene,checking];variants.append(tuple(row))
            for state in (1,2,3):row=list(base);row[5]=state;variants.append(tuple(row))  # 현재 비행체 상태 관문이다.
            for mutation in (5,6):row=list(base);row[8]=mutation;variants.append(tuple(row))  # getter 경계 뒤 현재 입력과 선저장 next다.
        for values in variants:yield kind,values
    base=(0,0,bits(30),144,61,15,0,2,1,0,0,0)
    for scene in range(9):  # 검색 순서·빈 목록·동률·중복·범위 경계다.
        for origin in range(5):
            for parent in (73,144):
                row=list(base);row[0:2]=[scene,origin];row[3]=parent;yield 'F',tuple(row)
    for length in (0,.25,1,2,10,30,100,-1):  # 사거리0/음수와 정상 양수다.
        for mask in (0,1,2,4,8,15):
            row=list(base);row[2]=bits(length);row[5]=mask;yield 'F',tuple(row)
    for flags in (0,0x10000,0x20000,0x100000,0x200000,0x120000,0x210000):  # 공통/훨리기그 전용 genus 배제다.
        for parent in (73,144):
            for foot in (1,2):row=list(base);row[3]=parent;row[6:8]=[flags,foot];yield 'F',tuple(row)
    for home in (0,60,61,62,63):  # 현재 홈은 초기 snapshot으로 배제한다.
        for server in (0,1):
            for checking in (0,1):row=list(base);row[4]=home;row[8:10]=[server,checking];yield 'F',tuple(row)
    for parent in (73,144):  # 타이머 초기0만 훨리기그 명령을 만든다.
        for timer in (0,1,2):
            for mutation in range(10):row=list(base);row[3]=parent;row[10:12]=[timer,mutation];yield 'F',tuple(row)


def main():
    """실제 명령 결과를 생성하거나 보존 입력·출력·소스 SHA를 감사한다."""
    fixture=ROOT/'cpppj/tests/fixtures/flyer-target-x86.tsv';report=ROOT/'cpppj/flyer-target.json'
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for name,value in facts['sha256'].items():assert digest(ROOT/name)==value,name  # 과거 기대값을 현재 출력으로 조용히 덮지 않는다.
        rows=list(csv.reader(fixture.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({(r[0],r[1]) for r in rows});assert facts['normal_returns']==len(rows)*2 and not facts['extended_differences'];print('Flyer target native/SHA:',len(rows));return
    inputs=list(dict.fromkeys(cases()));oracle=Oracle();rows=[oracle.run(case) for case in inputs];alternate=Oracle(0x037f);other=[alternate.run(case) for case in inputs];differences=[i for i,(a,b) in enumerate(zip(rows,other)) if a!=b]
    if differences:
        with (ROOT/'extracted/goal-flyer-target-20261016-extended.tsv').open('w',encoding='utf-8',newline='') as stream:csv.writer(stream,delimiter='\t',lineterminator='\n').writerows(other)
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['kind','input','fields','objects','globals','result','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_flyer_target.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawFlyerTarget.h','cpppj/src/o/RawFlyerTarget.cpp','cpppj/tests/FlyerTargetTests.cpp','cpppj/tests/fixtures/flyer-target-x86.tsv']
    facts={'host':socket.gethostname(),'date':datetime.now().date().isoformat(),'last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'scopes':dict(Counter(r[0] for r in rows)),'precision_replays':[0x027f,0x037f],'normal_returns':len(rows)*2,'extended_differences':len(differences),'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'limits':'실제 선택·산술·공간 사각형 정수변환·의존 finder/상대 getter를 실행한다. 일반 공간 finder의 후보 순서/MayTarget/Host 프로세스 조회/명령은 선언한 외부 입력이다. 실행된 원문 밖의 값을 C++로 계산해 기대값을 만들지 않는다. 전체112BYTE/22객체1100BYTE/현재 전역28BYTE/반환/호출/스택·비휘발·FPU·SEH·쓰기guard를 확인한다.','sha256':{name:digest(ROOT/name) for name in paths}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Flyer target native:',len(rows),facts['scopes'],'extended differences',len(differences))


if __name__=='__main__':main()
