#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재 Player 송신/중앙섬 metadata의 실제 원본 쓰기와 observer를 격리 관찰한다."""
import csv
import itertools
import json
import struct
import sys
from decomp_chat_channels import ChatOracle, ROOT, TEXT, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 Player9 입력과 외부 주소 문자열은 원본 PE와 겹치지 않는다.
PLAYERS=TEXT+0x2000
FIXTURE=ROOT/'cpppj/tests/fixtures/mission-server-packets-x86.tsv'
REPORT=ROOT/'cpppj/mission-server-packets.json'


def cases():
    """현재 이름·연결flag·metamap진단·서버진단과 자기 metadata 생략을 독립 입력한다."""
    for owner,name,flags,server,meta in itertools.product((1,8),(b'Old Name',b'',b'A %.~ B',b'X'*31),(0,1,3,0x104,0xffffffff),(0,1),(0,1)):
        yield ('P',owner,name.hex(),flags,server,meta,2,0,0x82000004,0x1234)
    for target,created,server,arch in itertools.product((1,2,0xffffffff),(0,1),(0,1),(0,0x1234)):
        yield ('M',1,'-',0,server,0,target,created,0x82000004,arch)
    for zone,text in ((0,b''),(0x82000004,b'$Random'),(0x100,b'Static')):
        yield ('H',1,text.hex(),0,1,0,1,0,zone,7)


def initial(owner,name,flags):
    """기대값 계산 없이 실제 전체 Player9와 지정 이름/현재 필드만 준비한다."""
    data=bytearray((i*37+11)&255 for i in range(1620));offset=owner*180
    data[offset+12:offset+44]=b'\0'*32
    data[offset+12:offset+12+len(name)]=name;data[offset+12+len(name)]=0
    struct.pack_into('<I',data,offset+0x2c,0x12345680+owner);struct.pack_into('<I',data,offset+0x50,flags)
    return bytes(data)


class Oracle(ChatOracle):
    """기존 실제 이름 정제/CRT 관찰기를 재사용하고 송신 writer·두 observer를 추가한다."""
    def __init__(self):
        """전체 정밀 함수표의 명시 몸체만 허용하며 실제 쓰기 guard를 유지한다."""
        super().__init__();self.entries={0x48f9f0,0x48f550,0x4a6b40,0x4b3bc0,0x40a5d0,0x40b330,0x40a590,0x40b110,0x4df590,0x4df230,0x4e4a40,0x42b0e0};self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):
                if int(row['entry'],16) in self.entries:
                    for part in row['ranges'].split(';'):
                        low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.native={};self.external={}

    def guard(self,mu,access,address,size,value,data):
        """이름 정제의 실제 Player와 문자표/한정 스택 밖 쓰기는 거부한다."""
        if not (PLAYERS<=address and address+size<=PLAYERS+1620 or 0x531750<=address and address+size<=0x531752 or STACK-4096<=address and address+size<=STACK+40):
            raise RuntimeError(f'서버 패킷 범위 밖 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """실제 writer/observer는 실행하며 주소/서식/송신만 명시 ABI 경계다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,8)
        if address in self.entries:self.native[address]=self.native.get(address,0)+1
        if address==0x4a68e0:self.events.append('L,'+self.string(args[0]).hex());self.ret();return
        if address==0x4e0620:self.events.append('A,'+self.string(args[0]).hex());self.ret();return
        if address==0x4e2a50:
            self.events.append('IP,'+str(args[1]));self.external[address]=self.external.get(address,0)+1;self.ret(args[0]);return
        if address==0x4e0a30:
            if tuple(args[:3])!=(1,0,0):raise RuntimeError('현재 주소 서식 인자 오류')
            self.external[address]=self.external.get(address,0)+1;self.ret(TEXT+0x800,12);return
        if address==0x4e2dc0:
            self.target,self.packet=args[:2];kind=mu.mem_read(self.packet+1,1)[0];observer={0x2b:0x40a590,0x72:0x40b110}[kind]
            self.put(esp-8,STOP+0x100);self.put(esp-4,1);mu.reg_write(UC_X86_REG_ECX,self.packet);mu.reg_write(UC_X86_REG_ESP,esp-8);mu.reg_write(UC_X86_REG_EIP,observer);return
        if address==STOP+0x100:
            length=int.from_bytes(mu.mem_read(self.packet+2,2),'little');self.events.append('S,'+str(self.target)+','+bytes(mu.mem_read(self.packet,length)).hex());self.ret();return
        super().instruction(mu,address,size,data)

    def run(self,c):
        """정상 반환/스택/보존 레지스터와 전체 Player/전역/송신 바이트를 그대로 읽는다."""
        kind,owner,name,flags,server,meta,target,created,zone,arch=c;self.events=[];self.mu.mem_write(PLAYERS,initial(owner,b'' if name=='-' else bytes.fromhex(name),flags));self.mu.mem_write(TEXT+0x800,b'127.0.0.1\0')
        for address,value in ((0x59534c,PLAYERS),(0x540bc0,server),(0x594fc8,meta),(0x540c70,1),(0x5c85a0,created),(0x5c86c0,zone),(0x5c8bb4,arch)):self.put(address,value)
        packet=bytearray(b'\xff'*32);packet[10:10+len(bytes.fromhex(name)) if name!='-' else 10]=b'' if name=='-' else bytes.fromhex(name)
        if kind=='H':
            text=bytes.fromhex(name);packet[10:10+len(text)]=text;packet[10+len(text)]=0;self.mu.mem_write(TEXT+0x1000,bytes(packet))
        saved={UC_X86_REG_EBX:0x11112222,UC_X86_REG_ESI:0x33334444,UC_X86_REG_EDI:0x55556666,UC_X86_REG_EBP:0x77778888}
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_FPCW,0x27f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,PLAYERS+owner*180)
        args=[STOP,target] if kind!='H' else [STOP,9,0,TEXT+0x1000];self.mu.mem_write(STACK,struct.pack('<'+'I'*len(args),*args))
        self.mu.emu_start({'P':0x48f9f0,'M':0x4a6b40,'H':0x4b3bc0}[kind],STOP,count=100000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+(8 if kind=='P' else 4):raise RuntimeError('서버 패킷 정상 반환/스택 오류')
        if any(self.mu.reg_read(r)!=v for r,v in saved.items()):raise RuntimeError('서버 패킷 보존 레지스터 오류')
        return [bytes(self.mu.mem_read(PLAYERS,1620)).hex(),','.join(str(self.words(a,1)[0]) for a in (0x540bc0,0x540c70,0x5c85a0,0x5c86c0,0x5c8bb4)),';'.join(self.events) or '-']


def main():
    """원본 독립 관찰을 생성하거나 현재 원본/제품/기대값 SHA를 감사한다."""
    if sys.argv[1:]==['--verify']:
        r=json.loads(REPORT.read_text(encoding='utf-8'))
        for path,sha in r['sha256'].items():
            if digest(ROOT/path)!=sha:raise RuntimeError('서버 패킷 SHA 불일치 '+path)
        with FIXTURE.open(encoding='utf-8') as stream:rows=list(csv.reader(stream,delimiter='\t'))[1:]
        if len(rows)!=r['native_unique_inputs'] or len({row[0] for row in rows})!=len(rows):raise RuntimeError('서버 패킷 고유입력 수 오류')
        print('서버 패킷 native/SHA:',len(rows));return
    if sys.argv[1:]:raise SystemExit('사용법: decomp_mission_server_packets.py [--verify]')
    oracle=Oracle();rows=[]
    for c in cases():rows.append([','.join(map(str,c)),*oracle.run(c)])
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','players','globals','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_mission_server_packets.py','tools/decomp_chat_channels.py','cpppj/src/o/RawMissionServerPackets.h','cpppj/src/o/RawMissionServerPackets.cpp','cpppj/tests/MissionServerPacketsTests.cpp','cpppj/tests/fixtures/mission-server-packets-x86.tsv']
    r={'host':'HJOW-Athlon','date':'2026-10-15','status':'native-observed-product-unbuilt','native_unique_inputs':len(rows),'normal_returns':len(rows),'native_entries':{f'{a:08x}':n for a,n in oracle.native.items()},'explicit_address_boundaries':{f'{a:08x}':n for a,n in oracle.external.items()},'game_gui_os_calls':0,'sha256':{p:digest(ROOT/p) for p in paths if (ROOT/p).exists()}}
    REPORT.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('서버 패킷 native:',len(rows))


if __name__=='__main__':main()
