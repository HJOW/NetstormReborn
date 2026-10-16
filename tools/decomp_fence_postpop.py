#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""썬 펜스00457560의 전체 접두/호출/쓰기만 실제10.78 명령으로 격리 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, HEAP, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

# 보호 원본/게임 OS와 겹치지 않는 다섯 객체의 관찰 입력이다.
POOL, IDS = PLAYERS, tuple(range(50,55))
FIXTURE = ROOT/'cpppj/tests/fixtures/fence-postpop-x86.tsv'
REPORT = ROOT/'cpppj/fence-postpop.json'


class Oracle(BaseOracle):
    """실제 부모 접두를 실행하고 이웃/선/연결 수/힙/예약/Damageable은 명시 외부 입력으로 둔다."""
    def __init__(self, control=0x027f):
        """동일 PC 정밀 함수의 불연속 범위와 허용 쓰기를 한정한다."""
        super().__init__()
        self.control=control  # 원문 x87의 두 정밀도 재생을 구분한다.
        self.allowed=[]  # 검토한 부모의 반개방 실제 명령 범위다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==0x457560)
        for part in row['ranges'].split(';'):  # 원문 함수가 가진 모든 불연속 범위를 포함한다.
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self, mu, access, address, size, value, data):
        """SEH/스택과 다섯 객체 외의 실제 명령 쓰기를 거부한다."""
        ranges=[(0,4),(STACK-4096,STACK+128)]+[(POOL+sid*50,POOL+(sid+1)*50) for sid in IDS]
        if not any(low<=address and address+size<=high for low,high in ranges):
            raise RuntimeError(f'펜스 후처리 선택 밖 쓰기 {address:x}/{size}')

    def instruction(self, mu, address, size, data):
        """각 하위 호출의 인자와 외부 상태 변경만 공급하고 부모의 판단은 원문에 맡긴다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,9);ecx=mu.reg_read(UC_X86_REG_ECX)
        if address==0x4ac970:
            self.events.append('Theme,50')
            if self.mutation==1:self.put(POOL+50*50+34,7,1)
            self.ret(self.theme);return
        if address==0x456220:
            direction=args[0];self.events.append(f'Neighbor,50,{direction}')
            if self.mutation==2 and direction==0:self.put(POOL+50*50+34,6,1)
            self.ret(51+direction//2 if self.neighbors&(1<<(direction//2)) else 0,purge=4);return
        if address==0x4e4391:
            assert args[0]==40;self.events.append('Reserve,40');success=(self.mask>>self.allocations)&1;self.allocations+=1
            if self.mutation==3 and self.allocations==1:self.put(POOL+51*50+14,0x42f00000)
            self.ret(HEAP+self.allocations*64 if success else 0);return
        if address==0x496e80:
            self.events.append(f'Regular,{args[0]},{args[1]},{args[2]}');self.ret(purge=12);return
        if address==0x455b20:
            assert args[4:]==(0,0,0,0,0);self.events.append('Clear,'+','.join(map(str,args[:4])));self.lines+=1
            if self.mutation==4 and self.lines==1:self.put(POOL+52*50+18,0x42fa0000);self.put(POOL+50*50+34,5,1)
            self.ret();return
        if address==0x4573d0:
            self.events.append('Connections,50')
            if self.mutation==5:self.put(POOL+50*50+46,77)
            self.ret(self.count);return
        if address==0x44bf80:
            self.events.append('Base,'+str(args[0]))
            if self.mutation==6:self.put(POOL+50*50+12,0x7777,2)
            self.ret(purge=4);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'펜스 후처리 미검토 명령 {address:x}')

    def run(self, case):
        """부모/네 이웃의 전체250BYTE와 호출 순서를 원문 정상 반환 뒤 읽는다."""
        flags,self.theme,self.neighbors,self.mask,self.count,self.mutation=case
        self.events=[];self.allocations=0;self.lines=0  # 원문이 실제 선택한 호출의 현재 장부다.
        self.put(0,0x12345678);self.put(0x5c8464,POOL)
        for sid in IDS:  # 서로 다른 좌표와 보존 패턴으로 현재 객체 입력을 구별한다.
            raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x509470);raw[10]=136;raw[11]=0;raw[34]=1;raw[40]=0
            struct.pack_into('<ff',raw,14,sid+.25,sid+.5);self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.mu.reg_write(UC_X86_REG_FPCW,self.control)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 비휘발 레지스터의 입력을 구별한다.
            self.mu.reg_write(reg,0x12340000+reg)
        self.mu.reg_write(UC_X86_REG_ECX,POOL+50*50);self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK,struct.pack('<II',STOP,flags));self.mu.emu_start(0x457560,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8 and self.words(0)[0]==0x12345678
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 정상 반환 시 비휘발 보존을 전수 확인한다.
            assert self.mu.reg_read(reg)==0x12340000+reg
        return [','.join(map(str,case)),b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in IDS).hex(),';'.join(self.events)]


def cases():
    """최초 플래그·원소·네 방향/두 쌍·개별 할당 실패·현재 상태 변경을 교차한다."""
    for flags in (1,3):  # 최초 조건과 다른 동시 비트의 영향을 구별한다.
        for theme in (0,3):  # 원소3만 사건0을 예약한다.
            for neighbors in range(16):  # 각 방향 유무를 독립 입력으로 둔다.
                for mask in (0,1,21,63):yield flags,theme,neighbors,mask,0xdeadbeef,0  # 뒤 할당의 선택도 앞 실패와 독립이다.
    for mask in range(64):yield 1,3,15,mask,4,0  # 여섯 개별 할당의 모든 성공/실패 조합이다.
    for flags in (0,2,4,0x80000000):yield flags,3,15,63,4,0  # 최초 접두를 선택하지 않는 입력이다.
    for mutation in range(1,7):yield 1,3,15,63,0xdeadbeef,mutation  # 각 선택 효과 뒤의 현재 슬롯을 다시 읽는다.


def main():
    """전체 관찰/두 정밀도 재생 또는 기존 SHA와 고유 입력 감사를 수행한다."""
    if sys.argv[1:]==['--verify']:
        facts=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in facts['sha256'].items():assert digest(ROOT/name)==expected,name  # 제품/관찰기/보호 입력의 바이트를 함께 감사한다.
        rows=list(csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t'))[1:]
        assert len(rows)==facts['observations']==len({row[0] for row in rows});print('Fence PostPop native/SHA:',len(rows));return
    if sys.argv[1:]:raise SystemExit('decomp_fence_postpop.py [--verify]')
    oracle=Oracle();rows=[oracle.run(case) for case in cases()]
    assert len(rows)==len({row[0] for row in rows})
    alternate=Oracle(0x037f);assert rows==[alternate.run(case) for case in cases()]  # 같은 고유 입력을 두 번으로 합산하지 않는다.
    with FIXTURE.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','objects','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_fence_postpop.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawFencePostPop.h','cpppj/src/o/RawFencePostPop.cpp','cpppj/tests/FencePostPopTests.cpp','cpppj/tests/fixtures/fence-postpop-x86.tsv']
    facts={'host':socket.gethostname(),'date':'2026-10-16','last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-16','observations':len(rows),'precision_replays':[0x027f,0x037f],'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'boundary':'원본00457560만 실행한다. TYPE원소/이웃/연결선삭제/연결수/힙/Regular/Damageable은 명시 외부 입력이다. 전체250BYTE/호출/SEH/반환/스택/비휘발/허용쓰기를 대조하며 게임 자연 플레이 관찰은 아니다.','sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('Fence PostPop native:',len(rows))


if __name__=='__main__':main()
