#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""맨 오 워 PostPop/공통 비행 생성·홈 연결·상태 writer의 실제10.78 명령을 격리 관찰한다."""
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

# 원본 객체5개/112BYTE 본체/64BYTE 출력과 TYPE188개를 격리한다.
POOL,FIELDS,PACKET,TYPES=PLAYERS,HEAP,IMAGE,TABLE
IDS=(50,60,61,70,71)


def bits(value):
    """명시 float 입력의 IEEE 비트만 만든다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class Oracle(BaseOracle):
    """본체/조회/그림자 offset은 실제 명령, 힙/Host/공간/시간 설정/장부는 선언한 외부 입력이다."""
    def __init__(self,control=0x027f):
        """실제 정련 표의 검토한 연속/불연속 범위만 실행한다."""
        super().__init__();self.allowed=[];self.control=control
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x45fac0,0x45e6b0,0x45c340,0x45c1b0,0x45c310,0x45aed0,0x45faa0,0x4ac550,0x4ac200,0x49a840,0x4b1a90):  # 검토한 실제 함수다.
            for part in functions[address]['ranges'].split(';'):  # 불연속 코드도 누락하지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """선언한 객체/본체/스택/SEH 밖 원문 쓰기는 거부한다."""
        ranges=[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]+[(FIELDS,FIELDS+112),(PACKET,PACKET+64),(STACK-4096,STACK+128),(0,4)]
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'비행 선택 밖 쓰기 {address:x}/{size}')

    def now(self,value):
        """단일 게임 시각 입력이다."""
        self.mu.mem_write(0x55b4d0,struct.pack('<d',value))

    def instruction(self,mu,address,size,data):
        """외부 큰 몸체의 현재 인수/쓰기 입력만 공급하고 원문 결과를 계산하지 않는다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,5);ecx=mu.reg_read(UC_X86_REG_ECX);parent=POOL+50*50
        if address==0x4e0620:
            self.events.append('Assert,'+str(args[2]))
            if self.mutation==1:self.now(200);self.put(0x5410ec,30);self.put(FIELDS+8,0x10,1)
            self.ret();return
        if address==0x4e4391:
            self.events.append('Allocate,'+str(args[0]));assert args[0]==112
            if self.mutation==7:self.put(parent+10,143,1);self.put(parent+12,13,2)
            if self.mutation==8:self.put(0x540bc0,0);self.put(0x5410ec,30)
            self.ret(FIELDS if self.allocated else 0);return
        if address==0x41bd20:
            assert ecx==FIELDS+48 and args[2]==0;self.attach_image=bytes(mu.mem_read(FIELDS,112)).hex();self.events.append(f'Attach,{args[0]},{args[1]},{args[3]}')
            if self.mutation==2:self.put(FIELDS+84,77);self.put(parent+10,143,1);self.put(parent+12,13,2);self.now(200)
            self.ret(purge=16);return
        if address==0x4ae2d0:self.events.append('Fields,'+str((ecx-POOL)//50));self.ret(FIELDS+48);return
        if address==0x4af530:
            self.events.append(f'Create,{args[0]},{args[1]}')
            if self.mutation==3:self.put(parent+36,7);self.put(parent+14,bits(90.25))
            self.ret(POOL+61*50);return
        if address in (0x4b02d0,0x4afe50):
            sid=(ecx-POOL)//50
            if address==0x4b02d0:
                self.events.append('Pop,'+str(sid)+','+','.join(map(str,args[:3])));self.pop_count+=1
                if self.mutation==4 and self.pop_count==1:self.put(parent+36,9);self.put(parent+18,bits(70.25))
                self.ret(purge=12)
            else:
                self.events.append(f'Unpop,{sid},{args[0]}')
                if self.mutation==6:self.put(parent+14,bits(100.25));self.put(parent+18,bits(110.25))
                self.ret(purge=4)
            return
        if address==0x4d4e10:
            self.events.append('SetTime,'+str((ecx-POOL)//50)+','+str(args[0])+','+str(args[1]))
            if self.mutation==5:self.put(parent+8,60,2);self.put(parent+36,11);self.put(parent+14,bits(80.25))
            self.ret(purge=8);return
        if address==0x44bf80:self.events.append('Base,'+str(args[0]));self.ret(purge=4);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'비행 미검토 명령 {address:x}')

    def run(self,case):
        """입력만 설치하고 전체 결과를 원본 정상 반환 뒤 읽는다."""
        self.kind,values=case;self.events=[];self.mutation=0;self.allocated=1;self.attach_image='-';self.pop_count=0
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59ab20,TYPES);self.put(0x541348,188);self.put(0x5410ec,31);self.put(0x540bc0,1);self.now(100.25);self.put(0,0x12340000)
        for type_id in (31,143,144):  # 실제500BYTE TYPE의 해당 입력 필드다.
            record=TYPES+type_id*500;self.mu.mem_write(record,bytes(500));self.put(record+0xe0,bits(1.25 if type_id==143 else 0.75));self.put(record+0xe8,0x400000);self.put(record+0xec,0x100000);self.put(record+0x114,12)
        for sid in IDS:  # 관찰할 전체50BYTE 객체를 서로 구별한다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,0x50a388,71 if sid==70 else 0,70 if sid==50 else 0,0,144 if sid<70 else 31,0,17,50.25,50.5);struct.pack_into('<I',raw,36,2);raw[40]=0;self.mu.mem_write(POOL+sid*50,bytes(raw))
        fields=bytearray(b'\xa5'*112);fields[8]=0x10;struct.pack_into('<I',fields,52,77);struct.pack_into('<I',fields,56,50)
        entry,receiver,args,purge=0x45c340,FIELDS,[50,17,bits(.75),bits(.5)],16;packet='-';result='-'
        if self.kind=='C':
            server,home,speed,parameter,pattern,self.mutation=values;self.put(0x540bc0,server);fields=bytearray([pattern]*112);args=[50,home,speed,parameter]
        elif self.kind=='P':
            server,flags,self.allocated,frame,count,shadow,existing,self.mutation=values;self.put(0x540bc0,server);self.put(TYPES+144*500+0x114,count);self.put(TYPES+144*500+0xe8,0x400000 if shadow else 0);self.put(POOL+50*50+36,frame);self.put(POOL+50*50+8,60 if existing else 0,2);fields=bytearray(b'\xa5'*112);entry=0x45fac0;receiver=POOL+50*50;args=[flags];purge=4
        elif self.kind=='B':
            genus,layout,home=values;self.put(TYPES+144*500+0xec,genus);entry=0x45c1b0;receiver=0;args=[50,home];purge=0
            if layout==0:self.put(POOL+50*50+6,0,2)
            if layout==2:self.put(POOL+70*50+11,2,1)
            if layout==3:self.put(POOL+70*50+11,1,1)
            if layout==4:self.put(POOL+70*50+10,32,1);self.put(POOL+71*50+11,2,1)
        elif self.kind=='W':
            dirty,include,force=values;fields[8]=dirty;struct.pack_into('<HH',fields,10,65534,65533);struct.pack_into('<III',fields,16,0x3f400000,0x3f000000,0xffffffff);struct.pack_into('<dd',fields,32,123.25,-5.75);entry=0x45c310;receiver=FIELDS+48;args=[PACKET,include,force];purge=12;self.mu.mem_write(PACKET,b'\xa7'*64)
        self.mu.mem_write(FIELDS,bytes(fields));self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 비휘발 입력도 매 호출 구별한다.
        self.mu.reg_write(UC_X86_REG_ECX,receiver);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start(entry,STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 매 정상 반환의 보존 레지스터다.
        assert self.mu.reg_read(UC_X86_REG_FPCW)==self.control and ((self.mu.reg_read(UC_X86_REG_FPSW)>>11)&7)==0
        assert self.words(0)[0]==0x12340000
        if self.kind=='W':packet=bytes(self.mu.mem_read(PACKET,64)).hex();result=str(self.mu.reg_read(UC_X86_REG_EAX)&65535)
        image=bytes(self.mu.mem_read(FIELDS,112)).hex() if self.kind!='P' or self.attach_image!='-' else '-'
        objects=b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex()
        globals_image=(bytes(self.mu.mem_read(0x540bc0,4))+bytes(self.mu.mem_read(0x55b4d0,8))+bytes(self.mu.mem_read(0x5410ec,4))).hex()
        return [self.kind,','.join(map(str,values)),image,objects,globals_image,packet,result,self.attach_image,';'.join(self.events) or '-']


def cases():
    """생성/부착 전후/실제 그림자/홈 조회와256개 변경비트의 고유 입력이다."""
    for server in (0,1):  # 비서버 assert 후에도 원문은 계속 진행한다.
        for home in (0,17,65535):  # WORD 홈의 경계다.
            for speed in (bits(.75),0xffffffff):  # 원문은 이 DWORD를 해석 없이 쓴다.
                for pattern in (0,0x5a,0xa5):yield 'C',(server,home,speed,bits(.5),pattern,0)  # padding 입력을 구별한다.
    yield 'C',(0,65535,bits(.75),bits(.5),0xa5,1)
    yield 'C',(1,17,bits(.75),bits(.5),0xa5,2)
    for server in (0,1):  # 프로세스 생성만 서버로 제한한다.
        for flags in (0,1,2,0xffffffff):  # 최초/비최초와 무관비트를 확인한다.
            for allocated in (0,1):  # new NULL은 후속 기본 몸체를 계속 실행한다.
                for frame in (-1,0,11,12,13):  # signed frame과 실제 전체 프레임 경계다.
                    for shadow in (0,1):  # flyershadow 비트다.
                        for existing in (0,1):yield 'P',(server,flags,allocated,frame,12,shadow,existing,0)  # 기존 그림자의 비최초 재배치도 포함한다.
    for mutation in range(2,9):yield 'P',(1,1,1,2,12,1,1,mutation)  # 할당/큰 효과 뒤 현재 전역/부모/QA/좌표를 재조회한다.
    for frame in (-2147483648,2147483647):yield 'P',(1,1,1,frame,0,1,1,0)  # low DWORD 프레임 감김과0개 입력이다.
    for genus in (0,0x100000):  # genus assert도 원문 정상 반환 범위에서 관찰한다.
        for layout in range(5):  # 없음/live/dead/free/다른타입+dead 체인이다.
            for home in (0,50,65535):yield 'B',(genus,layout,home)  # 실제 첫 nondead process만 갱신한다.
    for dirty in range(256):  # 헤더의 미사용 비트도 보존한다.
        for include in (0,1):  # 부모 전송을 독립 입력으로 선택한다.
            for force in (0,1):yield 'W',(dirty,include,force)  # 강제 전송의 모든 필드와 dirty 초기화다.


def main():
    """원문 자료를 생성하거나 현재 보호 원본/검증 파일 SHA와 고유 입력을 감사한다."""
    fixture=ROOT/'cpppj/tests/fixtures/flyer-postpop-x86.tsv';report=ROOT/'cpppj/flyer-postpop.json'
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 이전 보고서를 현재 값으로 조용히 덮지 않는다.
        rows=list(csv.reader(fixture.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({(row[0],row[1]) for row in rows});print('Flyer native/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(case) for case in cases()];alternate=Oracle(0x037f);assert rows==[alternate.run(case) for case in cases()]
    assert len(rows)==len({(row[0],row[1]) for row in rows})
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['kind','input','fields','objects','globals','packet','result','attach_image','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_flyer_postpop.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawFlyerProcess.h','cpppj/src/o/RawFlyerProcess.cpp','cpppj/tests/FlyerPostPopTests.cpp','cpppj/tests/fixtures/flyer-postpop-x86.tsv']
    facts={'host':socket.gethostname(),'date':datetime.now().date().isoformat(),'last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'scopes':dict(Counter(row[0] for row in rows)),'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'precision_replays':[0x027f,0x037f],'boundary':'실제 PostPop/기본 생성/홈 연결/writer/TYPE·종속 조회/그림자 offset을 실행한다. 힙/Host 부착·PID 조회/생성/Pop/Unpop/004d4e10 압축 시각/최종 Damageable는 명시 경계다. 실제 Session은 기존 RawGunHit::SetTime과 공통 장부를 연결한다. 비행 Frame/의존 해제/파생 삭제는 이 관찰에 포함하지 않는다. 전체112BYTE·객체5개250BYTE·현재16BYTE전역·부착직전112BYTE·64BYTE패킷·스택/SEH/비휘발/쓰기guard를 대조한다.','sha256':{name:digest(ROOT/name) for name in paths}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Flyer native:',len(rows),facts['scopes'])


if __name__=='__main__':main()
