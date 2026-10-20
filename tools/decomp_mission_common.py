#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""미션 demo 덮어쓰기와 재시작 캡처의 실제 내부 분기·CRT를 제품 작성 전에 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_cursor_lifecycle import Oracle as BaseOracle, ROOT, POOL, STACK, STOP, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP

MISSION, GUIDE, TEXT, THREAD = POOL, POOL+0x1000, POOL+0x3000, POOL+0x4000  # 격리 미션/Guide/문자열/TLS다.
FIXTURE=ROOT/'cpppj/tests/fixtures/mission-common-x86.tsv'  # 제품 전 동결하는 선택 필드/명령 관측이다.
REPORT=ROOT/'cpppj/mission-common-native.json'  # 함수 반환과 구별한 내부 분기 근거다.


def cases():
    """빈 demo/숫자 감김/공백과 현재 재시작 필드의 비제로 정규화를 교차한다."""
    rows=[]
    # 원본 Init의 demo 문자열이 비어 있을 때 기존 DWORD가 남는지 구별한다.
    for old, text in itertools.product((0, 1, 7, 0xffffffff), ('','0','1','-1','  +2tail','garbage','4294967296','2147483648','\t-4294967297','+','  ')):
        rows.append(('I',old,0,text))
    # 재시작은 이름을 먼저 복사하고 두 현재 필드를 bool DWORD로 정규화한다.
    for mode, demo, name in itertools.product((0, 1, 7, 0xffffffff), (0, 1, 3, 0xffffffff), ('TheWarBegins','DemoStormRages','GuardMyBack','DissolvedAlliance')):
        rows.append(('R',mode,demo,name))
    return rows


class Oracle(BaseOracle):
    """Init/Reload의 선택된 실제 명령과 원본 C locale atol만 실행한다."""
    def __init__(self):
        """전체 게임 대신 격리 PE/스택과 기존 원본 C locale만 준비한다."""
        super().__init__();self.allowed=[];ranges(self,(0x4e6656,))
        self.allowed.extend(((0x4c32f6,0x4c3307),(0x463cf0,0x463d3b),(0x4e66de,0x4e66e3)))
        self.put(THREAD+0x64,self.get(0x543efc));self.instructions=0;self.atolCalls=0

    def instruction(self, mu, address, size, data):
        """TLS만 명시 공급하고 두 분기의 도착점을 함수 정상 반환으로 세지 않는다."""
        self.instructions+=1
        if address==0x4ea61d:self.ret(THREAD);return
        if address==0x4e6656:self.atolCalls+=1
        if address==(0x4c3307 if self.operation=='I' else 0x463d3b):self.reached=True;mu.emu_stop();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미검토 명령 {address:08x}')

    def write(self, mu, access, address, size, value, data):
        """실제 명령은 demo DWORD와 한정 스택 이외에 쓸 수 없다."""
        if STACK-0x1000<=address and address+size<=STACK+0x100:return
        if MISSION+0x68<=address and address+size<=MISSION+0x6c:return
        raise RuntimeError(f'공통 미션 분기 쓰기 범위 밖 {address:08x}/{size}')

    def sample(self, case):
        """원본 도착점의 demo 또는 EBX/EDI/복사 이름을 그대로 수집한다."""
        self.operation, mode, demo, text=case;self.reached=False
        self.mu.mem_write(STACK-0x1000,b'\xa5'*0x1100);self.put(MISSION+0x64,mode);self.put(MISSION+0x68,mode if self.operation=='I' else demo)
        self.mu.mem_write(TEXT,text.encode('ascii')+b'\0');self.mu.mem_write(GUIDE,text.encode('ascii')+b'\0');self.put(GUIDE+0x800,MISSION)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_EAX,TEXT);self.mu.reg_write(UC_X86_REG_ESI,MISSION);self.mu.reg_write(UC_X86_REG_ECX,GUIDE)
        self.mu.emu_start(0x4c32f6 if self.operation=='I' else 0x463cf0,STOP,count=10000)
        if not self.reached:raise RuntimeError('공통 미션 내부 도착점 미도달')
        if self.operation=='I':
            if self.mu.reg_read(UC_X86_REG_ESP)!=STACK:raise RuntimeError('demo 분기 스택 변화')
            return str(self.get(MISSION+0x68))
        # 원본 sub100과push3개 뒤 지역 복사 시작은 현재 ESP+0xc다.
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        if sp!=STACK-0x10c:raise RuntimeError('Reload 캡처 스택 변화')
        copied=bytes(self.mu.mem_read(sp+12,80)).split(b'\0',1)[0]
        return ','.join((str(self.mu.reg_read(UC_X86_REG_EBX)),str(self.mu.reg_read(UC_X86_REG_EDI)),copied.hex()))


def main():
    """원문을 최초 한 번 보존하거나 현재 해시만 감사하며 C++/게임은 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        for name,expected in record['sha256'].items():
            if digest(ROOT/name)!=expected:raise RuntimeError('보호 해시 불일치: '+name)
        print('공통 미션 최초 내부 분기/CRT 관측 해시 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=cases();lines=['# 함수 반환이 아닌 실제 Init demo/Reload 캡처 내부 분기의 독립 관측이다.']
    # 입력/출력은 제품이 없을 때 실제 PE의 현재 필드에서만 얻는다.
    for row in rows:lines.append('\t'.join((row[0],str(row[1]),str(row[2]),row[3].encode('ascii').hex(),oracle.sample(row))))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','uniqueInputs':len(rows),'normalFunctionReturns':0,'sliceObservations':len(rows),'instructions':oracle.instructions,'actualAtolCalls':oracle.atolCalls,
        'scope':'Init004c32f6..004c3307 demo 덮어쓰기와 Reload00463cf0..00463d3b의 필드/이름 캡처만. 실제 C locale atol004e66de/004e6656을 실행하며 TLS 포인터만 공급한다. 전체 Init/Reload·해제/공장/실제 UI 실행 근거가 아니다.',
        'gameExecuted':False,'osExecuted':False,'sha256':{str(path.relative_to(ROOT)).replace('\\','/'):digest(path) for path in (ROOT/'originals/Netstorm.exe',ROOT/'originalCD/NETSTORM.EXE',ROOT/'extracted/refined/originals/Netstorm.c',ROOT/'extracted/refined/originals/functions.tsv',ROOT/'tools/decomp_mission_common.py',FIXTURE)}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('uniqueInputs','sliceObservations','instructions','actualAtolCalls')}))


if __name__=='__main__':
    main()
