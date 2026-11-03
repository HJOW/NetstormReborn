#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제4dae60/407171/File Read·소멸과 정상 작은 LZW를 메모리에서 관측한다."""
import argparse,itertools,json,socket,struct,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]  # 채택한 tools/에서는 실제저장소를읽기만한다.
if not (ROOT/'originals/Netstorm.exe').is_file():ROOT=Path('H:/T5Workspace/NetstormReborn')  # 격리준비본은같은호스트의확인된보호저장소만빌린다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'));sys.path.insert(0,str(ROOT/'tools'))
from decomp_basefile_whole_read import WholeReadOracle,BoundaryFailure,PLAYERS,TYPES,TEXT,STACK,STOP,ALLOCATION,KEY,digest
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_GDTR,UC_X86_REG_CS,UC_X86_REG_SS,UC_X86_REG_DS,UC_X86_REG_ES

OUTPUT,PIXELS=TEXT+0x200,TYPES+0x10000  # 치수 출력과 bitmap은 입력할당/키와 다른 주소다.

def gif(width,height,color,invalid=False):
    """기대 픽셀을 decoder에서 얻지 않고 명시clear/color 코드로 입력만 쓴다."""
    codes=[code for unused in range(width*height) for code in (4,color)]+[5]
    bits=sum(code<<(3*index) for index,code in enumerate(codes));payload=bits.to_bytes((len(codes)*3+7)//8,'little')
    return b'GIF89a'+struct.pack('<HHBBB',width,height,0,0,0)+b'\x2c'+struct.pack('<HHHHB',0,0,width,height,0)+bytes((1 if invalid else 2,len(payload)))+payload+b'\x00\x3b'

class GifOracle(WholeReadOracle):
    """파일부모/치수/정리와 선택한 정상 LZW만 실제 명령으로 수행한다."""
    def __init__(self):
        """기존 File 관측장치를 공유하며 이 후보의 확인한 함수만 허용한다."""
        super().__init__();add_ranges(self,(0x4dae60,0x41aad0,0x407171,0x4fc1a0,0x406ef7,0x406d90,0x406dd8,0x406df1,0x406e37,0x406e7d))
        self.mu.mem_write(TEXT+0x4000,struct.pack('<QQQ',0,0x00cf9a000000ffff,0x00cf92000000ffff));self.mu.reg_write(UC_X86_REG_GDTR,(0,TEXT+0x4000,23,0));self.mu.reg_write(UC_X86_REG_CS,8)
        for register in (UC_X86_REG_SS,UC_X86_REG_DS,UC_X86_REG_ES):self.mu.reg_write(register,16)  # 실제 LZW 행의 ES 복구에도 같은32비트 평면을 유지한다.

    def instruction(self,mu,address,size,data):
        """OS/할당/행복사만 관측하고 부모 치수 쓰기와 File/key 반납은 실제코드다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4)
        if address==0x41b4c0:
            assert mu.reg_read(UC_X86_REG_ECX)==self.object and args[1:3]==(0,1);self.events.append('O')
            mu.mem_write(self.object,struct.pack('<6I',0 if self.scenario==2 else PLAYERS,0x80|self.subfile,37 if self.subfile else 0,len(self.payload) if self.subfile else 0,KEY if self.key else 0,3 if self.key else 0));self.position=37 if self.subfile else 0
            if self.scenario==1:raise BoundaryFailure('open')
            self.ret(0,12);return
        if address==0x4e0620:self.events.append('D:'+str(args[2]));raise BoundaryFailure('assertion')
        if address==0x43fa80:
            stage='I' if not self.inputCalled else 'B';self.events.append(stage+':'+str(args[0]));self.inputCalled=True
            if self.scenario==(3 if stage=='I' else 5):raise BoundaryFailure('input' if stage=='I' else 'bitmap')
            target=ALLOCATION if stage=='I' else PIXELS;mu.mem_write(target,b'\xcd'*args[0]);self.ret(target);return
        if address==0x4e566c:self.events.append('T');self.ret(self.position);return
        if address==0x4e573c:self.events.append('S:'+str(args[2]));self.position=len(self.payload) if args[2]==2 else args[1];self.ret();return
        if address==0x4e586e:
            target,length,count,file=args;actual=min(length*count,self.available)
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes(4096))  # 부분 암호읽기의 미정 stack은 명시0 seed로만 관측한다.
            if not self.key:assert bytes(mu.mem_read(target,length))==bytes(length)  # 부모가 exactLength 전체를 이미0으로 썼다.
            self.events.append('R:'+str(length)+':'+str(actual))
            if self.scenario==4:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,self.stored[self.used:self.used+actual])
            self.used+=actual;self.available-=actual;self.position+=actual;self.ret(count if length and actual==length*count else 0);return
        if address==0x406ef7:
            assert self.fields()[0]==PLAYERS and self.words(OUTPUT,2)==(self.width,self.height);self.events.append('X');self.inputHex=bytes(mu.mem_read(ALLOCATION,len(self.payload))).hex()
            pane,source=args[:2];assert source==ALLOCATION
            if self.scenario==6:raise BoundaryFailure('decode')  # 잘못된LZW 입력의 host예외 경계이며 원문decoder가throw한다고 주장하지 않는다.
            if not self.actual:self.ret(0,8);return
        if address==0x406939:
            pane,y,row,width=args;assert width==self.width and y<self.height;mu.mem_write(PIXELS+y*width,bytes(mu.mem_read(row,width)));self.rows+=1;self.ret(0,16);return
        if address==0x43fa90:
            self.events.append('F')
            if self.scenario==7:raise BoundaryFailure('inputFree')
            self.ret();return
        if address==0x4e54b9:
            self.events.append('C')
            if self.scenario==8:raise BoundaryFailure('close')
            self.ret();return
        if address==0x4e4354:self.events.append('K');self.ret();return
        if not any(low<=address<high for low,high in self.allowed):raise RuntimeError('범위밖 '+hex(address))

    def sample(self,case,precision):
        """같은 Native 결과에서 정확 입력·두 출력·현재File·효과열·선택 픽셀을 읽는다."""
        self.width,self.height,color,self.key,self.subfile,partial,self.scenario,self.actual=case;self.payload=gif(self.width,self.height,color,self.scenario==6);self.stored=bytes(value^(b'abc'[i%3] if self.key else 0) for i,value in enumerate(self.payload));self.available=len(self.payload)-partial;self.used=0;self.scratch=None;self.inputCalled=False;self.events=[];self.inputHex='-';self.rows=0;self.object=STACK-108
        self.mu.mem_write(STACK-0x10000,b'\x65'*0x10100);self.mu.mem_write(TEXT,b'input.gif\0');self.mu.mem_write(KEY,b'abc\0');self.put(OUTPUT,0xdeadbeef);self.put(OUTPUT+4,0x12345678);self.put(0,0);self.mu.mem_write(ALLOCATION,b'\xcd'*256);self.mu.mem_write(PIXELS,b'\xcd'*256)
        self.mu.mem_write(STACK,struct.pack('<4I',STOP,TEXT,OUTPUT,OUTPUT+4));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(0x4dae60,STOP,count=200000)
        except BoundaryFailure as error:
            outcome=str(error)
            if self.words(STACK-4,1)[0]==0:self.mu.reg_write(UC_X86_REG_EBP,STACK);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4fc1a0,STOP,count=10000)
        if outcome=='ok':assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4 and (not self.actual or self.rows==self.height)
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision
        pixels=bytes(self.mu.mem_read(PIXELS,self.width*self.height)).hex() or '-'
        return '\t'.join([','.join(map(str,case)),outcome,','.join(map(str,self.words(OUTPUT,2))),','.join(map(str,self.fields())),';'.join(self.events),self.inputHex,pixels,str(self.rows)])

def cases():
    """정상 실제LZW/명시decoder와부분읽기/출력앞뒤예외를분리한다."""
    result=[(*dimensions,key,sub,0,0,actual) for dimensions,key,sub,actual in itertools.product(((2,1,0),(3,2,3)),range(2),range(2),range(2))]
    result.extend((3,2,3,key,sub,2,0,0) for key,sub in itertools.product(range(2),range(2)))
    result.extend((3,2,3,key,sub,0,scenario,0) for scenario,key,sub in itertools.product(range(1,9),range(2),range(2)))
    result.extend((width,height,0,0,sub,0,0,0) for (width,height),sub in itertools.product(((0,1),(1,0)),range(2)))
    return result

def main():
    """새 fixture 최초관측 또는 명시verify만 수행한다. 제품/기존자료는 쓰지 않는다."""
    parser=argparse.ArgumentParser();parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--verify',action='store_true');args=parser.parse_args();assert not args.output.exists() and (args.fixture.exists() if args.verify else not args.fixture.exists())
    oracle=GifOracle();rows=['# case outcome dimensions fields events inputHex pixelHex actualRows']
    for case in cases():  # 두 x87 정밀도의 부모/치수/실제정리/선택정상픽셀을 대조한다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    content='\n'.join(rows)+'\n'
    if args.verify:assert args.fixture.read_text('utf-8')==content
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(content,encoding='utf-8',newline='\n')
    report={'unit':781,'host':socket.gethostname(),'rows':len(rows)-1,'observations':2*(len(rows)-1),'actualLzwCases':sum(case[-1] for case in cases()),'actualLzwOutputRowsPerPrecision':sum(case[1] for case in cases() if case[-1]),'lastFullDecompilation':'HJOW-Athlon / 2026-10-23','gameExecutions':0,'newMultiplayerTests':0,'boundary':'실제4dae60/407171/41a7a0/41aa80/41b410/4fc1a0. 정상8행은실제LZW/행복사경계이며그외decoder는명시반환/throw장치. validDims+invalidLZW의decode throw는host경계이고원문LZW의예외/성공합성이아니다. Open/CRT/malloc/free는명시. 전체SEH dispatcher/OS미정힙일치없음.','sha256':{str(path):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',Path(__file__),args.fixture)}};args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps({'rows':len(rows)-1,'observations':2*(len(rows)-1)}))

if __name__=='__main__':main()
