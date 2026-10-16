#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PATCH00467610의 실제 Gunanim 표/경계/정수 나눗셈 반환을 격리 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

# 원본 반환값과 현재 관찰기/제품의 바이트 근거다.
FIXTURE = ROOT/'cpppj/tests/fixtures/gun-angle-x86.tsv'
REPORT = ROOT/'cpppj/gun-angle.json'


class Oracle(BaseOracle):
    """원본 함수/실제20개float만 읽으며 OS나 큰 외부 함수를 대신하지 않는다."""
    def __init__(self, control=0x027f):
        """현재 PC의 정밀 함수 범위와 스택 쓰기만 허용한다."""
        super().__init__();self.control=control;self.allowed=[]  # 명시 x87 정밀도와 검토 범위다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x467610)
        for part in row['ranges'].split(';'):  # 원본의 불연속 범위를 모두 포함한다.
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.put(0x5e4794,0);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self, mu, access, address, size, value, data):
        """현재 스택 밖 전역/각도 표 쓰기를 거부한다."""
        if not STACK-4096<=address or address+size>STACK+128:raise RuntimeError(f'Gunanim 허용 밖 쓰기 {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """진단/미검토 함수의 실행을 정상 입력으로 가장하지 않는다."""
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'Gunanim 미검토 명령 {address:x}')

    def run(self, frame):
        """기대 방향을 계산하지 않고 실제 EAX/스택/비휘발 보존을 읽는다."""
        self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 서로 다른 초기 레지스터 값이다.
            self.mu.reg_write(reg,0x12340000+reg)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<II',STOP,frame&0xffffffff));self.mu.emu_start(0x467610,STOP,count=1000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # cdecl 비휘발을 확인한다.
        return [str(frame),str(self.mu.reg_read(UC_X86_REG_EAX))]


def main():
    """실제 원문518입력/두 정밀도를 관찰하거나 보존 SHA를 감사한다."""
    if sys.argv[1:]==['--verify']:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 원본/도구/제품/fixture를 함께 감사한다.
        rows=list(csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({row[0] for row in rows});print('Gun angle native/SHA:',len(rows));return
    if sys.argv[1:]:raise SystemExit('decomp_gun_angle.py [--verify]')
    frames=list(range(-4,513))+[2147483647];oracle=Oracle();rows=[oracle.run(frame) for frame in frames];alternate=Oracle(0x037f);assert rows==[alternate.run(frame) for frame in frames]
    assert {row[1] for row in rows}=={'0','1','2','3'}
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['frame','graph']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_gun_angle.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawGunAngle.h','cpppj/tests/GunAngleTests.cpp','cpppj/tests/fixtures/gun-angle-x86.tsv']
    facts={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'precision_replays':[0x027f,0x037f],'game_gui_runs':0,'status':'native-observed-cpp-validation-pending','boundary':'원본00467610과 실제float 표/진단꺼짐을 사용한다. 외부 함수 대체는 없으며 음수 인덱스의 무효 메모리는 범위 밖이다. 반환/스택/비휘발/스택 밖 쓰기를 확인한다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Gun angle native:',len(rows))


if __name__=='__main__':main()
