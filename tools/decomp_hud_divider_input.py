#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 divider 입력/프레임 본체와 실제 로컬 변환/anchor/capture의 결과를 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

ROOT = Path(__file__).resolve().parents[1]  # 저장소 루트이며 보호 파일에는 쓰지 않는다.
PE_PATH = ROOT/'originals/Netstorm.exe'  # 읽기 전용 10.78 입력이다.
PE_SHA = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'  # 기존 보호 파일 식별자다.
FIXTURE = ROOT/'cpppj/tests/fixtures/hud-divider-input-x86.tsv'  # 실제 native 관측만 담는 새 자료다.
REPORT = ROOT/'extracted/cpp-hud-divider-input-20261023-native.json'  # 범위와 SHA/한계를 기록한다.
BASE, HEAP, STACK = 0x400000, 0x1000000, 0x2100000  # 원문/격리 노드/스택 주소다.
NODE, PARENT, GRANDPARENT, OTHER = HEAP, HEAP+0x200, HEAP+0x400, HEAP+0x600  # 실제 포인터 입력을 구별한다.
VTABLE, DETACH, ATTACH, STOP = HEAP+0x1000, HEAP+0x2000, HEAP+0x2010, HEAP+0x2020  # 두 가상 경계만 별도 주소로 관측한다.
HOVER = 0x0200003B  # 보호 PE의 005427e4 초기값이며 원문이 직접 비교한다.
RANGES = ((0x4C49F0,0x4C4AF8),(0x4C4B00,0x4C4BF6),(0x4650A0,0x4651AB),
          (0x464CF0,0x464D0A),(0x465250,0x465260),(0x464D10,0x464D57),
          (0x475EF0,0x475EFE))  # 입력/방송과 실제 좁은 하위 함수 전체다.
STUBS = (0x452E00,0x464990,0x4A4410,DETACH,ATTACH)  # OS 폴링/Help/커서/다음 배치의 명시 관측 경계다.
CONTROLS = (0x27F,0x37F)  # 원본 x87 두 정밀도에서 관측을 반복한다.
SCHEMA = ('name','operation','event','x','y','axis','flags','limits','geometry','parent',
          'capture','pollCode','modal','baseResult','completed','result','state','trace',
          'stackdelta','nativeinstructions')  # 앞14열 입력, 뒤6열 관측이다.


def digest(path):
    """보호 입력과 독립 산출물의 실제 바이트를 식별한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bits(value):
    """단정도 입력의 원시 DWORD를 만든다. 출력 좌표는 계산하지 않는다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


def packed(values):
    """탭과 충돌하지 않는 원시 DWORD/정수 목록을 만든다."""
    return ':'.join(str(value) for value in values)


class Oracle:
    """분기/실수 연산을 대체하지 않고 원문의 실제 전체 함수를 실행한다."""
    def __init__(self):
        """보호 PE와 일반 부모 체인/독립 가상 표를 격리 주소에 준비한다."""
        pe=pefile.PE(str(PE_PATH))  # 보호 입력을 메모리에만 적재한다.
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32)  # 실제 x86 명령 실행기다.
        self.mu.mem_map(BASE,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
        self.mu.mem_write(BASE,pe.get_memory_mapped_image())
        self.mu.mem_map(HEAP,0x3000)
        self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)
        self.original={low:bytes(self.mu.mem_read(low,high-low)) for low,high in RANGES}  # 명령 불변 검사다.
        self.totals,self.visited,self.stub_totals={},set(),Counter()  # 실제 명령과 외부 경계를 집계한다.

    def number(self,address,value=None,width=4):
        """비정렬 원시 입력과 실제 native 상태를 읽고 준비할 때만 쓴다."""
        if value is not None:
            self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def arguments(self,count):
        """실제 호출 스택의 DWORD 인자를 순서대로 읽는다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP)  # 현재 경계 진입의 스택이다.
        return [self.number(esp+4+4*index) for index in range(count)]

    def capture(self):
        """실제 포인터 전역을 0/self/other 값으로만 표현한다."""
        value=self.number(0x565DD0)  # 원본 캡처 전역이다.
        if value not in (0,NODE,OTHER): raise RuntimeError('미명시 capture 포인터')
        return {0:0,NODE:1,OTHER:2}[value]

    def finish_stub(self,pop,result=0):
        """명시 장치/가상 경계의 ABI만 종료한다. 원문 분기는 대체하지 않는다."""
        esp=self.mu.reg_read(UC_X86_REG_ESP)  # 경계가 받은 실제 return 주소다.
        self.mu.reg_write(UC_X86_REG_EAX,result&0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_EIP,self.number(esp))
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+pop)

    def guard(self,mu,access,address,size,value,data):
        """CPU가 명시 노드/캡처/스택 밖의 상태를 바꾸면 관측을 실패시킨다."""
        if NODE<=address and address+size<=NODE+0x11C: return
        if address==0x565DD0 and size==4: return
        if STACK-0x1000<=address and address+size<=STACK+16: return
        raise RuntimeError(f'미명시 CPU 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """실제 콜백 호출 순서/인자와 정상RET만 관측한다."""
        if address==STOP:
            self.completed=True;mu.emu_stop();return
        if address in STUBS:
            self.stub_totals[f'{address:08x}']+=1
            if address==0x452E00:
                output,code=self.arguments(2)  # 원문이 요청한 폴링 코드/출력 버퍼다.
                self.trace.append('poll:'+str(code))
                mu.mem_write(output,struct.pack('<III',self.case['pollCode'],0x11223344,0x55667788))
                self.finish_stub(0,output)
            elif address==0x464990:
                self.trace.append('base:'+packed(self.arguments(3)))
                self.finish_stub(12,self.case['baseResult'])
            elif address==0x4A4410:
                self.trace.append('cursor:'+packed(self.arguments(1)))
                self.finish_stub(0)
            elif address==DETACH:
                self.trace.append('detach')
                self.finish_stub(0)
            else:
                self.trace.append('attach:'+packed(self.arguments(4)))
                self.finish_stub(16)
            return
        if not any(low<=address<high for low,high in RANGES): raise RuntimeError(f'미명시 실행 {address:08x}')
        self.instructions+=1;self.visited.add(address)
        if address==0x4650A0: self.trace.append('local')
        elif address==0x4C4A1B: self.trace.append('localout:'+packed((self.number(STACK+4),self.number(STACK+8))))
        elif address==0x464CF0: self.trace.append('anchor:'+packed(self.arguments(2)))
        elif address==0x465250: self.trace.append('capture:'+str(self.capture()))
        elif address==0x475EF0: self.trace.append('modal:'+str(self.number(0x593024)))
        elif address==0x464D10: self.trace.append('move:'+packed(self.arguments(3)))

    def run(self,case,control):
        """한 원시 입력을 native 진입부터 정상RET까지 실행하고 상태/ABI를 읽는다."""
        self.case=case;self.trace=[];self.completed=False;self.instructions=0  # 각 관측은 독립 초기 상태다.
        self.mu.mem_write(HEAP,bytes(0x3000))
        self.mu.mem_write(VTABLE,bytes(self.mu.mem_read(0x514FD8,0x3C)))
        self.number(VTABLE+0x18,DETACH);self.number(VTABLE+0x20,ATTACH)
        self.number(NODE,VTABLE);self.number(NODE+4,case['flags']);self.number(NODE+0xB8,case['axis'])
        # 정수 제한과 단정도 기하/anchor는 입력 비트를 그대로 제공한다.
        for offset,value in zip((0xBC,0xC0,0xC4,0xC8),map(int,case['limits'].split(':'))): self.number(NODE+offset,value)
        for offset,value in zip((0x4C,0x50,0x54,0x58),map(int,case['geometry'].split(':'))): self.number(NODE+offset,value)
        depth,px,py=map(int,case['parent'].split(':'))  # world 투영 없는 일반 부모 체인이다.
        self.number(NODE+0x14,PARENT if depth else 0)
        self.number(PARENT+0x14,GRANDPARENT if depth==2 else 0)
        self.number(PARENT+0x4C,px);self.number(PARENT+0x50,py)
        self.number(0x565DD0,{0:0,1:NODE,2:OTHER}[case['capture']])
        self.number(0x593024,case['modal']);self.number(0x5427E4,HOVER)
        self.mu.mem_write(STACK-0x1000,bytes(0x2000))
        self.mu.mem_write(STACK,struct.pack('<4I',STOP,case['event']&0xFFFFFFFF,case['x']&0xFFFFFFFF,case['y']&0xFFFFFFFF))
        preserved=(UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP)  # thiscall 보존 레지스터다.
        for index,register in enumerate(preserved): self.mu.reg_write(register,0x12345670+index)
        self.mu.reg_write(UC_X86_REG_EAX,0x11223344);self.mu.reg_write(UC_X86_REG_EDX,0x55667788)
        self.mu.reg_write(UC_X86_REG_ECX,NODE);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(0x4C49F0 if case['operation']=='handle' else 0x4C4B00,STOP+1,count=10000)
        if not self.completed: raise RuntimeError('정상RET 미도달')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control: raise RuntimeError('x87 제어어 변경')
        for index,register in enumerate(preserved):
            if self.mu.reg_read(register)!=0x12345670+index: raise RuntimeError('thiscall 보존 레지스터 변경')
        state=packed((self.number(NODE+4),self.number(NODE+0x54),self.number(NODE+0x58),self.capture()))  # 실제 출력 상태다.
        output=dict(completed=1,result=self.mu.reg_read(UC_X86_REG_EAX) if case['operation']=='handle' else '-',
                    state=state,trace=';'.join(self.trace) or '-',stackdelta=self.mu.reg_read(UC_X86_REG_ESP)-(STACK+16),
                    nativeinstructions=self.instructions)  # void 방송의 EAX는 계약으로 만들지 않는다.
        for low,expected in self.original.items():
            if bytes(self.mu.mem_read(low,len(expected)))!=expected: raise RuntimeError('보호 본체 변경')
        total=self.totals.setdefault(f'{control:03x}',dict(runs=0,instructions=0))  # 실제 실행량이다.
        total['runs']+=1;total['instructions']+=self.instructions
        return output


def cases():
    """축/입력/release/모달/제한/부모/anchor의 원시 입력만 생성한다."""
    rows,seen=[],set()  # 결과와 무관하게 같은 입력의 중복만 없앤다.

    def add(name,operation='handle',**changes):
        """명시 입력 레코드를 추가하며 원문 기대값은 계산하지 않는다."""
        case=dict(name=name,operation=operation,event=0x10000,x=91,y=37,axis=1,flags=0x60201,
                  limits='20:170:18:748',geometry=packed(map(bits,(80,20,7,9))),parent=packed((1,bits(11),bits(13))),
                  capture=0,pollCode=0x10000,modal=0,baseResult=0)
        case.update(changes)
        key=tuple(case[field] for field in SCHEMA[1:14])  # 이름/출력은 중복 판정에서 제외한다.
        if key not in seen: seen.add(key);rows.append(case)

    # 모든 낮은 축/제한 조합과 modifier/release/hover/가까운 비대상 사건의 입력 경계를 교차한다.
    for axis,event,modifier,capture in itertools.product(range(16),(0x10000,0x80010000,HOVER,HOVER|0x80000000,0x10001,0x20000,0,ord('Q')),
                                                        (0,0x08000000,0x10000000,0x18000000,0x20000000,0x28000000,0x30000000,0x38000000,
                                                         0x04000000,0x3C000000,0x40000000,0x7C000000),(0,1,2)):
        add(f'handle-{axis}-{event}-{modifier}-{capture}',axis=axis,event=event|modifier,capture=capture)
    # 로컬 변환은 모든 사건에서 먼저 실행하며 기존 capture와 다른 현재 anchor도 덮을 수 있다.
    for depth,axis,event,point in itertools.product((0,1,2),(0,1,2,3),(0x10000,0x80010000,HOVER),(0,1,80,91,16777217,-16777217)):
        add(f'parent-{depth}-{axis}-{event}-{point}',parent=packed((depth,bits(11.5),bits(-13.25))),axis=axis,event=event,x=point,y=-point)
    for event,result in itertools.product((0,0x10001,ord('E')),(1,0x12345678,0xFFFFFFFF)):
        add(f'base-result-{event}-{result}',event=event,baseResult=result)
    # Broadcast 자체는 visible/notify/dead를 다시 검사하지 않고 모달 전역만 먼저 읽는다.
    for axis,modal,poll,flag,point in itertools.product(range(16),(0,1,0xFFFFFFFF),(0x10000,0x80010000),(0x60201,0x60319),(0,19,20,21,169,170,171,767)):
        add(f'broadcast-{axis}-{modal}-{poll}-{flag}-{point}',operation='broadcast',axis=axis,modal=modal,pollCode=poll,flags=flag,x=point,y=point)
    # signed 제한의 순차 min/max 적용과 단정도 저장 경계를 실제 x87 본체에서 관측한다.
    for axis,limits,point,anchor in itertools.product((1,2,3,5,10,15),('-20:20:-10:10','170:20:748:18','-2147483648:2147483647:-2147483648:2147483647'),
                                                   (-2147483648,-16777217,-21,-1,0,1,21,16777217,2147483647),(0,13.5)):
        add(f'limits-{axis}-{limits}-{point}-{anchor}',operation='broadcast',axis=axis,limits=limits,x=point,y=-point if point!=-2147483648 else point,
            geometry=packed(map(bits,(80.25,20.75,anchor,-anchor))),flags=0x60311)
    for flag in (0,1,0x100,0x101,0x10000,0x10100):
        add(f'broadcast-common-filter-outside-{flag}',operation='broadcast',flags=flag)
    return rows


def main():
    """보호 SHA를 검증한 뒤 독립 TSV와 범위 보고서만 새로 작성한다."""
    if digest(PE_PATH)!=PE_SHA: raise RuntimeError('보호 PE SHA 불일치')
    oracle=Oracle();inputs=cases();lines=['\t'.join(SCHEMA)];different=[]  # 실제 관측을 정밀도별로 유지한다.
    for index,case in enumerate(inputs):
        results=[]  # Python 제품 수식이 아니라 native 두 관측을 서로 비교한다.
        for control in CONTROLS:
            result=oracle.run(case,control);results.append(result)
            row={**case,**result,'name':case['name']+f'-pc{control:03x}'}
            lines.append('\t'.join(str(row[field]) for field in SCHEMA))
        if results[0]!=results[1]: different.append(index)
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    report=dict(host=socket.gethostname(),edition='10.78',pe=dict(path=str(PE_PATH.relative_to(ROOT)),sha256=PE_SHA),
                refined=dict(path='extracted/refined/originals/Netstorm.c',sha256=digest(ROOT/'extracted/refined/originals/Netstorm.c')),
                input_cases=len(inputs),observations=len(inputs)*2,precision_differences=different,totals=oracle.totals,
                native_ranges=[[f'{low:08x}',f'{high:08x}'] for low,high in RANGES],
                boundary_stubs=dict(oracle.stub_totals),
                executed_instruction_addresses=len(oracle.visited),schema=SCHEMA,
                coverage=['전체004c49f0/004c4b00 진입부터 정상RET','실제 일반 부모 ScreenToLocal/SetAnchor/Capture/MoveAnchor/모달 getter',
                          '모든 낮은 axis/제한bit, 대소문자와 무관한 hover/좌우버튼/일반키/가까운 비대상/모든modifier/release',
                          '기존 capture0/self/other, 일반 부모0/1/2, signed 정상/역전/전체제한, 단정도 경계',
                          'MenuGumpState.instances0/비영, release폴링 뒤 그 프레임 이동, 공통 프레임 필터 밖 직접 호출'],
                limitations=['00452e00 OS 폴링/00464990 기본 Help/004a4410 커서 장치의 내부는 실행하지 않음',
                             '원본 VT를 변경하지 않고 격리 복사VT의+18/+20만 detach/attach 호출 관측기로 교체함',
                             '최종 실제004c47f0 노예 재배치/dirty/viewport 콜백과 실제 detach 링크 변경은 다른 관측 범위임',
                             '일반 부모만 제공하며 world부모2000h 투영/잘못된 포인터/순환은 제외함',
                             '00464100 Broadcast 선택/00466200 Route의 capture 해제/필터 및 GUI 드래그는 별도 검증 필요',
                             '방송 입력flags가 공통 visible/notify/dead 필터를 통과하지 않는 행은 본체 직접 관측만 뜻함',
                             '게임/GUI 실행, 새 디컴파일, 제품 C++ 수식 재계산은 하지 않음'],
                artifacts={str(FIXTURE.relative_to(ROOT)):digest(FIXTURE),str(Path(__file__).relative_to(ROOT)):digest(Path(__file__))})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(inputs=len(inputs),observations=len(inputs)*2,precision_differences=len(different),totals=oracle.totals,
                          fixture_sha=report['artifacts'][str(FIXTURE.relative_to(ROOT))]),ensure_ascii=False))


if __name__=='__main__':
    main()
