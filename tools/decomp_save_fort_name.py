#!/usr/bin/env python3
"""Save as 확정/취소의 실제 x86 부모를 명시 문자·경로·전체이름 장치에서 독립 관찰한다."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

# 현재 원문 부모와 명시 장치의 가상 자료이며 실제 파일/게임을 쓰지 않는다.
PARENT, HEADER, PATH = SCRATCH+0x1000, 0x5c89f0+20, SCRATCH+0x3000
FIXTURE = ROOT/'cpppj/tests/fixtures/save-fort-name-x86.tsv'
REPORT = ROOT/'cpppj/save-fort-name-native.json'


class NameOracle(Oracle):
    """원문 LF/치환/조건/readonly 직접쓰기를 실제 실행하고 외부 장치만 관찰한다."""
    def __init__(self):
        """확정 함수 범위와 현재 export에 누락된22바이트 Cancel leaf를 명시 허용한다."""
        super().__init__('originals',[dict(name='Confirm',originals='004cf9a0')],[0x4e63e0])
        self.entries['Cancel']=0x4cfab0
        self.allowed.append((0x4cfab0,0x4cfad2))
        self.devices={0x451e80:'copy',0x4e7054:'alnum',0x460650:'path',0x4dd560:'exists',
            0x460980:'set',0x4cf730:'tell',0x4c7910:'store',0x452fe0:'queue'}

    def put(self, address, value):
        """명시 입력과 장치 반환의 실제 DWORD를 쓴다."""
        self.mu.mem_write(address,struct.pack('<I',value & 0xffffffff))

    def on_instruction(self, mu, address, size, data):
        """이름을 결정하는 부모 loop는 대체하지 않고 CRT/OS/UI 경계를 별도 표기한다."""
        kind=self.devices.get(address)
        if not kind:
            return super().on_instruction(mu,address,size,data)
        sp=mu.reg_read(UC_X86_REG_ESP)
        values=struct.unpack('<4I',mu.mem_read(sp,16))
        result,purge=0,0
        if kind=='copy':
            self.trace.append('copy')
            if values[2]!=256: raise RuntimeError('CopyText 원문256폭 불일치')
            mu.mem_write(values[1],self.text+b'\0');purge=8
        elif kind=='alnum':
            code=values[1]
            if code>127: raise RuntimeError('이번 명시 CRT 관찰은 ASCII 입력만 허용')
            self.trace.append('alnum:'+str(code))
            result=int(48<=code<=57 or 65<=code<=90 or 97<=code<=122)
        elif kind=='path':
            name=self.read_string(values[1]);self.trace.append('path:'+name.hex())
            mu.mem_write(PATH,b'd/'+name+b'.fort\0');result=PATH
        elif kind=='exists':
            self.trace.append('exists');result=self.exists
        elif kind=='set':
            self.name=self.read_string(values[1]);readonly=struct.unpack('<I',mu.mem_read(HEADER+8,4))[0]
            self.trace.append('set:'+self.name.hex()+':'+str(readonly))
        elif kind=='tell':
            text=self.read_string(values[1]);self.trace.append('tell:'+text.decode('ascii'))
        elif kind=='store':
            self.name=self.read_string(values[1]);self.trace.append('store:'+self.name.hex());result=0x11223344
        elif kind=='queue':
            self.trace.append('queue:'+','.join(map(str,values[1:4])))
        mu.reg_write(UC_X86_REG_EAX,result);mu.reg_write(UC_X86_REG_EIP,values[0]);mu.reg_write(UC_X86_REG_ESP,sp+4+purge)

    def run(self, action, mode, cancel, manager, readonly, exists, text):
        """새 명시 입력을 설치하고 실제 ret/현재 header/장치 trace를 반환한다."""
        self.trace,self.name,self.text,self.exists=[],b'',text,exists
        self.mu.mem_write(PARENT,bytes(0xdc));self.put(PARENT+0xd0,SCRATCH+0x2000)
        self.put(PARENT+0xd4,mode);self.put(PARENT+0xd8,cancel)
        self.put(0x540cac,1);self.put(HEADER,0x12345678 if manager else 0);self.put(HEADER+8,readonly)
        self.put(0x542a1c,0x020000ce)
        result=self.call(action,[],PARENT)
        return result,struct.unpack('<I',self.mu.mem_read(HEADER+8,4))[0],self.name.hex() or '-', ';'.join(self.trace) or '-'


def inputs():
    """빈/LF/금지 기호/공백·모드/현재manager/읽기전용/존재 입력을 독립 교차한다."""
    names=(b'',b'Alpha',b'Alpha_ 01',b'Alpha\nIgnored',b'\nIgnored',b'a.b/c?d',b'   ',b'_A-+!')
    return [('Confirm',*values,name) for values in itertools.product((0,1),(0,1),(0,1),(0,1),(0,1)) for name in names]+[
        ('Cancel',mode,cancel,manager,readonly,exists,b'Alpha') for mode,cancel,manager,readonly,exists in itertools.product((0,1),repeat=5)]


def main():
    """최초 자료는 새 파일에만 쓰고 verify는 전체 실제 부모를 재실행하여 기존 바이트와 비교한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if FIXTURE.exists() and not args.verify: raise RuntimeError('최초 독립 fixture 덮어쓰기 금지')
    oracle=NameOracle();rows=[]
    # callback 결과는 원문 CPU 반환이며 C++ 구현을 불러 기대값을 만들지 않는다.
    for action,mode,cancel,manager,readonly,exists,name in inputs():
        result=oracle.run(action,mode,cancel,manager,readonly,exists,name)
        rows.append('\t'.join(map(str,(action,mode,cancel,manager,readonly,exists,name.hex() or '-',*result))))
    encoded=('# action/mode/cancel/manager/readonly/exists/inputhex/result/afterReadonly/namehex/trace\n'+'\n'.join(rows)+'\n').encode('utf-8')
    if args.verify:
        if FIXTURE.read_bytes()!=encoded: raise RuntimeError('Save as 최초 fixture와 재관찰 불일치')
    else: FIXTURE.write_bytes(encoded)
    report=dict(host='HJOW-Athlon',lastFullDecompilation='HJOW-Athlon / 2026-10-23',observations=len(rows),gameRuns=0,osCalls=0,
        binarySha256=oracle.sha256,fixtureSha256=hashlib.sha256(encoded).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        cancelLeaf=dict(entry='004cfab0',range='004cfab0..004cfad1',bytes=bytes(oracle.mu.mem_read(0x4cfab0,34)).hex()),
        limits=['실제 확정/취소 부모의 반환/readonly직접쓰기/선택된 장치 순서다. CopyText·ASCII CRT 분류·path/exists·전체이름·Tell·고리/queue는 명시 경계다.',
            '전체 GUI/전체이름부모/snapshot/파일저장 완료는 실제 공통 장치 통합검사와 별도 게임증거로 확인해야 한다.'])
    REPORT.write_bytes((json.dumps(report,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__': main()
