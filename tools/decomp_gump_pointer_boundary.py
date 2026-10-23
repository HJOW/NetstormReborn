#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 입력 앞단의 실제 폴링·밖 기록·재진입 release 보정 명령만 격리 실행한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_EFLAGS)

# 실제 UserInput의 디버그 생성 뒤부터 Guide에 지역 사건 주소를 push하기 직전까지다.
ENTRY,END=0x4d6379,0x4d65a1
FLAGS,INSIDE,LEFT,RIGHT,LANGUAGE=0x5cafe8,0x54354c,0x5cafdc,0x5cafd0,0x542384
FIXTURE=ROOT/'cpppj/tests/fixtures/gump-pointer-boundary-x86.tsv'  # 제품과 독립인 새 원문 관측이다.
REPORT=ROOT/'extracted/cpp-gump-pointer-boundary-20261023-native.json'  # 범위/정밀도/해시 기록이다.


def event(code,x=0,y=0):
    """입력 사건의 세 DWORD를 명시 바이트 값으로만 구성한다."""
    return code&0xffffffff,x&0xffffffff,y&0xffffffff


def encode(value):
    """원문 DWORD 세 개를 부호 변환 없는 fixture 필드로 만든다."""
    return ','.join(map(str,value))


def inputs():
    """결과 계산 없이 언어·정적 초기화·안팎·네 버튼 상태 비트의 입력을 교차한다."""
    values=[]
    for language,flags,inside,outside,bits in itertools.product((0,3,7),(0,1,2,3,0x80000003),(0,1,0xffffffff),(0,1),range(32)):
        secondary_states=(0,1) if language==3 else (0,)
        for secondary_release in secondary_states:  # 언어3의 Ctrl 조합만 두 상태를 관측한다.
            primary=event(0x120000|(outside<<26)|((bits&1)<<31)|0x40000000,0xffffffff,0x80000000)
            secondary=event(0x110000|(secondary_release<<31),13,14)
            left=event(0x10000|(((bits>>1)&1)<<31),101,102)
            right=event(0x20000|(((bits>>2)&1)<<31),201,202)
            current_left=event(0x10000|(((bits>>3)&1)<<31)|0x18000000,0x87654321,0xffffffff)
            current_right=event(0x20000|(((bits>>4)&1)<<31)|0x24000000,0x80000000,0x76543210)
            values.append((language,flags,inside,left,right,primary,secondary,current_left,current_right,0,0))
    # 콜백이 기록을 바꾸는 명시 입력은 원문 재조회 시점을 확인한다. 큐 포화도 같은 원문 큐로 실행한다.
    for language,mutation,prefill in itertools.product((0,3),range(1,7),(0,38,39)):
        values.append((language,3,0,event(0x10000,1,2),event(0x20000|(0x80000000 if mutation==3 else 0),3,4),
            event(0x80120000,11,12),event(0x80110000,13,14),event(0x98010000,21,22),event(0xa4020000,31,32),mutation,prefill))
    # 첫 폴링 callback 뒤 초기화와 outside 진입 중 기록 덮어쓰기 순서를 별도로 관측한다.
    for mutation in (4,5):
        values.append((0,0,1,event(1,2,3),event(4,5,6),event(0x84120000,11,12),event(0x80110000),event(0x10000,21,22),event(0x20000,31,32),mutation,0))
    return values


class Oracle(MemoryOracle):
    """GetAsyncKeyState/좌표를 폴링 결과로만 공급하고 분기·기록·큐 본체는 실제 PE로 실행한다."""
    def __init__(self):
        """기존 보호 PE 매핑 위에 선택한 원문 명령 두 범위만 허용한다."""
        super().__init__()
        facts={int(r['entry'],16):r for r in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.allowed=[(ENTRY,END)]
        for part in facts[0x452f90]['ranges'].split(';'):  # 실제 Push의 코드/좌표/40칸 포화 처리를 실행한다.
            low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.trace=[];self.poll_count=0;self.queue_count=0;self.normal=0;self.record=False

    def write_event(self,address,value):
        """실제 사건 저장소에 세 DWORD를 그대로 쓴다."""
        self.mu.mem_write(address,struct.pack('<III',*value))

    def mutate(self,stage):
        """poll/queue 공급자가 현재 기록을 바꾸는 명시 외부 효과만 주입한다."""
        mutation=self.case[9]
        if mutation==1 and stage=='P3':self.write_event(LEFT,event(0x10000,41,42))
        elif mutation==2 and stage=='Q1':self.write_event(RIGHT,event(0x80020000,51,52))
        elif mutation==3 and stage=='Q1':self.write_event(RIGHT,event(0x20000,61,62))
        elif mutation==4 and stage=='P3':
            self.write_event(LEFT,event(0x80010000,71,72));self.put(INSIDE,0x12345678);self.put(FLAGS,0x40000003)
        elif mutation==5 and stage=='P1':
            self.write_event(LEFT,event(0x80010000,81,82));self.write_event(RIGHT,event(0x80020000,83,84));self.put(FLAGS,0x80000002)
        elif mutation==6 and stage=='Q1':self.put(INSIDE,0x12345678);self.put(FLAGS,0x40000003)
        else:return
        self.trace.append('M'+str(mutation))

    def instruction(self,mu,address,size,data):
        """실제 기계어 사이의 폴링만 cdecl 출력 인자로 공급하며 Push는 중단하지 않는다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        args=self.words(esp+4,3)
        if address==0x452e00:
            self.poll_count+=1;code=args[1]
            sample={0x120000:self.case[5],0x110000:self.case[6],0x10000:self.case[7],0x20000:self.case[8]}[code]
            self.trace.append('P,'+str(code)+','+encode(sample));self.mutate('P'+str(self.poll_count))
            self.write_event(args[0],sample);self.ret(args[0]);return
        if address==0x452f90 and self.record:
            self.queue_count+=1;self.trace.append('Q,'+encode(args));self.mutate('Q'+str(self.queue_count))
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'미등록 포인터 경계 명령 {address:08x}')
        self.instructions+=1

    def sample(self,case,precision):
        """앞단 지역 스택과 실제 전역을 독립 초기화하고 Guide 직전의 결과만 관측한다."""
        self.case=case;self.trace=[];self.poll_count=0;self.queue_count=0;self.record=False
        language,flags,inside,left,right,*_=case
        self.put(LANGUAGE,language);self.put(FLAGS,flags);self.put(INSIDE,inside);self.write_event(LEFT,left);self.write_event(RIGHT,right)
        self.put(0x55a508,0);self.put(0x55a50c,0);self.mu.mem_write(0x55a510,bytes(480))
        # 포화 입력의 기존 사건도 실제 Push로 넣는다. 관측 대상 보정의 로그에는 포함하지 않는다.
        for index in range(case[10]):
            self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<IIII',STOP,0x02000100+index,index,index+100))
            self.mu.emu_start(0x452f90,STOP,count=1000)
            assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        self.record=True;self.mu.mem_write(STACK,bytes(0x100))
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_EBX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,0),
            (UC_X86_REG_ESI,0),(UC_X86_REG_EDI,0),(UC_X86_REG_EBP,0),(UC_X86_REG_ESP,STACK),
            (UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            self.mu.reg_write(register,value)  # 원문 프롤로그 뒤 선택 구간의 명시 진입 상태다.
        self.mu.emu_start(ENTRY,END,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==END and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        assert self.mu.reg_read(UC_X86_REG_FPCW)==precision and not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        self.normal+=1
        queue=';'.join(encode(self.words(0x55a510+index*12,3)) for index in range(self.words(0x55a50c,1)[0])) or '-'
        return (encode(self.words(STACK+0x14,3)),str(self.words(FLAGS,1)[0]),str(self.words(INSIDE,1)[0]),
            encode(self.words(LEFT,3)),encode(self.words(RIGHT,3)),queue,';'.join(self.trace) or '-')


def main():
    """새 fixture를 실제 PE 결과로 생성하거나 기존 원문 해시만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
        assert report['rows']==len(inputs());print('포인터 앞단 원문 해시 확인',report['rows']);return
    oracle,rows=Oracle(),[]
    for case in inputs():  # 제품 구현에 기대값을 묻지 않고 실제 두 정밀도의 같은 결과만 보관한다.
        first=oracle.sample(case,0x27f);assert first==oracle.sample(case,0x37f),case
        language,flags,inside,left,right,primary,secondary,current_left,current_right,mutation,prefill=case
        rows.append((str(language),str(flags),str(inside),encode(left),encode(right),encode(primary),encode(secondary),
            encode(current_left),encode(current_right),str(mutation),str(prefill),*first))
    FIXTURE.write_text('# language flags inside leftRecord rightRecord primary ctrl leftNow rightNow mutation prefill pointer flagsOut insideOut leftOut rightOut queue effects\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    names=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv',
        'tools/decomp_priestmenu.py','tools/decomp_gump_pointer_boundary.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastDecompileHost='VM-W11-Codex',lastDecompileDate='2026-10-23',rows=len(rows),
        normalFragmentExits=oracle.normal,nativeInstructions=oracle.instructions,precisionDifferences=0,
        sha256={name:digest(ROOT/name) for name in names},limits=[
            'UserInput4d6379..4d65a0과452f90전체만 실제 실행한다. 디버그 객체 생성/Guide4d65a7/카메라/Overlay/Broadcast/일반 큐 드레인은 범위 밖이다.',
            '452e00의OS폴링만명시code/x/y로공급한다. poll/queue콜백의전역변경은명시입력이다.',
            '부분구간은Guide직전EIP/동일ESP/x87상태를확인한것이며UserInput전체의정상return/게임GUI증거는아니다.',
            '원본보호파일수정/게임/GUI/새디컴파일/빌드/제품실행은없다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('포인터 앞단 실제 명령 관측',len(rows),oracle.normal,oracle.instructions)


if __name__=='__main__':
    main()
