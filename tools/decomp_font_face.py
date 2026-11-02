#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""글꼴 인덱스·열거 콜백·재생성/일반 입력 부모를 격리 CPU에서만 관측한다."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import socket
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]  # 보호 PE는 같은 정밀 디컴파일 호스트에서 읽기만 한다.
OUT = None  # 명시 --output으로 새 원시 디렉터리를 지정하기 전에는 파일을 쓰지 않는다.
sys.path.insert(0, str(ROOT / 'tools'))
from decomp_oracle import Oracle as BaseOracle, SCRATCH, STACK, STOP
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

TABLE, SOURCE, LOGFONT, ENUM_STATE, ENUM_TABLE = (SCRATCH + offset for offset in (0x1000, 0x4000, 0x8000, 0x9000, 0xa000))  # 실제 PE 주소가 아닌 명시 입력 저장소다.
FACE, WIDTH, HEIGHT = 0x5accd8, 0x5c78f0, 0x5c78f4  # 원문의 현재 이름32BYTE와 화면 크기 DWORD다.
CREATE = STOP + 0x100  # CreateFontA만 실제 OS 대신 명시 GDI 경계로 기록한다.


class PartialStop(Exception):
    """선택한 외부 경계에 도달한 부분 결과와 정상 복귀를 구별한다."""


class Oracle(BaseOracle):
    """순수 원문/CRT 몸체를 실행하고 GDI·캐시 파일·dirty·Config는 호출 인자로 기록한다."""
    def __init__(self):
        """원문 함수표 범위와 큰 부모의 해당 분기만 허용한다."""
        names = dict(Enum=0x4a2d30, Lookup=0x4a2df0, Current=0x4a2e30, Rebuild=0x4a39f0, Copy=0x4e51f0)
        super().__init__('originals', [dict(name=name, originals=f'{address:08x}') for name,address in names.items()], helpers=[])
        self.allowed.append((0x4d7295, 0x4d72d3))
        self.mu.hook_add(UC_HOOK_MEM_WRITE, self.guard)
        self.word(0x50008c, CREATE)
        self.trace, self.stop_at, self.kind, self.assertions = [], '', '', 0

    def word(self, address, value=None):
        """원시 DWORD를 읽고 명시 경계 입력만 기록한다."""
        if value is not None:
            self.mu.mem_write(address, struct.pack('<I', value & 0xffffffff))
        return struct.unpack('<I', self.mu.mem_read(address, 4))[0]

    def string(self, address):
        """NULL과 유효 빈 이름을 별도로 직렬화한다."""
        return 'NULL' if not address else self.read_string(address).hex()

    def guard(self, mu, access, address, size, value, data):
        """CPU 쓰기를 격리 스택·명시 입력·원문 글꼴 전역으로 제한한다."""
        assert (0x30000000 <= address and address + size <= 0x30010000) or (SCRATCH <= address and address + size <= SCRATCH + 0x20000) or (0x5c7978 <= address and address + size <= 0x5c7a20) or (FACE <= address and address + size <= FACE + 32), (hex(address), size)

    def returned(self, value=0, purge=0):
        """명시 외부 경계의 반환만 처리하며 호출자 정리는 원문 CPU에 맡긴다."""
        sp=self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
        self.mu.reg_write(UC_X86_REG_ESP, sp + 4 + purge)
        self.mu.reg_write(UC_X86_REG_EIP, self.word(sp))

    def observed(self, event):
        """관측을 순서대로 남기고 요청한 한 경계에서만 부분 실행을 끝낸다."""
        self.trace.append(event)
        if self.stop_at and event.split(':')[0] == self.stop_at:
            raise PartialStop(event)

    def on_instruction(self, mu, address, size, data):
        """원문 분류/strncpy를 대체하지 않고 외부 효과만 명시적으로 구별한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address == 0x4d7b1a:
            mu.emu_stop()
            return
        if address == 0x4e0620:
            self.assertions += 1
            self.observed('Assert:' + str(self.word(sp + 12)))
            self.returned()
            return
        if address == 0x4a3950:
            self.observed('Clear:' + str(self.word(sp + 4)))
            self.returned()
            return
        if address == CREATE:
            args=[self.word(sp + 4 + index * 4) for index in range(14)]
            self.observed('Create:' + ':'.join(map(str, (args[0],args[4],args[5],args[6],args[7]))) + ':' + self.string(args[13]))
            if self.mutate_create:
                self.word(WIDTH, 1280);self.word(HEIGHT,720)
            self.returned(0x7000 + len(self.trace), 56)
            return
        if address == 0x4a34a0:
            args=[self.word(sp + 4 + index * 4) for index in range(3)]
            self.observed('Cache:' + str(mu.reg_read(UC_X86_REG_ECX)) + ':' + self.string(args[0]) + ':' + str(args[1]) + ':' + str(args[2]))
            self.returned()
            return
        if address == 0x499370:
            self.observed('Dirty:' + ':'.join(str(self.word(sp + 4 + index * 4)) for index in range(5)))
            if self.mutate_dirty:
                self.mu.mem_write(FACE, b'ChangedAtDirty\0' + bytes(17))
            self.returned()
            return
        if address == 0x4414d0:
            self.observed('Set:' + self.string(self.word(sp + 4)) + ':' + self.string(self.word(sp + 8)))
            self.returned()
            return
        return super().on_instruction(mu,address,size,data)

    def reset(self, kind, stop_at='', mutate_create=0, mutate_dirty=0):
        """관측마다 현재 이름/메타데이터/명시 입력을 다시 준비한다."""
        self.trace,self.kind,self.stop_at,self.assertions=[],kind,stop_at,0
        self.mutate_create,self.mutate_dirty=mutate_create,mutate_dirty
        self.mu.mem_write(FACE,b'PreviousFace\0'+bytes(19))
        self.mu.mem_write(0x5c7978,bytes(0xa8))
        self.word(WIDTH,640);self.word(HEIGHT,480)
        self.mu.mem_write(SCRATCH,bytes(0x10000))

    def lookup(self, index, count, present):
        """음수 assert 이후의 포인터 산술까지만 실행하며 반환 주소를 역참조하지 않는다."""
        self.reset('Lookup');self.word(0x5c7a20,count);self.word(0x5c7a28,TABLE if present else 0)
        result=self.call('Lookup',[index])
        return dict(result=result,assertions=self.assertions,trace=';'.join(self.trace) or '-')

    def enum(self, font_type, charset, name):
        """동일 LOGFONT를 count/기입 두 형태에 실제 콜백과 strncpy로 통과시킨다."""
        self.reset('Enum');self.mu.mem_write(LOGFONT,bytes(0x100));self.mu.mem_write(LOGFONT+0x17,bytes([charset]));self.mu.mem_write(LOGFONT+0x1c,name+b'\0');self.mu.mem_write(ENUM_TABLE,bytes([0xa5])*128)
        self.mu.mem_write(ENUM_STATE,struct.pack('<III',17,2,0))
        first=self.call('Enum',[LOGFONT,0,font_type,ENUM_STATE],purge=16)
        count=self.word(ENUM_STATE)
        self.mu.mem_write(ENUM_STATE,struct.pack('<III',17,2,ENUM_TABLE))
        second=self.call('Enum',[LOGFONT,0,font_type,ENUM_STATE],purge=16)
        return dict(first=first,second=second,count=count,index=self.word(ENUM_STATE+4),unchangedCount=self.word(ENUM_STATE),record=bytes(self.mu.mem_read(ENUM_TABLE+64,32)).hex())

    def rebuild(self, name, parent=False, stop_at='', mutate_create=0, mutate_dirty=0):
        """GDI·캐시 외부 경계 뒤 현재 이름 기록/부모 dirty·현재 getter 순서를 관측한다."""
        self.reset('Parent' if parent else 'Rebuild',stop_at,mutate_create,mutate_dirty)
        self.word(0x5c7a20,1 if name is not None else 0);self.word(0x5c7a28,TABLE if name is not None else 0)
        pointer=0 if name is None else SOURCE
        if name is not None:
            self.mu.mem_write(SOURCE,name+b'\0')
            self.mu.mem_write(TABLE,name[:31]+b'\0'+bytes(max(0,31-len(name))))
        completed=True
        try:
            if parent:
                for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):  # 큰 부모의 현재 index와0인EDI 외 가변 레지스터는 명시 초기화한다.
                    self.mu.reg_write(reg,0)
                self.mu.reg_write(UC_X86_REG_ESP,STACK-0x3000);self.mu.reg_write(UC_X86_REG_EFLAGS,2)
                self.mu.emu_start(0x4d7295,0x4d7b1a,timeout=500000,count=20000)
                assert self.mu.reg_read(UC_X86_REG_ESP)==STACK-0x3000
            else:
                self.call('Rebuild',[pointer])
        except PartialStop:
            completed=False
        return dict(completed=int(completed),trace=';'.join(self.trace),face=bytes(self.mu.mem_read(FACE,32)).hex(),logicalHeights=[self.word(0x5c7a04+index*4) for index in range(7)])


def main():
    """원문 관측을 독립 반복한 뒤 새 TSV/JSON만 기록한다."""
    global OUT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True,help='기존 파일을 덮어쓰지 않을 새 원시 디렉터리')
    OUT=parser.parse_args().output.resolve()
    if OUT.exists(): raise FileExistsError(OUT)
    OUT.mkdir(parents=True)
    oracle,rows=Oracle(),[]
    for index,count,present in itertools.product((-2,-1,0,1,2,3,7,0x80000000,0x7fffffff),(0,1,3,0xffffffff),(0,1)):  # signed count/왼쪽 shift의32비트 감김도 반환 주소까지만 관측한다.
        values=dict(index=index & 0xffffffff,count=count,present=present);observations=[oracle.lookup(index,count,present) for _ in range(2)];assert observations[0]==observations[1];rows.append(dict(kind='Lookup',input=values,output=observations[0]))
    names=(b'',b'Arial',b'A\0tail',b'A'*31,b'B'*32,b'C'*40,b'\xffFace')
    for font_type,charset,name in itertools.product((0,1,2,3,4,5,6,7,0xffffffff),(0,1,2,128,255),names):  # type의 비트 포함이 아니라 정확한4와charset0만 통과한다.
        values=dict(type=font_type,charset=charset,name=name.hex());observations=[oracle.enum(font_type,charset,name) for _ in range(2)];assert observations[0]==observations[1];rows.append(dict(kind='Enum',input=values,output=observations[0]))
    for name,parent in itertools.product((None,)+names,(False,True)):  # NULL/유효 빈 이름/첫NUL/31·32·40BYTE를 서로 합치지 않는다.
        values=dict(name=None if name is None else name.hex(),parent=int(parent),stop='',mutateCreate=0,mutateDirty=0);observations=[oracle.rebuild(name,parent) for _ in range(2)];assert observations[0]==observations[1];rows.append(dict(kind='Parent' if parent else 'Rebuild',input=values,output=observations[0]))
    # 부분 실패와 현재 이름 재조회 변경을 독립 입력으로 관측한다.
    for stop,mc,md in (('Clear',0,0),('Create',0,0),('Cache',0,0),('Dirty',0,0),('Set',0,0),('',1,0),('',0,1)):
        values=dict(name=b'Arial'.hex(),parent=1,stop=stop,mutateCreate=mc,mutateDirty=md);observations=[oracle.rebuild(b'Arial',True,stop,mc,md) for _ in range(2)];assert observations[0]==observations[1];rows.append(dict(kind='Parent',input=values,output=observations[0]))
    payload='# kind\tinputJson\toutputJson\n'+''.join(row['kind']+'\t'+json.dumps(row['input'],sort_keys=True,separators=(',',':'))+'\t'+json.dumps(row['output'],sort_keys=True,separators=(',',':'))+'\n' for row in rows)
    fixture=OUT/'font-face-native-x86.tsv'
    payloads={fixture.name:payload}
    lookup=['# index\tcount\tpresent\tresult\tassertions\ttrace\n'];enum=['# type\tcharset\tnameHex\tfirst\tsecond\tcount\tindex\tunchangedCount\trecord32Hex\n'];rebuild=['# nameHex\tparent\tstop\tmutateCreate\tmutateDirty\tcompleted\tface32Hex\tlogicalHeights\ttrace\n']
    for row in rows:  # C++에서 별도 JSON 파서 없이 재생할 새 평면 fixture도 같은 관측에서 직렬화한다.
        data,result=row['input'],row['output']
        if row['kind']=='Lookup':
            lookup.append('\t'.join(str(data[key]) for key in ('index','count','present'))+'\t'+'\t'.join(str(result[key]) for key in ('result','assertions','trace'))+'\n')
        elif row['kind']=='Enum':
            enum.append('\t'.join(str(data[key]) if data[key]!='' else '-' for key in ('type','charset','name'))+'\t'+'\t'.join(str(result[key]) for key in ('first','second','count','index','unchangedCount','record'))+'\n')
        else:
            rebuild.append(('NULL' if data['name'] is None else data['name'] or '-')+'\t'+'\t'.join(str(data[key]) if data[key]!='' else '-' for key in ('parent','stop','mutateCreate','mutateDirty'))+'\t'+str(result['completed'])+'\t'+result['face']+'\t'+','.join(map(str,result['logicalHeights']))+'\t'+result['trace']+'\n')
    payloads.update({'font-face-lookup-x86.tsv':''.join(lookup),'font-face-enum-x86.tsv':''.join(enum),'font-face-rebuild-x86.tsv':''.join(rebuild)})
    for name,text in payloads.items():  # 첫 관측을 뒤 실행 결과로 덮어쓰지 않고 물리 바이트가 같은지만 확인한다.
        path=OUT/name;raw=text.encode('utf-8')
        if path.exists():
            assert path.read_bytes()==raw,name
        else:
            path.write_bytes(raw)
    report=dict(unit=764,host=socket.gethostname(),lastPreciseDecompile=dict(host='HJOW-Athlon',date='2026-10-23'),observations=len(rows),repetitions=2,observationRuns=len(rows)*2,nativeEntryCalls=1450,gameExecutions=0,multiplayerTests=0,builds=0,fixtureSha256={name:hashlib.sha256((OUT/name).read_bytes()).hexdigest() for name in payloads},toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),sourceSha256={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('originals/Netstorm.exe','extracted/refined/originals/raw.c','extracted/refined/originals/functions.tsv')},ranges=[f'{a:08x}..{b-1:08x}' for a,b in oracle.allowed],codeSha256={f'{a:08x}..{b-1:08x}':hashlib.sha256(bytes(oracle.mu.mem_read(a,b-a))).hexdigest() for a,b in oracle.allowed},limitations=['CreateFontA는실제OS/GDI대신인자기록/명시handle반환이다.','4a3950 Clear/4a34a0 Cache/499370 dirty/4414d0 Config는호출경계이며실제파일/Gump/Config몸체는실행하지않는다.','음수Lookup은assert뒤실제pointer산술까지만관측하고반환주소에서글꼴이름을읽지않는다.','일반입력부모4d7295진입전Guide/Gump/release관문은별도담당범위다.'],rows=rows)
    (OUT/'font-native.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),'utf-8',newline='\n')
    print(json.dumps({name:report[name] for name in ('observations','repetitions','gameExecutions','multiplayerTests','builds')},ensure_ascii=False))


if __name__=='__main__':
    main()
