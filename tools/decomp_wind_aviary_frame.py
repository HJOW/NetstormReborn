#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Wind 생산 프레임/후처리의 실제 명령과 명시 월드 효과를 두 x87 정밀도로 관찰한다."""
import csv
import json
import socket
import struct
import sys
from collections import Counter
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, HEAP, PLAYERS, TABLE, IMAGE, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

# 실제 주소와 구별한 원문80BYTE/풀/가상 표/TYPE/SHP의 독립 저장소다.
FIELDS, POOL, VT, TYPE, SHAPE = HEAP, PLAYERS, TABLE+0xf000, TABLE+0x10000, IMAGE+0x10000
IDS = (50,60,70)  # 원본 부모/임시 플라이어/종속 Form을 구별한다.


class Oracle(BaseOracle):
    """상태 판단과 타이머/프레임 계산은 실제 명령으로만 얻는다."""
    def __init__(self, control=0x027f):
        """이 PC의 전체 정밀 범위 중 실제 호출한 생산 함수만 허용한다."""
        super().__init__();self.control=control;self.allowed=[];self.total=0
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for address in (0x45f0a0,0x45edf0,0x45d730,0x4b1a90,0x40ead0,0x40eab0,0x41bfb0,0x4ac220,0x4e49c0):  # 하위 계산/조회와 CRT 절삭도 원본 몸체를 실행한다.
            for part in facts[address]['ranges'].split(';'):  # 불연속 코드 조각까지 정확히 허용한다.
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """힙/가상 월드 효과는 입력으로 공급하고 미검토 진입은 즉시 거부한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,8)
        if address==0x4e0620:self.events.append('Assert,'+str(args[2]));self.ret();return
        if address==0x4e4391:self.events.append('Allocate,'+str(args[0]));self.ret(FIELDS if self.kind=='P' else HEAP+0x1000);return
        if address==0x41bd20:self.events.append(f'Attach,{args[0]},{args[1]},{args[3]}');self.attached=True;self.ret(purge=16);return
        if address==0x44bf80:self.events.append('Base,'+str(args[0]));self.ret(purge=4);return
        if address==0x4acee0:
            sid=(ecx-POOL)//50;frame=struct.unpack('<i',struct.pack('<I',args[0]))[0];self.events.append(f'Frame,{sid},{frame}');self.put(ecx+36,args[0]);self.ret(purge=8);return
        if address==0x4ac550:self.ret(TYPE);return
        if address==0x422460:self.events.append('Height');self.ret(SHAPE);return
        if address==0x419850:self.events.append('SpecialHeight');self.ret(SHAPE,purge=8);return
        if address==0x4adea0:self.events.append('Center');self.mu.mem_write(args[0],struct.pack('<ff',50.25,50.5));self.ret(args[0],purge=4);return
        if address==0x41e020:self.events.append(f'Circle,{args[3]},{args[4]}');self.mu.mem_write(args[0],struct.pack('<ff',61.25,62.5));self.ret();return
        if address==0x4af530:self.events.append('Create,'+str(args[0]));self.ret(POOL+60*50);return
        if address==STOP+0x200:self.events.append('Owner,'+str(args[0]));self.put(POOL+60*50+34,args[0],1);self.ret(purge=4);return
        if address==STOP+0x300:self.events.append(f'Pop,{args[0]},{args[1]}');self.ret(purge=12);return
        if address==0x45e1c0:self.events.append(f'Process,{args[0]},{args[1]},{args[2]},{args[3]}');self.ret(ecx,purge=16);return
        if address==0x45c1b0:self.events.append(f'Bind,{args[0]},{args[1]}');self.ret();return
        if address==0x41bf10:self.events.append('Finish,'+str(args[0]));self.ret(purge=4);return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'미검토 Wind 명령 {address:x}')
        self.total+=1

    def run(self,case):
        """입력 설치 뒤 전체80BYTE/세 슬롯/효과 순서를 독립 결과로 반환한다."""
        self.kind,values=case;self.events=[];self.attached=False
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x54115c,59);self.put(0x541280,138);self.put(0x541284,139);self.put(0x540bc0,1);self.put(0x540bc4,1);self.put(0x594fb8,0);self.put(0x55a490,0);self.put(0x5c85a4,0);self.put(0x5e4794,0);self.mu.mem_write(0x55b4d0,struct.pack('<d',100))
        self.put(VT+0x74,STOP+0x200);self.put(VT+0x90,STOP+0x300);self.put(TYPE+0xe0,0x40800000);self.put(TYPE+0x1d8,3);self.mu.mem_write(SHAPE+0x14,struct.pack('<f',44.0))
        for sid in IDS:  # 세 입력 레코드의 미정 바이트도 서로 같은 원문 패턴으로 설치한다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,VT,0,70 if sid==50 else 0,0,139 if sid==50 else 138 if sid==60 else 59,0,0,float(sid)+.25,float(sid)+.5);raw[34]=1;raw[40]=0;struct.pack_into('<I',raw,36,0);self.mu.mem_write(POOL+sid*50,bytes(raw))
        fields=bytearray(b'\xa5'*80);struct.pack_into('<I',fields,0,0x509dfc);fields[8]=0x10
        struct.pack_into('<dI',fields,16,100,1);struct.pack_into('<ddI',fields,32,99,100,0);struct.pack_into('<5I',fields,56,0x509ddc,77,50,4,2)
        entry,receiver,args,purge=0x45f0a0,FIELDS+56,[],0
        if self.kind=='F':
            stage,elapsed,timer,temporary,server,boss,parent_type,editor,extra,frame=values
            struct.pack_into('<I',fields,24,stage);struct.pack_into('<d',fields,32,timer);struct.pack_into('<d',fields,40,100-elapsed);struct.pack_into('<I',fields,48,temporary);self.put(0x540bc0,server);self.put(0x540bc4,boss);self.put(0x5c85a4,editor);self.put(POOL+50*50+10,parent_type,1);self.put(POOL+50*50+40,extra,1);self.put(POOL+50*50+36,frame)
        else:
            server,fort,friendly,flags=values;self.put(0x540bc0,server);self.put(0x594fb8,fort);self.put(0x55a490,friendly);self.put(POOL+50*50+6,0,2);fields=bytearray(b'\xa5'*80);entry,receiver,args,purge=0x45edf0,POOL+50*50,[flags],4
        self.mu.mem_write(FIELDS,bytes(fields));self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 원래 비휘발 레지스터를 검사한다.
        self.mu.reg_write(UC_X86_REG_ECX,receiver);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start(entry,STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 정상 반환의 보존 규약이다.
        return [self.kind,','.join(map(str,values)),bytes(self.mu.mem_read(FIELDS,80)).hex(),b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex(),';'.join(self.events) or '-']


def cases():
    """모든 상태/서버/표시 유무와 실제4/6/26초 경계 및 가감속을 구별한다."""
    for server in (0,1):  # 서버 여부는 생성 효과의 선택만 바꾼다.
        for boss in (0,1):  # 비권한 클라이언트의 대기 해제도 관찰한다.
            for stage in range(15):  # 정상1..13과 양끝 개발자 비활성 입력이다.
                for temporary in (0,60):  # 표시 중인 실제 플라이어의 유무다.
                    for elapsed in (-1.,0.,1.,1.999,2.,3.999,4.,4.001,5.999,6.,6.001,26.,26.001):yield 'F',(stage,elapsed,99.,temporary,server,boss,139,0,0,0)  # float 저장 이후 비교다.
    for timer in (0.,99.,100.,101.):  # 미설정/과거/일치/미래의 애니메이션 예약이다.
        for stage in (2,7):  # 가속/감속의 별도 상태다.
            for frame in (-1,0,5,6,2147483647):yield 'F',(stage,1.,timer,60,1,1,139,0,0,frame)  # signed 프레임 나머지도 원문 입력이다.
    for stage in (1,11,12,13):yield 'F',(stage,0.,99.,0,1,1,175,0,0,0)  # 주문 부모의 특별 생성 단계다.
    for editor,extra in ((1,0),(0,32),(0,33)):yield 'F',(1,0.,99.,0,1,1,139,editor,extra,0)  # 두 독립 early return이다.
    for server in (0,1):  # 전체 편집/서버/최초 비트 조합이다.
        for fort in (0,1):  # 편집 금지 관문을 따로 둔다.
            for friendly in (0,1):  # 편집 중 예외 활성화다.
                for flags in (0,1,2,17):yield 'P',(server,fort,friendly,flags)  # 공통 후처리는 항상 호출된다.


def main():
    """기존 원문 fixture는 건드리지 않고 새 생산 근거를 생성/검사한다."""
    fixture=ROOT/'cpppj/tests/fixtures/wind-aviary-frame-x86.tsv';report=ROOT/'cpppj/wind-aviary-frame-20261022.json'
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for path,value in facts['sha256'].items():assert digest(ROOT/path)==value,path  # 독립 근거 전체 바이트를 확인한다.
        print('Wind frame SHA verified:',facts['observations']);return
    first=Oracle();rows=[first.run(case) for case in cases()];second=Oracle(0x037f);assert rows==[second.run(case) for case in cases()]
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['kind','input','fields','objects','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','originalCD/NETSTORM/NETSTORM.EXE','extracted/refined/originals/functions.tsv','tools/decomp_wind_aviary_frame.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawWindAviaryProcess.h','cpppj/src/o/RawWindAviaryProcess.cpp','cpppj/tests/WindAviaryProcessTests.cpp','cpppj/tests/fixtures/wind-aviary-frame-x86.tsv']
    paths=[p for p in paths if (ROOT/p).is_file()]
    facts={'host':socket.gethostname(),'last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-22','observations':len(rows),'scopes':dict(Counter(r[0] for r in rows)),'precision_replays':[0x027f,0x037f],'normal_returns':len(rows)*2,'instructions':first.total+second.total,'boundary':'생산 상태/타이머/프레임/부모 조회와 CRT 절삭은 실제 명령이다. 생성/소유자/Pop/비행 생성자/바인딩/TYPE·SHP 제공/특별 부모 원주 좌표/삭제는 명시 외부 효과다. checking0이며 GUI 플레이 검증과 구별한다.','sha256':{p:digest(ROOT/p) for p in paths}}
    report.write_bytes((json.dumps(facts,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print('Wind frame native:',len(rows),facts['scopes'])


if __name__=='__main__':main()
