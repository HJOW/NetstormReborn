#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""004c7160 전체 Uber 부모와004c77c0 루트교체를 원본 PE에서 격리 관측한다."""
import argparse
from collections import Counter
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import decomp_production_shortcuts as image
from unicorn import UC_HOOK_MEM_READ
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]  # 현재 작업본과 보호 입력의 루트다.
FIXTURE=ROOT/'cpppj/tests/fixtures/uber-root-x86.tsv'  # 최초 부모 관측이며 verify는 덮어쓰지 않는다.
REPORT=ROOT/'extracted/cpp-uber-root-395-native.json'  # 명시 장치/실제 명령/ABI/해시다.
NODES,STRIDE,COUNT=0x1100000,0x200,32  # 고정 원시 노드/격리 간격/물리 슬롯 수다.
TABLE,MOVE,PUSH,ATTACH,END=image.HEAP,image.HEAP+0x2000,image.HEAP+0x2010,image.HEAP+0x2020,image.HEAP+0x3000  # 하위 가상 함수/정상 반환점이다.
REGISTRY,COUNT_AT,SERIAL=0x563e38,0x565dc8,0x540418  # 실제 Gump 등록표/개수/serial이다.
TITLE,FORMAT=0x565e70,image.HEAP+0x3100  # 현재 메시지와 명시 formatter 결과 토큰이다.
RANGES=((0x4c7160,0x4c7796),(0x4c77a0,0x4c77b3),(0x4c77c0,0x4c786e))  # 특수 메시지의 별도 정상 epilogue도 원문이다.
GLOBALS=(0x540451,0x5411c8,0x542664,0x5ca920,0x5ca924,0x5ca928,0x5c78f0,0x5c78f4,0x531860,0x531864,0x540cec,0x540cf8,0x542a04)  # 현재 읽기/쓰기 전역이다.
FIELDS={'flags':4,'parent':0x14,'child':0x18,'next':0xc,'previous':0x10,'id':0x1c,'kind':0x20,'serial':0x2c,'scale':0x38,'baseWidth':0x3c,'baseHeight':0x40,'width':0x44,'height':0x48,'x':0x4c,'y':0x50,'vertical':0xa0,'minimapDivider':0xa4,'chatDivider':0xa8,'minimap':0xac,'production':0xb0,'chat':0xb8,'knowledge':0xc0,'message':0xc4}  # callback 입력의 원시 주소 계약이다.
SCHEMA=('name','operation','globals','scales','message','test','children','fill','mutation','stop','fail','completed','result','globalsout','registry','nodes','trace','instructions')  # 앞11열만 실행 입력이다.

def bits(value):
    """단정도 원시 DWORD다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]

def signed(value):
    """현재 DWORD를 signed32로 읽는다."""
    return struct.unpack('<i',struct.pack('<I',value&0xffffffff))[0]

def node(index):
    """고정 물리 노드의 원본 포인터 토큰이다."""
    return NODES+index*STRIDE

def pairs(values):
    """주소/값 순서를 보존한 명시 DWORD 목록이다."""
    return ','.join(f'{address}:{value&0xffffffff}' for address,value in values)

def rle(values):
    """실제 등록표의 연속 동일값만 압축한다."""
    return ';'.join(f'{len(list(group))}:{value}' for value,group in itertools.groupby(values))

class Stop(Exception):
    """명시 callback 중단이며 정상 RET와 구별한다."""

class Fault(Exception):
    """원본의 null 자식 역참조를 정상 생성으로 취급하지 않는다."""

class Oracle(image.Oracle):
    """원본 두 부모를 실행하고 이미 복원된 외부 자식만 명시 장치로 공급한다."""
    def __init__(self):
        """보호 PE/고정 하위 노드/가상 함수/SEH 페이지를 마련한다."""
        super().__init__();self.mu.mem_map(NODES,COUNT*STRIDE);self.mu.mem_map(0,0x1000)
        self.number(TABLE+0x10,MOVE);self.number(TABLE+0x18,PUSH);self.number(TABLE+0x20,ATTACH)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read_guard)
        self.original={lo:bytes(self.mu.mem_read(lo,hi-lo)) for lo,hi in RANGES}

    def string(self,address):
        """명시 PE 문자열/현재 메시지의 첫 NUL까지만 읽는다."""
        result=bytearray()
        for offset in range(512):  # 원문 C문자열의 유한 입력 계약이다.
            value=self.number(address+offset,width=1)
            if not value:return bytes(result)
            result.append(value)
        raise RuntimeError('Uber 문자열 NUL 없음')

    def read_guard(self,mu,access,address,size,value,data):
        """FS:0의 SEH외에는 null 노드의 가짜 zero페이지 성공을 허용하지 않는다."""
        if address<0x1000 and not (address==0 and size==4 and self.number(mu.reg_read(UC_X86_REG_EIP),width=1)==0x64):raise Fault(f'null-read:{address:x}')

    def guard(self,mu,access,address,size,value,data):
        """원문 CPU 쓰기를 노드/등록표/세 배경전역/스택/SEH로 한정한다."""
        if image.STACK-0x8000<=address and address+size<=image.STACK+0x1000: key='stack'
        elif NODES<=address and address+size<=NODES+COUNT*STRIDE:key='nodes'
        elif REGISTRY<=address and address+size<=REGISTRY+4000:key='registry'
        elif any(a<=address and address+size<=a+4 for a in (0x5ca920,0x5ca924,0x5ca928)):key='globals'
        elif address==0 and size==4 and self.number(mu.reg_read(UC_X86_REG_EIP),width=1)==0x64:key='seh'
        else:raise RuntimeError(f'미명시 Uber 쓰기 {address:x}/{size}')
        self.writes[key]+=1

    def event(self,name,values=()):
        """명시 기본효과 뒤 기록/현재 주소 변경/선택 중단을 적용한다."""
        self.trace.append(':'.join((name,*(str(v) for v in values))));self.calls[name]+=1;self.boundaries[name]+=1
        current=name+str(self.calls[name])
        for when,address,value in self.effects:  # 동일 callback의 변경은 입력 순서대로 적용한다.
            if when==current:self.number(address,value,1 if address==0x540451 else 4)
        if self.case['stop']==current:raise Stop()

    def fresh(self,kind):
        """하위 장치의 고정 슬롯만 확보하며 부모의 기대 결과를 계산하지 않는다."""
        result=node(self.next);self.next+=1
        if self.next>COUNT:raise RuntimeError('Uber 장치 슬롯 한도')
        self.used[result]=kind;return result

    def base_device(self,token,command=0,width=0,height=0,flags=1):
        """기존 Native GumpCreate fixture와 같은 공통 생성 경계의 물리 입력 효과다."""
        form,untouched=self.number(token+0x28),self.number(token+0x74)
        self.mu.mem_write(token,b'\0'*0xa0);self.number(token,TABLE);self.number(token+0x28,form);self.number(token+0x74,untouched)
        self.number(token+4,flags|8);self.number(token+0x20,command);self.number(token+0x30,0xffffffd8);self.number(token+0x38,bits(1))
        for at,value in ((0x3c,width),(0x40,height),(0x44,width),(0x48,height),(0x98,1),(0x9c,1)):self.number(token+at,value)  # 폭/문맥의 원시 DWORD다.
        serial=(self.number(SERIAL)+1)&0xffffffff;self.number(SERIAL,serial);self.number(token+0x2c,serial)
        for index in range(1000):  # Native 공통 생성 장치의 첫 빈 등록칸이다.
            if not self.number(REGISTRY+index*4):self.number(REGISTRY+index*4,token);self.number(token+0x1c,index);break
        self.number(COUNT_AT,self.number(COUNT_AT)+1)

    def instruction(self,mu,address,size,data):
        """실제 Uber 분기/쓰기/x87만 실행하며 하위 함수의 전체 인자를 기록한다."""
        if address==END:self.completed=True;mu.emu_stop();return
        if any(lo<=address<hi for lo,hi in RANGES):self.instructions+=1;return
        esp,token=mu.reg_read(UC_X86_REG_ESP),mu.reg_read(UC_X86_REG_ECX)
        args=lambda count:[self.number(esp+4+i*4) for i in range(count)]  # 현재 실제 인자만 조회한다.
        if address==0x4e4391:
            request=args(1)[0];self.alloc_calls+=1;result=0 if self.alloc_calls==self.case['fail'] else self.fresh('r' if request==0xc8 else 'd' if request==0x11c else 'm')
            self.event('alloc',(request,result));self.returned(result);return
        if address==0x4642c0:
            values=args(4);self.base_device(token,*values);self.event('base',(token,*values));self.returned(token,16);return
        if address==0x4c6c50:
            values=args(6);axis,width,height,flags,kind,frame=values;self.base_device(token,55,bits(float(signed(width))),bits(float(signed(height))))
            self.number(token+4,self.number(token+4)|0x60200);self.number(token+0xb8,axis);self.mu.mem_write(token+0xcc,b'\0'*80)
            self.event('divider',(token,*values));self.returned(token,24);return
        factories={0x4815b0:'minimap',0x43f600:'production',0x47ca50:'knowledge',0x431ee0:'chat'}  # 이미 복원된 네 자식 공장 경계다.
        if address in factories:
            name=factories[address];result=self.fresh('b');self.base_device(result,flags=0 if name=='knowledge' else 1)
            if name in ('minimap','knowledge'):self.number(result+0x38,self.scales[0 if name=='minimap' else 1])
            self.event(name,(result,));self.returned(result);return
        if address==0x41a030:
            name=args(1)[0];self.base_device(token,width=self.scales[2],height=self.scales[3]);self.event('message',(token,name));self.returned(token,4);return
        if address==ATTACH:
            values=args(4);self.number(token+4,self.number(token+4)&~8);self.number(token+0x14,values[2]);self.number(token+0x4c,values[0]);self.number(token+0x50,values[1])
            self.event('attach',(token,*values));self.returned(0,16);return
        if address==MOVE:self.event('move',(token,*args(3)));self.returned(0,12);return
        if address==PUSH:self.event('push',(token,));self.returned();return
        simple={0x4437f0:'map',0x42f540:'refresh',0x4662b0:'cleanup'}  # 무인자 기존 자식이다.
        if address in simple:self.event(simple[address]);self.returned();return
        if address==0x4c4790:self.event('slave',(token,*args(2)));self.returned(0,8);return
        if address==0x49a840:self.event('type',args(1));self.returned(0);return
        if address==0x4de9a0:
            value=args(1)[0];self.event('translate',(self.string(value).hex(),));self.returned(0x12345678,4);return
        if address==0x464ed0:self.event('help',args(2));self.returned();return
        if address==0x464f80:
            target,text=args(2);self.event('helpword',(target,self.string(text).hex()));self.returned();return
        if address==0x4e65ed:
            first,second=args(2);result=0 if self.string(first).lower()==self.string(second).lower() else 1
            self.event('equal',(first,self.string(second).hex(),int(result==0)));self.returned(result);return
        if address==0x459c60:self.event('format',args(9));self.returned(FORMAT);return
        if address==0x41ab10:self.event('test',args(1));self.returned(self.case['test']);return
        if address==0x4b6100:self.event('visit',args(1));self.returned();return
        if address==0x419e10:self.event('event',(token,*args(3)));self.returned(0,12);return
        if address==0x466600:self.event('overlay',(token,*args(1)));self.returned(0,4);return
        if address==0x4652f0:self.event('depth',(token,*args(1)));self.returned(0,4);return
        if address==0x4666a0:self.event('replace',args(1));self.returned();return
        raise RuntimeError(f'미명시 Uber 실행 {address:x}')

    def run(self,case,control):
        """같은 물리 입력에서 전체 쓰기/현재표/호출/보존 GP·스택·SEH·x87을 관측한다."""
        self.case=case;self.used={node(0):'r',node(1):'b'};self.next=6;self.alloc_calls=0;self.completed=False;self.instructions=0;self.trace=[];self.calls=Counter()
        self.effects=[(p[0],int(p[1]),int(p[2])) for part in case['mutation'].split(';') if part for p in [part.split('/')]]
        self.scales=list(map(int,case['scales'].split(',')));self.mu.mem_write(NODES,bytes([case['fill']])*COUNT*STRIDE)
        self.mu.mem_write(REGISTRY,b'\0'*4000);self.number(COUNT_AT,0);self.number(SERIAL,31)
        for pair in case['globals'].split(','):a,v=pair.split(':');self.number(int(a),int(v),1 if int(a)==0x540451 else 4)  # 현재 실제 전역 입력이다.
        self.mu.mem_write(TITLE,b'\0'*256);self.mu.mem_write(TITLE,bytes.fromhex(case['message']) if case['message']!='-' else b'')
        self.base_device(node(1));self.number(node(1)+4,0x801)
        self.number(node(1)+0x18,node(2) if case['children'] else 0)
        for index in range(case['children']):  # 기존 직계 자식은 원문 부모의 순회 입력이다.
            child=node(index+2);self.used[child]='b';self.base_device(child);self.number(child+0x14,node(1));self.number(child+0xc,node(index+3) if index+1<case['children'] else 0)
            self.number(child+0x4c,bits(13.5+index));self.number(child+0x50,bits(-7.25-index))
        self.number(0,0x11110000);self.number(image.STACK,END);self.mu.reg_write(UC_X86_REG_ESP,image.STACK)
        preserved={UC_X86_REG_EBX:0x11223344,UC_X86_REG_ESI:0x22334455,UC_X86_REG_EDI:0x33445566,UC_X86_REG_EBP:0x44556677}
        for reg,value in preserved.items():self.mu.reg_write(reg,value)  # 보존 레지스터의 독립 guard다.
        self.mu.reg_write(UC_X86_REG_ECX,node(0));self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.fault='-'
        try:self.mu.emu_start(0x4c7160 if case['operation']=='construct' else 0x4c77c0,END+1,count=100000)
        except Stop:pass
        except Fault as error:self.fault=str(error)
        if self.completed:
            assert self.mu.reg_read(UC_X86_REG_ESP)==image.STACK+4 and self.number(0)==0x11110000
            assert all(self.mu.reg_read(reg)==value for reg,value in preserved.items()),{reg:(value,self.mu.reg_read(reg)) for reg,value in preserved.items()}
            assert self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==0
        assert all(bytes(self.mu.mem_read(lo,len(raw)))==raw for lo,raw in self.original.items())
        actual_globals=pairs([(a,self.number(a, width=1 if a==0x540451 else 4)) for a in GLOBALS]+[(COUNT_AT,self.number(COUNT_AT)),(SERIAL,self.number(SERIAL))])
        return dict(completed=int(self.completed),result=self.mu.reg_read(UC_X86_REG_EAX) if self.completed else '-',globalsout=actual_globals,
                    registry=rle([self.number(REGISTRY+i*4) for i in range(1000)]),nodes=';'.join(f'{a}:{kind}:{bytes(self.mu.mem_read(a,0x180)).hex()}' for a,kind in sorted(self.used.items())),trace=';'.join(self.trace),instructions=self.instructions)

def cases():
    """결과를 계산하지 않는 차원/스케일/메시지/전환/현재 메모리 변경 입력이다."""
    default={0x540451:11,0x5411c8:153,0x542664:1,0x5ca920:999,0x5ca924:7,0x5ca928:8,0x5c78f0:1024,0x5c78f4:768,0x531860:1024,0x531864:768,0x540cec:0x10203040,0x540cf8:0x50607080,0x542a04:0x020000bf}
    seed=dict(name='normal',operation='construct',globals=pairs(default.items()),scales=','.join(map(str,(bits(1),bits(1.25),bits(301.5),bits(127.25)))),message='-',test=0,children=0,fill=0x53,mutation='',stop='',fail=0)
    output=[]
    for operation,dimensions,title in itertools.product(('construct','replace'),((640,480),(1024,768),(1280,720),(641,479),(16777217,16777219),(-5,-3)),('', 'DEFEAT','titleMenu','victory','Briefing')):  # 정상 네 분기와FILD→FST→FMUL을 가르는 큰 정수다.
        current=dict(default);current[0x5c78f0],current[0x5c78f4]=dimensions
        output.append(dict(seed,name=f'{operation}-{dimensions[0]}-{dimensions[1]}-{title or "empty"}',operation=operation,globals=pairs(current.items()),message=title.encode().hex() or '-',test=len(output)%2,children=len(output)%4))
    for stop in ('base1','divider2','minimap1','production1','knowledge1','chat1','slave5','cleanup2','message1','format1','event1','replace1'):  # 각 필수 하위의 선행 부분 효과를 확인한다.
        output.append(dict(seed,name='stop-'+stop,operation='replace',message=b'Briefing'.hex(),test=1,children=3,stop=stop))
    for event,target,value in (('alloc1',0x5c78f4,721),('minimap1',0x5c78f0,801),('knowledge1',0x5c78f4,901),('cleanup1',0x531860,1281),('format1',0x542a04,0xf1234567),('push1',node(2)+0xc,0),('push1',node(2)+0x4c,bits(123.75))):  # callback 뒤 현재 값/캡처 next의 차이다.
        output.append(dict(seed,name=f'mutate-{event}-{target}',operation='replace',message=b'Briefing'.hex(),test=1,children=3,mutation=f'{event}/{target}/{value}'))
    output.append(dict(seed,name='top-new-fails',operation='replace',fail=1))
    return output

def main():
    """최초 관측을 새 파일에 쓰거나 같은 실제 PE를 재관측해 기존 fixture와 비교한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    assert image.digest(image.PE_PATH)==image.PE_SHA;oracle=Oracle();rows=[];inputs=cases();totals={}
    for index,case in enumerate(inputs):  # 실제 두 정밀도를 별개로 실행하며 관측을 기대값으로 되먹이지 않는다.
        previous=None
        for control in image.CONTROLS:
            actual=oracle.run(case,control);comparison={k:v for k,v in actual.items() if k!='instructions'}
            assert previous is None or previous==comparison,(case['name'],control);previous=comparison
            record=dict(case,name=case['name']+f'-cw{control:03x}',**actual);rows.append('\t'.join(str(record[name]) for name in SCHEMA));totals[control]=totals.get(control,0)+actual['instructions']
        if index%20==0:print('observed',index,flush=True)
    text='# '+'\t'.join(SCHEMA)+'\n'+'\n'.join(rows)+'\n'
    if args.verify:assert FIXTURE.read_text('utf-8')==text;print('verified',len(rows));return
    assert not FIXTURE.exists() and not REPORT.exists();FIXTURE.write_text(text,encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',inputs=len(inputs),observations=len(rows),schema=SCHEMA,nativeRanges=RANGES,nativeInstructions=totals,cpuWrites=dict(oracle.writes),boundaries=dict(oracle.boundaries),gameExecuted=False,guiExecuted=False,
        limits=['실제004c7160/004c77c0와특수메시지정상epilogue전체를실행한다.콜백별다른현재주소/정상RET/보존GP/ESP/SEH/x87/원시노드·등록표를비교한다.',
                '기존공통Gump/구분선/4개전체자식공장/가상Attach·Move·Push/정리·도킹/메시지·번역·검사·Overlay·깊이·최종ReplaceRoot는명시장치다.이자식들의기존모듈/실제Client접합은별도로검증해야한다.',
                '명시Attach장치는flags/parent/x/y만쓰며최종ReplaceRoot는호출만기록한다.실제하위Kill·트리재연결·GPU·게임·창·전체소멸을검증한것이아니다.'],
        sha256={str(path.relative_to(ROOT)).replace('\\','/'):image.digest(path) for path in (image.PE_PATH,ROOT/'extracted/refined/originals/Netstorm.c',Path(image.__file__),Path(__file__),FIXTURE)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(dict(inputs=len(inputs),observations=len(rows),instructions=totals)))

if __name__=='__main__':main()  # 명시 CLI에서만 새 부모를 관측한다.
