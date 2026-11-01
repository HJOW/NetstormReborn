#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""4ddc90의 실제 8비트 BMP 바이트·미작성 지역 바이트·파일 호출을 동결한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
from decomp_oracle import Oracle as BaseOracle,ROOT,STACK
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

ARENA=0x11000000  # 실제 파일/OS 없이 입력만 놓는 별도 물리다.
SOURCE,OTHER,PALETTE,OTHER_PALETTE,NAME=(ARENA+offset for offset in (0,0x1000,0x2000,0x3000,0x4000))  # 명시된 원문 포인터 값이다.
SCRATCH_PARTS=((STACK-0x448,14),(STACK-0x438,40),(STACK-0x40c,1024))  # 기록할 세 지역 구간이며 이웃 FILE/SEH는 포함하지 않는다.


class Oracle(BaseOracle):
    """부모의 헤더/색표/행 순서는 실행하고 BaseFile 경계만 대체한다."""
    def __init__(self):
        """완료된 정밀 함수와 고정된 입력 장치를 준비한다."""
        super().__init__('originals',[dict(name='Bitmap',originals='004ddc90')],[])
        self.mu.mem_map(0,0x1000);self.mu.mem_map(ARENA,0x10000)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def number(self,address,value=None):
        """명시 효과와 실제 인자를 같은 DWORD 폭으로 읽고 쓴다."""
        if value is not None:
            self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def returned(self,value=0,purge=0):
        """thiscall의 실제 ret N과 cdecl을 경계별로 구별한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX,value);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,self.number(sp))

    def guard(self,mu,access,address,size,value,data):
        """실제 부모의 지역 메모리와 SEH 연결 밖 쓰기를 거부한다."""
        if 0x30000000<=address<=address+size<=0x30010000 or address==0 and size==4:
            return
        raise RuntimeError(f'BMP 미명시 쓰기 {address:08x}+{size}')

    def event(self,name,*values):
        """관측 직후 명시한 인자/미정 물리 변이만 적용한다."""
        self.events.append(':'.join((name,*map(str,values))))
        self.calls[name]=self.calls.get(name,0)+1
        trigger=name+str(self.calls[name])
        if self.mode==1 and trigger=='construct1':
            self.number(STACK+20,5);self.number(STACK+24,2)
        if self.mode==2 and trigger=='write1' or self.mode==3 and trigger=='write2':
            self.number(STACK+32,OTHER_PALETTE)
        if self.mode==4 and trigger=='write3':
            for offset,value in ((8,OTHER),(12,1),(16,1),(28,7)):
                self.number(STACK+offset,value)
        if self.mode==5 and trigger=='write4':
            self.number(STACK+28,3);self.number(STACK+8,OTHER)
        if self.mode==6 and trigger=='write4':
            self.number(STACK+20,17);self.number(STACK+24,1)
        if self.mode==7 and trigger=='write1':
            self.mu.mem_write(STACK-0x438+24,b'\xa1\xb2\xc3\xd4');self.mu.mem_write(STACK-0x40c+3,b'\x5a')

    def on_instruction(self,mu,address,size,data):
        """모든 파일 쓰기의 실제 바이트와 순서를 관측한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x41b4c0:
            self.file=mu.reg_read(UC_X86_REG_ECX)
            self.event('construct',self.read_string(self.number(sp+4)).hex(),self.number(sp+8),self.number(sp+12));self.returned(self.file,12);return
        if address==0x41aad0:
            assert mu.reg_read(UC_X86_REG_ECX)==self.file
            self.event('open');self.returned(self.opened);return
        if address==0x4ddd8f:
            self.event('read',self.number(STACK+32),0,1023)
        if address==0x41a910:
            assert mu.reg_read(UC_X86_REG_ECX)==self.file
            source,length,offset=[self.number(sp+value) for value in (4,8,12)]
            if self.calls.get('write',0)>=3:
                root=SOURCE if SOURCE<=source<=SOURCE+512 else OTHER if OTHER<=source<=OTHER+512 else None
                assert root is not None and source+length<=root+512,(source,length)
                self.event('read',root,source-root,length)
            payload=bytes(mu.mem_read(source,length)) if length else b''
            self.output.extend(payload)
            self.event('write',length,offset);self.returned(0,12);return
        if address==0x41b410:
            assert mu.reg_read(UC_X86_REG_ECX)==self.file
            self.event('destroy');self.returned();return
        super().on_instruction(mu,address,size,data)

    def scratch(self):
        """원문에서 실제 파일로 내보내는 지역 구간만 연결한다."""
        return b''.join(bytes(self.mu.mem_read(address,length)) for address,length in SCRATCH_PARTS)

    def observe(self,x,y,width,height,pitch,seed,opened,mode,control):
        """초기 물리와 인자를 설치하고 실제 출력/변경 지역 바이트를 읽는다."""
        self.events=[];self.calls={};self.output=bytearray();self.mode=mode;self.opened=opened
        fill=bytes([seed])*0x10000 if seed in (0,167) else bytes((index*17+seed)&255 for index in range(0x10000))
        self.mu.mem_write(0x30000000,fill);self.number(0,0)
        for root,multiplier,addition,length in ((SOURCE,37,11,512),(OTHER,19,17,512),(PALETTE,13,7,1024),(OTHER_PALETTE,29,3,1024)):
            self.mu.mem_write(root,bytes((index*multiplier+addition)&255 for index in range(length)))  # 각색/행과예약바이트를구별하는실제입력이다.
        self.mu.mem_write(NAME,b'capture.bmp\0')
        before=self.scratch()
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.call('Bitmap',[NAME,SOURCE,x,y,width,height,pitch,PALETTE])
        args=tuple(self.number(STACK+offset) for offset in range(8,36,4))
        return (before.hex(),self.scratch().hex(),bytes(self.output).hex() or '-',':'.join(map(str,args)),';'.join(self.events))


def main():
    """유효한 표면 범위의336입력을 반복 관측해 새 디렉터리에만 저장한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    oracle=Oracle();rows=[]
    shapes=((0,0,0,0,8),(0,0,4,3,8),(1,1,3,2,8),(2,0,7,3,11),(0,0,0,3,8),(0,0,4,0,8),(0,0,1,1,1))  # 0폭/높이·잘라낸홀수행과아래에서위순서다.
    for shape,seed,opened,mode in itertools.product(shapes,(0,167,1),(0,1),range(8)):
        values=(*shape,seed,opened,mode)
        observations=[oracle.observe(*values,control) for control in (0x27f,0x37f,0x27f)]  # 기대값은실제부모출력으로만만든다.
        assert observations[0]==observations[1]==observations[2]
        rows.append((*values,*observations[0]))
    fixture=out/'indexed-bitmap-x86.tsv'
    schema=('x','y','width','height','pitch','seed','opened','mode','scratchIn','scratchOut','fileOut','argsOut','trace')
    fixture.write_text('\t'.join(schema)+'\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows),encoding='utf-8',newline='\n')
    report=dict(unit=694,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),cases=len(rows),observations=len(rows)*3,differences=0,fixtures={fixture.name:hashlib.sha256(fixture.read_bytes()).hexdigest()},boundaries='실제4ddc90의헤더/색표/행/현재인자순서를실행한다. 41b4c0생성·41aad0FILE유무·41a910쓰기·41b410소멸만명시경계다. 생성뒤FILE0도경계입력이며실제fopen실패/32비트SEH를모델링한것이아니다. 지역초기물리3종과콜백변이7종을비교하며파일/게임/MP실행0.',sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_indexed_bitmap.py','tools/decomp_oracle.py')})
    (out/'native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(cases=len(rows),observations=len(rows)*3,differences=0)))


if __name__=='__main__':
    main()
