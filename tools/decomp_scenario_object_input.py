#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""메뉴 없는 시나리오 객체 명령의 원문 일반 입력과 실제 이름표를 제품 변경 전에 동결한다."""
import argparse
import hashlib
import itertools
import json
import socket
import struct
from pathlib import Path
from decomp_oracle import Oracle as BaseOracle, ROOT, STACK, STOP
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP

POOL = 0x12000000  # 원문이 SID*50으로 찾는 독립 슬롯 저장소다.
TABLE = POOL + 0x8000  # 가상6c만 명시 경계로 교체한 검사 표다.
OBJECT = STOP + 0x100  # 객체 몸체 효과를 실행하지 않는 호출 관측 경계다.
COMMANDS = (('MakeMine',0x5429b0),('InflictDamage',0x542a7c),('DestroyPiece',0x54274c),('TurnToObelisk',0x542a2c),('Meltdown',0x542ad4))  # PE의 실제 이름/명령 전역이다.
FIXTURE = ROOT / 'cpppj/tests/fixtures/scenario-object-input-x86.tsv'  # 제품에 의존하지 않는 입력/관측 파일이다.
REPORT = ROOT / 'cpppj/scenario-object-input-native-20261101.json'  # 원문 범위·명시 경계와 보호 해시를 기록한다.


class Oracle(BaseOracle):
    """Guide/Gump 뒤 실제4d6eca..4da477만 실행하고 가상6c의 인자/반환 무시를 관측한다."""
    def __init__(self):
        """OS·게임 프로세스 없이 보호 PE와 독립 메모리/쓰기 감시를 설치한다."""
        super().__init__('originals',[dict(name='General',originals='004d62b0')],[0x413d50,0x4e63e0,0x445330])
        self.mu.mem_map(POOL,0x10000)
        self.mu.mem_write(TABLE+0x6c,struct.pack('<I',OBJECT))
        self.mu.mem_write(POOL+50*50,struct.pack('<I',TABLE)+bytes(46))
        self.instructions=0
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def word(self,address):
        """원문 인자의 비정렬 DWORD를 읽는다."""
        return struct.unpack('<I',self.mu.mem_read(address,4))[0]

    def on_instruction(self,mu,address,size,data):
        """실제 분기는 원문에 맡기고 가상 객체와 팝업 조회만 명시 장치로 관측한다."""
        self.instructions+=1
        if address==0x4da477:
            mu.emu_stop()
            return
        if address in (OBJECT,0x460de0):
            sp=mu.reg_read(UC_X86_REG_ESP)
            if address==OBJECT:
                event=self.word(sp+4)
                args=struct.unpack('<III',self.mu.mem_read(event,12))
                self.trace.append(':'.join(map(str,((mu.reg_read(UC_X86_REG_ECX)-POOL)//50,*args,self.result))))
            else:
                self.popups+=1
            mu.reg_write(UC_X86_REG_EAX,self.result if address==OBJECT else 0)
            mu.reg_write(UC_X86_REG_EIP,self.word(sp))
            mu.reg_write(UC_X86_REG_ESP,sp+(8 if address==OBJECT else 4))
            return
        super().on_instruction(mu,address,size,data)

    def write(self,mu,access,address,size,value,data):
        """실제 원문은 검사 스택 외 상태에 쓰지 않아야 한다."""
        if not 0x30000000<=address<=address+size<=0x30010000:
            raise RuntimeError(f'미명시 일반 입력 쓰기 {address:08x}+{size}')

    def observe(self,code,parameter,result,mode):
        """서로 다른 x/y, 낮은 SID, 보조키/뗌/반환값을 독립 입력으로 넣는다."""
        self.trace=[]
        self.popups=0
        self.result=result
        for address,value in ((0x5c8464,POOL),(0x540c70,1),(0x5c85a4,mode==1),(0x594fa4,mode==2),(0x55a480,0),(0x5325c8,1)):
            self.mu.mem_write(address,struct.pack('<I',value))  # 앞선 UI 처리가 제공한 현재 전역이다.
        self.mu.mem_write(STACK,bytes(0x100))
        self.mu.mem_write(STACK+0x14,struct.pack('<III',code,0xfedcba98,parameter))
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_EDI,0)
        self.mu.emu_start(0x4d6eca,STOP,count=20000)
        end=self.mu.reg_read(UC_X86_REG_EIP)
        delta=self.mu.reg_read(UC_X86_REG_ESP)-STACK
        assert end==0x4da477 and delta==0
        return ';'.join(self.trace) or '-',self.popups,delta


def main():
    """읽기 전용 이름표와480개 원문 관측을 새 파일에만 기록하거나 재검증한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    oracle=Oracle()
    import pefile
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
    lookup=[]
    for name,address in COMMANDS:  # 실제 NUL 문자열→포인터 쌍→명령 DWORD를 연결한다.
        offset=pe.__data__.find(name.encode('ascii')+b'\0')
        assert offset>=0
        pointer=0x400000+pe.get_rva_from_offset(offset)
        entry=pe.__data__.find(struct.pack('<II',pointer,address))
        assert entry>=0
        lookup.append(dict(name=name,entry=f'{0x400000+pe.get_rva_from_offset(entry):08x}',globalAddress=f'{address:08x}',command=struct.unpack('<I',pe.get_data(address-0x400000,4))[0]))
    rows=[]
    for item,flags,parameter,result,mode in itertools.product(lookup,(0,0x80000000,0x10000000,0x20000000),(0,50,0x10032,0x10000),(0,1),(0,1,2)):
        code=item['command']|flags  # 앞6열만 입력이며 뒤3열은 실제 원문 관측이다.
        rows.append((item['name'],code,0xfedcba98,parameter,result,mode,*oracle.observe(code,parameter,result,mode)))
    raw='# name\tevent\tfirst\tsecond\tobjectReturn\tmode\ttrace\tpopupCalls\tstackdelta\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n'
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_scenario_object_input.py','tools/decomp_oracle.py',FIXTURE.relative_to(ROOT).as_posix())
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():
            assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==wanted,path  # 출처/기대값을 같이 보존한다.
        assert FIXTURE.read_text('utf-8')==raw
    else:
        assert not FIXTURE.exists() and not REPORT.exists(),'기존 기대값은 덮어쓰지 않는다'
        FIXTURE.write_text(raw,encoding='utf-8',newline='\n')
        report=dict(unit=686,date='2026-11-01',host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),uniqueInputs=len(rows),instructions=oracle.instructions,lookup=lookup,gameExecutions=0,boundaries='Guide/Gump 뒤4d6eca..다음 입력4da477. 가상6c는 실제 인자를 기록하고 지정0/1을 반환하는 명시 경계이며 객체 효과는 실행하지 않는다. 팝업460de0은 없음0 경계다. 메뉴/OS/통신/공격/승리 실행은0. 앞선Guide/Gump 소비와 실제 객체 몸체는 이 도구의 새 검증 범위가 아니다.',sha256={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in paths})
        REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(uniqueInputs=len(rows),instructions=oracle.instructions,verified=args.verify)))


if __name__=='__main__':
    main()
