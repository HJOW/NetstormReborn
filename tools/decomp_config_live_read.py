#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제40380→41baa0/조건XOR/NUL/Append복귀후free와File완료수명을관측한다."""
import argparse
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_basefile_whole_read import WholeReadOracle,BoundaryFailure,ROOT,PLAYERS,TYPES,TEXT,STACK,STOP,ALLOCATION,KEY,digest
from decomp_file_name_leaf import add_ranges
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_FPCW

CONFIG=0x12050000  # Append가받는동일Config의격리포인터다.
POISON=0x65656565  # 미작성File필드의독립입력seed다.


class ConfigOracle(WholeReadOracle):
    """Config부모/전체읽기/문자열순회는실제코드이며ConfigAppend와FILE/new만경계다."""
    def __init__(self):
        """보호이미지를읽고40380의전체실제범위만더한다."""
        super().__init__();add_ranges(self,(0x440380,));self.inside=False

    def instruction(self,mu,address,size,data):
        """같은할당의해독/NUL을원문에맡기고선택장치와Append/free순서를기록한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4)
        if address==0x41baa0:
            assert args[1]==1 and args[3]==1;self.entry=esp;self.returnAddress=self.words(esp,1)[0];self.object=esp-36;self.inside=True;mu.mem_write(self.object,struct.pack('<6I',*([POISON]*6)));self.events.append('W:1:1')
        if address==self.returnAddress:self.fileFields=','.join(map(str,self.fields()));self.inside=False
        if address==0x41ab10:self.events.append('P:'+self.string(args[0]).hex());self.ret(self.probe);return
        if address==0x4e586e:
            target,length,count,file=args;actual=min(length*count,self.available)
            if self.key and self.scratch is None:self.scratch=target;mu.mem_write(target,bytes([101])*4096)
            self.events.append(f'R:{file}:{length}:{count}:{actual}')
            if self.scenario==4:raise BoundaryFailure('read')
            if actual:mu.mem_write(target,self.payload[self.used:self.used+actual])
            self.used+=actual;self.available-=actual;self.position+=actual;self.ret(count if length and actual==length*count else 0);return
        if address==0x43ffa0:
            assert mu.reg_read(UC_X86_REG_ECX)==CONFIG and args[0]==ALLOCATION;self.events.append('A:'+self.string(args[0]).hex())
            if self.scenario==6:raise BoundaryFailure('append')
            self.ret(0,4);return
        if address==0x4e4354 and args[0]==ALLOCATION:
            self.events.append('B:'+str(args[0]))
            if self.scenario==8:raise BoundaryFailure('release')
            self.ret();return
        super().instruction(mu,address,size,data)

    def sample(self,case,precision):
        """본문/미정paddingseed만주고원문의실제할당전체와Append입력을보존한다."""
        payload,self.key,self.subfile,self.probe,self.available,self.scenario=case;self.payload=bytes.fromhex(payload);self.length=len(self.payload);self.used=0;self.scratch=None;self.events=[];self.allocated=0;self.entry=0;self.inside=False;self.returnAddress=0;self.fileFields=None
        self.mu.mem_write(STACK-0x10000,bytes([101])*0x10100);self.mu.mem_write(TEXT,b'input.cfg\0');self.mu.mem_write(KEY,b'abc\0');self.put(0,0);self.mu.mem_write(STACK,struct.pack('<2I',STOP,TEXT));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,CONFIG);self.mu.reg_write(UC_X86_REG_FPCW,precision);outcome='ok'
        try:self.mu.emu_start(0x440380,STOP,count=500000)
        except BoundaryFailure as failure:
            outcome=str(failure)
            if self.inside and self.words(self.entry-4,1)[0]==0:
                self.mu.reg_write(UC_X86_REG_EBP,self.entry);self.mu.reg_write(UC_X86_REG_ESP,STACK+0x100);self.put(STACK+0x100,STOP);self.mu.emu_start(0x4f93a0,STOP,count=10000)
            if self.fileFields is None:self.fileFields=','.join(map(str,self.fields()))
        physical=bytes(self.mu.mem_read(ALLOCATION,self.allocated)).hex() if self.allocated else '-'
        return '\t'.join([str(x) for x in case]+[outcome,self.fileFields,';'.join(self.events),physical])


def stored(payload,key):
    """명시plaintext파일입력에TAFF3BYTE키만적용해저장바이트를만든다."""
    return bytes(value^(b'abc'[index%3] if key else 0) for index,value in enumerate(payload)).hex()


def cases():
    """빈/1·2BYTE/선두NUL·조건XOR/복호화/예외·짧은I/O를독립공급한다."""
    encoded=bytes(value^b'mydoghasfleas'[index%13] for index,value in enumerate(b'mQdsT\r\nx=2\r\n'))
    inputs=(b'',b'a',b'ab',b'x=1\n',b'\0',b'\0\0',b'\0x\0',encoded)
    result=[(stored(payload,key),key,sub,2,len(payload),0) for payload,key,sub in itertools.product(inputs,range(2),range(2))]
    result += [(stored(b'x=1',key),key,sub,0,3,0) for key,sub in itertools.product(range(2),range(2))]
    result += [(stored(b'x=1',key),key,sub,2,3,scenario) for scenario,key,sub in itertools.product((1,2,3,4,5,6,8),range(2),range(2))]
    result += [(stored(encoded,key),key,sub,2,len(encoded)//2,0) for key,sub in itertools.product(range(2),range(2))]
    return result


def main():
    """준비fixture최초관측이나읽기전용재관측만수행한다."""
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert not a.output.exists() and (a.fixture.exists() if a.verify else not a.fixture.exists())
    oracle=ConfigOracle();rows=['# payloadHex key subfile probe available scenario outcome fileFields events allocationHex']
    for case in cases():  # 기대값은실제두정밀도명령의결과에서만읽는다.
        first=oracle.sample(case,0x027f);assert first==oracle.sample(case,0x037f);rows.append(first)
    text='\n'.join(rows)+'\n'
    if a.verify:assert a.fixture.read_text('utf-8')==text
    else:a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(text,encoding='utf-8',newline='\n')
    report=dict(unit=771,rows=len(rows)-1,observations=(len(rows)-1)*2,host=socket.gethostname(),lastFullDecompilation='HJOW-Athlon / 2026-10-23',gameExecutions=0,multiplayerTests=0,boundary='실제40380→41baa0/조건XOR/종료NUL/Append복귀후free·관측marker0의실제4f93a0. FILE/Open/Probe/new/ConfigAppend는명시경계. ConfigAppend내부정규화/UTF-8확장·실제SEH dispatcher/24BYTE ABI는별도. 부분암호scratch101/할당205는독립seed이며미정메모리동일성주장아님',sha256={name:digest(ROOT/name) for name in ('originals/Netstorm.exe','extracted/refined/originals/functions.tsv')});a.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rows=report['rows'],observations=report['observations'])))


if __name__=='__main__':main()
