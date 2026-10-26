#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제품 변경 전에 실제 소유자 색9칸과 다리 스캔 초기화의 쓰기 폭/순서를 관측한다."""
import argparse
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

SPECS={'colors':(0x43b540,0x43b59b),'scan':(0x422490,0x4224bc)} #두 실제 RET를 포함하며 이 밖 진입은 거부한다.
REGIONS=((0x531af8,68),(0x52f958,40),(0x545398,64),(0x55b4c0,40)) #네 원문 물리와 양쪽 canary를 서로 겹치지 않게 보관한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/state-bridge-reset-x86.tsv' #최초 관측을 제품 결과로 덮어쓰지 않는다.
REPORT=ROOT/'cpppj/state-bridge-reset-native-20261026.json' #실행하지 않은 CD 몸체를 같은 결과로 취급하지 않는다.

class Oracle(MemoryOracle):
    """게임/OS/하위 치환 없이 두 실제 PE 몸체만 실행한다."""
    def __init__(self):
        """원본 읽기 사본에만 입력과 관측 hook을 설치한다."""
        super().__init__();self.running=False;self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def instruction(self,mu,address,size,data):
        """현재 실제 함수의 범위와 유한 명령 수를 검사한다."""
        self.instructions+=1;low,high=SPECS[self.kind]
        if not low<=address<high or self.instructions>40:raise RuntimeError('State 다리 초기화 몸체 이탈: '+hex(address))

    def write(self,mu,access,address,size,value,data):
        """스택 밖의 모든 실제 store를 순서와 폭 그대로 기록한다."""
        if self.running and not STACK-256<=address<STACK+64:
            if not any(base<=address and address+size<=base+length for base,length in REGIONS):raise RuntimeError('State 다리 초기화의 예상 밖 store')
            self.stores.append(f'{address}:{size}:{value&((1<<(8*size))-1)}')

    def snapshot(self):
        """읽지 않는 이웃 바이트까지 같은 순서로 직렬화한다."""
        return b''.join(bytes(self.mu.mem_read(base,length)) for base,length in REGIONS)

    def observe(self,kind,case,control):
        """초기 값/큰 시각/음수/float 지연과 이웃 canary의 명시 입력을 실행한다."""
        self.kind=kind
        for region,(base,length) in enumerate(REGIONS): #서로 다른 최초 물리를 채우며 결과를 역산하지 않는다.
            self.mu.mem_write(base,bytes((case*31+region*47+index*17)&255 for index in range(length)))
        period=(10.0,0.0,-0.0,0.125,-10.0,0.0001,7.75,10.0)[case%8]
        now=(0.0,-0.0,0.25,-123.5,16777217.25,1099511627776.125,12345.75,0.00000001)[case%8]
        self.mu.mem_write(0x52f968,struct.pack('<f',period));self.mu.mem_write(0x55b4d0,struct.pack('<d',now));before=self.snapshot()
        self.mu.mem_write(STACK-256,b'\xcd'*320);self.put(STACK,STOP)
        saved=((UC_X86_REG_EBX,0x456789ab),(UC_X86_REG_ESI,0x56789abc),(UC_X86_REG_EDI,0x6789abcd),(UC_X86_REG_EBP,0x789abcde))
        for register,value in ((UC_X86_REG_EAX,0x89abcdef),(UC_X86_REG_ECX,0x12345678),(UC_X86_REG_EDX,0x3456789a),*saved): #비영 레지스터 입력에서 비휘발 보존도 확인한다.
            self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPSW,0);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.instructions=0;self.stores=[];self.running=True
        try:self.mu.emu_start(SPECS[kind][0],STOP)
        finally:self.running=False
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('State 다리 초기화 복귀 차이')
        for register,value in saved: #원문의 비휘발 레지스터는 모두 보존돼야 한다.
            if self.mu.reg_read(register)!=value:raise RuntimeError('State 다리 초기화 비휘발 레지스터 차이')
        status=self.mu.reg_read(UC_X86_REG_FPSW)
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or status&0x3800 or self.mu.reg_read(UC_X86_REG_FPTAG)!=0xffff:raise RuntimeError('State 다리 초기화 FPU 스택/제어 보존 차이')
        return [kind,case,control,before.hex(),self.snapshot().hex(),';'.join(self.stores),status,self.instructions]

def main():
    """최초 독립 관측 생성과 기존 근거의 읽기 전용 SHA 검증을 분리한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,wanted in report['sha256'].items(): #도구/원문/동결 기대를 최종 제품에 맞춰 바꾸지 않는다.
            if digest(ROOT/name)!=wanted:raise RuntimeError('State 다리 초기화 근거 SHA 차이: '+name)
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('State 다리 초기화 최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=[oracle.observe(kind,case,control) for kind in SPECS for case in range(16) for control in (0x27f,0x37f)]
    FIXTURE.write_bytes(('# kind / case / control / input / output / stores / fpsw / instructions\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows)).encode('utf-8'))
    names=['originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_state_bridge_reset.py',FIXTURE.relative_to(ROOT).as_posix()]
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','edition':'PATCH10.78','observations':len(rows),'effectsStubbed':False,'originalGameExecuted':False,'functions':{kind:{'entry':hex(low),'bytes':high-low,'bodyHex':bytes(oracle.mu.mem_read(low,high-low)).hex()} for kind,(low,high) in SPECS.items()},'regions':REGIONS,'sha256':{name:digest(ROOT/name) for name in names},'cdNote':'10.72 대응은 색 표00495d10/005365a0, 스캔00449200이다. 이번에는 실행하지 않았다.'}
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print(json.dumps({'passed':True,'observations':len(rows),'host':report['host']}))

if __name__=='__main__': #CLI 명시 호출에서만 격리 CPU를 실행한다.
    main()
