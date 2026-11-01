#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""화면 연속 저장의 시작·프레임 복사·종료 순서를 실제10.78 몸체에서 동결한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle,ROOT,STACK
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

ARENA=0x11000000  # 화면/프레임/문자열 장치의 명시 가상 메모리다.
SOURCE,OTHER,BUFFER,SECOND,PREFIX,NOTICE=(ARENA+offset for offset in (0,0x1000,0x2000,0x3000,0x4000,0x5000))  # 상태의포인터도그대로관측한다.
SLOTS,COUNT,ACTIVE,FIRST=0x5de880,0x5df824,0x5df828,0x5df82c  # 서로독립된원문1000포인터와세DWORD다.
WIDTH,HEIGHT,PITCH,PIXELS,PALETTE=0x5c78f0,0x5c78f4,0x5c78ec,0x5c78f8,0x5c7954  # 할당/복사/쓰기위치에서다시읽는다.


class Oracle(BaseOracle):
    """상태와복사는원문으로실행하며화면잠금·할당·파일·설정·통지장치만관측한다."""
    def __init__(self):
        """완료된정밀범위세개와명시입력장치를준비한다."""
        super().__init__('originals',[dict(name=name,originals=f'{address:08x}') for name,address in (('Start',0x4db210),('Frame',0x4db230),('Stop',0x4db2a0))],[])
        self.mu.mem_map(ARENA,0x10000)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def number(self,address,value=None):
        """원문전체DWORD를읽고명시경계의현재상태변이만쓴다."""
        if value is not None:
            self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def returned(self,value=0):
        """cdecl명시하위를반환하며인자정리는실제caller에맡긴다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value)
        self.mu.reg_write(UC_X86_REG_ESP,sp+4)
        self.mu.reg_write(UC_X86_REG_EIP,self.number(sp))

    def guard(self,mu,access,address,size,value,data):
        """명시상태·프레임복사·스택밖의CPU쓰기를거부한다."""
        if 0x30000000<=address<=address+size<=0x30010000 or SLOTS<=address<=address+size<=SLOTS+4000 or COUNT<=address<=address+size<=FIRST+4 or BUFFER<=address<=address+size<=BUFFER+512:
            return
        raise RuntimeError(f'연속저장미명시쓰기 {address:08x}+{size}')

    def event(self,name,*args):
        """각장치의진입인자뒤현재상태를입력에따라바꾼다."""
        self.events.append(':'.join((name,*map(str,args))))
        self.calls[name]=self.calls.get(name,0)+1
        trigger=name+str(self.calls[name])
        if self.mode==1 and trigger=='lock1':
            self.number(HEIGHT,3);self.number(PITCH,7)
        if self.mode==2 and trigger=='allocate1':
            self.number(PIXELS,OTHER)
        if self.mode==3 and trigger=='unlock1':
            self.number(COUNT,1)
        if self.mode==4 and trigger=='prefix1':
            self.number(WIDTH,5);self.number(HEIGHT,2);self.number(PALETTE,12345)
        if self.mode==5 and trigger=='free1':
            self.number(COUNT,1)
        if self.mode==6 and trigger=='write1':
            self.number(SLOTS,SECOND)
        if self.mode==7 and trigger=='prefix3':
            self.number(COUNT,7)

    def on_instruction(self,mu,address,size,data):
        """실제push순서와접근시점만관측하고CPU복사를대체하지않는다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address in (0x4a14e0,0x4a1550):
            self.event('lock' if address==0x4a14e0 else 'unlock');self.returned();return
        if address==0x43fa80:
            self.event('allocate',self.number(sp+4));self.returned(BUFFER);return
        if address==0x4410f0:
            assert self.read_string(self.number(sp+4))==b'GifSequenceName'
            assert [self.number(sp+offset) for offset in (8,12,16)]==[0,0,0]
            self.event('prefix')
            prefix=b'folder/seq' if self.calls['prefix']&1 else b'next%name'
            self.mu.mem_write(PREFIX,prefix+b'\0');self.returned(PREFIX);return
        if address==0x4a89c0:
            destination,format_,prefix,index=[self.number(sp+offset) for offset in (4,8,12,16)]
            assert self.read_string(format_)==b'%s%03d.bmp'
            signed=index if index<0x80000000 else index-0x100000000
            value=self.read_string(prefix)+f'{signed:03d}.bmp'.encode('ascii')
            self.mu.mem_write(destination,value+b'\0');self.returned(destination);return
        if address==0x4ddc90:
            values=[self.number(sp+4*index) for index in range(1,9)]
            self.event('write',self.read_string(values[0]).hex(),*values[1:]);self.returned();return
        if address==0x43fa90:
            self.event('free',self.number(sp+4));self.returned();return
        if address==0x4b9390:
            style,format_,first,last=[self.number(sp+offset) for offset in (4,8,12,16)]
            assert style==80 and self.read_string(format_)==b'Wrote %s through %s.\n'
            self.event('report',self.read_string(first).hex(),self.read_string(last).hex())
            self.mu.mem_write(NOTICE,b'formatted report\0');self.returned(NOTICE);return
        if address==0x4cf550:
            assert self.read_string(self.number(sp+4))==b'formatted report'
            self.event('notice',self.number(sp+8),self.number(sp+12));self.returned();return
        super().on_instruction(mu,address,size,data)

    def observe(self,kind,count,active,first,width,height,pitch,mode,control):
        """상태와전체물리표를재설치하고원문출력만반환한다."""
        self.mode=mode;self.events=[];self.calls={}
        self.mu.mem_write(0x30000000,bytes(0x10000))
        slots=[0]*1000
        for index,token in ((0,BUFFER),(1,SECOND),(2,0),(999,SECOND)):
            slots[index]=token
        self.mu.mem_write(SLOTS,struct.pack('<1000I',*slots))
        for address,value in ((COUNT,count),(ACTIVE,active),(FIRST,first),(WIDTH,width),(HEIGHT,height),(PITCH,pitch),(PIXELS,SOURCE),(PALETTE,24680)):
            self.number(address,value)
        self.mu.mem_write(SOURCE,bytes((index*37+11)&255 for index in range(512)))
        self.mu.mem_write(OTHER,bytes((index*19+17)&255 for index in range(512)))
        self.mu.mem_write(BUFFER,bytes([0xa7])*512)
        self.mu.mem_write(SECOND,bytes([0x52])*512)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.call(kind,[])
        return (':'.join(map(str,(self.number(COUNT),self.number(ACTIVE),self.number(FIRST)))),bytes(self.mu.mem_read(SLOTS,4000)).hex(),bytes(self.mu.mem_read(BUFFER,512)).hex(),';'.join(self.events) or '-')


def main():
    """유효한표범위의시작/프레임/종료입력만독립관측해새경로에저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    oracle=Oracle();rows=[]
    inputs=[]
    inputs += [('Start',count,active,first,4,3,8,0) for count,active,first in itertools.product((0,2,999),(0,1,0xffffffff),(0,7))]  # 기존슬롯을지우지않는세전역쓰기다.
    inputs += [('Frame',count,active,7,width,height,pitch,mode) for count,active,(width,height,pitch),mode in itertools.product((0,999,1000,1001),(0,1),((3,2,5),(4,3,8),(1,1,1)),range(4))]  #복사끝1~3BYTE와상한·할당전후현재입력이다.
    inputs += [('Stop',count,active,7,4,3,8,mode) for count,active,mode in itertools.product((0,1,2,3),(0,1,0xffffffff),(0,4,5,6,7))]  #중간null과free/write/마지막prefix뒤현재값이다.
    for values in inputs:  #제품출력을기대값생성에쓰지않는다.
        observations=[oracle.observe(*values,control) for control in (0x27f,0x37f,0x27f)]
        assert observations[0]==observations[1]==observations[2]
        rows.append((*values,*observations[0]))
    fixture=out/'capture-sequence-x86.tsv'
    schema=('kind','count','active','first','width','height','pitch','mode','stateOut','slotsOut','bufferOut','trace')
    fixture.write_text('\t'.join(schema)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')
    report=dict(unit=693,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),cases=len(rows),observations=len(rows)*3,differences=0,fixtures={fixture.name:hashlib.sha256(fixture.read_bytes()).hexdigest()},boundaries='실제4db210/4db230/4db2a0. 화면잠금·할당/free·prefix/formatter·8인자BMPwriter·보고/안내는명시경계다. 실제1000슬롯·512BYTE복사와현재상태를대조하며실제파일/게임/OS/MP는0. 유효한0..1001 count와표범위만입력한다.',sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_capture_sequence.py','tools/decomp_oracle.py')})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=len(rows),observations=len(rows)*3,differences=0)))


if __name__=='__main__':
    main()
