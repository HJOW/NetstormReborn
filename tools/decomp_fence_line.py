#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""00455b20(mode0)의 실제 좌표 보정/포함 끝점/100회 상한을 격리 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

# 검색 후보가 없는 입력의 독립 좌표 관찰과 보고서다.
FIXTURE = ROOT/'cpppj/tests/fixtures/fence-line-x86.tsv'
REPORT = ROOT/'cpppj/fence-line.json'


class Oracle(BaseOracle):
    """좌표/CRT는 원문이며 실제 finder의 후보 없음만 명시 외부 입력이다."""
    def __init__(self, control=0x027f):
        """세 원문 함수의 실제 범위와 스택 쓰기만 허용한다."""
        super().__init__();self.control=control;self.allowed=[]  # 명시 정밀도와 반개방 검토 범위다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 지정한 함수 밖의 실행을 거부한다.
                if int(row['entry'],16) in (0x455b20,0x41d770,0x4e49c0):
                    for part in row['ranges'].split(';'):  # 불연속 몸체도 모두 포함한다.
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self, mu, access, address, size, value, data):
        """선택한 좌표 관찰은 스택의 현재float/검색 커서만 쓸 수 있다."""
        if not STACK-4096<=address or address+size>STACK+256:raise RuntimeError(f'펜스 선 좌표 범위 밖 쓰기 {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """원문이 요청한 검색점을 기록하고 후보0을 실제 커서에 설치한다."""
        if address==0x4b16d0:
            esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4);assert args[0]==args[2] and args[1]==args[3]
            self.points.append(','.join(str(value if value<0x80000000 else value-0x100000000) for value in args[:2]))
            self.put(mu.reg_read(UC_X86_REG_ECX)+0x34,0);self.ret(purge=16);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'펜스 선 미검토 명령 {address:x}')

    def run(self, case):
        """네float 인자를 원문에 공급하고 cdecl 정상 반환과 검색 순서를 읽는다."""
        self.points=[];self.put(0x5c8464,PLAYERS);self.put(0x5c847c,128)
        self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 비휘발의 입력을 각각 구별한다.
            self.mu.reg_write(reg,0x12340000+reg)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<10I',STOP,*case,0,0,0,0,0));self.mu.emu_start(0x455b20,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # cdecl 비휘발 보존이다.
        return [','.join(map(str,case)),';'.join(self.points)]


def cases():
    """양 축/양 방향/소수/짧은 길이/보드 끝/100회 상한을 교차한다."""
    seen=set()  # 길이0의 동일 네float 입력을 중복 집계하지 않는다.
    for x,y in ((50.25,50.5),(2,2),(254.75,254.5)):  # 중앙과 두 보드 끝의 입력이다.
        for distance in (-105,-101,-8.5,-4,-3,-2,-1,-.5,0,.5,1,2,3,4,8.5,101,105):  # 보정값1/3의 앞뒤와100회 이상 길이를 구별한다.
            for axis in (0,1):  # 수직과 수평의 서로 다른 보정을 선택한다.
                points=(x,y,x if axis==0 else x+distance,y+distance if axis==0 else y);case=tuple(struct.unpack('<I',struct.pack('<f',value))[0] for value in points)
                if case not in seen:seen.add(case);yield case


def main():
    """원본 관찰 또는 현재 SHA/고유 입력 감사를 수행한다."""
    if sys.argv[1:]==['--verify']:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 관찰기/제품/입력 바이트를 함께 감사한다.
        rows=list(csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({row[0] for row in rows});print('Fence line native/SHA:',len(rows));return
    if sys.argv[1:]:raise SystemExit('decomp_fence_line.py [--verify]')
    oracle=Oracle();rows=[oracle.run(case) for case in cases()];alternate=Oracle(0x037f);assert rows==[alternate.run(case) for case in cases()]
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','points']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_fence_line.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawFenceLine.h','cpppj/src/o/RawFenceLine.cpp','cpppj/tests/FenceLineTests.cpp','cpppj/tests/fixtures/fence-line-x86.tsv']
    facts={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'precision_replays':[0x027f,0x037f],'game_gui_runs':0,'status':'native-observed-cpp-validation-pending','boundary':'00455b20의 mode0과 좌표 CRT를 실행한다. finder 후보0은 명시 외부 입력이며 후보 필터/실제 가상 삭제/mode1 생성의 관찰은 아니다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Fence line native:',len(rows))


if __name__=='__main__':main()
