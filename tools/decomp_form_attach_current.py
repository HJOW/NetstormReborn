#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""공통 Form 부착 부모의 두 PE 명령만 관측한다. 실제 자식/게임/통신은 실행하지 않는다."""
import argparse
import hashlib
import itertools
import json
import socket
from pathlib import Path
from decomp_oracle import Oracle, ROOT, SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
POOL, VT, DEVICE, TYPE = 0x13000000, 0x14000000, SCRATCH+0x1000, SCRATCH+0x2000  # 독립 합성 입력/선택 장치 주소다.
FIXTURE = ROOT/'cpppj/tests/fixtures/form-attach-current-x86.tsv'  # 최초 유효 관측만 새 파일에 동결한다.
SPEC = {  # 정밀 소스와 PE에서 ret8/실제 두 인자를 확인한 부모다.
    'originals': dict(entry=0x4ac800, pool=0x5c8464, stride=50, genus=0x49a840, pop=0x4ac560, content=0x541088, assertion=0x4e0620),
    'originalCD': dict(entry=0x4ac2e0, pool=0x5395dc, stride=36, genus=0x4abae0, pop=0x4abfb0, content=0x51c978, assertion=0x490040)}
class AttachOracle(Oracle):
    """현재 genus/setter/Pop/진단만 명시 장치로 정상 복귀시킨다."""
    def __init__(self, edition):
        """보호 PE의 부모 범위와 격리 입력 슬롯을 준비한다."""
        self.s = SPEC[edition]  # 현재 판본의 실제 함수/전역이다.
        super().__init__(edition, [dict(name='attach', **{edition: f'{self.s["entry"]:08x}'})], helpers=[])
        self.mu.mem_map(POOL, 0x10000)
        self.mu.mem_map(VT, 0x1000)
        self.events = []  # 원문 선택 호출 순서다.
        self.profile = 0  # 장치가 입력하는 명시 현재 상태 변경이다.
    def put(self, address, value, width=4):
        """명시 입력의 BYTE/WORD/DWORD만 쓴다."""
        self.mu.mem_write(address, (value & ((1 << (width*8))-1)).to_bytes(width, 'little'))
    def get(self, address, width=4):
        """현재 에뮬레이터의 실제 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address, width), 'little')
    def slot(self, sid, offset=0):
        """명시 장치가 같은 물리 입력 슬롯을 선택한다."""
        return POOL+sid*self.s['stride']+offset
    def resume(self, value=0, purge=0):
        """자식 호출의 스택 규약만 공급한다. 부모 ret8은 직접 실행한다."""
        sp = self.mu.reg_read(UC_X86_REG_ESP)
        self.mu.reg_write(UC_X86_REG_EAX, value)
        self.mu.reg_write(UC_X86_REG_ESP, sp+4+purge)
        self.mu.reg_write(UC_X86_REG_EIP, self.get(sp))
    def on_instruction(self, mu, address, size, data):
        """실제 호출 위치의 현재 인자/VT와 정상 복귀 후 부분 변경을 관측한다."""
        sp = mu.reg_read(UC_X86_REG_ESP)
        if address == self.s['assertion']:
            expression = self.read_string(self.get(sp+4)).decode('ascii')
            file = self.read_string(self.get(sp+8)).decode('ascii')
            self.events.append(f'D{expression}|{file}|{self.get(sp+12)}')
            if self.profile == 1:
                self.put(self.slot(5), VT+0x100)
                self.put(self.slot(5,10), 69, 1)
            if self.profile == 3 and expression == 'n != getSid()':
                self.put(self.slot(self.parent,6), 7, 2)
                self.put(self.slot(5,16), 11, 2)
            if self.profile == 5 and expression == 'getType()!=contentFormType':
                self.put(self.slot(5,10), 71, 1)
            self.resume()
            return
        if address == self.s['genus']:
            number = self.get(sp+4) if self.edition == 'originals' else self.get(self.slot(5,10),1)
            self.events.append(f'G{number}')
            self.put(TYPE+0xec, self.genus)
            self.resume(TYPE if self.edition == 'originals' else self.genus)
            return
        if DEVICE <= address < DEVICE+0x100:
            index = (address-DEVICE)//0x20
            kind = (address-DEVICE)%0x20
            sid = (mu.reg_read(UC_X86_REG_ECX)-POOL)//self.s['stride']
            value = self.get(sp+4)
            self.events.append(f'{"P" if kind==0 else "R"}{sid},{VT+index*0x100},{value}')
            self.put(self.slot(sid,16 if kind==0 else 14), value, 2)
            if self.profile == 2 and kind == 0:
                self.put(self.slot(5), VT+0x200)
                self.put(self.slot(self.parent,6), 7, 2)
                self.put(self.slot(5,16), 11, 2)
            if self.profile == 4 and kind != 0 and value == 5:
                self.put(self.slot(5), VT+0x100)
                self.put(self.slot(5,16), 11, 2)
                self.put(self.slot(self.parent,6), 13, 2)
            self.resume(purge=4)
            return
        if address == self.s['pop']:
            flags = self.get(sp+4)
            self.events.append(f'O{flags},{self.get(self.slot(5,11),1)},{self.get(self.slot(5,4),2)},{self.get(self.slot(5,14),2)},{self.get(self.slot(5,16),2)},{self.get(self.slot(self.parent,6),2)}')
            self.put(self.slot(5,11), self.get(self.slot(5,11),1) & ~4, 1)
            if self.profile == 5:
                self.put(self.slot(5,10), 70, 1)
                self.put(self.s['content'], 70)
            self.resume(purge=4)
            return
        super().on_instruction(mu,address,size,data)
    def observe(self, case):
        """합성 입력 뒤 원문 trace와 네 슬롯 전체를 읽는다. 기대값 모델은 사용하지 않는다."""
        self.parent, number, self.genus, head, self.profile, flags = case
        self.events = []
        self.mu.mem_write(POOL, bytes((i*37+19)&255 for i in range(16*self.s['stride'])))
        self.put(self.s['pool'],POOL)
        self.put(self.s['content'],6)
        for index, sid in enumerate((5,7,9)):  # 현재 VT의 두 setter 주소를 명시 장치에 연결한다.
            self.put(self.slot(sid), VT+index*0x100)
            self.put(VT+index*0x100+8, DEVICE+index*0x20)
            self.put(VT+index*0x100+12, DEVICE+index*0x20+8)
        self.put(self.slot(5,10),number,1)
        self.put(self.slot(5,11),4,1)
        self.put(self.slot(self.parent,6),head,2)
        self.call('attach',[self.parent,flags],this=self.slot(5),purge=8)
        slots=';'.join(bytes(self.mu.mem_read(self.slot(sid),self.s['stride'])).hex() for sid in (0,5,7,9))
        return ';'.join(self.events),str(self.get(self.s['content'])),slots

def main():
    """최초 관측을 동결하거나 같은 입력을 다시 실행해 바이트를 확인한다."""
    parser=argparse.ArgumentParser()
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    cases=list(itertools.product((0,9),(1,6,70),(0,0x44200),(0,5,7),range(6),(0,0xa5)))
    rows=['# 공통 부착 PE 부모만 실행. genus/setter/Pop/진단은 명시 장치이며 게임/통신 미실행.']
    binaries={}
    for edition in SPEC:  # 두 판본 원문을 서로 독립 실행한다.
        oracle=AttachOracle(edition)
        for case in cases:  # 첫 원문 결과의 trace/현재 DWORD/전체 슬롯만 동결한다.
            rows.append('\t'.join((edition,','.join(map(str,case)),*oracle.observe(case))))
        binaries[edition]=oracle.sha256
    data=('\n'.join(rows)+'\n').encode('utf-8')
    if args.verify:
        assert FIXTURE.read_bytes()==data
    else:
        with FIXTURE.open('xb') as stream:
            stream.write(data)
    report=dict(host=socket.gethostname(),rows=len(rows)-1,rowsPerEdition=len(cases),binaries=binaries,fixtureSha256=hashlib.sha256(data).hexdigest(),toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),mode='verify' if args.verify else 'first',actualChildren=False,game=False,multiplayer=False)
    args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(report['mode'],report['rows'],'공통 Form 부착 부모 관측',flush=True)
if __name__=='__main__':
    main()
