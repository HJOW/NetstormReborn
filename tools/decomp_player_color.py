#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본0043ca70/0040eb40/004421a0의 색 표와 전체 풀 순회·프레임 요청을 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, TABLE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

# 실제 풀 경계와 명시 입력16슬롯 및 독립 관찰 결과의 경로다.
CAPACITY = 24000
FIXTURE = ROOT/'cpppj/tests/fixtures/player-color-x86.tsv'
REPORT = ROOT/'cpppj/player-color.json'


class Oracle(BaseOracle):
    """색 계산·iterator는 실제 명령이며 큰 Frame 효과만 인자/쓰기를 명시 관찰한다."""
    def __init__(self):
        """선택한 세 함수만 허용하며 실제 풀과 표/스택 쓰기를 구분한다."""
        super().__init__();self.allowed=[];self.events=[]  # 검토한 명령 범위와 현재 호출 기록이다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 현재 PC의 전체 정밀 범위만 사용한다.
                if int(row['entry'],16) not in {0x43ca70,0x40eb40,0x4421a0}:continue
                for part in row['ranges'].split(';'):  # 불연속 함수 범위도 포함한다.
                    low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.put(0x5c8464,TABLE);self.put(0x5c847c,CAPACITY);self.put(0x5411d0,94);self.put(0x5411d4,95)
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self, mu, access, address, size, value, data):
        """원본 본문의 색9DWORD/현재 스택 밖 쓰기는 허용하지 않는다."""
        if not (0x531b08<=address and address+size<=0x531b08+36 or STACK-4096<=address and address+size<=STACK+128):raise RuntimeError(f'색 변경 허용 밖 쓰기 {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """Frame 호출은 원래 thiscall로 기록하고 그 DWORD만 공급하는 명시 외부 경계다."""
        if address==0x4acee0:
            esp=mu.reg_read(UC_X86_REG_ESP);frame,flags=self.words(esp+4,2);pointer=mu.reg_read(UC_X86_REG_ECX);sid=(pointer-TABLE)//50
            assert pointer==TABLE+sid*50 and 5<=sid<=20
            self.events.append(f'{sid},{struct.unpack("<i",struct.pack("<I",frame))[0]},{flags}');self.put(pointer+36,frame);self.ret(purge=8);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'색 변경 미검토 명령 {address:x}')

    def run(self, values):
        """전투/소유자/색/슬롯 상태 입력 뒤 표 전체·16슬롯·호출 순서를 읽는다."""
        owner,color,battle,layout=values;self.events=[];data=bytearray(CAPACITY*50)  # 다른 풀 슬롯은 free이며 관찰 슬롯과 구별한다.
        for sid in range(2,CAPACITY):data[sid*50+11]=1  # 원본 iterator가 풀 끝까지 실제 free 비트를 읽게 한다.
        for sid in range(5,21):  # C++ 검사와 동일한 명시 입력이며 기대 프레임은 계산하지 않는다.
            raw=bytearray((sid*13+byte*29+layout*17)&255 for byte in range(50));raw[10]=(94,95,82,96)[(sid+layout)%4];raw[11]=(0,1,2,4,128,34)[(sid+layout)%6];raw[34]=(sid*3+layout)%9;data[sid*50:(sid+1)*50]=raw
        self.mu.mem_write(TABLE,bytes(data));self.mu.mem_write(0x531b08,struct.pack('<9I',*range(9)));self.put(0x594fbc,battle)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # ABI 보존을 확인하는 서로 다른 입력이다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<III',STOP,owner,color));self.mu.emu_start(0x43ca70,STOP,count=1000000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # cdecl 비휘발 보존이다.
        assert self.mu.mem_read(TABLE,250)==data[:250] and self.mu.mem_read(TABLE+1050,len(data)-1050)==data[1050:]
        return [','.join(map(str,values)),bytes(self.mu.mem_read(0x531b08,36)).hex(),bytes(self.mu.mem_read(TABLE+250,800)).hex(),';'.join(self.events) or '-']


def main():
    """216고유 입력을 관찰하거나 현재 원본/제품/기대값 바이트를 감사한다."""
    if sys.argv[1:]==['--verify']:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 기록된 원본·도구·제품을 함께 확인한다.
        rows=list(csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({row[0] for row in rows});print('Player color native/SHA:',len(rows));return
    if sys.argv[1:]:raise SystemExit('decomp_player_color.py [--verify]')
    oracle=Oracle();rows=[oracle.run([owner,color,battle,layout]) for owner in range(9) for color in (0,1,8,0xffffffff) for battle in (0,1) for layout in range(3)]
    assert any(row[-1]!='-' for row in rows)
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','colors','objects','events']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_player_color.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawPlayerColor.h','cpppj/tests/PlayerColorTests.cpp','cpppj/tests/fixtures/player-color-x86.tsv']
    facts={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'boundary':'원본 색 변경/전체 iterator/색 프레임 계산을 실행한다. 큰004acee0은 순서/인자/Frame DWORD 쓰기의 명시 외부 대체다. 나머지 풀/표/ABI와 허용 밖 쓰기를 확인한다. 렌더링/원본 게임 실행은 아니다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Player color native:',len(rows))


if __name__=='__main__':main()
