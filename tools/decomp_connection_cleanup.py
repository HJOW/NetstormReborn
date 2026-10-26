#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""연결 정리 여섯 실제 PATCH 몸체와 지연 삭제를 제품 변경 전에 격리 관측한다."""
import argparse
import csv
import itertools
import json
import socket
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

# 원문 고정 표·동기화·개수와 격리 연결/버퍼 주소다.
FUNCTIONS=(0x4df440,0x4df520,0x4e2940,0x4e29c0,0x4e2a10,0x4df4a0)
MAIN,BUFFERS,PENDING,SYNC,COUNT,SERVER=0x5e4a18,0x5e4a70,0x5e46f0,0x5c8bf4,0x543604,0x540bc0
NODES,BUFFER=POOL+256,POOL+2048
FIXTURE=ROOT/'cpppj/tests/fixtures/connection-cleanup-x86.tsv' #최초 관측을 C++ 출력으로 덮지 않는다.
REPORT=ROOT/'cpppj/connection-cleanup-native-20261026.json' #게임/OS 실행과 구별한 실제 CPU 근거다.


class Oracle(MemoryOracle):
    """여섯 실제 본문만 허용하고 고정 소멸/CRT free/진단만 외부 장치로 둔다."""
    def __init__(self):
        """원본 파일은 읽기 전용이며 가상 메모리 사본만 수정한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[]
        for entry in FUNCTIONS: #함수표의 실제 분할 범위를 그대로 허용한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(x,16) for x in part.split('-'))
                self.allowed.append((low,high+1))

    def blob(self):
        """이웃 버퍼표·pending0·현재 동기화/개수·노드 전체 BYTE도 읽는다."""
        return b''.join(bytes(self.mu.mem_read(at,size)) for at,size in ((MAIN,176),(PENDING,128),(SYNC,4),(COUNT,4),(SERVER,4),(NODES,576)))

    def instruction(self,mu,address,size,data):
        """하위 효과의 중단 전에 원문이 완료한 store/call prefix를 유지한다."""
        self.instructions+=1
        if self.instructions>20000:
            raise RuntimeError('연결 정리 명령 한도')
        if address in (0x4e03d0,0x4e4354,0x4e0620):
            kind='D' if address==0x4e03d0 else 'F' if address==0x4e4354 else 'A'
            value=mu.reg_read(UC_X86_REG_ECX) if kind=='D' else self.words(mu.reg_read(UC_X86_REG_ESP)+4,1)[0] if kind=='F' else self.words(mu.reg_read(UC_X86_REG_ESP)+12,1)[0]
            self.calls.append(kind+':'+str(value))
            if self.change and not self.changed and kind=='D':
                self.changed=True
                self.put(MAIN+21*4,NODES+384);self.put(PENDING+31*4,NODES+384)
                self.put(SYNC,NODES+384);self.put(COUNT,0xffffffff);self.put(SERVER,0)
                self.put(value+0x8c,7) #소멸 callback 뒤 depth를 다시 읽어 free를 생략하면 원문과 다르다.
            if self.stop==kind:
                self.stopped=True;mu.emu_stop();return
            self.ret()
        elif not any(low<=address<high for low,high in self.allowed):
            raise RuntimeError('연결 정리 몸체 이탈: '+hex(address))

    def observe(self,operation,case,change,stop,control):
        """실제 표와 세 연결 입력을 정하고 독립 출력/효과열을 수집한다."""
        self.mu.mem_write(MAIN,b'\0'*176);self.mu.mem_write(PENDING,b'\0'*128)
        self.mu.mem_write(NODES,bytes((case*31+i*17)&255 for i in range(576)))
        for index in range(3): #연결 외 BYTE는 패턴을 보존하고 flags/depth 두 필드만 입력한다.
            self.put(NODES+index*192+0x34,0x20 if index!=1 else 0x44)
            self.put(NODES+index*192+0x8c,1 if case==7 or (case==6 and index==1) else 0)
        self.put(SYNC,NODES if case%2 else NODES+192);self.put(COUNT,0xffffffff if case==5 else 3);self.put(SERVER,0 if case==0 else 1)
        if case:
            self.put(MAIN,NODES);self.put(MAIN+4,NODES);self.put(MAIN+8*4,NODES+192);self.put(MAIN+9*4,NODES+192);self.put(MAIN+21*4,NODES+384)
            self.put(PENDING,NODES);self.put(PENDING+4,NODES+192);self.put(PENDING+31*4,NODES+384)
            for index in (0,1,8,9,21): #단일 양수 종료만 해제하는 인접 송신 버퍼 표다.
                self.put(BUFFERS+index*4,BUFFER+index*16)
        argument=(0,1,8,9,21,-1,-31,-32,22)[case]
        if operation==1:argument=(-1,-1,-31,-32,-33,0,1,-2147483648,22)[case]
        before=self.blob();self.calls=[];self.instructions=0;self.change=change;self.changed=False;self.stop=stop;self.stopped=False
        self.put(STACK,STOP);self.put(STACK+4,argument);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,NODES);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.emu_start(FUNCTIONS[operation],STOP)
        if not self.stopped and (self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4):
            raise RuntimeError('연결 정리 정상 복귀 차이')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control:
            raise RuntimeError('연결 정리 FPU 변경')
        return [operation,case,change,stop,control,argument,before.hex(),self.blob().hex(),';'.join(self.calls) or '-',int(self.stopped),self.instructions]


def main():
    """최초 CPU 관측을 한 번만 기록하고 이후에는 SHA만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,wanted in report['sha256'].items(): #원본·함수표·도구·최초 관측의 변경을 거부한다.
            if digest(ROOT/name)!=wanted:raise RuntimeError('연결 근거 SHA 차이: '+name)
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('연결 최초 관측 덮어쓰기 금지')
    oracle=Oracle()
    rows=[oracle.observe(*values) for values in itertools.product(range(6),range(8),(0,1),('none','D','F'),(0x27f,0x37f))]
    rows.extend(oracle.observe(2,8,change,'A',control) for change,control in itertools.product((0,1),(0x27f,0x37f)))
    FIXTURE.write_bytes(('# operation / case / change / stop / control / argument / input / output / calls / stopped / instructions\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows)).encode('utf-8'))
    names=['originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_connection_cleanup.py',FIXTURE.relative_to(ROOT).as_posix()]
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','edition':'PATCH10.78','observations':len(rows),'stoppedObservations':sum(row[9] for row in rows),'realFunctions':[hex(x) for x in FUNCTIONS],'stubbedEffects':['004e03d0 connection destructor','004e4354 CRT free','004e0620 assertion'],'originalGameExecuted':False,'sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print(json.dumps({'passed':True,'observations':len(rows),'stopped':report['stoppedObservations']}))


if __name__=='__main__': #명시 실행에서만 격리 CPU 사본을 사용한다.
    main()
