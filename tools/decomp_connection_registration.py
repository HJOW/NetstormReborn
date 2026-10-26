#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 연결 등록/재배치/제거 세 몸체를 제품 변경 전에 독립 관측한다."""
import argparse
import csv
import itertools
import json
import socket
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

# 실제 등록표와 같은 원문 전역 주소 및 격리 연결 노드다.
FUNCTIONS=(0x4e2870,0x4df4f0,0x4e27b0)
MAIN,PENDING,COUNT,SERVER=0x5e4a18,0x5e46f0,0x543604,0x540bc0
NODES=POOL+256 #원문 포인터 대신 최초 fixture의 격리 실제 주소다.
FIXTURE=ROOT/'cpppj/tests/fixtures/connection-registration-x86.tsv' #최초 출력은 제품으로 덮지 않는다.
REPORT=ROOT/'cpppj/connection-registration-native-20261026.json' #게임/OS 실행과 구별한 CPU 근거다.


class Oracle(MemoryOracle):
    """세 실제 부모만 실행하고 buffered 설정/진단만 명시 외부 장치로 둔다."""
    def __init__(self):
        """읽기 전용 원본의 가상 메모리 사본을 사용한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[] #기존 Oracle의 허용 범위를 이번 실제 세 함수로 한정한다.
        for entry in FUNCTIONS: #각 함수의 실제 분할 범위를 그대로 보존한다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(x,16) for x in part.split('-'));self.allowed.append((low,high+1))

    def blob(self):
        """현재 표/개수/서버/세 노드 전체 BYTE를 읽는다."""
        return b''.join(bytes(self.mu.mem_read(at,size)) for at,size in ((MAIN,88),(PENDING,128),(COUNT,4),(SERVER,4),(NODES,576)))

    def instruction(self,mu,address,size,data):
        """buffered callback 뒤 현재 개수와 슬롯이 다시 사용되는지 관측한다."""
        self.instructions+=1
        if self.instructions>2000:raise RuntimeError('연결 등록 명령 한도')
        if address in (0x4df470,0x4e0620):
            kind='B' if address==0x4df470 else 'A'
            value=mu.reg_read(UC_X86_REG_ECX) if kind=='B' else self.words(mu.reg_read(UC_X86_REG_ESP)+12,1)[0]
            self.calls.append(kind+':'+str(value))
            if kind=='B':
                if self.words(mu.reg_read(UC_X86_REG_ESP)+4,1)[0]!=1:raise RuntimeError('실제 buffered 모드 인자 차이')
                self.put(value+0x34,self.words(value+0x34,1)[0]|2)
                if self.change:
                    self.put(COUNT,0);self.put(MAIN+self.target*4,NODES+384);self.put(PENDING-self.pending*4,NODES+384)
            if self.stop==kind:self.stopped=True;mu.emu_stop();return
            self.ret(purge=4 if kind=='B' else 0)
        elif not any(low<=address<high for low,high in self.allowed):
            raise RuntimeError('연결 등록 몸체 이탈: '+hex(address))

    def observe(self,operation,case,change,stop,control):
        """정상/진단/개수 감김/현재 슬롯 변경을 명시 입력으로 실행한다."""
        self.mu.mem_write(MAIN,b'\0'*88);self.mu.mem_write(PENDING,b'\0'*128)
        self.mu.mem_write(NODES,bytes((case*31+i*17)&255 for i in range(576)))
        for index in range(3): #실제 표현 가능한 flags/sender 입력만 초기화한다.
            self.put(NODES+index*192+0x34,0x24);self.put(NODES+index*192+0x40,71+index)
        self.pending=(-1,-31,0,-1,-31,-1,-1,0)[case]
        self.target=(1,8,9,0,9,21,1,9)[case]
        self.put(COUNT,0 if case==6 else 7);self.put(SERVER,0 if case in (2,7) else 1)
        self.put(PENDING,NODES);self.put(PENDING+4,NODES+192);self.put(PENDING+31*4,NODES)
        if case in (3,5):self.put(MAIN+self.target*4,NODES+384)
        arguments=(self.target,NODES) if operation==0 else (self.pending,) if operation==1 else (self.pending,self.target)
        before=self.blob()
        self.calls=[];self.instructions=0;self.change=change;self.stop=stop;self.stopped=False #효과열·실행한도·현재 변경/중단 입력과 실제 중단 결과다.
        self.put(STACK,STOP)
        for index,value in enumerate(arguments):self.put(STACK+4+index*4,value) #실제 cdecl 스택 인자 순서다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.emu_start(FUNCTIONS[operation],STOP)
        if not self.stopped and (self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('연결 등록 정상 복귀 차이')
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control:raise RuntimeError('연결 등록 FPU 변경')
        return [operation,case,change,stop,control,self.pending,self.target,before.hex(),self.blob().hex(),';'.join(self.calls) or '-',int(self.stopped)]


def main():
    """최초 입력/출력을 한 번 기록하고 이후 SHA만 확인한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,wanted in report['sha256'].items(): #최초 원본/도구/fixture의 변경을 거부한다.
            if digest(ROOT/name)!=wanted:raise RuntimeError('연결 등록 SHA 차이: '+name)
        print(json.dumps({'verified':True,'observations':report['observations']}));return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 연결 등록 관측 덮어쓰기 금지')
    oracle=Oracle();rows=[oracle.observe(*values) for values in itertools.product(range(3),range(8),(0,1),('none','A','B'),(0x27f,0x37f))]
    FIXTURE.write_bytes(('# operation / case / change / stop / control / pending / target / input / output / calls / stopped\n'+''.join('\t'.join(map(str,row))+'\n' for row in rows)).encode('utf-8'))
    names=['originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_connection_registration.py',FIXTURE.relative_to(ROOT).as_posix()]
    report={'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','edition':'PATCH10.78','observations':len(rows),'stoppedObservations':sum(row[10] for row in rows),'realFunctions':[hex(x) for x in FUNCTIONS],'stubbedEffects':['004df470 buffered setting','004e0620 assertion'],'originalGameExecuted':False,'sha256':{name:digest(ROOT/name) for name in names}}
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print(json.dumps({'passed':True,'observations':len(rows),'stopped':report['stoppedObservations']}))


if __name__=='__main__': #명시 CLI에서만 원본 사본의 CPU 명령을 실행한다.
    main()
