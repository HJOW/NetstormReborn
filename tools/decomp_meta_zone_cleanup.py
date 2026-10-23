#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""0042b1b0 실제68BYTE 부모와 선택된 하위 호출 경계를 제품 전에 고정한다."""
import argparse
from collections import Counter
import json
import socket
import struct
from decomp_priestmenu import Oracle as MemoryOracle, ROOT, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 부모의 실제 초기 플래그·51개 기록·인접 보존 BYTE다. 새 Client owner를 만드는 주소가 아니다.
FLAG, TABLE, RECORD_COUNT, RECORD_BYTES = 0x531760, 0x546908, 51, 144
TABLE_BYTES, CANARY_BYTES, ENTRY, END, CHILD = 51*144, 16, 0x42b1b0, 0x42b1f4, 0x479520
FIXTURE = ROOT/'cpppj/tests/fixtures/meta-zone-cleanup-x86.tsv'
REPORT = ROOT/'cpppj/meta-zone-cleanup-native.json'


class StopChild(Exception):
    """선택된 필수 자식 장치에서의 명시 중단이며 원문 OS fault가 아니다."""


class Oracle(MemoryOracle):
    """원문 부모 전체를 실행하고00479520 본문만 명시 필수 장치로 분리한다."""
    def __init__(self):
        """보호 PE를 읽기 복사해 부모·스택·고정 표 쓰기를 감사한다."""
        super().__init__()
        self.running = False
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def write(self, mu, access, address, size, value, data):
        """rep stosd의 각 폭·주소·순서와 별도 플래그의 쓰기 시점을 확인한다."""
        if not self.running:
            return
        if STACK-4096 <= address and address+size <= STACK+64:
            return
        if TABLE <= address < TABLE+TABLE_BYTES:
            if size != 4 or address != TABLE+self.zero_stores*4 or value != 0:
                raise RuntimeError('MetaZone 부모 rep stosd 순서 또는 폭 차이')
            self.zero_stores += 1
        elif address == FLAG and size == 4:
            if self.zero_stores != TABLE_BYTES//4 or value != 0:
                raise RuntimeError('MetaZone 플래그가 표 초기화 완료 전에 작성됨')
            self.flag_writes += 1
        else:
            raise RuntimeError('MetaZone 예상 밖 CPU 쓰기: '+hex(address))

    def instruction(self, mu, address, size, data):
        """고정 자식 인자·부분 효과·중단은 장치이며 부모 명령은 실제 PE다."""
        self.instructions += 1
        if self.instructions > 5000:
            raise RuntimeError('MetaZone 부모 명령 상한')
        if address == CHILD:
            index = self.words(self.mu.reg_read(UC_X86_REG_ESP)+4,1)[0]
            self.calls.append(index)
            self.trace.append('C:'+str(index))
            if len(self.calls) == self.stop:
                raise StopChild()
            at, action = self.mutation.split(':')
            if int(at) == len(self.calls):
                self.trace.append('M:'+action)
                if action == 'flag1':
                    self.put(FLAG,1)
                elif action == 'flagMax':
                    self.put(FLAG,0xffffffff)
                elif action == 'touch':
                    self.put(TABLE+index*RECORD_BYTES+76,0x89abcdef)
                else:
                    raise RuntimeError('MetaZone 미해석 명시 자식 효과')
            self.ret(0)
            return
        if not ENTRY <= address < END:
            raise RuntimeError('MetaZone 부모 외 명령: '+hex(address))

    def observe(self, seed, flag, mutation, stop, control):
        """독립 초기 메모리에서 한 부모를 실행하고 정상/명시 중단을 구별한다."""
        self.running = False
        self.instructions = self.zero_stores = self.flag_writes = 0
        self.calls, self.trace = [], []
        self.mutation, self.stop = mutation, stop
        raw = bytes((seed+byte*17)&255 for byte in range(TABLE_BYTES+CANARY_BYTES))
        self.mu.mem_write(TABLE, raw)
        self.put(FLAG, flag)
        self.mu.mem_write(STACK-4096,b'\xcd'*(4096+64))
        self.put(STACK, STOP)
        registers = ((UC_X86_REG_EAX,0x89abcdef),(UC_X86_REG_ECX,0x23456789),(UC_X86_REG_EDX,0x3456789a),(UC_X86_REG_EBX,0x456789ab),(UC_X86_REG_ESI,0x56789abc),(UC_X86_REG_EDI,0x6789abcd),(UC_X86_REG_EBP,0x789abcde))
        for register, value in registers:  # 비휘발 값은 정상 반환에서 별도 감사한다.
            self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.reg_write(UC_X86_REG_FPCW,control)
        self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_FPTAG,0xffff)
        self.running = True
        status = 'normal'
        try:
            self.mu.emu_start(ENTRY, STOP)
        except StopChild:
            status = 'stop'
        finally:
            self.running = False
        if status == 'normal':
            if self.mu.reg_read(UC_X86_REG_EIP) != STOP or self.mu.reg_read(UC_X86_REG_ESP) != STACK+4:
                raise RuntimeError('MetaZone 정상 cdecl 복귀 차이')
            for register, value in registers[3:]:  # 부모가 보존하는 EBX/ESI/EDI/EBP다.
                if self.mu.reg_read(register) != value:
                    raise RuntimeError('MetaZone 정상 비휘발 차이')
            if self.zero_stores != TABLE_BYTES//4 or self.flag_writes != int(flag != 0):
                raise RuntimeError('MetaZone 정상 전체 표 또는 flag store 차이')
            if self.calls != ([] if flag else list(range(RECORD_COUNT))):
                raise RuntimeError('MetaZone 진입 관문 또는 고정51호출 차이')
        elif self.zero_stores or self.flag_writes:
            raise RuntimeError('MetaZone 자식 중단 뒤 전체 초기화를 앞당김')
        if self.mu.reg_read(UC_X86_REG_FPCW) != control or self.mu.reg_read(UC_X86_REG_FPSW) != 0 or self.mu.reg_read(UC_X86_REG_FPTAG) != 0xffff:
            raise RuntimeError('MetaZone FPU 보존 차이')
        table = bytes(self.mu.mem_read(TABLE,len(raw)))
        if table[-CANARY_BYTES:] != raw[-CANARY_BYTES:]:
            raise RuntimeError('MetaZone 인접 canary 변경')
        return status, str(self.words(FLAG,1)[0]), table.hex(), ','.join(map(str,self.calls)) or '-', ';'.join(self.trace) or '-', str(self.zero_stores), str(self.flag_writes), str(self.instructions)


def main():
    """제품이 없을 때 최초 자료를 고정하고 이후에는 원시 SHA만 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    if arguments.verify:
        report=json.loads(REPORT.read_text(encoding='utf-8'))
        for relative, expected in report['sha256'].items():  # 최초 CPU 관측과 제품·후속 빌드를 섞지 않는다.
            if digest(ROOT/relative) != expected:
                raise RuntimeError('MetaZone 최초 SHA 차이: '+relative)
        print(json.dumps({'verified':True,'observations':report['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('MetaZone 최초 관측 덮어쓰기 금지')
    for relative in ('cpppj/src/o/RawMetaZoneCleanup.h','cpppj/src/o/RawMetaZoneCleanup.cpp','cpppj/tests/MetaZoneCleanupTests.cpp'):  # 이 파일은 관측 후에만 작성한다.
        if (ROOT/relative).exists():
            raise RuntimeError('MetaZone 제품 선작성 금지')
    inputs=[(index*19,flag,'0:none',0) for index,flag in enumerate((0,1,2,0x80000000,0xffffffff,0x12345678,0,0))]
    inputs += [(31,0,f'{at}:{action}',0) for at,action in ((1,'flag1'),(26,'flagMax'),(51,'flag1'),(1,'touch'),(26,'touch'),(51,'touch'))]
    inputs += [(53,0,'0:none',stop) for stop in (1,26,51)]
    inputs += [(71,0,'1:flag1',26),(89,0,'26:flagMax',51)]
    oracle=Oracle()
    initial_flag=oracle.words(FLAG,1)[0]
    if initial_flag != 1:
        raise RuntimeError('MetaZone 실제 PE 초기 플래그가1이 아님')
    rows=['# control / seed,initialFlag,mutation,stop / status / currentFlag / table7344+canary16 / childIndices / callbackTrace / zeroDwordStores / flagStores / instructions']
    statuses=Counter()
    instructions=0
    different=[]
    for index,values in enumerate(inputs):  # 두 정밀도에서 같은 명시 입력을 빠짐없이 실행한다.
        observed=[]
        for control in (0x27f,0x37f):
            outputs=oracle.observe(*values,control)
            rows.append('\t'.join((str(control),','.join(map(str,values)),*outputs)))
            statuses[outputs[0]]+=1
            instructions+=oracle.instructions
            observed.append(outputs)
        if observed[0] != observed[1]:
            different.append(index)
    FIXTURE.write_text('\n'.join(rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','extracted/refined/originalCD/Netstorm.c','extracted/refined/originalCD/functions.tsv','tools/decomp_priestmenu.py','tools/decomp_meta_zone_cleanup.py',FIXTURE.relative_to(ROOT).as_posix())
    report={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-23','entry':'0042b1b0','bodyBytes':68,'cdSourceMapAddress':'0045a230','cdExecuted':False,'tableAddress':'00546908','recordCount':RECORD_COUNT,'recordStride':RECORD_BYTES,'tableBytes':TABLE_BYTES,'initialFlagPeAddress':'00531760','initialFlagPeValue':initial_flag,'inputs':len(inputs),'observations':len(rows)-1,'statuses':dict(statuses),'instructions':instructions,'precisionDifferentInputs':different,'productAbsentBeforeObservation':True,'externalChildren':['00479520: 실물 marker삭제 자식은 명시 장치; 고정index/선택변경/중단만 관측'],'scope':'68BYTE 부모 전체와51회 호출·0x72c DWORD 쓰기. 실제 marker가상소멸/메타맵 전체/Client장기owner/게임실행은 미검증. TotalMade·Core·State5블록과 별도 표.','sha256':{relative:digest(ROOT/relative) for relative in paths}}
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({key:report[key] for key in ('observations','statuses','instructions','precisionDifferentInputs')}))


if __name__=='__main__':
    main()
