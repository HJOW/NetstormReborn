#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""신전004dca50과실제removeAll의현재조회/Player/목록쓰기를격리관찰한다."""
import csv
import json
import socket
import struct
import sys
from datetime import datetime
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, HEAP, PLAYERS, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP

# 원본범위와분리한Pool/Player/두현재목록/Outpost저장소다.
POOL,RECORDS,LISTS=PLAYERS,HEAP+0x1000,HEAP+0x2000
PARENT=50


class Oracle(BaseOracle):
    """부모와removeAll은실제명령이며섬/지역/Damageable/UI는명시외부경계다."""
    def __init__(self):
        """선택된두함수의정밀범위와실제쓰기저장소만허용한다."""
        super().__init__();self.allowed=[]
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:table={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x4dca50,0x40ea00):
            for part in table[address]['ranges'].split(';'):
                low,high=(int(x,16) for x in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)

    def guard(self,mu,access,address,size,value,data):
        """현재세객체/Player/목록/스택이외의실제명령쓰기를거부한다."""
        ranges=[(POOL+sid*50,POOL+(sid+1)*50) for sid in (50,60,61)]+[(RECORDS,RECORDS+1620),(LISTS,LISTS+64),(0x5e189c,0x5e18a8),(0x59a9d8,0x59a9e4),(STACK-4096,STACK+128)]
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'신전선택밖쓰기{address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """큰효과의현재인자와선택된변경입력을기록하고원본문체는실행한다."""
        esp=mu.reg_read(UC_X86_REG_ESP);args=self.words(esp+4,4)
        if address==0x46f530:self.events.append('Island,'+str(args[0])+','+str(args[1]));self.ret(2,8);return
        if address==0x46ee30:self.events.append('Exists,'+str(args[0]));self.ret(0 if self.variant&512 else 1,4);return
        if address==0x46f260:self.events.append('Temple,'+str(args[0]));self.ret(99 if self.variant&1024 else 50,4);return
        if address==0x4db890:
            self.events.append('Clear,'+str(args[1]));self.ret();return
        if address==0x46ef10:self.events.append('Owner,'+str(args[0])+','+str(args[1]));self.ret(purge=8);return
        if address==0x455380:self.events.append('Region,'+str(args[0])+','+str(args[1]));self.ret();return
        if address==0x44b4b0:
            self.events.append('Base,'+str(args[0]))
            if self.mutation==1:self.put(POOL+PARENT*50+34,2,1)
            if self.mutation==2:self.put(POOL+PARENT*50+40,9,1)
            if self.mutation==3:self.put(0x5c89b8,1)
            self.ret(purge=4);return
        if address==0x47d890:
            self.events.append('Refresh')
            if self.mutation==4:self.put(POOL+PARENT*50+34,8,1)
            self.ret();return
        if address==0x4e0620:self.events.append('Assert,'+str(args[2]));self.ret();return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'신전미검토명령{address:x}')

    def run(self,case):
        """실제입력만설치하고출력은원본명령이쓴전체기록에서읽는다."""
        self.variant,self.mutation,flags=case;self.events=[];owner=(self.variant>>2)&15;extra=(0,1,8,9)[self.variant&3];match=(self.variant>>6)&3
        self.put(0x5c8464,POOL);self.put(0x5c847c,128);self.put(0x59534c,RECORDS);self.put(0x5c89b8,1 if self.variant&256 else 0)
        self.mu.mem_write(RECORDS,b'\xa5'*1620)
        for player in range(9):
            for offset,matched in ((0x7c,match&1),(0x80,False),(0x84,match&2),(0x88,False)):self.put(RECORDS+player*180+offset,50 if matched else 99)
        for sid,own in ((50,owner),(60,owner),(61,(owner+1)%9)):
            raw=bytearray(50);raw[10]=124 if sid==50 else 126;raw[11]=2 if sid==50 else 0;raw[34]=own;raw[40]=extra if sid==50 else 0;struct.pack_into('<ff',raw,14,float(sid)+.25,float(sid)+.5);self.mu.mem_write(POOL+sid*50,bytes(raw))
        self.mu.mem_write(LISTS,struct.pack('<10I',50,60,50,61,62,50,63,50,60,61))
        for at,offset,count in ((0x5e189c,0,4),(0x59a9d8,16,4),(0x55a70c,32,2)):self.put(at,LISTS+offset);self.put(at+4,4);self.put(at+8,count)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)
        self.mu.reg_write(UC_X86_REG_ECX,POOL+50*50);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<II',STOP,flags));self.mu.emu_start(0x4dca50,STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+8
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg
        objects=b''.join(bytes(self.mu.mem_read(POOL+sid*50,50)) for sid in (50,60,61))
        counts=struct.pack('<II',self.words(0x5e18a4)[0],self.words(0x59a9e0)[0])
        return [','.join(map(str,case)),bytes(self.mu.mem_read(RECORDS,1620)).hex(),objects.hex(),(counts+bytes(self.mu.mem_read(LISTS,40))).hex(),str(self.words(0x5c89b8)[0]),';'.join(self.events) or '-']


def cases():
    """owner/extra/참조필드/로딩과현재효과변경을명시입력으로공급한다."""
    for owner in (0,1,8):
        for extra in range(4):
            for match in range(4):
                for loading in (0,256):yield owner*4+extra+match*64+loading,0,0x400000
    for mutation in range(1,5):yield 4+192,mutation,0x400000
    for assertion in (512,1024):yield 4+192+assertion,0,0x400000


def main():
    """새원본기록/SHA를보존하고보호원본은읽기만한다."""
    fixture=ROOT/'cpppj/tests/fixtures/vortex-pre-x86.tsv';report=ROOT/'cpppj/vortex-pre.json'
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for path,expected in facts['sha256'].items():assert digest(ROOT/path)==expected,path
        rows=list(csv.reader(fixture.open(encoding='utf-8'),delimiter='\t'))[1:];assert len(rows)==facts['observations']==len({r[0] for r in rows});print('Vortex Pre native/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(case) for case in cases()];assert len(rows)==len({r[0] for r in rows})
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','players','objects','lists','loading','trace']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_vortex_pre.py','cpppj/src/o/RawVortexPreDestroy.h','cpppj/src/o/RawVortexPreDestroy.cpp','cpppj/tests/VortexPreDestroyTests.cpp','cpppj/tests/fixtures/vortex-pre-x86.tsv']
    facts={'host':socket.gethostname(),'date':datetime.now().date().isoformat(),'last_full_refined_host':'HJOW-Athlon','last_full_refined_date':'2026-10-15','observations':len(rows),'status':'native-observed-cpp-validation-pending','game_gui_runs':0,'boundary':'실제004dca50/removeAll만실행. 섬/지역/Damageable/UI는명시외부경계. 전체Player/세객체/물리목록/호출순서/정상스택과보존레지스터/쓰기주소를대조한다.','sha256':{p:digest(ROOT/p) for p in paths if (ROOT/p).is_file()}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Vortex Pre native:',len(rows))


if __name__=='__main__':main()
