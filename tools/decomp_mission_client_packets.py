#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""로그인8종 클라이언트 수신기의 원본 명령과 현재쓰기/필수효과를 격리 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, TEXT, IMAGE, HEAP, PLAYERS, TABLE, ZONE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 현재수신결과를순서대로직렬화할원본DWORD와정밀함수주소다.
GLOBALS=[0x540c70,0x540cac,0x595344,0x5409c4,0x595088,0x5949d4,0x5424bc,0x5c7a74,0x594fb0,0x594fb4,0x5c8758,0x5c87e8,0x5c8820,0x5c8878,0x55a420,0x595348,0x540bc0]
ENTRIES={0x29:0x4b3b40,0x67:0x4b4900,0x2b:0x4912d0,0x2a:0x4b3d30,0x28:0x4b8940,0x81:0x4b3e90,0x84:0x4a6f30,0x6e:0x4a7050}


def signed(value):
    """DWORD의원본signed해석이다."""
    return struct.unpack('<i',struct.pack('<I',value&0xffffffff))[0]


def packet(kind,variant):
    """결과를계산하지않고패킷필드와원문바이트를명시입력으로작성한다."""
    raw=bytearray(b'\xff'*128);raw[0]=1;raw[1]=kind;size=8
    if kind==0x29:
        size=34;struct.pack_into('<I',raw,4,variant%9);raw[8:14]=bytes(range(31,37));struct.pack_into('<I',raw,22,0x12345678);struct.pack_into('<I',raw,30,0x87654321)
    elif kind==0x67:
        length=(0,1,4,87)[variant%4];size=7+length;raw[4]=0;struct.pack_into('<H',raw,5,length);raw[7:size]=bytes((index*13+variant)&255 for index in range(length))
    elif kind==0x2a:
        size=22;struct.pack_into('<III',raw,4,variant%2+1,17,5);raw[16]=0;raw[17:22]=b'\x01\x03\x05\x07\x09'
    elif kind==0x2b:
        name=(b'A %.?=()[]~"`;: B\xa0',b' %~ ',b'Network')[variant%3];size=59+len(name)+1;raw[4:14]=b'*\0'+b'\xff'*8 if variant&1 else b'1.2.3.4\0\xff\xff'
        struct.pack_into('<III',raw,0x22,0x106,77+variant,(0,1,2,3,0x40,0x103)[variant%6]);raw[0x32]=variant%2+1;raw[0x33]=len(name)+1;raw[0x34]=5;struct.pack_into('<H',raw,0x35,0x1234);raw[0x37:0x3b]=bytes([6,7,8,9]);raw[59:59+len(name)+1]=name+b'\0'
    elif kind==0x28:
        size=18;struct.pack_into('<HH',raw,4,9 if variant&4 else 10,77 if variant&8 else 78);raw[8]=1;raw[9]=variant&1;struct.pack_into('<d',raw,10,12.25)
    elif kind==0x81:
        size=44;struct.pack_into('<I',raw,4,9)
        for index,value in enumerate((-1,1250,0,0x7fffffff,42,-99,600,700,800)):struct.pack_into('<I',raw,8+index*4,value&0xffffffff)
    else:struct.pack_into('<I',raw,4,variant)
    struct.pack_into('<H',raw,2,size);return bytes(raw[:size])


class Oracle(BaseOracle):
    """필수큰하위만경계로남기고작은수신기/이름/동맹/SP지정wrapper는원본을실행한다."""
    def __init__(self):
        """현재PC전체정밀함수표만허용하고실제쓰기주소도감사한다."""
        super().__init__();self.allowed=[]
        selected=set(ENTRIES.values())|{0x48f3b0,0x40aec0,0x40a580,0x40a5d0,0x42b0e0,0x490ea0,0x40b4a0,0x447810,0x4b41e0,0x4b54e0,0x4a6f70,0x40b010,0x4df230,0x4e49c0}
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in selected:
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write_guard)

    def write_guard(self,mu,access,address,size,value,data):
        """선택Player/동맹/현재전역/TYPE/문자표/스택밖실제명령쓰기는실패시킨다."""
        ranges=[(PLAYERS,PLAYERS+1620),(0x595200,0x595200+324),(STACK-0x7f000,STACK+0x1000),(0x531750,0x531752),(0x5c86c0,0x5c86c6),(0x52f7e0,0x52f7e0+87),(0x5953f0,0x5953fa),(TABLE+0xc4,TABLE+0xc8),(0x5c8618,0x5c8620),(0,4)]
        ranges += [(entry,entry+4) for entry in GLOBALS]+[(0x595350,0x595354)]
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'선택밖수신쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """원본수신조건을계산하지않고명시외부효과의전체인자/순서를기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,8)
        if address in (0x4a68e0,0x4c2630):self.ret();return
        if address==0x4e0620:self.events.append('Assert,'+self.string(args[0]).hex());self.ret();return
        if address==0x460d70:
            self.events.append('Wall')
            if self.mutate==1:self.put(0x5c8758,16)
            self.ret(TEXT+0x800);return
        if address==0x4e78d0:self.events.append('Clock');self.ret(7501);return
        if address==0x441090:self.events.append('Config,'+self.string(args[0]).decode());self.ret(1 if self.variant&32 else 0);return
        if address==0x4a89c0:
            pattern=self.string(args[1]);value=(pattern.decode()%tuple(args[2:4])).encode() if pattern==b'GameVerDiff(%d.%d)' else ('Cheat%d'%args[2]).encode();mu.mem_write(args[0],value+b'\0');self.ret(args[0]);return
        if address==0x4cf960:self.events.append('Tell,'+self.string(args[0]).hex());self.ret();return
        if address==0x4b6dd0:self.events.append('State,'+str(self.words(0x5c87e8)[0]));self.ret();return
        if address==0x4340d0:self.events.append('Camera');self.ret();return
        if address==0x42c920:self.events.append('Outpost,'+str(args[0]));self.ret(purge=8);return
        if address==0x4b4860:self.events.append('Range');self.ret();return
        if address==0x41c8b0:self.events.append('Energy');self.ret(1251);return
        if address==0x49a840:self.events.append('Type,'+str(args[0]));self.ret(TABLE);return
        if address==0x41ca70:self.events.append('Option');self.ret(17);return
        if address==0x4c7880:self.events.append('ReconnectUI');self.ret();return
        if address==0x4b54e0 and not self.words(0x594fa4)[0]:self.events.append('Save');self.ret();return
        if address==0x449200:self.events.append('Busy');self.ret(1 if self.variant&64 else 0);return
        if address==0x4d3090:self.events.append('Notice,'+','.join(str(value) for value in args[:3]));self.ret();return
        if address==0x48fb30:self.events.append('Restore,'+str(args[0]));self.ret();return
        if address==0x444ff0:self.events.append('Remove,'+str(args[0])+','+str(args[1]));self.ret();return
        if address==0x444ee0:
            self.events.append('Chat,'+str(args[0])+','+self.string(args[1]).hex()+','+str(args[5]))
            if self.mutate==2 and not self.mutated:self.put(0x540bc0,0);self.mutated=True
            self.ret();return
        if address==0x445080:self.events.append('Clear,'+str(args[0]));self.ret();return
        if address==0x4e2a50:self.events.append('IP,'+str(args[1]));self.ret(args[0]);return
        if address==0x4e0a30:self.ret(TEXT+0x200,12);return
        if address==0x40edb0:self.events.append('Set,'+str(args[0])+','+str(args[1]));self.put(HEAP+args[0]*4,args[1]);self.ret();return
        if address==0x43dad0:
            self.events.append('Refresh')
            if self.mutate==3:self.put(ZONE+4,2)
            self.ret();return
        if address in (0x4e51f0,0x4e63e0,0x4e5b40):
            if address==0x4e51f0:value=self.string(args[1])[:args[2]];mu.mem_write(args[0],value+bytes(args[2]-len(value)));self.ret(args[0])
            elif address==0x4e63e0:value=self.string(args[0])+b'\0';position=value.find(bytes([args[1]&255]));self.ret(0 if position<0 else args[0]+position)
            else:mu.mem_write(args[0],bytes(mu.mem_read(args[1],args[2])));self.ret(args[0])
            return
        if address==0x4606d0:self.events.append('ImageName,'+str(args[0]));self.ret(TEXT);return
        if address==0x4bfba0:self.events.append('Open,'+str((ecx-0x5c89f0)//20));self.ret(purge=8);return
        if address==0x4bfc60:self.events.append('Image,'+str((ecx-0x5c89f0)//20)+','+bytes(mu.mem_read(args[0],args[1])).hex());self.ret(purge=8);return
        if address==0x46d9b0:self.events.append('Builder');self.ret(ecx);return
        if address==0x46da60:self.events.append('Attach,'+str(args[0]));self.ret(purge=4);return
        if address==0x46e2c0:self.events.append('Build');self.ret();return
        if address in (0x4e2dc0,0x4e2e30):
            target=args[0] if address==0x4e2dc0 else 0;pointer=args[1] if address==0x4e2dc0 else args[0];self.events.append('Send,'+str(target)+','+bytes(mu.mem_read(pointer,8)).hex())
            if self.mutate==4:self.mu.mem_write(0x5c85f0,struct.pack('<d',0.0))
            self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미검토클라이언트수신 {address:08x}')
        self.instructions+=1
        if self.instructions>200000:raise RuntimeError('수신명령상한')

    def run(self,case):
        """전체현재입력에서모든Player/동맹/선택전역/SP/효과를정상반환후보존한다."""
        kind,server,boss,mission,known,local,self.variant,self.mutate,sender=case;self.events=[];self.instructions=0;self.mutated=False
        self.mu.mem_write(PLAYERS,b'\0'*1620);self.mu.mem_write(0x595200,b'\0'*324);self.mu.mem_write(0x52f81e,b'\0'*9);self.put(0x59534c,PLAYERS);self.put(0x595350,PLAYERS+180+0x6c)
        for owner in range(9):
            self.mu.mem_write(PLAYERS+owner*180+12,('N%d'%owner).encode()+b'\0');self.put(PLAYERS+owner*180+0x50,0x200);self.put(PLAYERS+owner*180+0x54,1<<owner);self.put(0x595200+(owner*9+owner)*4,1);self.put(HEAP+owner*4,struct.unpack('<I',struct.pack('<f',900.0+owner))[0])
        self.mu.mem_write(0x5c86c0,b'\x11\x12\x13\x14\x15\x16');self.mu.mem_write(0x52f7e0,b'\xa5'*87);self.mu.mem_write(0x5953f0,b'\x55'*10);self.put(TABLE+0xc4,0)
        self.mu.mem_write(TEXT,b'owner.fort\0');self.mu.mem_write(TEXT+0x100,b'My Name\0');self.mu.mem_write(TEXT+0x200,b'127.0.0.1\0');self.mu.mem_write(TEXT+0x800,struct.pack('<d',100.5));self.put(0x540ca8,TEXT+0x100)
        for address,value in [(0x540bc0,server),(0x540bc4,boss),(0x594fa4,mission),(0x5c85d4,1),(0x594fc8,0),(0x594fbc,1),(0x5c85a4,self.variant&1),(0x5467d8,self.variant&1),(0x595344,known),(0x540c70,local),(0x540cac,local),(0x5409c4,7),(0x595088,8),(0x5949d4,9),(0x5424bc,10),(0x5c7a74,0),(0x594fb0,11),(0x594fb4,12),(0x5c8758,(10,16,9)[(self.variant>>1)%3]),(0x5c87e8,13),(0x5c8820,14),(0x5c8878,15),(0x55a420,16),(0x595348,17),(0x541240,122)]:self.put(address,value)
        self.mu.mem_write(0x5c8618,struct.pack('<d',77.25));self.mu.mem_write(0x5c85f0,struct.pack('<d',0.0 if self.variant&1 else 30.0));raw=packet(kind,self.variant);self.mu.mem_write(ZONE,raw)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x76543210+register)
        self.mu.reg_write(UC_X86_REG_FPCW,0x27f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<IIII',STOP,sender,0,ZONE));self.mu.emu_start(ENTRIES[kind],STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(register)==0x76543210+register
        core=self.words(0x595350)[0];identity=core-PLAYERS+1 if PLAYERS<=core<PLAYERS+1620 else 0xfffffffe if core==0x5953f0 else 0
        states=[self.words(address)[0] for address in GLOBALS]+[identity,int.from_bytes(self.mu.mem_read(0x5c8618,8),'little'),int.from_bytes(self.mu.mem_read(0x5c85f0,8),'little')]
        storage=bytes(self.mu.mem_read(0x5c86c0,6))+bytes(self.mu.mem_read(0x52f7e0,87))+bytes(self.mu.mem_read(0x5953f0,10))+bytes(self.mu.mem_read(TABLE+0xc4,4))
        return [','.join(map(str,case)),raw.hex(),bytes(self.mu.mem_read(PLAYERS,1620)).hex(),bytes(self.mu.mem_read(0x595200,324)).hex(),','.join(map(str,states)),storage.hex(),bytes(self.mu.mem_read(HEAP,36)).hex(),';'.join(self.events) or '-']


def cases():
    """미션에서자연선택되는모드와경계의현재재조회를유한독립입력으로교차한다."""
    for kind in (0x29,0x67,0x2b,0x2a,0x28,0x81,0x84,0x6e):
        variants={0x29:(0,1,2,8),0x67:(0,1,2,3),0x2b:(0,1,2,3,4,5,64),0x2a:(0,1),0x28:(0,1,2,3,4,8,36),0x81:(0,1),0x84:(0,1,0x80000000,0xffffffff),0x6e:(0,2,0x100,0x102,0x80000000,0xffffffff)}[kind]
        for server in (0,1):
            for known in (0,1):
                for variant in variants:yield kind,server,server,1,known,1,variant,0,9
    for kind,variant,mutate,sender in ((0x28,1,1,9),(0x2b,3,2,9),(0x81,0,3,9),(0x6e,2,4,1)):
        yield kind,1,1,1,1,1,variant,mutate,sender
    for variant in (0,1,2,0x100,0x80000000):yield 0x6e,1,1,1,1,1,variant,0,2
    for kind,variant in ((0x67,0),(0x67,1),(0x2b,1),(0x2b,2)):yield kind,0,0,0,1,1,variant,0,9


def main():
    """보호파일을읽기만하고현재새fixture/SHA보고서를생성하거나검증한다."""
    fixture=ROOT/'cpppj/tests/fixtures/mission-client-packets-x86.tsv';report=ROOT/'cpppj/mission-client-packets.json'
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_mission_client_packets.py','cpppj/src/o/RawMissionClientPackets.h','cpppj/src/o/RawMissionClientPackets.cpp','cpppj/tests/MissionClientPacketsTests.cpp','cpppj/tests/fixtures/mission-client-packets-x86.tsv']
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for path,sha in facts['sha256'].items():assert digest(ROOT/path)==sha,path
        with fixture.open(encoding='utf-8') as stream:rows=list(csv.DictReader(stream,delimiter='\t'))
        assert len(rows)==facts['nativeUniqueInputs']==len({row['input'] for row in rows});print('미션수신원본/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(case) for case in cases()]
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','packet','players','allies','state','storage','sp','trace']);writer.writerows(rows)
    facts={'host':socket.gethostname(),'date':'2026-10-15','nativeUniqueInputs':len(rows),'status':'native-observed-cpp-validation-pending','boundaries':['실제8수신기와이름/방향별동맹/양수·0 owner/SP지정wrapper/버전검증/Ready송신helper를현재PC전체정밀범위에서실행','CRT문자열/시계/설정/옵션파생/TYPE조회/Template·Builder/SP하위저장/Chat/UI/전송은명시경계','실제쓰기범위guard·정상반환stack·EBX/ESI/EDI/EBP와Player1620/동맹324/현재전역/논리SP9/모든외부효과비교','새source연결/게임·GUI/OS소켓은실행하지않음'],'sha256':{path:digest(ROOT/path) for path in paths if (ROOT/path).is_file()}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('미션수신원본고유입력:',len(rows))


if __name__=='__main__':main()
