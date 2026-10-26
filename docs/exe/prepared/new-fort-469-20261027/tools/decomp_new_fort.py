#!/usr/bin/env python3
"""NewFort 실제 x86 세 부모를 격리 실행하고 큰 외부 장치만 명시 관찰한다."""
import argparse
import hashlib
import itertools
import json
import struct
import sys
from pathlib import Path

# 보호 원본과 기존 Oracle는 읽기만 하며 추가 자료는 staging에만 기록한다.
STAGE=Path(__file__).resolve().parents[1]
ROOT=STAGE.parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from decomp_oracle import Oracle,SCRATCH
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

# 아래 주소는 모두 격리된 명시 입력의 물리이며 실제 게임/OS 주소공간이 아니다.
HEADER,NAME,OTHER,CORE,STATES,ALTERNATE,PATH=SCRATCH+0x1000,SCRATCH+0x2000,SCRATCH+0x2200,SCRATCH+0x3000,SCRATCH+0x4000,SCRATCH+0x5000,SCRATCH+0x5500
ALLOCATIONS=(SCRATCH+0x6000,SCRATCH+0x6200)


class NewFortOracle(Oracle):
    """보호 부모·manager 생성자·Core10BYTE clear 본문을 실제 CPU에서 실행한다."""
    def __init__(self):
        """새 함수와 기존 순수 하위를 허용하고 모든 Win32/파일/GUI는 금지한다."""
        super().__init__('originals',[dict(name='manager',originals='004bfba0'),dict(name='data',originals='004b3920'),dict(name='missing',originals='004b5ca0')],[0x4bcc60,0x42b200])
        self.allowed.extend(((0x4bcca0,0x4bcca4),(0x44d870,0x44d879)));self.code={lo:bytes(self.mu.mem_read(lo,hi-lo)) for lo,hi in self.allowed} #누락된4BYTE가상이름getter와NULL검사용초기몸체는보호PE의직접바이트범위로명시한다.
        self.put(0x514690,0x4bcca0)

    def put(self,address,value):
        """고정DWORD 전역을 낮은 BYTE부터 기록한다."""
        self.mu.mem_write(address,struct.pack('<I',value&0xffffffff))

    def get(self,address):
        """동일 물리의 현재DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def ret(self,result=0,purge=0):
        """cdecl/thiscall의 실제 인자 정리 폭만큼 명시 장치에서 복귀한다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP);target=self.get(sp);self.mu.reg_write(UC_X86_REG_EAX,result&0xffffffff);self.mu.reg_write(UC_X86_REG_ESP,sp+4+purge);self.mu.reg_write(UC_X86_REG_EIP,target)

    def event(self,name):
        """실제 도달한 장치 순서를 기록한 뒤 선택한 외부 재진입 효과만 적용한다."""
        self.trace.append(name);key=name.split(':',1)[0];self.counts[key]=self.counts.get(key,0)+1;count=self.counts[key]
        if self.mutation==1 and key=='translate':self.put(0x540ca8,OTHER)
        if self.mutation==2 and key=='title' and count==1:self.put(0x5c85a4,1-self.get(0x5c85a4))
        if self.mutation==3 and key=='sp':self.put(0x540cac,2)
        if self.mutation==4 and key=='core':self.put(0x540cac,3)
        if self.mutation==5 and key=='prepare' and count==1:self.put(0x5c89b4,0xbbbb0000|(self.get(0x5c89b4)&0xffff))
        if self.mutation==6 and key=='next' and name.startswith('next:65536') and count==1+(not self.editor):self.put(0x5c89b4,0x77770000|(self.get(0x5c89b4)&0xffff))
        if self.mutation==7 and key=='subscriber':self.put(0x540cac,4)
        if self.mutation==8 and key=='money':self.put(0x5c85a4,1)
        if self.mutation==9 and key=='state':self.put(0x540cac,5);self.put(0x5c85a4,1-self.get(0x5c85a4))
        if self.mutation==10 and key=='config' and count==2:self.put(0x540ca8,OTHER)
        if self.mutation==11 and key=='register':self.put(0x540c70,3)
        if self.mutation==12 and key=='coreWrite':self.put(0x5c89b4,0xdeadffff)

    def on_instruction(self,mu,address,size,data):
        """이름복사·부분쓰기·loop는원문에남기고하위장치만독립trace로대체한다."""
        sp=mu.reg_read(UC_X86_REG_ESP);this=mu.reg_read(UC_X86_REG_ECX)
        def arg(index):
            """현재 실제 스택의 한DWORD 인자를 읽는다."""
            return self.get(sp+4+index*4)
        def slot():
            """현재 this의 고정 Template 번호를 원문20BYTE 간격에서 구한다."""
            return (this-0x5c89f0)//20
        if address==0x4e0620:self.event('assert:'+str(arg(2)));self.ret();return
        if self.action=='manager':
            if address==0x41ab10:self.event('exists:'+self.read_string(arg(0)).hex());self.ret(self.exists);return
            if address==0x4e4391:
                self.event('allocate:'+str(arg(0)));value=self.allocations[self.allocation_index];self.allocation_index+=1;self.ret(value);return
            if address==0x4bcc60:self.event('construct:'+str(this)+':'+self.read_string(arg(0)).hex())
            elif address==0x4bcca0:self.event('fileName:'+str(this))
            elif address==0x41aef0:self.event('erase:'+self.read_string(arg(0)).hex());self.ret();return
            elif address==0x44da60:self.event('destroy:'+str(this));self.ret();return
            elif address==0x4e4354:self.event('release:'+str(arg(0)));self.ret();return
            elif address==0x44d870:
                self.event('initialize:'+str(this))
                if this:self.ret();return
        if self.action=='data':
            if address==0x4de9a0:self.event('translate');self.ret(PATH,4);return
            if address==0x4414d0:self.event('config:'+self.read_string(arg(0)).decode('ascii')+':'+self.read_string(arg(1)).hex());self.ret();return
            if address==0x460730:self.event('title');self.ret();return
            if address==0x4b38d0:self.event('register');self.put(0x540c74,0xfedcba98);self.ret();return
            if address==0x4559d0:self.event('prepare');self.ret();return
            if address==0x4558c0:
                bound=arg(0);self.event('next:'+str(bound));value=self.random254 if bound==254 else self.randoms.pop(0);self.ret(value);return
            if address==0x40ec50:self.event('sp:'+str(arg(0)));self.put(arg(1),self.spbits);self.ret();return
            if address==0x40eec0:self.event('addSp:'+str(arg(0))+':'+str(self.get(0x540c70)));self.ret();return
            if address==0x48f0d0:self.event('core');self.ret(CORE);return
            if address==0x4beb30:self.event('coreWrite:'+str(slot()));self.ret();return
            if address==0x4bd400:self.event('subscriber:'+str(slot())+':'+str(arg(0))+':'+self.read_string(arg(1)).hex());self.ret(purge=8);return
            if address==0x4bd600:self.territory=bytes(mu.mem_read(arg(0),120));self.event('territory:'+str(slot())+':'+self.territory.hex());self.ret(purge=4);return
            if address==0x4beec0:self.event('money:'+str(slot()));self.ret();return
            if address==0x4bd550:
                chosen=slot();self.event('state:'+str(chosen));self.ret(STATES+chosen*32);return
            if address==0x4bd360:self.event('notify:'+str(slot()));self.ret();return
        if self.action=='missing':
            if address==0x460680:self.event('path');self.ret(PATH);return
            if address==0x41ab10:self.event('exists:'+self.read_string(arg(0)).hex());self.ret(0);return
            if address==0x4bfba0:self.event('manager:'+str(slot())+':'+self.read_string(arg(0)).hex()+':'+str(arg(1)));self.ret(purge=8);return
            if address==0x4bd550:
                chosen=slot();self.event('state:'+str(chosen));self.ret(STATES+chosen*32);return
            if address==0x4bd540:self.event('notify:'+str(slot()));self.ret();return
            if address==0x4b3920:self.event('data');self.ret();return
            if address==0x4bd450:self.event('subscriberLoad:'+str(slot())+':'+str(arg(0))+':'+self.read_string(arg(1)).hex());self.ret(purge=8);return
            if address==0x4414d0:self.event('config:'+self.read_string(arg(0)).decode('ascii')+':'+self.read_string(arg(1)).hex());self.ret();return
            if address==0x460730:self.event('title');self.ret();return
        return super().on_instruction(mu,address,size,data)

    def reset(self,action,mutation=0):
        """각 관측의 raw 입력과 외부 장치 자료를 독립 초기화한다."""
        self.action=action;self.mutation=mutation;self.trace=[];self.counts={};self.editor=1;self.territory=b''
        self.mu.mem_write(NAME,b'Before\0'+bytes([0xa5])*249);self.mu.mem_write(OTHER,b'Other\0'+bytes([0xa5])*250);self.mu.mem_write(CORE,bytes([0xa5])*16);self.mu.mem_write(STATES,bytes([0xa5])*288)
        self.put(0x540ca8,NAME);self.put(0x540cac,1);self.put(0x540c70,1);self.put(0x540c74,123);self.put(0x5c85a4,1);self.put(0x595344,1);self.put(0x595348,0x12345678);self.put(0x5c89b4,0xa55a6789);self.put(ALTERNATE,0xabcdef01);self.mu.mem_write(PATH,b'd/NewFort.fort\0')

    def manager(self,old,exists,argument,pattern):
        """실제 생성자와 header부분쓰기·파일삭제·공개 순서를 관찰한다."""
        self.reset('manager');self.exists=exists;self.allocation_index=0;self.allocations=[ALLOCATIONS[i] if bit=='1' else 0 for i,bit in enumerate(pattern)];self.mu.mem_write(HEADER,bytes([0xa5])*20);self.put(HEADER,old);self.put(0x5c89c0,0xdeadbeef)
        for pointer in ALLOCATIONS:self.mu.mem_write(pointer,bytes([0xa5])*276)
        completed=1;fault=0
        try:self.call('manager',[PATH,argument],HEADER,8)
        except UcError:completed=0;fault=self.mu.reg_read(UC_X86_REG_EIP)
        return completed,hex(fault),bytes(self.mu.mem_read(HEADER,20)).hex(),self.get(0x5c89c0),','.join(bytes(self.mu.mem_read(p,276)).hex() for p in ALLOCATIONS),';'.join(self.trace)

    def data(self,editor,known,spbits,seed,random254,randoms,mutation):
        """직접 쓰는 handle/Core/seed/State와 전체120BYTE 및 현재 재조회 trace다."""
        self.reset('data',mutation);self.editor=editor;self.spbits=spbits;self.random254=random254;self.randoms=list(randoms);self.put(0x5c85a4,editor);self.put(0x595344,known);self.put(0x5c89b4,seed);self.mu.mem_write(PATH,b'Unnamed\0');self.call('data',[])
        return self.get(0x540c74),bytes(self.mu.mem_read(NAME,16)).hex(),bytes(self.mu.mem_read(OTHER,16)).hex(),self.get(0x595348),bytes(self.mu.mem_read(CORE,16)).hex(),self.get(0x5c89b4),bytes(self.mu.mem_read(STATES,288)).hex(),self.territory.hex(),';'.join(self.trace)

    def missing(self,owner,mutation):
        """실제 파일 부재 분기를 정상 RET까지 실행하며 뒤 owner/state를 다시 읽는다."""
        self.reset('missing',mutation);self.put(0x540cac,owner);self.call('missing',[ALTERNATE]);return self.get(ALTERNATE),bytes(self.mu.mem_read(STATES,288)).hex(),';'.join(self.trace)


def main():
    """최초 추가 fixture만 만들거나 실제 원문 재실행 전체 바이트를 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args();oracle=NewFortOracle();outputs={}
    rows=[]
    for old,exists,argument,pattern in itertools.product((0,0x12345678),(0,1),(0,1,0x89abcdef),('00','01','10','11')):
        inputs=(old,exists,argument,pattern);rows.append('\t'.join(map(str,(*inputs,*oracle.manager(*inputs)))))
    outputs['new-fort-manager-x86.tsv']=('# old/exists/argument/allocPattern/completed/faultPC/header/version/managers/trace\n'+'\n'.join(rows)+'\n').encode('utf8')
    rows=[]
    for editor,known,spbits,seed,random254,sequence,mutation in itertools.product((0,1),(0,1),(0,0x44fa0000,0x80000000),(0xa55a0000,0x12345678),(0,253),((0x1234,),(0,0,0x9876)),range(13)):
        inputs=(editor,known,spbits,seed,random254,','.join(map(str,sequence)),mutation);observed=oracle.data(editor,known,spbits,seed,random254,sequence,mutation);rows.append('\t'.join(map(str,(*inputs,*observed))))
    outputs['new-fort-data-x86.tsv']=('# editor/known/spbits/seed/random254/sequence/mutation/subscriber/handle/other/coreActive/core/seedAfter/states/territory/trace\n'+'\n'.join(rows)+'\n').encode('utf8')
    rows=[]
    for owner,mutation in itertools.product((1,8),range(13)):rows.append('\t'.join(map(str,(owner,mutation,*oracle.missing(owner,mutation)))))
    outputs['new-fort-missing-x86.tsv']=('# owner/mutation/alternate/states/trace\n'+'\n'.join(rows)+'\n').encode('utf8')
    target=STAGE/'fixtures';target.mkdir(exist_ok=True);report=dict(host='HJOW-Athlon',lastFullDecompile='HJOW-Athlon / 2026-10-23',originalGameRuns=0,cloneGameRuns=0,osCalls=0,binarySha256=oracle.sha256,toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixtures={},nativeRanges={hex(k):v.hex() for k,v in oracle.code.items()},limits=['실제 세 부모와manager004bcc60/가상이름getter004bcca0/Core10BYTEclear0042b200 실행','파일/GUI/공유난수값/등록/섹션직렬화/Stategetter등큰외부하위는명시장치','manager 할당0의 NULL가상접근/initialize0은원문접근실패이며정상RET아님','NewDataEditor/known/float/seed0재시도와13개재진입효과를교차','기존fixture/제품/검사/디컴파일변경없음; staging C++ 미빌드'])
    for name,payload in outputs.items():
        path=target/name
        if args.verify:
            if path.read_bytes()!=payload:raise RuntimeError('NewFort 최초 fixture와 재관찰 불일치:'+name)
        else:
            if path.exists():raise RuntimeError('NewFort 최초 fixture 덮어쓰기 금지:'+name)
            path.write_bytes(payload)
        report['fixtures'][name]=dict(rows=len(payload.splitlines())-1,sha256=hashlib.sha256(payload).hexdigest())
    (STAGE/'new-fort-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8');print(json.dumps(report['fixtures'],ensure_ascii=False))


if __name__=='__main__':main()
