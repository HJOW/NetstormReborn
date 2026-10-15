#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 상태7~10/13/14와 다섯144BYTE 블록의 확정/재조회만 격리 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, TEXT, HEAP, ZONE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 다섯 상태 블록과 외부현재전역의 관찰 순서다.
BLOCKS=(0x5c8630,0x5c8758,0x5c86c8,0x5c87e8,0x5c8878)
GLOBALS=(0x540bc0,0x540bc4,0x594fbc,0x594fa4,0x5c85d4,0x595344,0x540c70,0x540cac,0x5c85c0,0x55b4ac,0x55a420,0x5c85b8,0x5c85d8,0x5c85dc)


class Oracle(BaseOracle):
    """상태알고리즘/공통부/만료판정/정수변환은원본,큰하위는명시효과입력이다."""
    def __init__(self):
        """같은PC 전체정밀 범위만 허용하고 블록/현재전역/실제Core 쓰기를 감사한다."""
        super().__init__();self.allowed=[]
        selected={0x4b6dd0,0x4b88c0,0x4b2bc0,0x4437f0,0x4180a0,0x4180d0,0x4e49c0}
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 인접 미검토 명령까지 허용범위를 넓히지 않는다.
        for address in selected:
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write_guard)

    def write_guard(self,mu,access,address,size,value,data):
        """선택한 원문 쓰기와 정확한 SEH4BYTE 이외의 명령 쓰기를 거부한다."""
        ranges=[(entry,entry+144) for entry in BLOCKS]+[(entry,entry+4) for entry in GLOBALS]+[(0,4),(STACK-0x7f000,STACK+0x1000),(0x565e70,0x565e71),(0x5c85f8,0x5c8600),(0x544d58,0x544d60),(HEAP+0x1000,HEAP+0x10c0)]
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'선택밖 상태쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """큰외부효과의현재인자/순서를보존하며선택된변경입력만공급한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX);args=self.words(esp+4,5)
        if address==0x45ad00:
            if self.string(args[0]).startswith(b'line'):self.events.append('ConnLog,'+str(args[1])+','+str(args[2]))
            else:
                self.events.append('State,'+str(self.words(0x5c87e8)[0]))
                if self.mutation==4:self.put(0x5c87e8,8)
            self.ret();return
        if address==0x4e0620:self.events.append('Assert');self.ret();return
        if address==0x460d70:
            self.events.append('Wall')
            if self.mutation==1:self.mu.mem_write(0x5c85f8,struct.pack('<d',99.0))
            self.ret(TEXT);return
        if address==0x4978d0:self.events.append('Selection');self.ret(purge=4);return
        if address==0x4b2df0:
            self.events.append('Cleanup,'+str(args[0]));self.put(0x540bc0,0);self.put(0x540bc4,0);self.put(0x594fa4,1);self.put(0x5c85c0,0);self.put(0x5c85d4,0);self.put(0x55b4ac,0);self.put(0x55a420,0)
            if self.mutation==7:self.put(0x5c886c,0)
            self.ret();return
        if address==0x440c60:self.events.append('Seed');self.ret(123);return
        simple={0x42e440:'Commands',0x469fc0:'Music',0x463a40:'Mission',0x4c1e70:'MapHandlers',0x460980:'ReconnectFort',0x4b63d0:'SendLogin',0x4b6090:'ConnectedProgress',0x4c8dc0:'CloseProgress',0x460730:'RefreshFort',0x479760:'MetaFirst',0x47dd60:'MetaSecond',0x47cab0:'MetaThird'}
        if address in simple:self.events.append(simple[address]);self.ret();return
        if address==0x4b4be0:
            self.events.append('Zone,'+str(args[0]));self.put(0x540bc0,1);self.put(0x540bc4,1)
            if self.mutation==2:self.put(0x5c8868,ZONE+16)
            self.ret();return
        if address==0x4e4354:self.events.append('Free,'+str(args[0]));self.ret();return
        if address==0x4993c0:self.events.append('Cursor');self.ret(purge=4);return
        if address==0x466280:self.events.append('UI,'+str(args[1]));self.ret();return
        if address==0x4de9a0:self.ret(args[0],4);return
        if address==0x4cf570:
            pattern=self.string(args[0]);event='MissionStarting' if pattern==b'Initializing.' else 'ReconnectStarting' if b'Reconnect' in pattern or b'reconnect' in pattern else 'ServerLost'
            if b'Countdown' in pattern:event='ReconnectCountdown' if b'Reconnect' in pattern else 'ConnectingCountdown'
            # Progress는문자열포인터한인자다. Countdown정수는앞선실제formatter의네번째인자에서관찰한다.
            self.events.append('Progress,'+event+','+str(self.progress_countdown if b'Countdown' in pattern else 0));self.ret();return
        if address==0x4a89c0:
            pattern=self.string(args[1])
            if b'Countdown' in pattern:self.progress_countdown=struct.unpack('<i',struct.pack('<I',args[3]))[0]
            self.mu.mem_write(args[0],pattern+b'\0');self.ret(args[0]);return
        if address==0x4e0cd0:self.ret(ecx,8);return
        if address==0x4410f0:self.events.append('Palette');self.ret(77);return
        if address==0x4b6100:self.events.append('ApplyPalette,'+str(args[0]));self.ret();return
        if address==0x4e2620:self.events.append('Connect,'+bytes(mu.mem_read(args[1],20)).hex()+','+str(args[2]));self.ret();return
        if address==0x4e2b70:
            target=struct.unpack('<i',struct.pack('<I',args[0]))[0];self.events.append('Status,'+str(target))
            if self.mutation==5:self.put(0x540bc0,1)
            self.ret((1 if self.variant&512 else 0) if target==-1 else -1 if self.variant&2048 else 0 if self.variant&8 else 1);return
        if address==0x4e2b10:self.events.append('Open,'+str(args[0]));self.ret(0 if self.variant&16 else 1);return
        if address==0x4e2940:self.events.append('Close,'+str(args[0]));self.ret();return
        if address==0x4e27b0:self.events.append('Rename,-1,9');self.ret();return
        if address==0x4e2220:self.events.append('ServerConnection');self.ret(55);return
        if address==0x4c1eb0:self.events.append('Commander,'+str(args[0]));self.ret();return
        if address==0x460680:self.events.append('FortName');self.ret(TEXT+16);return
        if address==0x4bf550:self.events.append('LoadFort,'+str((ecx-0x5c89f0)//20));self.ret(purge=4);return
        if address==0x4bd550:self.events.append('Core,'+str((ecx-0x5c89f0)//20));self.ret(HEAP+0x1000+((ecx-0x5c89f0)//20)*0x40);return
        if address==0x4e2a50:
            self.events.append('Address');self.mu.mem_write(args[0],self.address)
            if self.mutation==3:self.put(0x540cac,2)
            self.ret();return
        if address==0x4e3d58:self.ret(((args[0]&255)<<8)|((args[0]>>8)&255),4);return
        if address==0x4bd540:self.events.append('Touch,'+str((ecx-0x5c89f0)//20));self.ret();return
        if address==0x4477d0:self.events.append('Sp,'+str(args[0]));self.ret();return
        if address==0x4b67c0:self.events.append('Timer,'+str(args[0]));self.ret();return
        if address==0x42f270:self.events.append('Input,'+str(args[0]));self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미검토 상태명령 {address:08x}')
        self.instructions+=1
        if self.instructions>20000:raise RuntimeError('상태명령상한')

    def run(self,case):
        """전체블록/외부전역/두현재시계/실제Core의모든바이트와정상반환을보존한다."""
        kind,selected,self.variant,self.mutation=case;self.events=[];self.instructions=0;self.progress_countdown=None
        # 모든상태의부가인자는구별되는원문DWORD로공급한다.
        for block_index,base in enumerate(BLOCKS):self.mu.mem_write(base,struct.pack('<'+'I'*36,*[block_index*1000+index for index in range(36)]))
        current=0x5c8878 if kind=='Poll' else 0x5c87e8;self.put(current,selected);self.put(current+0x80,ZONE);self.put(current+0x84,1 if self.variant&2 else 0);self.put(current+0x88,1250)
        if kind=='Poll':self.put(0x5c87e8,0)
        if kind=='Request':self.put(0x5c86c8,7 if self.variant&1 else 0)
        self.mu.mem_write(TEXT,struct.pack('<d',100.5)+b'\0'*8+b'local.fort\0');self.mu.mem_write(0x5c85f8,struct.pack('<d',0.0 if self.variant&1024 else 90.0 if self.variant&32 else 120.0));self.mu.mem_write(0x544d58,struct.pack('<d',77.0));self.mu.mem_write(0x55b4d0,struct.pack('<d',45.75));self.mu.mem_write(0x55b4d8,struct.pack('<d',100.5))
        values=(1 if self.variant&1 else 0,1,0,1,1,9,1,1,1 if self.variant&4 else 0,42,17,88,0,1 if self.variant&256 else 0)
        for address,value in zip(GLOBALS,values):self.put(address,value)
        self.put(0x594fa0,1);self.put(0x54db50,0);self.put(0x594fc8,1 if self.variant&256 else 0);self.put(0x565e70,90,1);self.put(0x5c85d0,7,1)
        self.mu.mem_write(HEAP+0x1000,bytes((index*3+5)&255 for index in range(192)))
        for owner in range(3):self.put(0x5c89f0+owner*20,0 if self.variant&64 else HEAP+0x1000+owner*64)
        address=bytearray(range(20));struct.pack_into('<H',address,0,6 if self.variant&128 else 2);address[2:4]=b'\x12\x34';address[12:14]=b'\x56\x78';self.address=bytes(address)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x76543210+register)
        args=[selected] if kind=='Request' else [];self.mu.reg_write(UC_X86_REG_FPCW,0x27f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start({'Run':0x4b6dd0,'Poll':0x4b88c0,'Request':0x4b2bc0}[kind],STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(register)==0x76543210+register
        assert self.mu.reg_read(UC_X86_REG_FPTAG)==0xffff
        state=b''.join(bytes(self.mu.mem_read(base,144)) for base in BLOCKS);globals_text=','.join(str(self.words(address)[0]) for address in GLOBALS)+','+str(self.mu.mem_read(0x565e70,1)[0])
        timers=bytes(self.mu.mem_read(0x5c85f8,8))+bytes(self.mu.mem_read(0x544d58,8))
        return [','.join(map(str,case)),state.hex(),globals_text,timers.hex(),bytes(self.mu.mem_read(HEAP+0x1000,192)).hex(),';'.join(self.events) or '-']


def cases():
    """필요최소선택흐름과현재값변경만공급하며거대한부모상태는관찰수에합산하지않는다."""
    yield 'Request',8,0,0;yield 'Request',0,1,0;yield 'Request',14,1,0
    for variant in (4,6):yield 'Run',7,variant,0
    yield 'Run',7,4,2;yield 'Run',7,6,7
    for variant in (0,1024):yield 'Run',8,variant,0
    for variant in (1,0,8,8|512,16,32,2048):yield 'Run',9,variant,0
    yield 'Run',9,1,1;yield 'Run',9,8|512,5
    for variant in (0,16):yield 'Run',10,variant,0
    for variant in (1,0,1|2,1|64,1|128,1|16):yield 'Run',13,variant,0
    yield 'Run',13,1,3;yield 'Run',14,0,0;yield 'Run',14,0,4
    yield 'Poll',0,0,0;yield 'Poll',8,1024,0;yield 'Poll',9,1,0;yield 'Poll',0,256,0


def main():
    """새fixture/SHA를기록하고보호파일은읽기만한다."""
    fixture=ROOT/'cpppj/tests/fixtures/local-mission-state-x86.tsv';report=ROOT/'cpppj/local-mission-state.json';paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_state.py','cpppj/src/o/RawLocalMissionState.h','cpppj/src/o/RawLocalMissionState.cpp','cpppj/tests/LocalMissionStateTests.cpp','cpppj/tests/fixtures/local-mission-state-x86.tsv']
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for path,sha in facts['sha256'].items():assert digest(ROOT/path)==sha,path
        with fixture.open(encoding='utf-8') as stream:rows=list(csv.DictReader(stream,delimiter='\t'))
        assert len(rows)==facts['nativeUniqueInputs']==len({row['input'] for row in rows});print('로컬 상태 원본/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(case) for case in cases()];assert len(rows)==len({row[0] for row in rows})
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','blocks','globals','timers','cores','trace']);writer.writerows(rows)
    facts={'host':socket.gethostname(),'date':'2026-10-15','last_full_refined_host':'HJOW-Athlon','nativeUniqueInputs':len(rows),'status':'native-observed-cpp-validation-pending','normal_stack_saved_registers_x87_checked':True,'boundaries':['실제7~10/13/14와Request/Poll/Run공통부의전체정밀범위명령을실행한다','큰전투정리/Zone설치/파일/Templatewrapper/UI/음악/팔레트/네트워크/타이머시작은명시효과경계이며현재입력변경을일부공급한다','정수변환004e49c0/만료004180a0/활성벽타이머004180d0과빈refresh004437f0은실제원본몸체다','진행창은실제Initializing/재접속/연결실패문자열로분류하고Countdown정수는실제formatter의네번째인자를관찰한다','부모1/2/3전체의새native완료나실제게임완주를주장하지않는다'],'sha256':{path:digest(ROOT/path) for path in paths if (ROOT/path).is_file()}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('로컬 상태 원본 고유입력:',len(rows))


if __name__=='__main__':main()
