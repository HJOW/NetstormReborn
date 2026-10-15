#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""로그인 옵션의 실제 네 순수 몸체와 0x67 부모의 작은 하위를 격리 관찰한다."""
import csv
import itertools
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, TABLE, ZONE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG

# 실제 진입주소와 입력별 전체 관찰 순서다. 외부 함수 대체는 하지 않는다.
ENTRIES={'Range':0x4b4860,'Cost':0x41c8b0,'Value':0x41ca70,'Clear':0x41c320,'Apply':0x4b4900}
# 기본 범위표와 경계 입력의 float 비트다. 기대 결과를 계산하는 표가 아니다.
DEFAULT=(0x41600000,0x41b00000,0x41f00000)
SPECIAL=(0,0x80000000,1,0x80000001,0x41600000,0x41f00000,0x7f800000,0xff800000,0x7fc12345,0xffc12345)


def host_x87_diagnostic():
    """Unicorn과다른signaling NaN변환을현재CPU의동일FLD/FSTP쌍으로별도진단한다."""
    import ctypes
    if sys.platform!='win32' or ctypes.sizeof(ctypes.c_void_p)!=8:return {'status':'미실행: Windows AMD64 전용 진단'}
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.VirtualAlloc.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint32,ctypes.c_uint32];kernel.VirtualAlloc.restype=ctypes.c_void_p
    kernel.VirtualProtect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];kernel.VirtualProtect.restype=ctypes.c_int
    kernel.VirtualFree.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint32];kernel.VirtualFree.restype=ctypes.c_int
    # 전체x87환경을저장·복원하고027f로FLD[RCX]/FSTP[RDX]두명령만실행한다.
    code=bytes.fromhex('4883ec28d9342466c74424207f02d96c2420d901d91ad924244883c428c3')
    address=kernel.VirtualAlloc(None,4096,0x3000,4);assert address
    try:
        ctypes.memmove(address,code,len(code));old=ctypes.c_uint32();assert kernel.VirtualProtect(address,4096,0x20,ctypes.byref(old))
        call=ctypes.WINFUNCTYPE(None,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32))(address);rows=[]
        # 실제원본함수완주나새고유입력수로합산하지않는CPU명령진단5개다.
        for bits in (0x7f800001,0xff800001,0x7fc12345,0xffc12345,0x80000000):
            source=ctypes.c_uint32(bits);target=ctypes.c_uint32();call(ctypes.byref(source),ctypes.byref(target));rows.append({'input':hex(bits),'stored':hex(target.value)})
        return {'status':'현재CPU명령진단완료','architecture':'AMD64','body_hex':code.hex(),'control_word':'027f','complete_x87_environment_restored':True,'original_function_inputs':0,'observations':rows}
    finally:assert kernel.VirtualFree(address,0,0x8000)


class Oracle(BaseOracle):
    """현재 PC 정밀 함수 범위 안의 실제 명령만 실행하며 모든 쓰기를 감시한다."""
    def __init__(self):
        """실제 TYPE 조회도 실행하도록188개 원본500BYTE 레코드를 공급한다."""
        super().__init__();self.allowed=[];self.selected=set(ENTRIES.values())|{0x49a840,0x40aec0}
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 함수 범위를 넓혀 미분석 인접 주소를 실행하지 않는다.
        for address in self.selected:
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write_guard)
        self.type_bytes=bytearray((index*17+31)&255 for index in range(188*500))
        # 모든 기존 비용에 서로 다른 유한한 입력값을 공급한다.
        for index in range(188):struct.pack_into('<f',self.type_bytes,index*500+0xc4,100.25+index)

    def write_guard(self,mu,access,address,size,value,data):
        """스택과 선택 경로가 실제 수정하는 정확한 필드 밖 쓰기를 거부한다."""
        ranges=[(STACK-0x7f000,STACK+0x1000)]
        if self.kind in ('Range','Apply'):ranges.append((0x52f488,0x52f490))
        if self.kind=='Apply':ranges.extend([(0x52f7e0,0x52f837),(0x5424bc,0x5424c0),(TABLE+self.target*500+0xc4,TABLE+self.target*500+0xc8)])
        if self.kind=='Clear':ranges.append((0x5451f8,0x5452f8))
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'선택밖 옵션 쓰기 {address:08x}/{size}')

    def instruction(self,mu,address,size,data):
        """호출/CRT를 대체하지 않고 원본 함수 진입 순서와 명령 상한을 검사한다."""
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'미검토 옵션 명령 {address:08x}')
        if address in self.selected:self.trace.append(f'{address:08x}')
        self.instructions+=1
        if self.instructions>3000:raise RuntimeError('옵션 명령 상한')

    def run(self,case):
        """같은 현재 옵션/TYPE/사용표의 실제 결과와 정상 반환 규약을 보존한다."""
        self.kind,range_index,cost_index,value_index,self.target,pattern,a,b,c=case;self.instructions=0;self.trace=[]
        options=bytearray((index*11+pattern)&255 for index in range(87));options[4]=range_index;options[0x10]=cost_index;options[0xf]=value_index
        self.mu.mem_write(0x52f7e0,bytes(options));self.mu.mem_write(0x5425f4,struct.pack('<III',a,b,c));self.put(0x52f488,0x418c0000);self.put(0x52f48c,0x43992000);self.put(0x5424bc,0x12345678)
        self.mu.mem_write(TABLE,bytes(self.type_bytes));self.put(0x59ab20,TABLE);self.put(0x541240,self.target)
        # 이웃4BYTE 두 곳도 출력하여256BYTE만 초기화했음을 확인한다.
        self.mu.mem_write(0x5451f4,bytes((index*7+pattern)&255 for index in range(264)))
        self.put(0x5467d8,0);self.put(0x594fbc,1);self.put(0x594fa4,1);self.put(0x5c85d4,1)
        raw=bytes([1,0x67,94,0,0,87,0])+bytes(options);self.mu.mem_write(ZONE,raw)
        # caller-save도 독립된 값으로 채우며 반환값은 정수 getter에만 관찰한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(register,0x76543210+register)
        args=[9,0,ZONE] if self.kind=='Apply' else []
        self.mu.reg_write(UC_X86_REG_FPCW,0x27f);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.mu.emu_start(ENTRIES[self.kind],STOP)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+4
        # 실제 원본 cdecl의 보존 레지스터를 검사한다.
        for register in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(register)==0x76543210+register
        result=self.mu.reg_read(UC_X86_REG_EAX) if self.kind in ('Cost','Value') else 0
        costs=b''.join(bytes(self.mu.mem_read(TABLE+index*500+0xc4,4)) for index in range(188)) if self.kind=='Apply' else b''
        return [','.join(map(str,case)),str(result),bytes(self.mu.mem_read(0x52f488,8)).hex(),str(self.words(0x5424bc)[0]),costs.hex() or '-',bytes(self.mu.mem_read(0x5451f4,264)).hex() if self.kind=='Clear' else '-',bytes(self.mu.mem_read(0x52f7e0,87)).hex(),';'.join(self.trace)]


def cases():
    """정상 옵션 전부와 NaN/무한/부호0/비정규/역전 범위표를 중복 없이 공급한다."""
    for table in itertools.product(SPECIAL,repeat=3):
        for index in range(3):yield 'Range',index,0,0,0,0,*table
    for index in range(3):yield 'Range',index,0,0,0,0,*DEFAULT
    for index in range(4):yield 'Cost',0,index,0,0,0,*DEFAULT
    for index in range(6):yield 'Value',0,0,index,0,0,*DEFAULT
    for pattern in (0,1,127,255):yield 'Clear',0,0,0,0,pattern,*DEFAULT
    # 원본0x67 부모에서 실제 작은 하위의 모든 옵션조합/현재TYPE 경계를 실행한다.
    for range_index,cost,value,target in itertools.product(range(3),range(4),range(6),(0,1,122,187)):yield 'Apply',range_index,cost,value,target,17,*DEFAULT


def main():
    """보호 원본은 읽기만 하며 새 독립fixture와 현재 소스 SHA를 기록한다."""
    fixture=ROOT/'cpppj/tests/fixtures/mission-battle-options-x86.tsv';report=ROOT/'cpppj/mission-battle-options.json'
    paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_mission_battle_options.py','cpppj/src/o/RawMissionBattleOptions.h','cpppj/src/o/RawMissionBattleOptions.cpp','cpppj/tests/MissionBattleOptionsTests.cpp','cpppj/tests/fixtures/mission-battle-options-x86.tsv']
    if '--verify' in sys.argv:
        facts=json.loads(report.read_text(encoding='utf-8'))
        for path,sha in facts['sha256'].items():assert digest(ROOT/path)==sha,path
        with fixture.open(encoding='utf-8') as stream:rows=list(csv.DictReader(stream,delimiter='\t'))
        assert len(rows)==facts['nativeUniqueInputs']==len({row['input'] for row in rows});print('미션 옵션 원본/SHA:',len(rows));return
    oracle=Oracle();rows=[oracle.run(case) for case in cases()]
    assert len(rows)==len({row[0] for row in rows})
    with fixture.open('w',encoding='utf-8',newline='') as stream:
        writer=csv.writer(stream,delimiter='\t',lineterminator='\n');writer.writerow(['input','result','range','value','costs','typeUse','options','trace']);writer.writerows(rows)
    facts={'host':socket.gethostname(),'date':'2026-10-15','nativeUniqueInputs':len(rows),'status':'native-observed-cpp-validation-pending','counts':{kind:sum(row[0].startswith(kind+',') for row in rows) for kind in ENTRIES},'boundaries':['같은PC 전체정밀 범위의004b4860/0041c8b0/0041ca70/0041c320과004b4900/0040aec0/0049a840 실제명령을 실행한다. 외부함수 hook 대체0','0x67 부모는outpost0/missionPost1 자연분기를 사용하며87옵션을 복사한 뒤 실제순수하위를 실행한다','x87 기본control word027f와masked예외를 사용한다. 예외플래그/호스트별비정상trap은 대조대상이 아니다','Unicorn의FLD/FSTP는signaling NaN을그대로저장한다. 실제현재CPU의동일명령쌍은quiet로변환하므로해당입력은원본fixture에서제외하고분리진단한다','정상반환stack/EBX/ESI/EDI/EBP/전체실제쓰기guard/범위float비트/선택TYPE비용188/사용표256+이웃8/옵션87/현재파생값을 보존한다','제품검사와 실제Session/Client 연결 및 게임GUI는 아직 실행하지 않았다'],'host_x87_diagnostic':host_x87_diagnostic(),'sha256':{path:digest(ROOT/path) for path in paths if (ROOT/path).is_file()}}
    report.write_text(json.dumps(facts,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('미션 옵션 원본 고유입력:',len(rows))


if __name__=='__main__':main()
