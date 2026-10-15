#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""buried00462170의 실제 QA/개별 할당/예약 순서를 OS 없이 격리 관찰한다."""
import csv
import json
import socket
import struct
import sys
from datetime import datetime
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, HEAP, PLAYERS, TABLE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP

# 게임/OS 주소와 분리한 실제 입력 Pool 및 관찰 대상 SID다.
POOL, SID = PLAYERS, 50


class Oracle(BaseOracle):
    """부모/QA helper/타입 주소계산은 실제 코드이며 추첨/힙/Regular/공통은 명시 외부 경계다."""
    def __init__(self):
        """정밀 함수 범위 네 개만 실행하고 실제 명령의 쓰기를 제한한다."""
        super().__init__()
        self.allowed=[]  # 검토한 원본 명령의 반개방 실행 범위 목록이다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 타입 조회는 주소만 계산하고 해당 반환 주소를 역참조하지 않는다.
        for address in (0x462170,0x4ac220,0x4acd10,0x49a840):  # 부모와 세 직접 helper만 선택한다.
            for part in functions[address]['ranges'].split(';'):  # 정밀 표의 불연속 범위를 모두 보존한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """SEH의 FS0/스택과 현재 QA 포함 한 객체 이외의 쓰기는 거부한다."""
        if not any(low<=address and address+size<=high for low,high in ((0,4),(POOL+SID*50,POOL+(SID+1)*50),(STACK-4096,STACK+128))):
            raise RuntimeError(f'buried 선택 밖 명령 쓰기 {address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """실제 호출 인자를 기록하고 각 할당의 독립 성공/효과 변경 입력을 공급한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4)
        if address==0x461f00:
            self.events.append('Choose,'+str(args[0]))
            if self.mutation==3:self.put(0x540bc4,0);self.put(0x5c89b8,1)
            self.ret(self.chosen);return
        if address==0x4e4391:
            self.events.append('Reserve,'+str(args[0]));success=(self.mask>>self.allocations)&1;self.allocations+=1
            if self.mutation==1 and self.allocations==1:self.put(POOL+SID*50+12,0xbeef,2)
            self.ret(HEAP+self.allocations*64 if success else 0);return
        if address==0x496e80:
            self.events.append('Regular,'+str(args[0])+','+str(args[1])+','+str(args[2]))
            if self.mutation==2 and args[0]==0:self.put(0x5c89b8,1)
            self.ret(purge=12);return
        if address==0x4b0d30:
            self.events.append('Base,'+str(args[0]))
            if self.mutation==4:self.put(POOL+SID*50+12,0x7777,2)
            self.ret(purge=4);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'buried 미검토 명령 {address:x}')

    def run(self,case):
        """정답을 만들지 않고 실제 원문이 쓴 전체50BYTE와 현재 전역/호출을 읽는다."""
        boss,loading,flags,self.mask,qa,self.chosen,self.mutation=case  # mask는 두 개별 할당, chosen은 추첨 반환, mutation은 효과 변경 입력이다.
        self.events=[];self.allocations=0  # 실제 선택 호출의 순서와 각 new의 현재 번호다.
        self.put(0,0x12345678);self.put(0x5c8464,POOL);self.put(0x59ab20,TABLE);self.put(0x5411a0,85)
        self.put(0x540bc4,boss);self.put(0x5c89b8,loading)
        raw=bytearray(b'\xa5'*50);struct.pack_into('<I',raw,0,0x50a748);raw[10]=112;raw[11]=0;raw[34]=1;raw[40]=0;struct.pack_into('<H',raw,12,qa)
        self.mu.mem_write(POOL+SID*50,bytes(raw))
        # 원문이 보존해야 하는 비휘발 레지스터의 입력을 서로 구별한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 호출 전 서로 다른 레지스터 입력이다.
        self.mu.reg_write(UC_X86_REG_ECX,POOL+SID*50);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<II',STOP,flags));self.mu.emu_start(0x462170,STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+8 and self.words(0)[0]==0x12345678
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # thiscall 비휘발 보존을 전수 확인한다.
        return [','.join(map(str,case)),bytes(self.mu.mem_read(POOL+SID*50,50)).hex(),str(self.words(0x540bc4)[0]),str(self.words(0x5c89b8)[0]),';'.join(self.events)]


def cases():
    """현재 boss/로딩·최초와 무관 플래그·두 할당 실패·QA 폭과 효과 변경을 교차한다."""
    for boss in (0,1):  # 현재 권한 표식의 두 값을 교차한다.
        for loading in (0,1):  # 재추첨을 억제하는 현재 로딩 경계다.
            for flags in (0,1,2,0x11,0x100000,0xffffffff):  # 최초와 무관 비트의 영향을 구별한다.
                for mask in range(4):  # 두 할당 각각의 성공/실패 네 조합이다.
                    for qa in (0,150,65535):  # 기존 QA WORD의 값과 끝 경계를 보존한다.
                        for chosen in (187,0x12340071):yield boss,loading,flags,mask,qa,chosen,0  # 반환 DWORD의 상위 비트 절삭도 대조한다.
    for mutation in range(1,5):yield 1,0,1,3,150,187,mutation  # 선택 효과가 현재 전역과 QA를 바꾸는 입력이다.


def main():
    """기계어 관찰 자료를 새로 생성하거나 기존 SHA/고유 입력을 감사한다."""
    fixture=ROOT/'cpppj/tests/fixtures/buried-postpop-x86.tsv';report=ROOT/'cpppj/buried-postpop.json'
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for path,expected in facts['sha256'].items():assert digest(ROOT/path)==expected,path  # 현재 입력/제품/관찰 자료를 전체 바이트로 감사한다.
        rows=list(csv.reader(fixture.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({row[0] for row in rows});print('Buried PostPop native/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(case) for case in cases()];assert len(rows)==len({row[0] for row in rows})
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','object','boss','loading','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_buried_postpop.py','cpppj/src/o/RawBuriedPostPop.h','cpppj/src/o/RawBuriedPostPop.cpp','cpppj/tests/BuriedPostPopTests.cpp','cpppj/tests/fixtures/buried-postpop-x86.tsv']
    facts={'host':socket.gethostname(),'date':datetime.now().date().isoformat(),'last_full_refined_host':'HJOW-Athlon','last_full_refined_date':'2026-10-15','observations':len(rows),'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'boundary':'00462170/QA helper/타입 주소계산은 원본 명령이다. 추첨/힙/Regular/공통 장부는 명시 외부 경계다. 전체 객체/현재 전역/호출 순서와 스택/SEH/비휘발 레지스터/쓰기 주소를 대조한다.','sha256':{path:digest(ROOT/path) for path in paths if (ROOT/path).is_file()}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Buried PostPop native:',len(rows))


if __name__=='__main__':main()
