#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의004cc210 전체 부모를 실행하고 하위 메뉴/GUI만 명시 경계로 관측한다."""
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path

import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

ROOT = Path(__file__).resolve().parents[1]  # 새 산출물의 저장소 루트다.
PE = ROOT / 'originals/Netstorm.exe'  # 변경하지 않는 보호 입력이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/game-menu-bar-x86.tsv'  # 새 독립 관측이다.
REPORT = ROOT / 'extracted/cpp-game-menu-bar-20261023-native.json'  # 실행 범위/해시다.
BASE, HEAP, STACK = 0x400000, 0x1000000, 0x2100000  # 원본 이미지와 격리 장치다.
ENTRY, END, STOP = 0x4CC210, 0x4CC4C5, HEAP + 0x3000  # 실제 부모 전체와 호스트 반환점이다.
NODE, TABLE, ATTACH = HEAP+0x1000, HEAP+0x2000, HEAP+0x3010  # 가로 노드와 실제 가상 부착 장치다.
FIELDS = ('editor','battle','cheat','mission','checksum','technical','width','left','mutation')  # 기대를 포함하지 않는 입력 순서다.
GLOBALS = (0x5C85A4,0x594FBC,0x5CAB30,0x594FA4,0x54D4E0)  # 각 독립 전역이다.
CHILDREN = {0x4CBB80:0,0x4CB9C0:1,0x4CA4D0:2,0x4CA850:3,0x4CBF10:4}  # 실행하지 않는 하위 부모다.
CONTROLS = (0x27F,0x37F)  # 두 x87 정밀도를 독립 실행한다.


def digest(path):
    """현재 파일의 전체 SHA256을 읽는다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Oracle:
    """실제 원문 명령과 가상/하위 GUI의 명시 입력만 실행한다."""
    def __init__(self):
        """보호 이미지를 읽기 전용 입력으로 복사한다."""
        pe = pefile.PE(str(PE))  # 실제 PE 매핑을 제공한다.
        self.mu = Uc(UC_ARCH_X86,UC_MODE_32)  # 격리된32비트 CPU다.
        self.image = pe.get_memory_mapped_image()  # 입력 파일은 고치지 않는다.
        self.mu.mem_map(BASE,(pe.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
        self.mu.mem_write(BASE,self.image)
        self.mu.mem_map(0,0x1000)
        self.mu.mem_map(HEAP,0x10000)
        self.mu.mem_map(STACK-0x10000,0x20000)
        self.mu.hook_add(UC_HOOK_CODE,self.instruction)
        self.totals = {}  # 두 정밀도의 실제 실행 합계다.

    def word(self,address,value=None):
        """비정렬DWORD를 읽고 명시 입력만 갱신한다."""
        if value is not None:
            self.mu.mem_write(address,struct.pack('<I',value&0xFFFFFFFF))
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def text(self,address):
        """현재 원문 C문자열을 바이트 그대로 읽는다."""
        if not address:
            return b''
        result = bytearray()  # 첫 NUL 이전의 바이트 저장소다.
        for index in range(256):  # 잘못된 외부 포인터의 무한 접근을 거부한다.
            byte = self.mu.mem_read(address+index,1)[0]
            if not byte:
                return bytes(result)
            result.append(byte)
        raise RuntimeError('원문 문자열 NUL 누락')

    def returned(self,value=0,cleanup=0):
        """명시 경계의 실제 반환주소와 thiscall 인자만 소비한다."""
        esp = self.mu.reg_read(UC_X86_REG_ESP)  # 현재 경계 스택이다.
        target = self.word(esp)  # CPU CALL이 기록한 반환주소다.
        self.mu.reg_write(UC_X86_REG_EAX,value&0xFFFFFFFF)
        self.mu.reg_write(UC_X86_REG_ESP,esp+4+cleanup)
        self.mu.reg_write(UC_X86_REG_EIP,target)

    def event(self,*fields):
        """실제 도달 경계와 전달값만 순서대로 기록한다."""
        self.trace.append(','.join(str(field) for field in fields))

    def instruction(self,mu,address,size,data):
        """원문 전체를 실행하고 목록/번역/GUI 장치만 반환한다."""
        if address == STOP:
            self.completed = True
            mu.emu_stop()
            return
        if ENTRY <= address < END:
            self.instructions += 1
            return
        esp = mu.reg_read(UC_X86_REG_ESP)  # 각 실제CALL의 전달 인자다.
        arg = lambda index:self.word(esp+4+index*4)  # cdecl/thiscall 공통 스택 인자를 읽는다.
        if address == 0x491680:
            self.event('P');self.returned()
        elif address == 0x477A10:
            self.event('H');self.returned(NODE)
        elif address == 0x4DE9A0:
            self.event('T',self.text(arg(0)).hex());self.returned(arg(0),4)
        elif address in CHILDREN:
            child = CHILDREN[address]  # 실제 호출된 하위 종류다.
            self.event('C',child)
            if self.case['mutation']&1 and child == 0:
                self.word(GLOBALS[0],self.word(GLOBALS[0])^1)
            if self.case['mutation']&2 and child == 3:
                self.word(GLOBALS[1],self.word(GLOBALS[1])^1)
                self.word(GLOBALS[4],9599)
            self.returned(child+2)
        elif address == 0x478160:
            self.event('S',self.text(arg(0)).hex(),arg(1));self.returned()
        elif address == 0x477BB0:
            self.event('V');self.returned()
        elif address == 0x477CB0:
            self.event('I',self.text(arg(0)).hex(),arg(1),self.text(arg(2)).hex(),arg(3),arg(4),arg(5));self.returned()
        elif address == 0x435EF0:
            self.event('D')
            if self.case['mutation']&4:
                self.word(GLOBALS[2],self.word(GLOBALS[2])^1)
                self.word(GLOBALS[3],self.word(GLOBALS[3])^1)
            self.returned(self.case['technical'])
        elif address == 0x478260:
            self.event('X');self.returned()
        elif address == 0x475C00:
            self.event('Q');self.takes += 1;self.returned(7 if self.takes == 1 else NODE)
        elif address == 0x4E4391:
            self.event('C',5)
            # 직접 new/Construct 전체는 명시 하위이며 이 구간의 번역도 그 경계 안으로 구분한다.
            self.cheat = True
            self.returned(HEAP+0x4000 if not self.case['mutation']&32 else 0)
        elif address == 0x4775F0:
            self.returned(8)
        elif address == 0x475F20:
            self.event('W',mu.reg_read(UC_X86_REG_ECX),arg(0))
            if self.case['mutation']&8:
                self.word(0x5CA99C,123)
            self.returned(0,4)
        elif address == ATTACH:
            self.event('A',mu.reg_read(UC_X86_REG_ECX),arg(0),arg(1),arg(2),arg(3))
            if self.case['mutation']&16:
                self.word(NODE+0x2C,991)
            self.returned(0,16)
        else:
            raise RuntimeError(f'미명시 명령 {address:08x}')

    def run(self,case,control):
        """입력 전역만 공급하고 원문 최종DWORD/경계순서를 읽는다."""
        self.case,self.trace,self.instructions,self.takes,self.cheat = case,[],0,0,False
        self.completed = False
        for address,name in zip(GLOBALS,FIELDS[:5]):  # 각 실제 전역을 별도로 초기화한다.
            self.word(address,case[name])
        self.word(0x5C78F0,case['width']);self.word(0x5CA99C,case['left']);self.word(0x563E38,0x01001234)
        self.word(0x5CAD24,0xA5A5A5A5);self.word(NODE,TABLE);self.word(NODE+0x2C,77);self.word(TABLE+0x20,ATTACH)
        self.mu.mem_write(HEAP+0x5000,b'About NetStorm\0');self.word(0x54DB38,HEAP+0x5000)
        self.mu.mem_write(STACK-0x8000,bytes(0x10000));self.word(STACK,STOP);self.word(0,0xAABBCCDD)
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBP):  # 원문이 보존할 일반 레지스터 입력이다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.emu_start(ENTRY,STOP+1,count=10000)
        if not self.completed or self.mu.reg_read(UC_X86_REG_ESP) != STACK+4:
            raise RuntimeError('원문 반환/스택 불일치')
        if bytes(self.mu.mem_read(ENTRY,END-ENTRY)) != self.image[ENTRY-BASE:END-BASE]:
            raise RuntimeError('보호 원문 변경')
        # Cheat의 직접 하위 생성 내부 번역은 C++도 child 콜백 안의 명시 경계로 둔다.
        self.trace = [event for event in self.trace if event != 'T,'+b'Get 10,000'.hex()]
        total = self.totals.setdefault(f'{control:03x}',{'runs':0,'instructions':0})  # 정밀도별 원문 합계다.
        total['runs']+=1;total['instructions']+=self.instructions
        return str(self.word(GLOBALS[4]))+'|'+str(self.word(0x5CAD24))+'|'+';'.join(self.trace)


def main():
    """기대 매핑 없이 조건/정수경계/재진입 입력을 두 정밀도로 관측한다."""
    if digest(PE) != 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236':
        raise RuntimeError('보호 PE SHA 불일치')
    oracle,rows,cases = Oracle(),[],[]  # 원문 실행기/실제관측/명시입력 저장소다.
    for editor,battle,cheat,mission,checksum,technical in itertools.product((0,1),(0,1),(0,1),(0,1),(0,9597,9598,0xFFFFFFFE),(0,1,0xFFFFFFFF)):  # 조건과 signed 감김을 교차한다.
        cases.append(dict(zip(FIELDS,(editor,battle,cheat,mission,checksum,technical,1024,85,0))))
    for mutation,width,left in itertools.product((1,2,4,8,16,31,32),(640,1280,0x7FFFFFFF),(0,85,0xFFFFFFF0)):  # 하위 효과 뒤 재조회와 단정도 경계를 독립 입력으로 준다.
        cases.append(dict(zip(FIELDS,(1,1,1,1,0x7FFFFFFE,1,width,left,mutation))))
    for index,case in enumerate(cases):  # 각 입력을 같은 원문에서 두 번 실행한다.
        previous = None
        for control in CONTROLS:
            observed = oracle.run(case,control)  # 기대 문자열은 계산하지 않는다.
            if previous is not None and previous != observed:
                raise RuntimeError('정밀도 차이')
            previous = observed
            rows.append(f'{index}-cw{control:03x}\t'+','.join(str(case[field]) for field in FIELDS)+'\t'+observed)
    FIXTURE.write_text('# name\tinput\toutput\n'+'\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    report = {'host':socket.gethostname(),'lastDecompileHost':'VM-W11-Codex / 2026-10-23','inputs':len(cases),'observations':len(rows),'precisionDifferences':0,'precisions':oracle.totals,'scope':'004cc210 전체 원문 부모. Game/Edit/View/Options/Players,491680,메뉴GUI/번역/기술도움말조회/Cheat직접생성은 명시 경계다. 폭/x/ID/검산과 재조회 분기는 실제 명령이다.','limits':['하위 부모의 본체/명령소비/메뉴 픽셀/실제 GUI 증거가 아니다.','Cheat하위 직접 new/Construct/내부 Get10,000 번역은 전체 하위 경계로 정규화했다.'],'gameExecuted':False,'guiExecuted':False,'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (PE,Path(__file__),ROOT/'extracted/refined/originals/Netstorm.c',FIXTURE)}}  # 범위와 식별자다.
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:report[key] for key in ('inputs','observations','precisionDifferences')}))


if __name__ == '__main__':
    main()
