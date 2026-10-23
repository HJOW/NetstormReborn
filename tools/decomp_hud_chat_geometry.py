#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 ChatWindow Attach/표시 본체의 기하와 가상 호출을 격리 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW

ROOT=Path(__file__).resolve().parents[1]  # 새 산출물의 저장소 루트다.
PE=ROOT/'originals/Netstorm.exe'  # 변경하지 않는 보호 입력이다.
PE_SHA='a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 10.78 식별자다.
FIXTURE=ROOT/'cpppj/tests/fixtures/hud-chat-geometry-x86.tsv'  # 실제 원문 출력만 담는다.
REPORT=ROOT/'extracted/cpp-hud-chat-geometry-20261023-native.json'  # 범위/대체 경계/SHA다.
BASE,HEAP,STACK=0x400000,0x1000000,0x2100000  # 원문/노드/스택의 격리 주소다.
NODES=tuple(HEAP+index*0x200 for index in range(5))  # root/Console/Text/helper/background 순서다.
PUSH,ATTACH,REDRAW,STOP=HEAP+0x2000,HEAP+0x2010,HEAP+0x2020,HEAP+0x2030  # 가상 경계와 정상RET 종료점이다.
RANGES=((0x42E6E0,0x42E8D0),(0x42F250,0x42F266),(0x42F270,0x42F301),(0x4BA3D0,0x4BA3F1),(0x4437F0,0x4437F1))  # 실제 전체 정상 몸체다.
SCHEMA=('name','operation','width','height','padding','helper','consoleFlags','scales','consoleHeight','positions','value','challenge','present','bottom',
        'completed','state','helperout','trace','stackdelta','nativeinstructions')  # 앞14열은 입력, 뒤6열은 실제 관측이다.


def sha(path):
    """원문과 새 파일의 실제 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """단정도 입력을 DWORD 비트로 표현한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def packed(values):
    """순서가 있는 원시 비트/인자를 탭 없는 목록으로 기록한다."""
    return ':'.join(map(str,values))


class Oracle:
    """기하·조건식은 대체하지 않고 실제 PE 함수 전체를 정상RET까지 실행한다."""
    def __init__(self):
        """보호 이미지를 적재하고 같은 형태의 다섯 원시 노드를 준비한다."""
        pe=pefile.PE(str(PE));self.mu=Uc(UC_ARCH_X86,UC_MODE_32)  # 실제32비트 실행기다.
        self.mu.mem_map(BASE,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095);self.mu.mem_write(BASE,pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP,0x3000);self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)
        self.original={low:bytes(self.mu.mem_read(low,high-low)) for low,high in RANGES}  # 보호 본체 불변 검사다.
        self.totals={};self.visited=set()  # 정밀도별 실제 실행량과 명령 주소 집합이다.

    def number(self,address,value=None,width=4):
        """원시 자료는 준비 단계에만 쓰고 출력은 메모리에서 직접 읽는다."""
        if value is not None:self.mu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def role(self,pointer):
        """실제 노드 주소는1..5로, 그 외 인자는 원시값 그대로 표현한다."""
        return NODES.index(pointer)+1 if pointer in NODES else pointer

    def arguments(self,count):
        """경계 진입의 실제 스택 인자만 읽는다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP)  # 현재 return 주소 위치다.
        return [self.number(esp+4+index*4) for index in range(count)]

    def finish(self,pop,result=0):
        """명시 외부 경계의 ABI만 종료하며 원문 계산은 수정하지 않는다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP)  # 실제 호출 프레임이다.
        self.mu.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF);self.mu.reg_write(UC_X86_REG_EIP,self.number(esp));self.mu.reg_write(UC_X86_REG_ESP,esp+4+pop)

    def guard(self,mu,access,address,size,value,data):
        """명시 노드/스택 외 CPU 쓰기는 관측을 실패시킨다."""
        if any(node<=address and address+size<=node+0x140 for node in NODES):return
        if STACK-0x1000<=address and address+size<=STACK+20:return
        raise RuntimeError(f'미명시 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """기본 트리/텍스트 조회와 가상 경계의 호출 순서만 관측한다."""
        if address==STOP:self.completed=True;mu.emu_stop();return
        role=self.role(mu.reg_read(UC_X86_REG_ECX))  # 실제 this가 가리키는 대상이다.
        if address in (PUSH,ATTACH,REDRAW,0x466750,0x4BC2C0):
            if address==PUSH:self.trace.append(f'push:{role}');self.finish(0)
            elif address==0x4BC2C0:self.trace.append(f'bottomIndex:{role}');self.finish(0,self.case['bottom'])
            elif address==REDRAW:self.trace.append('redraw:'+packed((role,*self.arguments(1))));self.finish(4)
            else:
                args=self.arguments(4);args[2]=self.role(args[2])  # 실제 parent 포인터만 정규화한다.
                self.trace.append(('baseAttach:' if address==0x466750 else 'attach:')+packed((role,*args)));self.finish(16)
            return
        if not any(low<=address<high for low,high in RANGES):raise RuntimeError(f'미명시 실행 {address:08x}')
        self.instructions+=1;self.visited.add(address)
        if address==0x4BA3D0:self.trace.append('bottom:'+packed((role,*self.arguments(1))))
        elif address==0x42F29C:self.trace.append('challenge:'+str(self.number(0x594FC8)))

    def run(self,case,control):
        """같은 입력을 독립 초기 상태에서 실행하고 실제 크기/호출/표시 BYTE를 읽는다."""
        self.case=case;self.trace=[];self.instructions=0;self.completed=False  # 이번 실행의 관측 상태다.
        self.mu.mem_write(HEAP,bytes(0x3000))
        scales=list(map(int,case['scales'].split(':')))  # Console/Text/helper/background의 명시 배율 비트다.
        for index,node in enumerate(NODES):
            vt=HEAP+0x1000+index*0x100  # 각 현재 노드의 격리 가상 표다.
            self.number(node,vt);self.number(vt+8,REDRAW);self.number(vt+0x18,PUSH);self.number(vt+0x20,ATTACH)
            self.number(node+4,0xE01 if index==0 else case['consoleFlags'] if index==1 else 0x601)
            self.number(node+0x38,bits(1) if index==0 else scales[index-1])
            self.number(node+0x3C,case['width'] if index==0 else bits((60,40,1,20)[index-1]))
            self.number(node+0x40,case['height'] if index==0 else case['consoleHeight'] if index==1 else bits((0,50,9,30)[index-1]))
            self.number(node+0x44,bits(123));self.number(node+0x48,bits(-456))
        for offset,pointer in zip((0xA0,0xA4,0xA8,0xAC),NODES[1:]):self.number(NODES[0]+offset,pointer)
        self.number(NODES[0]+0xB4,case['padding']);self.number(NODES[0]+0x138,case['helper'],1)
        positions=list(map(int,case['positions'].split(':')))  # root/Console 위치와 Attach 인자를 구별하는 명시 입력이다.
        for node,x,y in ((NODES[0],positions[0],positions[1]),(NODES[1],positions[2],positions[3])):
            self.number(node+0x4C,x);self.number(node+0x50,y)
        self.number(0x548740,NODES[0] if case['present'] else 0);self.number(0x594FC8,case['challenge'])
        self.mu.mem_write(STACK-0x1000,bytes(0x2000));self.mu.reg_write(UC_X86_REG_ECX,NODES[0]);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        if case['operation']=='attach':entry=0x42E6E0;args=(positions[4],positions[5],17,3);expected_stack=STACK+20
        else:entry=0x42F250 if case['operation']=='helper' else 0x42F270;args=(case['value'],);expected_stack=STACK+4
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.mu.emu_start(entry,STOP+1,count=10000)
        if not self.completed:raise RuntimeError('정상RET 미도달')
        state=';'.join(packed((index+1,*(self.number(node+offset) for offset in (4,0x3C,0x40,0x44,0x48)))) for index,node in enumerate(NODES))  # 실제 크기/표시만 읽는다.
        output=dict(completed=1,state=state,helperout=self.number(NODES[0]+0x138,width=1),trace=';'.join(self.trace) or '-',
                    stackdelta=self.mu.reg_read(UC_X86_REG_ESP)-expected_stack,nativeinstructions=self.instructions)
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control:raise RuntimeError('x87 제어어 변경')
        for low,expected in self.original.items():
            if bytes(self.mu.mem_read(low,len(expected)))!=expected:raise RuntimeError('원문 명령 변경')
        total=self.totals.setdefault(f'{control:03x}',dict(runs=0,instructions=0));total['runs']+=1;total['instructions']+=self.instructions
        return output


def cases():
    """기하 기대값 없이 입력 크기/배율/표시 관문만 교차한다."""
    rows=[]  # 입력 목록이며 결과 수식을 포함하지 않는다.
    positions=packed(map(bits,(5,7,11,13,19.25,-7.75)))  # 현재 위치와 전달 위치는 서로 다르다.
    scales=((1,1,1,1),(0.5,1.5,0.75,2),(0,0,0,0))  # 네 자식의 독립 배율 입력이다.
    for index,(width,height,padding,visible,helper,scale,console_height,bottom) in enumerate(itertools.product((-1,80,939),(0,75,500),(-2,0,3),(0,1),(0,1,255),scales,(0,14),(0,5))):
        rows.append(dict(name=f'attach-{index}',operation='attach',width=bits(width),height=bits(height),padding=padding,helper=helper,consoleFlags=0x600|visible,
                         scales=packed(map(bits,scale)),consoleHeight=bits(console_height),positions=positions,value=0,challenge=0,present=1,bottom=bottom))
    for operation,value,challenge,present,visible in itertools.product(('helper','input'),(0,1,0xFFFFFFFF),(0,1),(0,1),(0,1)):
        if operation=='helper' and not present:continue  # 원문 helper 설정은 유효한 전역 root를 요구한다.
        rows.append(dict(name=f'{operation}-{value}-{challenge}-{present}-{visible}',operation=operation,width=bits(200),height=bits(100),padding=0,helper=255,consoleFlags=0x600|visible,
                         scales=packed(map(bits,(1,1,1,1))),consoleHeight=bits(14),positions=positions,value=value,challenge=challenge,present=present,bottom=5))
    return rows


def main():
    """기존 보호 PE 식별 뒤 새 관측 TSV/범위 보고서만 생성한다."""
    if sha(PE)!=PE_SHA:raise RuntimeError('보호 PE SHA 불일치')
    oracle=Oracle();inputs=cases();lines=['\t'.join(SCHEMA)];different=[]  # 결과는 실제 실행에서만 얻는다.
    for index,case in enumerate(inputs):
        outputs=[]  # 원문 두 정밀도의 독립 실행 결과다.
        for control in (0x27F,0x37F):
            output=oracle.run(case,control);outputs.append(output);row={**case,**output,'name':case['name']+f'-pc{control:03x}'}
            lines.append('\t'.join(str(row[field]) for field in SCHEMA))
        if outputs[0]!=outputs[1]:different.append(index)
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),pe_sha256=PE_SHA,input_cases=len(inputs),observations=len(inputs)*2,precision_differences=different,totals=oracle.totals,
                native_ranges=[[f'{low:08x}',f'{high:08x}'] for low,high in RANGES],schema=SCHEMA,
                limitations=['00431810 생성과0042fab0 Handle은 실행하지 않음','00466750 기본 부착/자식 Push·Attach·Redraw는 호출 인자 관측기로 대체하며 실제 링크/좌표 변경은 없음',
                             '004bc2c0 표시 줄 조회는 명시 반환 입력으로 대체함; 줄 배치/스크롤의 전체 검증은 아님','004ba3d0 맨 아래 요청 본체/빈004437f0는 실제 실행함',
                             '최종state는5노드 flags/baseWidth/baseHeight/width/height와root helper BYTE이며 전체TextEntry 상태를 보증하지 않음',
                             '실제 게임/GUI/새 디컴파일/생성 성공/픽셀 동일성을 주장하지 않음'],
                artifacts={str(FIXTURE.relative_to(ROOT)):sha(FIXTURE),str(Path(__file__).relative_to(ROOT)):sha(Path(__file__))})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(inputs=len(inputs),observations=len(inputs)*2,precision_differences=len(different),totals=oracle.totals,fixture_sha=sha(FIXTURE)),ensure_ascii=False))


if __name__=='__main__':
    main()
