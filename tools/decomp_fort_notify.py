#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Notify004bd360의 첫파일명BYTE/진단/재조회/실제저장진입을 최초 독립 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from collections import Counter
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, POOL, STACK, STOP, digest
from unicorn import UcError, UC_HOOK_MEM_READ, UC_HOOK_MEM_READ_UNMAPPED, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW

HEADERS, HEADER, MISSION = 0x5c89f0, 0x5c8a04, 0x594fa0  # 실제9칸/고정Template1/inMission 원본 전역이다.
MANAGERS = {0:0, 11:POOL+0x100, 22:POOL+0x200}  # 명시 두 실제 가상manager와null이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/fort-notify-x86.tsv'  # 제품 patch 작성 전에 고정하고 덮어쓰지 않는다.
REPORT = ROOT/'cpppj/fort-notify-native.json'  # 실제 물리접근 실패와 진단/중단/정상반환을 구별한다.


def cases():
    """빈/점/일반/첫NUL이름과모든관문·callback변경입력만준비한다."""
    rows=[]
    for manager,name,mine,transient,prevent,mission in itertools.product((0,11,22),range(4),(0,1),(0,1),(0,1),(0,1)):
        rows.append((manager,name,mine,transient,prevent,mission,'none'))
    changes=('dot-manager0','dot-manager22','dot-mine0','dot-mine1','dot-transient1','dot-prevent1','dot-mission1','dot-mission0','dot-nameEmpty','dot-stop',
             'transient-mine0','transient-mine1','transient-manager0','transient-manager22','transient-mission1','transient-mission0','transient-prevent1','transient-stop',
             'mission-manager0','mission-manager22','mission-mine1','mission-prevent1','mission-prevent0','mission-nameDot','mission-stop',
             'dot-manager0+missing-manager22','dot-manager0+missing-prevent1','dot-manager0+missing-prevent0','dot-manager0+missing-stop',
             'mission-manager0+missing-manager22','mission-manager0+missing-prevent1','mission-manager0+missing-stop',
             'transient-mine0+mission-manager0+missing-manager22','save-manager0','save-mission1','save-prevent1','save-stop')
    for change,mine,transient,prevent,mission in itertools.product(changes,(0,1),(0,1),(0,1),(0,1)):
        rows.append((11,1,mine,transient,prevent,mission,change))
    # 각관문은BYTE저비트가아닌DWORD비영을사용한다.
    for mine,transient,prevent,mission in itertools.product((0,0x80000000),(0,0xffffffff),(0,0xffffffff),(0,1)):
        rows.append((11,0,mine,transient,prevent,mission,'none'))
    return list(dict.fromkeys(rows))


class Oracle(MemoryOracle):
    """Notify의 실제155BYTE를실행하고진단/SaveFile만명시외부장치로둔다."""
    def __init__(self):
        """첫manager0은0xc의실제미매핑읽기실패로관측한다.0페이지를매핑하지않는다."""
        super().__init__()
        self.total=0  # 실행한실제명령의전체수다.
        self.statuses=Counter()  # 정상/명시중단/실제물리경계수다.
        self.boundaries=Counter()  # 이름첫BYTE/진단/저장하위진입수다.
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)
        self.mu.hook_add(UC_HOOK_MEM_READ,self.read)
        self.mu.hook_add(UC_HOOK_MEM_READ_UNMAPPED,self.unmapped)

    def guard(self,mu,access,address,size,value,data):
        """원본부모의CPU쓰기는스택에만허용한다.외부callback효과와구별한다."""
        if not STACK-0x1000<=address<address+size<=STACK+4:
            raise RuntimeError(f'Notify CPU 쓰기 범위 밖 {address:08x}/{size}')

    def read(self,mu,access,address,size,value,data):
        """원본은파일명을함수로조회하지않고manager+c 첫BYTE를정확히한번읽는다."""
        for manager in (11,22):  # 두물리manager의파일명첫BYTE만원본접근trace로남긴다.
            if address==MANAGERS[manager]+12:
                if size!=1:
                    raise RuntimeError('Notify 파일명 첫BYTE 읽기 폭 오류')
                self.events.append(f'name:{manager}:{mu.mem_read(address,1)[0]}')
                self.boundaries['name']+=1

    def unmapped(self,mu,access,address,size,value,data):
        """초기nullmanager의실제0xc읽기실패를진단보고로바꾸지않는다."""
        if address!=12 or size!=1:
            raise RuntimeError(f'Notify 관측밖 물리읽기 {address:08x}/{size}')
        self.status='fault'
        self.events.append(f'fault:{address}:{size}')
        return False

    def mutate(self,boundary):
        """입력callback만적용한다.진단뒤+8/미션/manager/prevent재조회는원문이한다."""
        for part in self.case[6].split('+'):  # 같은관측에서여러서로다른진단의변경을허용한다.
            if '-' not in part:
                continue
            trigger,action=part.split('-',1)
            if trigger!=boundary:
                continue
            if action.startswith('manager'):
                self.put(HEADER,MANAGERS[int(action[7:])])
            elif action.startswith('mine'):
                self.put(HEADER+8,int(action[4:]))
            elif action.startswith('transient'):
                self.put(HEADER+12,int(action[9:]))
            elif action.startswith('prevent'):
                self.put(HEADER+16,int(action[7:]))
            elif action.startswith('mission'):
                self.put(MISSION,int(action[7:]))
            elif action=='nameEmpty':
                self.mu.mem_write(MANAGERS[11]+12,b'\0')
            elif action=='nameDot':
                self.mu.mem_write(MANAGERS[11]+12,b'.')
            elif action=='stop':
                self.status='stop'
                self.mu.emu_stop()

    def instruction(self,mu,address,size,data):
        """실제155BYTE·tailjmp를실행하고SaveFile은현재ECX를받은시점에관측한다."""
        self.instructions+=1
        self.total+=1
        if self.instructions>200:
            raise RuntimeError('Notify 명령 상한 초과')
        if address==0x4e0620:
            line=self.words(mu.reg_read(UC_X86_REG_ESP)+4,3)[2]
            self.events.append(f'assert:{line}')
            self.boundaries['assert']+=1
            self.mutate({449:'dot',451:'transient',454:'mission',455:'missing'}[line])
            if self.status=='return':
                self.ret()
        elif address==0x44d8d0:
            manager=next(key for key,pointer in MANAGERS.items() if pointer==mu.reg_read(UC_X86_REG_ECX))
            self.events.append(f'save:{manager}')
            self.boundaries['save']+=1
            self.mutate('save')
            if self.status=='return':
                self.ret()
        elif not 0x4bd360<=address<0x4bd3fb:
            raise RuntimeError(f'검토하지않은Notify명령 {address:08x}')

    def sample(self,case,control):
        """독립입력만설치하고정상/중단/첫물리오류의trace와전체raw를보존한다."""
        self.case=case  # manager/name/+8/+c/+10/inMission/변경의최초입력이다.
        self.events=[]  # 실제처음파일명읽기와하위진입순서다.
        self.status='return'  # 별도명시중단/물리오류와구분하는초기상태다.
        self.instructions=0  # 각관측의명령상한계수다.
        self.mu.mem_write(HEADERS,bytes((index*13+7)&255 for index in range(180)))
        self.mu.mem_write(HEADER,struct.pack('<5I',MANAGERS[case[0]],0x12345678,case[2],case[3],case[4]))
        self.put(MISSION,case[5])
        names=(b'\0',b'.map\0',b'map.fort\0',b'\0hidden\0')
        for manager in (11,22):  # manager전체64BYTE를초기화하고파일명뒤의원래바이트도보존한다.
            self.mu.mem_write(MANAGERS[manager],b'\xa5'*64)
            self.mu.mem_write(MANAGERS[manager]+12,names[case[1]] if manager==11 else b'B.fort\0')
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 이전CPU상태를재사용하지않는다.
            self.mu.reg_write(register,0)
        self.mu.reg_write(UC_X86_REG_ECX,HEADER)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.mem_write(STACK,struct.pack('<I',STOP))
        try:
            self.mu.emu_start(0x4bd360,STOP)
        except UcError:
            if self.status!='fault':
                raise
        if self.status=='return':
            assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        self.statuses[self.status]+=1
        raw=bytes(self.mu.mem_read(HEADERS,180)).hex()+bytes(self.mu.mem_read(MANAGERS[11],64)).hex()+bytes(self.mu.mem_read(MANAGERS[22],64)).hex()
        return '\t'.join((str(control),','.join(map(str,case)),self.status,str(self.words(MISSION,1)[0]),raw,';'.join(self.events)))


def main():
    """최초PE결과를고정하거나이미고정한원문/도구/fixtureSHA를읽기전용검사한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text(encoding='utf-8'))
        for name,expected in record['sha256'].items():  # 원본기대값을제품에맞춰변경하지못하게한다.
            if digest(ROOT/name)!=expected:
                raise RuntimeError('Notify 최초SHA불일치: '+name)
        print('Notify 최초SHA보존:',record['observations'])
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('Notify 최초fixture/보고서덮어쓰기금지')
    oracle,inputs=Oracle(),cases()
    lines=['# control / manager,name,mine,transient,prevent,mission,mutation / return-stop-fault / 현재inMissionDWORD / Template180+manager128BYTE전체 / 실제name-byte·진단·저장진입trace다.']
    for case in inputs:  # 두제어어의각원문실행을분리한다.
        first,second=oracle.sample(case,0x27f),oracle.sample(case,0x37f)
        assert first.split('\t')[1:]==second.split('\t')[1:]
        lines.extend((first,second))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_fort_notify.py','cpppj/tests/fixtures/fort-notify-x86.tsv')
    record={'status':'최초독립원문고정·제안patch미작성','host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entry':'004bd360','bodyBytes':155,'uniqueInputs':len(inputs),'observations':len(inputs)*2,'statuses':dict(oracle.statuses),'instructions':oracle.total,'boundaries':dict(oracle.boundaries),'precisionDifferences':0,'scope':'실제155BYTE Notify와첫manager+c BYTE읽기. manager0은실제0xc미매핑읽기실패다. 진단/SaveFile은명시경계이며save:0진입은하위파일저장성공이아니다. 진단복귀뒤+8재조회/현재미션/manager진단/preventSave/저장인자와부분중단을관측한다. OS·게임실행없음.','gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:record[key] for key in ('uniqueInputs','observations','statuses','instructions','boundaries')}))


if __name__=='__main__':
    main()
