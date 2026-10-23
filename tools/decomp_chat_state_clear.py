#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""00445140의 실제 100행 정리를 제품 구현 전에 PE 격리 실행으로 관측한다."""
import argparse
import csv
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW)

ENTRY, RECORDS, COUNT, STRIDE = 0x445140, 0x558ed0, 100, 48  # 원문 함수와 같은 정적 기록 주소/폭이다.
SIZE, GUARD = COUNT*STRIDE, 16  # 기록 앞뒤 경계도 실제 물리에서 확인한다.
MY, LOGIN = 0x540c74, 0x54265c  # 정리와 무관한 두 실제 Subscriber DWORD다.
FIXTURE = ROOT/'cpppj/tests/fixtures/chat-state-clear-x86.tsv'  # 최초 원문 출력만 저장한다.
REPORT = ROOT/'cpppj/chat-state-clear-native-20261024.json'  # 제품 전 관측과 이후 통합을 구별한다.


def cases():
    """기대 결과 없이 100개 위치와 희소/빈/전체 입력을 독립 지정한다."""
    rows=[]  # 같은 입력 id를 두 정밀도에 재사용한다.
    patterns=([],[(i,1000+i) for i in range(COUNT)],[(i,1) for i in range(0,COUNT,2)],
              [(0,0xffffffff)],[(99,0x80000000)],[(i,0x1000000) for i in range(0,COUNT,11)],
              [(1,0x100),(2,0x10000),(3,0x1000000),(4,0x80000000)],[(i,77) for i in (0,49,50,99)])  # DWORD의 높은 BYTE와 중복 ID도 유효 점유다.
    for index,occupied in enumerate(patterns):  # 경계와 전체 밀도를 별도 입력으로 만든다.
        rows.append((f'pattern{index}',(index*31+0x59)&255,occupied))
    for index in range(COUNT):  # 모든 기록 위치가 실제로 포함되는지 확인하는 한 행 점유 입력이다.
        rows.append((f'single{index}',(index*17+0xa5)&255,[(index,(1,0xffffffff,0x80000000,0x1000000)[index%4])]))
    return rows


def initial(seed,occupied):
    """행 선택 기대를 만들지 않고 비영 stale 패턴과 명시 Subscriber 입력만 쓴다."""
    result=bytearray((seed+index*37)&255 for index in range(SIZE))  # 빈 행의 이름/flags/padding도 그대로 남기는 입력이다.
    for index in range(COUNT):  # 입력의 모든 Subscriber를 우선0으로 지정한다.
        struct.pack_into('<I',result,index*STRIDE+44,0)
    for index,value in occupied:  # 점유 여부는 독립 호출자의 명시 입력이다.
        struct.pack_into('<I',result,index*STRIDE+44,value)
    return bytes(result)


class ClearOracle(MemoryOracle):
    """37BYTE 실제 몸체만 실행하며 OS·다른 게임 함수·외부 대체 효과를 허용하지 않는다."""
    def __init__(self):
        """현재 PC 전체 정밀 함수 범위를 확인한 뒤 메모리 쓰기를 감시한다."""
        super().__init__()
        self.allowed=[]  # 부모의 사제 메뉴 범위는 이 한 함수로 교체한다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 실제 함수표의 포함 범위를 그대로 사용한다.
                if int(row['entry'],16)==ENTRY:
                    self.allowed=[tuple(int(value,16) for value in part.split('-')) for part in row['ranges'].split(';')]
        if self.allowed!=[(ENTRY,ENTRY+36)]:
            raise RuntimeError('현재 정밀 Chat 정리37BYTE 범위 불일치')
        self.active=False  # 호스트의 명시 초기 입력 쓰기는 원문 store로 세지 않는다.
        self.writes=[]  # 원문이 쓴 기록의 실제 offset/폭/값이다.
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def instruction(self,mu,address,size,data):
        """다른 명령으로 빠지면 미복원 성공을 공급하지 않고 즉시 중단한다."""
        if not any(low<=address and address+size-1<=high for low,high in self.allowed):
            raise RuntimeError(f'Chat 정리 범위 밖 명령 {address:08x}')
        self.instructions+=1

    def write(self,mu,access,address,size,value,data):
        """기록과 실제 callee 스택 외의 쓰기를 거부하고 전체 guard를 보존한다."""
        if not self.active:
            return
        if RECORDS<=address and address+size<=RECORDS+SIZE:
            self.writes.append((address-RECORDS,size,value))
        elif not (STACK-32<=address and address+size<=STACK+4):
            raise RuntimeError(f'Chat 정리 범위 밖 쓰기 {address:08x}/{size}')

    def sample(self,case,control):
        """실제 반환·ABI·제어어·4800BYTE와 양쪽 guard를 동시에 관측한다."""
        name,seed,occupied=case
        before=initial(seed,occupied)  # 제품이나 기대 계산과 연결하지 않는 입력이다.
        self.active=False
        guards=bytes((index*19+0x63)&255 for index in range(GUARD*2))  # 두 guard는 다른 패턴을 갖는다.
        self.mu.mem_write(RECORDS-GUARD,guards[:GUARD]+before+guards[GUARD:])
        self.put(MY,0x12345678);self.put(LOGIN,0x89abcdef)
        self.mu.mem_write(STACK-32,bytes([0x7a])*64)
        self.mu.mem_write(STACK,struct.pack('<I',STOP))
        saved={UC_X86_REG_EBX:0x11112222,UC_X86_REG_ESI:0x33334444,UC_X86_REG_EDI:0x55556666,UC_X86_REG_EBP:0x77778888}  # cdecl 보존 GP의 명시 입력이다.
        for register,value in saved.items():  # 정상 반환 뒤 실제 원문 보존 여부를 검사한다.
            self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_EAX,0xccccdddd);self.mu.reg_write(UC_X86_REG_ECX,0xaabbccdd);self.mu.reg_write(UC_X86_REG_EDX,0x98765432)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.instructions=0;self.writes=[];self.active=True
        self.mu.emu_start(ENTRY,STOP,count=100000)
        self.active=False
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:
            raise RuntimeError('Chat 정리 정상 반환/ESP 불일치')
        if any(self.mu.reg_read(register)!=value for register,value in saved.items()) or self.mu.reg_read(UC_X86_REG_FPCW)!=control:
            raise RuntimeError('Chat 정리 보존 레지스터/제어어 불일치')
        after=bytes(self.mu.mem_read(RECORDS,SIZE))  # 전체 결과는 실제 PE 출력만 읽는다.
        if bytes(self.mu.mem_read(RECORDS-GUARD,GUARD))!=guards[:GUARD] or bytes(self.mu.mem_read(RECORDS+SIZE,GUARD))!=guards[GUARD:]:
            raise RuntimeError('Chat 정리 guard 변경')
        if self.words(MY,1)!=(0x12345678,) or self.words(LOGIN,1)!=(0x89abcdef,):
            raise RuntimeError('Chat 정리가 두 실제 Subscriber 전역을 변경함')
        return (name,f'{control:04x}',seed,','.join(f'{index}:{value:08x}' for index,value in occupied) or '-',
                after.hex(),len(self.writes),self.instructions)


def main():
    """최초 관측은 새 경로에만 쓰며 검증 모드는 실제 게임/PE 명령도 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)  # 관측과 읽기 전용 SHA 검증을 명시 구별한다.
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--observe',action='store_true');group.add_argument('--verify',action='store_true')
    args=parser.parse_args()  # 기본 실행으로 원문을 덮어쓰지 않는다.
    if args.verify:
        facts=json.loads(REPORT.read_text('utf-8'))
        assert all(digest(ROOT/name)==value for name,value in facts['inputs'].items())
        with FIXTURE.open(encoding='utf-8') as stream:
            rows=list(csv.DictReader(stream,delimiter='\t'))
        assert len(rows)==facts['observations']==216
        print(json.dumps(dict(verified=True,observations=len(rows))))
        return
    if socket.gethostname().casefold()!='hjow-athlon' or FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Chat 정리 관측의 현재 호스트/최초 파일 생성 계약 위반')
    oracle=ClearOracle()
    rows=[oracle.sample(case,control) for case in cases() for control in (0x027f,0x037f)]  # 두 원문 정밀도는 각각 독립 입력으로 실행한다.
    with FIXTURE.open('x',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n')
        writer.writerow(('id','control','seed','occupied','after','writes','instructions'));writer.writerows(rows)
    sources=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_chat_state_clear.py','cpppj/tests/fixtures/chat-state-clear-x86.tsv')  # 제품 전 Native 입력만 고정한다.
    facts=dict(host=socket.gethostname(),date='2026-10-24',lastFullDecompile='HJOW-Athlon / 2026-10-23',
               entry='00445140',bytes=37,observations=len(rows),distinctInputs=len(cases()),normalReturn=True,
               gameGuiOsCalls=0,externalSubstitutions=0,productWritten=False,cdExecuted=False,
               inputs={name:digest(ROOT/name) for name in sources})
    with REPORT.open('x',encoding='utf-8',newline='\n') as stream:
        json.dump(facts,stream,ensure_ascii=False,indent=2);stream.write('\n')
    print(json.dumps(dict(observations=len(rows),distinctInputs=len(cases()),normalReturn=True)))


if __name__=='__main__':
    main()
