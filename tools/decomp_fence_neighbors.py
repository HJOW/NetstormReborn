#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""펜스의 최근접 선택/검색 경계/축 사거리를 원본10.78 명령으로 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, PLAYERS, TABLE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

# 동일 보호 원본과 분리한 다섯 입력 객체다. finder 후보 순서만 명시 외부 입력이다.
POOL, IDS = PLAYERS, tuple(range(50,55))
FIXTURE = ROOT/'cpppj/tests/fixtures/fence-neighbors-x86.tsv'
REPORT = ROOT/'cpppj/fence-neighbors.json'


def bits(value):
    """입력float의 binary32 비트를 보존한다."""
    return struct.unpack('<I',struct.pack('<f',value))[0]


class Oracle(BaseOracle):
    """선택/타입/중심/거리/Rect/CRT는 원문이고 finder의 후보 나열만 외부 경계다."""
    def __init__(self, control=0x027f):
        """검토한 열세 함수의 실제 불연속 범위와 읽기 전용 입력을 준비한다."""
        super().__init__();self.control=control;self.allowed=[]  # 정밀도와 검토한 반개방 범위다.
        selected={0x455fb0,0x456220,0x455f00,0x4ac550,0x49a840,0x4adea0,0x49a900,0x41cfc0,0x40e880,0x40e8f0,0x41d0d0,0x40e800,0x4e49c0}
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):  # 지정한 함수 밖의 명령을 허용하지 않는다.
                if int(row['entry'],16) in selected:
                    for part in row['ranges'].split(';'):  # 각 함수의 불연속 범위도 포함한다.
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self, mu, access, address, size, value, data):
        """타입/동맹/객체 쓰기를 금지하고 현재 스택의 커서만 변경한다."""
        if not STACK-4096<=address or address+size>STACK+256:raise RuntimeError(f'펜스 이웃 읽기 전용 위반 {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """실제 finder에 넘긴 float Rect를 기록하고 준비된 후보만 순서대로 반환한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x41def0:
            args=self.words(esp+4,5);assert args[4]==1;self.bounds=','.join(map(str,args[:4]));self.cursor=ecx;self.index=0
            self.put(ecx+0x34,self.candidates[0]);self.ret(ecx,purge=20);return
        if address==0x4b1810:
            assert ecx==self.cursor;self.index+=1;self.put(ecx+0x34,self.candidates[self.index] if self.index<len(self.candidates) else 0);self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'펜스 이웃 미검토 명령 {address:x}')

    def run(self, case):
        """기대 후보와 거리 판단을 계산하지 않고 실제 반환/검색 경계를 기록한다."""
        kind,direction,allied,range_bits,scenario,order,axis,distance_bits=case
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59ab20,TABLE)
        self.mu.mem_write(0x595200,bytes(81*4));relation=1 if scenario==9 else 2 if scenario==10 else 0
        if relation:self.put(0x595200+(2*9+1 if relation==1 else 1*9+2)*4,1)
        foot=(3,5) if scenario==14 else (1,1)
        for number in (135,136):  # 같은 타입의 실제 중심과 사거리 입력을 준비한다.
            self.put(TABLE+number*500+0x44,range_bits);self.put(TABLE+number*500+0x1d4,foot[0]);self.put(TABLE+number*500+0x1d8,foot[1])
        positions=[(50.25,50.5),(50.25,48.5),(52.25,50.5),(50.25,52.5),(48.25,50.5)]
        if scenario==6:positions[1]=(50.75,48.5)
        if scenario==11:positions[1]=positions[0]
        if scenario==12:positions[1]=(50.25,47.5)
        if scenario==15:positions[1]=(0,50.5)
        for index,sid in enumerate(IDS):  # 전체 입력은 기대값 계산 없이 fixture에 그대로 보존한다.
            raw=bytearray(50);struct.pack_into('<I',raw,0,0x509470);raw[10]=135 if sid==51 and scenario==5 else 136
            raw[11]=(2 if scenario==1 else 1 if scenario==13 else 0) if sid==51 else 0
            raw[34]=2 if sid==51 and scenario in (8,9,10) else 1;raw[40]=({2:1,3:8,4:32}.get(scenario,0)) if sid==51 else 0
            struct.pack_into('<ff',raw,14,*positions[index]);self.mu.mem_write(POOL+sid*50,bytes(raw))
        objects=b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex()
        self.candidates=[51,52,53,54,50] if order==0 else [50,54,53,52,51];self.bounds='-';self.index=0
        entry=0x455fb0 if allied else 0x456220;args=[direction];purge=4;endpoints='-'
        if kind=='R':
            distance=struct.unpack('<f',struct.pack('<I',distance_bits))[0];coords=(50.25,50.5,50.25 if axis==0 else 50.25+distance,50.5+distance)
            args=[bits(value) for value in coords];endpoints=','.join(map(str,args));entry=0x455f00;purge=16
        self.mu.reg_write(UC_X86_REG_FPCW,self.control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 각 비휘발 입력을 서로 다르게 지정한다.
            self.mu.reg_write(reg,0x12340000+reg)
        self.mu.reg_write(UC_X86_REG_ECX,POOL+50*50);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));self.mu.emu_start(entry,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 정상 ABI의 보존을 확인한다.
        result=self.mu.reg_read(UC_X86_REG_EAX);result=result&255 if kind=='R' else result
        assert objects==b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex()
        return [','.join(map(str,case)),objects,','.join(map(str,self.candidates)),str(foot[0]),str(foot[1]),str(relation),endpoints,str(result),self.bounds]


def cases():
    """동률·dead/free·abstract/extra20·타입/정렬·역방향 동맹과 절삭 경계를 교차한다."""
    for direction in (0,2,4,6):  # 네 실제 방향의 Rect를 모두 관찰한다.
        for allied in (0,1):  # 동맹 검사 없는 선택과 선택 뒤 검사하는 몸체를 구별한다.
            for range_value in (0,0.5,1,2,2.75,8.5,-1.5):  # CRT 절삭과 최근접 cutoff를 구별한다.
                for scenario in range(16):  # 각 후보 필터와 현재 TYPE 발자국을 독립 입력으로 바꾼다.
                    for order in (0,1):yield 'N',direction,allied,bits(range_value),scenario,order,0,0  # 같은 거리의 첫 후보를 검사한다.
    for range_value in (0,0.5,1,2,2.75,8.5,-1.5):  # 사거리 비교는 원래float와 절삭 축 거리를 사용한다.
        for distance in (0,0.75,1,1.99,2,2.75,8.99,-2.75):  # 양수/음수 거리와 포함 경계다.
            for axis in (0,1):yield 'R',0,0,bits(range_value),0,0,axis,bits(distance)  # 수직/그 외 분기를 구별한다.


def main():
    """원본 격리 관찰 또는 현재 파일 SHA/고유 입력을 감사한다."""
    if sys.argv[1:]==['--verify']:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 입력/제품/관찰기를 함께 감사한다.
        rows=list(csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({row[0] for row in rows});print('Fence neighbors native/SHA:',len(rows));return
    if sys.argv[1:]:raise SystemExit('decomp_fence_neighbors.py [--verify]')
    oracle=Oracle();rows=[oracle.run(case) for case in cases()];assert len(rows)==len({row[0] for row in rows})
    assert any(row[7]!='0' for row in rows if row[0].startswith('N,'))  # 후보가 있는 양성 입력이 실제 원문 선택에 도달했는지 확인한다.
    alternate=Oracle(0x037f);assert rows==[alternate.run(case) for case in cases()]  # 두 재생은 같은 고유 입력이다.
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','objects','candidates','footX','footY','relation','endpoints','result','bounds']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_fence_neighbors.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawFenceNeighbors.h','cpppj/src/o/RawFenceNeighbors.cpp','cpppj/tests/FenceNeighborsTests.cpp','cpppj/tests/fixtures/fence-neighbors-x86.tsv']
    facts={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'precision_replays':[0x027f,0x037f],'game_gui_runs':0,'status':'native-observed-cpp-validation-pending','boundary':'최근접/동맹/타입/중심/거리/Rect/CRT와 축 사거리는 실제 명령이다. finder의 후보 순서만 외부 입력이며 실제 전체 해시 검색의 새 관찰이 아니다. 읽기 전용 전체250BYTE/반환/스택/비휘발/쓰기guard를 확인한다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Fence neighbors native:',len(rows))


if __name__=='__main__':main()
