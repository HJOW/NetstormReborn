# -*- coding: utf-8 -*-
"""원문 포인터 메뉴 열기/음향과 Ctrl+special 진단 부모의 호출 순서를 관측한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent  # 실제 보호 PE와 읽기 전용 내보내기 루트다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
from decomp_oracle import Oracle,SCRATCH,STACK,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL,TABLE=0x12000000,SCRATCH+0x1000  # 명시 외부 가상효과를 담는 격리된50BYTE 슬롯/표다.


class PointerOracle(Oracle):
    """Userinput 함수의 두 검토한 기본블록만 실행하고 하위 효과는 명시 경계로 분리한다."""
    def __init__(self):
        """게임/창/통신을 열지 않고 원문 코드와 외부 입력을 준비한다."""
        super().__init__('originals',[dict(name='Input',originals='004d62b0')],[])
        self.mu.mem_map(POOL,0x1000)
        self.trace=[]  # 선택된 하위가 받은 실제 인자/순서다.
        self.sequence=[]  # finder가 외부 경계로 반환할 명시 후보 순서다.
        self.index=0  # 현재 외부 finder의 진행 위치다.
        self.result=0  # 실제 가상ac 반환의 명시 입력이다.

    def ret(self,purge=0):
        """현재 cdecl/thiscall의 반환 폭을 원래대로 적용한다."""
        stack=self.mu.reg_read(UC_X86_REG_ESP);target=struct.unpack('<I',self.mu.mem_read(stack,4))[0]
        self.mu.reg_write(UC_X86_REG_ESP,stack+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def on_instruction(self,mu,address,size,data):
        """분기는 원문으로 실행하며 지정한 종료/외부 경계만 처리한다."""
        if address in (0x4da477,0x4da2e8):
            mu.emu_stop();return
        stack=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4adb30:
            pointer=struct.unpack('<I',mu.mem_read(stack+4,4))[0];args=struct.unpack('<Iii',mu.mem_read(pointer,12))
            self.trace.append('O,50,'+','.join(map(str,args)));mu.reg_write(UC_X86_REG_EAX,self.result);self.ret(4);return
        if address==0x4a9cb0:
            values=struct.unpack('<IIIIII',mu.mem_read(stack+4,24));name=bytes(mu.mem_read(values[0],20)).split(b'\0')[0].decode('ascii')
            self.trace.append('S,'+name+','+','.join(map(str,values[1:])));mu.reg_write(UC_X86_REG_EAX,999);self.ret();return
        if address==0x4971e0:
            pointer,x,y=struct.unpack('<Iii',mu.mem_read(stack+4,12));self.trace.append(f'P,{x},{y}')
            mu.mem_write(pointer,struct.pack('<ff',123.25,-9.5));self.ret();return
        if address==0x4202f0:
            x,y,flags=struct.unpack('<III',mu.mem_read(stack+4,12));self.trace.append(f'B,{x},{y},{flags}');self.index=0
            mu.mem_write(mu.reg_read(UC_X86_REG_ECX)+52,struct.pack('<I',self.sequence[0] if self.sequence else 0));self.ret(12);return
        if address==0x4b0fb0:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;self.trace.append(f'V,{sid}');self.ret();return
        if address==0x4b1810:
            self.index+=1;self.trace.append('N');mu.mem_write(mu.reg_read(UC_X86_REG_ECX)+52,struct.pack('<I',self.sequence[self.index] if self.index<len(self.sequence) else 0));self.ret();return
        return super().on_instruction(mu,address,size,data)

    def prepare(self,code,x,y):
        """큐의 같은12BYTE 사건과 가상 표만 설치한다."""
        self.trace=[];self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK+0x14,struct.pack('<Iii',code,x,y))
        self.mu.mem_write(0x5c8464,struct.pack('<I',POOL))
        for sid in (50,51):  # 후보 중복도 같은 원문 물리 슬롯으로 돌려준다.
            self.mu.mem_write(POOL+sid*50,struct.pack('<I',TABLE)+bytes(46))
        self.mu.mem_write(TABLE+0xac,struct.pack('<I',0x4adb30));self.mu.mem_write(TABLE+0x70,struct.pack('<I',0x4b0fb0))

    def open(self,result,x,y):
        """가상ac의0/비영 DWORD가 음향 분기를 고르는지 관측한다."""
        self.prepare(0x20000,x,y);self.result=result;self.mu.reg_write(UC_X86_REG_ESI,POOL+50*50)
        self.mu.emu_start(0x4d9df0,STOP,count=3000);assert self.mu.reg_read(UC_X86_REG_EIP)==0x4da477 and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        return ';'.join(self.trace)

    def area(self,special,control,sequence,x,y):
        """Select 뒤 현재special 값에서 검토한 순회 기본블록만 실행한다."""
        self.prepare(0x10000|(0x10000000 if control else 0),x,y);self.sequence=([],[50],[50,51,50])[sequence]
        self.mu.reg_write(UC_X86_REG_EAX,special);self.mu.emu_start(0x4da263,STOP,count=3000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==0x4da2e8 and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        return ';'.join(self.trace) or '-'


def save(name,rows,verify):
    """새 파일만 생성하거나 기존 파일의 바이트 재현 일치를 검사한다."""
    content=('\n'.join(rows)+'\n').encode('utf-8');path=ROOT/'cpppj/tests/fixtures'/name
    if verify:
        assert path.read_bytes()==content
    else:
        assert not path.exists();path.write_bytes(content)
    return dict(path=path.relative_to(ROOT).as_posix(),rows=len(rows)-1,sha256=hashlib.sha256(content).hexdigest())


def main():
    """메뉴8행과진단36행을서로다른fixture에고정한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();oracle=PointerOracle()
    bodies={f'{first:08x}-{last-1:08x}':bytes(oracle.mu.mem_read(first,last-first)) for first,last in ((0x4d9df0,0x4d9e1f),(0x4da263,0x4da2e8))}  #실제기본블록의PE바이트를고정한다.
    rows=['# result x y trace']
    for result,(x,y) in itertools.product((0,1,2,0xffffffff),((100,200),(-700,800))):  # 실제비영DWORD와음수사건좌표도유지한다.
        rows.append('\t'.join(map(str,(result,x,y,oracle.open(result,x,y)))))
    opening=save('pointer-object-menu-parent-x86.tsv',rows,args.verify);rows=['# special control sequence x y trace']
    for special,control,sequence,(x,y) in itertools.product((0,1,2),(0,1),range(3),((100,200),(-700,800))):  # 거부/빈순회/단일/중복후보를교차한다.
        rows.append('\t'.join(map(str,(special,control,sequence,x,y,oracle.area(special,control,sequence,x,y)))))
    area=save('pointer-special-trace-x86.tsv',rows,args.verify)
    assert all(body==bytes(oracle.mu.mem_read(int(key[:8],16),len(body))) for key,body in bodies.items())
    record=dict(host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',originalGameExecuted=False,verifiedExistingFixtures=args.verify,opening=opening,area=area,originalSha256=oracle.sha256,codeImageUnchanged=True,codeSha256={key:hashlib.sha256(body).hexdigest() for key,body in bodies.items()},scope='4d9df0..9e1e 및4da263..2e6 기본블록 호출/인자/분기. 가상ac/70·음향·투영·finder는명시외부경계이며실제하위/게임전체아님')
    (ROOT/'extracted/cpp-object-menu-open-parent-750/native.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(record,ensure_ascii=False))


if __name__=='__main__':
    main()  # 보호PE/기존fixture/MP상태를수정하지않는다.
