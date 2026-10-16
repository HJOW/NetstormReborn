#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""맨 오 워 풀 생성/후처리/프레임/상태 writer를 실제10.78 명령으로 격리 관찰한다."""
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

# 실제 게임/OS와 분리한 풀,48BYTE 본체와64BYTE 출력 버퍼다.
POOL, FIELDS, PACKET, VTABLE = PLAYERS, HEAP, IMAGE, TABLE+0xf000
IDS = (50,51,60,70,71)  # 두 부모, 새 플라이어와 두 종속 Form의 전체 기록이다.


class Oracle(BaseOracle):
    """실제 타이머/QA/부모/첫 Process 조회를 포함하며 장치/생성/부착 등 큰 몸체는 명시 입력이다."""
    def __init__(self,control=0x027f):
        """검토한 아홉 함수의 실제 불연속 범위만 실행하고 쓰기를 제한한다."""
        super().__init__();self.allowed=[];self.control=control  # 검토한 반개방 범위와 명시 x87 정밀도다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x45ed10,0x45d810,0x45dde0,0x40eab0,0x4ac220,0x41bfb0,0x45b2a0,0x45d870,0x4b1a90):  # 선택된 부모와 작은 직접 helper다.
            for part in functions[address]['ranges'].split(';'):  # 한 함수의 불연속 범위도 모두 허용한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """객체5개/본체/패킷/스택 이외의 실제 명령 쓰기는 거부한다."""
        ranges=[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]+[(FIELDS,FIELDS+48),(PACKET,PACKET+64),(STACK-4096,STACK+128)]
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'아비어리 선택 밖 쓰기 {address:x}/{size}')

    def now(self,value):
        """현재 게임 시각의 실제 double 전역을 바꾼다."""
        self.mu.mem_write(0x55b4d0,struct.pack('<d',value))

    def instruction(self,mu,address,size,data):
        """원문 분기를 유지하고 선택된 큰 효과의 인자와 현재 변경 입력만 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,5);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4b1a90:
            self.events.append(f'Find,{args[0]},{args[1]}')
            if self.kind=='P' and self.mutation==1:self.put(0x540bc0,0)
            if self.kind=='P' and self.mutation==2:self.put(0x541160,59)
        elif address==0x4e0620:self.events.append('Assert,'+str(args[2]));self.ret();return
        elif address==0x4e4391:self.events.append('Allocate,'+str(args[0]));assert args[0]==48;self.ret(FIELDS if self.allocated else 0);return
        elif address==0x41bd20:
            assert args[2]==0 and ecx in (0,FIELDS+32);self.attached=ecx!=0;self.events.append(f'Attach,{args[0]},{args[1]},{args[3]},{int(self.attached)}');self.ret(purge=16);return
        elif address==0x44bf80:self.events.append('Base,'+str(args[0]));self.ret(purge=4);return
        elif address==0x4acee0:
            sid=(ecx-POOL)//50;frame=struct.unpack('<i',struct.pack('<I',args[0]))[0];self.events.append(f'Frame,{sid},{frame}');self.put(ecx+36,args[0]);self.frame_calls+=1
            if self.frame_calls==1 and self.mutation==1:self.put(FIELDS+40,51);self.now(200)
            if self.frame_calls==1 and self.mutation==5:self.put(FIELDS+24,9)
            self.ret(purge=8);return
        elif address==0x4a9d70:
            assert self.string(args[2])==b'manOWarBuild.wav' and args[3]==0;self.events.append(f'Sound,{args[0]},{args[1]}')
            if self.mutation==2:self.put(FIELDS+24,9);self.put(FIELDS+44,10)
            self.ret();return
        elif address==0x4af530:self.events.append(f'Create,{args[0]},{args[1]}');self.ret(POOL+60*50);return
        elif address==STOP+0x200:
            self.events.append(f'Owner,60,{args[0]}');self.put(POOL+60*50+34,args[0],1)
            if self.mutation==3:self.put(FIELDS+40,51);self.now(200)
            self.ret(purge=4);return
        elif address==STOP+0x300:
            self.events.append(f'Pop,60,{args[0]},{args[1]},{args[2]}')
            if self.mutation==4:self.put(FIELDS+40,51);self.now(200)
            self.ret(purge=12);return
        elif address==0x45c1b0:self.events.append(f'Bind,{args[0]},{args[1]}');self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'아비어리 미검토 명령 {address:x}')

    def objects(self):
        """서로 다른 다섯 현재 슬롯의 전체 바이트를 읽는다."""
        return b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex()

    def globals(self):
        """server/editor/현재 double 시각/processType/fort/친군 사격의 실제28BYTE다."""
        return (bytes(self.mu.mem_read(0x540bc0,4))+bytes(self.mu.mem_read(0x5c85a4,4))+bytes(self.mu.mem_read(0x55b4d0,8))+bytes(self.mu.mem_read(0x541160,4))+bytes(self.mu.mem_read(0x594fb8,4))+bytes(self.mu.mem_read(0x55a490,4))).hex()

    def run(self,case):
        """입력만 설치하고 모든 비교 결과는 실제 명령의 메모리/호출/정상 반환에서 얻는다."""
        self.kind,values=case;self.events=[];self.frame_calls=0;self.mutation=0;self.allocated=1;self.attached=False  # 현재 요청과 명시 효과의 입력/관찰이다.
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x5411a0,85);self.put(0x541160,60);self.put(0x541298,144);self.put(0x594fb8,0);self.put(0x55a490,0);self.put(0x540bc0,1);self.put(0x5c85a4,0);self.put(0x5e4794,0);self.now(100)
        self.put(VTABLE+0x74,STOP+0x200);self.put(VTABLE+0x90,STOP+0x300)
        for sid in IDS:  # 원문의 부모/플라이어/종속 입력 기록을 서로 구별한다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,VTABLE,71 if sid==70 else 0,70 if sid in (50,51) else 0,127,143 if sid in (50,51) else 144 if sid==60 else 60 if sid==70 else 31,0,150 if sid==50 else 151 if sid==51 else 9 if sid==60 else 0,float(sid)+.25,float(sid)+.5)
            raw[34]=1 if sid==50 else 2 if sid==51 else 0;raw[40]=0;struct.pack_into('<I',raw,36,7 if sid==51 else 42 if sid==60 else 0);self.mu.mem_write(POOL+sid*50,bytes(raw))
        fields=bytearray(b'\xa5'*48);struct.pack_into('<I',fields,0,0x509e20);fields[8]=0x10;struct.pack_into('<d',fields,16,100);struct.pack_into('<I',fields,24,1);struct.pack_into('<4I',fields,32,0x509e00,77,50,0)
        entry,receiver,args,purge=0x45d810,FIELDS,[],0;packet='-';objects='-';result='-'
        if self.kind=='C':
            server,mode,pattern=values;self.put(0x540bc0,server);self.now((0.0,100.25,-5.25)[mode]);fields=bytearray([pattern]*48)
        elif self.kind=='P':
            server,fort,special,flags,self.allocated,layout,self.mutation=values;self.put(0x540bc0,server);self.put(0x594fb8,fort);self.put(0x55a490,special);fields=bytearray(b'\xa5'*48);entry=0x45ed10;receiver=POOL+50*50;args=[flags];purge=4
            if layout==0:self.put(POOL+50*50+6,0,2)
            if layout==2:self.put(POOL+70*50+11,2,1);self.put(POOL+71*50+10,60,1)
            if layout==3:self.put(POOL+70*50+11,1,1)
        elif self.kind=='F':
            server,editor,extra,stage,frame,counter,mode,self.mutation=values;self.put(0x540bc0,server);self.put(0x5c85a4,editor);self.put(POOL+50*50+40,extra,1);self.put(POOL+50*50+36,frame);struct.pack_into('<d',fields,16,(99.0,100.0,101.0)[mode]);struct.pack_into('<I',fields,24,stage);struct.pack_into('<I',fields,44,counter);entry=0x45dde0;receiver=FIELDS+32
        elif self.kind=='W':
            dirty,include,force=values;fields[8]=dirty;struct.pack_into('<d',fields,16,123.25);struct.pack_into('<I',fields,24,7);struct.pack_into('<I',fields,44,5);entry=0x45d870;receiver=FIELDS+32;args=[PACKET,include,force];purge=12;self.mu.mem_write(PACKET,b'\xa7'*64)
        self.mu.mem_write(FIELDS,bytes(fields))
        self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)  # 각 호출의 x87 정밀도/최근접/빈 스택을 명시한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 비휘발 레지스터의 입력을 구별한다.
        self.mu.reg_write(UC_X86_REG_ECX,receiver);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start(entry,STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 원래 비휘발 보존을 전수 확인한다.
        if self.kind=='C':assert self.mu.reg_read(UC_X86_REG_EAX)==FIELDS
        if self.kind=='W':packet=bytes(self.mu.mem_read(PACKET,64)).hex();result=str(self.mu.reg_read(UC_X86_REG_EAX)&65535)
        if self.kind in ('P','F'):objects=self.objects()
        image=bytes(self.mu.mem_read(FIELDS,48)).hex() if self.kind!='P' or self.attached else '-'
        return [self.kind,','.join(map(str,values)),image,objects,self.globals(),packet,result,';'.join(self.events) or '-']


def cases():
    """생성/할당/실제 상태 경계/현재 효과 변경/256개 dirty 조합을 분리한다."""
    for server in (0,1):  # 서버 초기화와 비서버 미정 상태를 구별한다.
        for mode in range(3):  # 0/양수/음수의 원본 double 입력이다.
            for pattern in (0,0x5a,0xa5):yield 'C',(server,mode,pattern)  # 생성자가 쓰지 않는 바이트를 검증한다.
    for server in (0,1):  # 서버와 fort/special의 원문 차단 조합이다.
        for fort in (0,1):  # 편집 fort의 현재 DWORD다.
            for special in (0,1):  # fort여도 이 DWORD가 비영이면 프로세스를 만든다.
                for flags in (0,1,2,0x11,0x100000,0xffffffff):  # 실제 최초 플래그와 무관 비트다.
                    for allocated in (0,1):  # 원본 개별 new0x30의 성공/NULL이다.
                        for layout in range(4):yield 'P',(server,fort,special,flags,allocated,layout,0)  # 없음/live/dead/free 체인의 실제 조회다.
    for mutation in (1,2):yield 'P',(1,0,0,1,1,2,mutation)  # 조회 뒤 현재server/processType을 다시 읽는다.
    frames={0:[0],1:[0,27],2:[-2147483648,-1,0,11,12,2147483647],3:[0],4:[0],5:[0],6:[0],7:[0],8:[0],9:[0],10:[0],11:[0]}
    for server in (0,1):  # 플라이어 생성만 서버로 제한됨을 확인한다.
        for stage,choices in frames.items():  # AP 상태마다 실제 비교 프레임 경계를 고른다.
            for frame in choices:  # signed 끝 값도 원래 DWORD 감김을 유지한다.
                for counter in (0,):  # Rain 생성자가 쓰지 않는 마지막 DWORD를 보존한다.
                    for mode in range(3):yield 'F',(server,0,0,stage,frame,counter,mode,0)  # 다음 시각 전/일치/후다.
    for server in (0,1):  # 차단/편집은 서버 여부와 무관한 early return이다.
        for stage in range(1,11):  # 모든 정상 AP 상태에서 early return을 검증한다.
            for editor,extra in ((1,0),(0,0x20),(0,0x21)):yield 'F',(server,editor,extra,stage,0,0,1,0)
    for mutation in range(1,7):yield 'F',(1,0,0,1 if mutation in (1,2,5) else 6,0,0,1,mutation)  # 선택 효과 뒤 현재 부모/시각/단계/dirty 변경이다.
    for dirty in range(256):  # 미사용 변경 비트도 헤더에서 보존해야 한다.
        for include in (0,1):  # 부모 포함 여부를 독립 입력으로 둔다.
            for force in (0,1):yield 'W',(dirty,include,force)  # force는 전체 비트를 켠 뒤 마지막에0으로 쓴다.


def main():
    """독립 원문 자료를 생성하거나 현재 SHA/고유 입력을 감사한다."""
    fixture=ROOT/'cpppj/tests/fixtures/rain-aviary-x86.tsv';report=ROOT/'cpppj/rain-aviary.json'
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 보호 원본과 현재 검증 자료를 전체 바이트로 확인한다.
        rows=list(csv.reader(fixture.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({(row[0],row[1]) for row in rows});print('Rain Aviary native/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(case) for case in cases()];assert len(rows)==len({(row[0],row[1]) for row in rows})
    alternate=Oracle(0x037f);assert rows==[alternate.run(case) for case in cases()]  # 서로 다른 두 x87 정밀도에서 같은 정상 입력을 별도 재생한다.
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['kind','input','fields','objects','globals','packet','result','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_rain_aviary.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawRainAviaryProcess.h','cpppj/src/o/RawRainAviaryProcess.cpp','cpppj/tests/RainAviaryProcessTests.cpp','cpppj/tests/fixtures/rain-aviary-x86.tsv']
    facts={'host':socket.gethostname(),'date':datetime.now().date().isoformat(),'last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'scopes':dict(Counter(row[0] for row in rows)),'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'boundary':'실제 생성/후처리/Frame/writer 및 타이머/QA/부모/첫 Process helper를 실행한다. 힙/부착/Damageable/프레임 지정/장치/플라이어 생성·소유자·Pop·BaseFlyer 연결은 명시 외부 경계다. 원본 Rain Frame의 재전송 호출은 없으며 미검토 함수로 허용하지 않는다. 개발자 Frame 진단은 꺼진 입력이며 전체48BYTE/객체5개/현재28BYTE 전역/64BYTE패킷/호출/스택/비휘발/쓰기 guard를 대조한다.','sha256':{name:digest(ROOT/name) for name in paths}}
    facts['precision_replays']=[0x027f,0x037f]  # 두 재생을 새 고유 입력으로 중복 합산하지 않는다.
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Rain Aviary native:',len(rows),facts['scopes'])


if __name__=='__main__':main()
