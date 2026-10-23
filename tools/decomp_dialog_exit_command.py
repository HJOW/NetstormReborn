#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78 OnExit의실제메타resolve명령구간과HTML부모소멸을독립실행한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_script_command import Oracle as ScriptOracle, ROOT, TEXT, POOL, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,UC_X86_REG_ESP,UC_X86_REG_EIP,
    UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_EFLAGS)

NODE, CLOCK = TEXT+0x1000, TEXT+0x5000  # 실제HTML부모와명시벽시각의격리주소다.
RESOLVE, RESOLVED, ASSIGN, ASSIGNED = 0x4c85d2,0x4c863e,0x4c8a50,0x4c8b77  # EAX→EDI저장까지포함하는실제c83b0선택경로다.
DESTROY = 0x4cf1c0  # 실제HTML부모소멸전체의입구다.
FIXTURE = ROOT/'cpppj/tests/fixtures/dialog-exit-command-x86.tsv'  # 제품코드전에동결하는새독립원문이다.
REPORT = ROOT/'extracted/cpp-dialog-exit-command-20261023-native.json'  # 범위/정상반환/해시기록이다.


def inputs():
    """원문기대값없이명령이름/숫자첫바이트/문자열/현재code0입력만교차한다."""
    commands=(b'QuitApp',b'Tell',b'Config',b'Pause',b'MISSING',b' Tell ')
    arguments=(b'',b'0',b'99',b'123tail',b'4294967296',b'2147483648',b'999999999999999999999',
        b'-1',b'+2',b' 99',b'\t99',b'Blank',b'"Blank"',b'a,b;>c',b'\xff',b'99\0tail',b'A'*255)
    return list(itertools.product(commands,arguments,(0,1)))


class Oracle(ScriptOracle):
    """명령표/숫자CRT/80칸고리/40칸큐는기존실제명령을그대로실행한다."""
    def __init__(self):
        """기존읽기전용PE와해석기위에새선택구간/HTML소멸만허용한다."""
        super().__init__()
        self.mu.mem_map(0,0x1000)
        self.allowed.extend(((RESOLVE,RESOLVED),(ASSIGN,0x4c8a69)))
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for entry in (DESTROY,0x4e66de):  # 실제소멸전체와메타atol진입의불연속범위다.
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'))
                self.allowed.append((low,high+1))
        self.trace=[]
        self.normal=0

    def event(self,*values):
        """새모듈의실제해석/예약경계만원문순서로기록한다."""
        self.trace.append(','.join(map(str,values)))

    def instruction(self,mu,address,size,data):
        """메타선택구간과소멸을실행하고부수루트/벽시각/기본소멸만명시한다."""
        if address in (RESOLVED,ASSIGNED):
            mu.emu_stop()
            return
        esp=mu.reg_read(UC_X86_REG_ESP)
        args=self.words(esp+4,4)
        if address==0x4d52c0:
            self.event('Lookup',self.string(args[0]).hex())
        elif address==0x4e6fa1:
            self.event('Digit',args[0])
        elif address==0x4e66de:
            self.event('Atol',self.string(args[0]).hex())
        elif address==0x4c7910:
            self.event('String',self.string(args[0]).hex())
        elif address==0x4c2630:
            self.event('Diagnostic',self.string(args[2]).hex())
            self.ret()
            return
        elif address==0x452f90:
            self.event('Queue',*args[:3])
        elif address==0x464020:
            assert args[0]==0
            self.ret()
            return
        elif address==0x460d70:
            mu.mem_write(args[0],struct.pack('<d',100.0))
            self.ret(args[0])
            return
        elif address==0x464420:
            assert mu.reg_read(UC_X86_REG_ECX)==NODE
            self.ret()
            return
        super().instruction(mu,address,size,data)

    def registers(self,precision):
        """분리구간에도이전호출의일반/x87상태를남기지않는다."""
        for register,value in ((UC_X86_REG_EAX,0),(UC_X86_REG_EBX,NODE),(UC_X86_REG_ECX,0),
            (UC_X86_REG_EDX,0),(UC_X86_REG_ESI,STACK+0x5c),(UC_X86_REG_EDI,0),
            (UC_X86_REG_EBP,0),(UC_X86_REG_ESP,STACK),(UC_X86_REG_EFLAGS,2),
            (UC_X86_REG_FPCW,precision),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
            self.mu.reg_write(register,value)  # 명시실행입력이다.

    def sample(self,case,precision):
        """정규식캡처문자열을이미얻은상태에서실제resolve/저장/소멸효과만관측한다."""
        command,argument,zero=case
        self.trace,self.events,self.allocations=[],[],0
        self.reset_ring()
        self.mu.mem_write(NODE,bytes(0x21e8))
        self.mu.mem_write(STACK-0x4000,bytes(0x5000))
        self.mu.mem_write(STACK+0x10c,command+b'\0')
        self.mu.mem_write(STACK+0x15c,argument+b'\0')
        self.put(0,0xffffffff)
        self.put(0x55a508,0)
        self.put(0x55a50c,0)
        self.put(0x5428ac,0 if zero else 0x0200006d)
        self.put(0x5cab2c,1)
        self.registers(precision)
        self.mu.emu_start(RESOLVE,RESOLVED+1,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==RESOLVED and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        self.mu.emu_start(ASSIGN,ASSIGNED+1,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==ASSIGNED and self.mu.reg_read(UC_X86_REG_ESP)==STACK
        code,parameter=self.words(NODE+0x21e0,2)
        ring=self.words(0x5cab28,1)[0]
        # 실제전체소멸의thiscall정상반환·비휘발레지스터·SEH를독립확인한다.
        saved={UC_X86_REG_EBX:0x12345678,UC_X86_REG_ESI:0x23456789,UC_X86_REG_EDI:0x3456789a,UC_X86_REG_EBP:0x456789ab}
        for register,value in saved.items():
            self.mu.reg_write(register,value)  # 원문소멸이보존해야하는입력이다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_ECX,NODE)
        self.put(STACK,STOP)
        self.mu.emu_start(DESTROY,STOP,count=200000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        assert self.words(0,1)==(0xffffffff,) and self.mu.reg_read(UC_X86_REG_FPCW)==precision
        assert not self.mu.reg_read(UC_X86_REG_FPSW)&0x3800
        assert self.words(0x5cab2c,1)==(0,)
        self.normal+=1
        return str(code),str(parameter),str(ring),str(self.words(0x55a50c,1)[0]),';'.join(self.trace)


def main():
    """독립원문생성또는기존해시확인만수행하며게임/창/빌드는실행하지않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():
            assert digest(ROOT/path)==wanted,path  # 기대값을재계산하지않는파일바이트검사다.
        assert report['rows']==len(inputs())
        print('OnExit 원문 SHA/입력 감사',report['rows'])
        return
    oracle,rows=Oracle(),[]
    for case in inputs():  # 명시입력마다두x87정밀도의실제명령을독립실행한다.
        first=oracle.sample(case,0x27f)
        assert first==oracle.sample(case,0x37f),case
        rows.append((case[0].hex() or '-',case[1].hex() or '-',str(case[2]),*first))
    FIXTURE.write_text('# command argument configCodeZero code parameter ring queueCount effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c',
        'extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_script_command.py',
        'extracted/script-command/originals/functions.tsv','tools/decomp_dialog_exit_command.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastDecompileHost='VM-W11-Codex',lastDecompileDate='2026-10-23',
        rows=len(rows),normalDestructorReturns=oracle.normal,nativeInstructions=oracle.instructions,
        precisionDifferences=0,sha256={path:digest(ROOT/path) for path in paths},limits=[
            'c83b0의regex캡처후resolve4c85d2..4c863c와OnExit저장4c8a50..4c8a64만실제실행한다. 전체regex/메타파서를실행한것이아니다.',
            '명령표d52c0/C로케일isdigit/atol/80칸문자열고리/40칸입력큐및HTML부모소멸cf1c0전체는실제명령이다.',
            '고리할당·반납/TLS/진단/직계자식조회/벽시각/기본Gump소멸은명시경계다. 유효한한번의부모소멸만실행한다.',
            '원본보호파일변경/게임/GUI/OS진입/새디컴파일/빌드를수행하지않는다. 현재페이지실제수명연결은후속단위다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('OnExit 실제명령 관측',len(rows),oracle.normal,oracle.instructions)


if __name__=='__main__':
    main()
