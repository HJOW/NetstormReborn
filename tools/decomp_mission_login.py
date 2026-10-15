#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""선택된 미션0x27 부모/참여/패킷 helper를 실제 기계어로 격리 실행한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, TEXT, IMAGE, HEAP, PLAYERS, TABLE, ZONE, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 현재 전역의 출력 순서이며 Player 배열과 함께 완전 비교한다.
GLOBALS = [0x540c70, 0x540cac, 0x595344, 0x5951f8, 0x5951fc, 0x5424b0, 0x5c7a74, 0x594fb0]


def signed(value):
    """원본32비트 정수의 부호만 다시 해석한다."""
    return struct.unpack('<i', struct.pack('<I', value & 0xffffffff))[0]


class Oracle(BaseOracle):
    """큰 Template/전송/시계 경계 외에는 현재 전체 정밀 범위의 실제 명령을 실행한다."""
    def __init__(self):
        """다른 작업의 선택 export를 사용하지 않고 현재 PC 전체 정밀 함수표만 읽는다."""
        super().__init__();self.allowed = []
        selected = {0x4a7800, 0x48f3b0, 0x48fb30, 0x4a6dc0, 0x4a6990, 0x4a6a70, 0x4a6c50, 0x4a6ab0, 0x491270, 0x48f550, 0x4a6920, 0x4a6e70, 0x4a6f70, 0x490a00, 0x490ab0, 0x490bc0, 0x40c300, 0x40a500, 0x40a4c0, 0x40b010, 0x40aec0, 0x40ae80, 0x40b4a0, 0x40b460, 0x40a4b0, 0x4df590, 0x4df230, 0x4e4a40, 0x4e49c0}
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in selected:
            for part in facts[address]['ranges'].split(';'):
                low, high = (int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def send_complete(self):
        """실제 observer 이후 선언 길이까지 packet 전체를 보존한다."""
        length = int.from_bytes(self.mu.mem_read(self.send_packet+2,2),'little');raw = bytes(self.mu.mem_read(self.send_packet,length))
        self.events.append('S,'+str(self.send_target)+','+raw.hex())
        if self.mutate == 1 and not self.mutated and raw[1] == 0x29:
            self.put(PLAYERS+2*180+0x2c,2);self.put(PLAYERS+2*180+0x50,0x143);self.mutated=True
        self.ret()

    def instruction(self,mu,address,size,data):
        """원본 분기를 외부에서 계산하지 않고 선택된 외부 몸체의 인자/순서를 기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,8)
        if address in (0x4a68e0,0x4c2630):self.ret();return
        if address == 0x4e0620:self.events.append('A,'+self.string(args[0]).hex());self.ret();return
        if address == 0x4e2940:self.events.append('Disconnect,'+str(signed(args[0])));self.ret();return
        if address == 0x4e27b0:self.events.append('Assign,'+str(signed(args[0]))+','+str(args[1]));self.ret();return
        if address == 0x4e2920:self.events.append('Flush');self.ret();return
        if address == 0x4e2dc0:
            self.send_target=signed(args[0]);self.send_packet=args[1];kind=mu.mem_read(args[1]+1,1)[0]
            observer={0x67:0x40ae80,0x81:0x40b460}.get(kind)
            if observer:
                self.put(esp-8,STOP+0x100);self.put(esp-4,1);mu.reg_write(UC_X86_REG_ECX,args[1]);mu.reg_write(UC_X86_REG_ESP,esp-8);mu.reg_write(UC_X86_REG_EIP,observer)
            else:self.send_complete()
            return
        if address == STOP+0x100:self.send_complete();return
        if address == STOP+0x200:
            raw=bytes(mu.mem_read(args[0],int.from_bytes(mu.mem_read(args[0]+2,2),'little')));self.events.append('Pending,'+raw.hex());self.ret(purge=4);return
        if address == 0x460d70:self.events.append('Wall');self.ret(TEXT+0x800);return
        if address == 0x4e78d0:self.events.append('Clock');self.ret(self.clock);return
        if address == 0x40ec50:
            owner=args[0];self.events.append('SP,'+str(owner));self.put(args[1],struct.unpack('<I',struct.pack('<f',owner*1250.25-1700.5))[0]);self.ret();return
        if address == 0x4b4900:
            raw=bytes(mu.mem_read(args[2],94));self.events.append('Options,'+raw.hex());self.ret();return
        if address == 0x41c320:self.events.append('ClearTypes');self.ret();return
        if address == 0x48f9f0:self.events.append('Player,'+str((ecx-PLAYERS)//180)+','+str(args[0]));self.ret(purge=4);return
        if address == 0x4a6b40:self.events.append('Metadata,'+str(args[0]));self.ret();return
        if address == 0x460680:self.events.append('FortName');self.ret(TEXT);return
        if address == 0x4bf550:self.events.append('FortFile,'+str((ecx-0x5c89f0)//20)+','+self.string(args[0]).hex());self.ret(purge=4);return
        if address == 0x4606d0:self.events.append('ImageName,'+str(args[0])+','+str(args[1]));self.ret(TEXT+0x100);return
        if address == 0x4bfba0:self.events.append('FortOpen,'+str((ecx-0x5c89f0)//20)+','+str(args[1]));self.ret(purge=8);return
        if address == 0x4bfc60:self.events.append('FortImage,'+str((ecx-0x5c89f0)//20)+','+bytes(mu.mem_read(args[0],args[1])).hex());self.ret(purge=8);return
        if address == 0x4e2ae0:self.events.append('IP,'+str(args[0]));self.ret(TEXT+0x200);return
        if address == 0x4a7240:self.events.append('Fort,'+str(args[0])+','+str(args[1])+','+str(args[2]));self.ret();return
        if address == 0x4a7350:self.events.append('Synchronize,'+str(args[0]));self.ret();return
        if address == 0x4b5300:
            self.events.append('World')
            if self.mutate == 2:self.put(0x540c70,3);self.put(0x540cac,4);self.put(PLAYERS+3*180+0x50,0x200)
            self.ret();return
        if address == 0x44dbc0:self.events.append('Settings,'+str(args[0]));self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError(f'미검토 로그인 명령 {address:08x}')
        self.instructions+=1
        if self.instructions>200000:raise RuntimeError('원본 로그인 명령 상한')

    def run(self,values):
        """Player9/전역/요청 packet 입력에서 전체 결과와 효과 순서를 보존한다."""
        server,known,local,request,flags,other,self.mutate,self.clock=values;self.events=[];self.instructions=0;self.mutated=False
        self.mu.mem_write(PLAYERS,b'\0'*1620);self.mu.mem_write(HEAP,b'\0'*0x100);self.mu.mem_write(TABLE,b'\0'*0x1000)
        self.put(HEAP,TABLE);self.put(TABLE+4,STOP+0x200);self.put(0x59534c,PLAYERS);self.put(0x595350,0)
        self.mu.mem_write(TEXT,b'mission.fort\0');self.mu.mem_write(TEXT+0x100,b'owner.fort\0');self.mu.mem_write(TEXT+0x200,b'127.0.0.1\0');self.mu.mem_write(TEXT+0x800,struct.pack('<d',100.5))
        for address,value in [(0x540bc0,server),(0x540bc4,1),(0x54db50,0),(0x594fa0,1),(0x594fa4,1),(0x594fc8,0),(0x595344,known),(0x540c70,local),(0x540cac,local),(0x540c74,1),(0x5951f8,0),(0x5951fc,7),(0x5424b0,17),(0x5c7a74,0),(0x5409c4,0x12345678),(0x595088,0x87654321),(0x54dbc4,0),(0x594fb0,0x55),(0x594fb4,3000)]:self.put(address,value)
        self.mu.mem_write(0x5c8618,struct.pack('<d',500.75));self.mu.mem_write(0x5c86c0,b'\x81\x82\x83\x84\x85\x86');self.mu.mem_write(0x52f7e0,bytes(range(87)))
        self.put(PLAYERS+180+0x2c,1);self.put(PLAYERS+180+0x50,flags)
        if other:
            self.put(PLAYERS+2*180+0x2c,2);self.put(PLAYERS+2*180+0x50,other);self.put(PLAYERS+2*180+0x5c,0);self.put(PLAYERS+2*180+0x60,88)
        packet=bytearray(b'\xff'*24);struct.pack_into('<BBH',packet,0,1,0x27,len(packet));struct.pack_into('<I',packet,8,request);packet[12]=2;struct.pack_into('<I',packet,13,5);packet[17:19]=b'N\0';packet[19:24]=b'\x01\x03\x05\x07\x09';self.mu.mem_write(ZONE,bytes(packet))
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x76543210+register)
        self.mu.reg_write(UC_X86_REG_FPCW,0x27f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<IIII',STOP,0xffffffff,HEAP,ZONE))
        self.mu.emu_start(0x4a7800,STOP);assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(register)==0x76543210+register
        core=self.words(0x595350)[0];identity=core-PLAYERS+1 if PLAYERS<=core<PLAYERS+1620 else 0
        return [','.join(map(str,values)),bytes(self.mu.mem_read(PLAYERS,1620)).hex(),','.join(str(self.words(address)[0]) for address in GLOBALS)+','+str(identity),';'.join(self.events)]


def cases():
    """새/기존·다른owner·거부·현재 callback 변경·Begin 집계의 서로 다른 입력이다."""
    for server,request in ((0,1),(0,9),(1,9)):yield server,0,8,request,8,0,0,7501
    yield 1,0,0,1,8,0,0,7501
    for known,local in ((0,8),(1,1),(1,3)):
        for flags in (8,0x108,2,3):
            for other in (0,0x103,0x143):yield 1,known,local,1,flags,other,0,7501
    for mutate in (1,2):
        for flags in (8,2):yield 1,0,8,1,flags,0,mutate,1001
    for clock in (0,2999,0x80000000,0xffffffff):yield 1,1,1,1,8,0,0,clock


def main():
    """원본/공유제품을 수정하지 않고 신규 fixture와 출처/SHA 보고서만 생성한다."""
    fixture=ROOT/'cpppj/tests/fixtures/mission-login-x86.tsv';report=ROOT/'cpppj/mission-login.json'
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_mission_login.py','cpppj/src/o/RawMissionLogin.h','cpppj/src/o/RawMissionLogin.cpp','cpppj/tests/MissionLoginTests.cpp','cpppj/tests/fixtures/mission-login-x86.tsv']
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for path,sha in facts['sha256'].items():assert digest(ROOT/path)==sha,path
        with fixture.open(encoding='utf-8') as stream:rows=list(csv.DictReader(stream,delimiter='\t'))
        assert len(rows)==facts['nativeUniqueInputs']==len({row['input'] for row in rows});print('미션 로그인 원본/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(values) for values in cases()]
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','players','globals','trace']);writer.writerows(rows)
    facts={'host':socket.gethostname(),'date':'2026-10-15','nativeUniqueInputs':len(rows),'status':'native-observed-cpp-validation-pending','boundaries':['같은PC 전체 정밀 함수표의 검토한 실제 범위만 실행; 게임/OS 진입 없음','004a7800 부모 및 CheckLogin/owner지정/가입Player 순회/Begin·SP·응답·Ready 실제helper 실행','Template 파일/이미지/직렬화·중앙 metadata·동기화·worldLoad·옵션 적용·SP 저장소·CRT clock·wall·전송은 명시 외부 몸체','현재 Player9 전체1620바이트/8전역/모든 송신 packet바이트/전체 효과순서·반환스택·callee저장레지스터 비교','generic Battle/simulconnect/debugBurst는 이번 정상입력 관찰 범위가 아니며 C++에서 명시 거부'],'sha256':{path:digest(ROOT/path) for path in paths if (ROOT/path).is_file()}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('미션 로그인 원본 고유입력:',len(rows))


if __name__=='__main__':main()
