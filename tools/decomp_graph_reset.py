#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""월드 초기 Graph 표00462b20의 정확한 WORD/BYTE 쓰기와 보존 영역을 관찰한다."""
import csv
import json
import socket
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, TABLE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

# 실제255개 레코드·1530BYTE 표의 전체 입력과 원문 출력이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/graph-reset-x86.tsv'
REPORT = ROOT/'cpppj/graph-reset.json'


class Oracle(BaseOracle):
    """큰 풀/Graph 연결을 대체하지 않고 작은 초기화 함수 자체만 실행한다."""
    def __init__(self):
        """정밀 범위와 전체1530BYTE 표/스택 쓰기만 허용한다."""
        super().__init__();self.allowed=[]  # 현재 PC에서 검토한 초기화 명령 범위다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x462b20)
        for part in row['ranges'].split(';'):  # 불연속 범위도 모두 포함한다.
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.put(0x562c18,TABLE);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self, mu, access, address, size, value, data):
        """선택한 표와 호출 스택 외의 쓰기를 금지한다."""
        if not (TABLE<=address and address+size<=TABLE+1530 or STACK-4096<=address and address+size<=STACK+128):raise RuntimeError(f'Graph 초기 표 밖 쓰기 {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """초기화 함수 밖의 명령/OS 호출을 거부한다."""
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'Graph 초기화 미검토 명령 {address:x}')

    def run(self, seed):
        """보존해야 하는 reserved/gap/sentinel WORD도 서로 다른 바이트로 채운다."""
        self.mu.mem_write(TABLE,bytes((index*37+seed)&255 for index in range(1530)))
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 서로 다른 비휘발 입력이다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.put(STACK,STOP);self.mu.emu_start(0x462b20,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # cdecl 보존을 전수 확인한다.
        return [str(seed),bytes(self.mu.mem_read(TABLE,1530)).hex()]


def main():
    """원본 초기화16입력을 관찰하거나 현재 근거 SHA를 감사한다."""
    if sys.argv[1:]==['--verify']:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 입력/도구/제품 바이트를 함께 감사한다.
        rows=list(csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({row[0] for row in rows});print('Graph reset native/SHA:',len(rows));return
    if sys.argv[1:]:raise SystemExit('decomp_graph_reset.py [--verify]')
    oracle=Oracle();rows=[oracle.run(seed) for seed in range(16)]
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['seed','records']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_graph_reset.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawGraph.h','cpppj/src/o/RawGraph.cpp','cpppj/tests/GraphResetTests.cpp','cpppj/tests/fixtures/graph-reset-x86.tsv']
    facts={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'game_gui_runs':0,'status':'native-observed-cpp-validation-pending','boundary':'원본00462b20은 외부 대체 없이 표255행 전체를 관찰한다. 원문 반환/스택/비휘발과 표 밖 쓰기를 확인한다. 전체 Graph 연결/재구성이나 자연 플레이 관찰은 아니다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Graph reset native:',len(rows))


if __name__=='__main__':main()
