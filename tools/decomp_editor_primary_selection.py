# -*- coding: utf-8 -*-
"""편집 좌클릭 후처리와 다리 순환의 원본 x86 출력을 새 관측에만 고정한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
from decomp_editor_bridges import BridgeOracle, ROOT, SCRATCH, POOL as BRIDGE_POOL, BRIDGE, CODES
from decomp_object_owner_command import OwnerOracle, POOL, SID, TABLE
from decomp_oracle import STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

CYCLE_CODES=CODES+bytes([67,80,20,64,68,80,20,64])  # Cracked 부재→Normal 복구와 둘 다 부재인 방향을 추가한 명시 입력 표다.


class PrimaryOracle(OwnerOracle):
    """4da2e8..357만 실행하고 가상74/Core/다리 호출을 명시 외부 경계로 관측한다."""
    def on_instruction(self, mu, address, size, data):
        """호출 인자와 순서는 CPU에서 읽으며 후처리 끝에서만 멈춘다."""
        if address == 0x4da477:
            mu.emu_stop();return
        if address == 0x4ac200:
            sid = (mu.reg_read(UC_X86_REG_ECX)-POOL)//50
            self.trace.append(f'F,{sid}')
            mu.reg_write(UC_X86_REG_EAX, self.flags)
            self.finish();return
        if address == 0x422640:
            sid = struct.unpack('<I',mu.mem_read(mu.reg_read(UC_X86_REG_ESP)+4,4))[0]
            self.trace.append(f'B,{sid}')
            self.finish();return
        return super().on_instruction(mu,address,size,data)

    def primary(self, editor, control, selected, owner, flags, mutation, revision):
        """보관 선택과 owner/Editor 변경 입력을 원문 기본블록에 설치한다."""
        self.trace,self.mutation,self.flags=[],mutation,flags
        self.mu.mem_write(POOL,bytes(0x1000))
        self.mu.mem_write(POOL+SID*50,struct.pack('<I',TABLE)+bytes(46))
        self.mu.mem_write(POOL+SID*50+34,bytes([owner]))
        self.mu.mem_write(TABLE+0x74,struct.pack('<I',0x4adf00))
        for address,value in ((0x5c8464,POOL),(0x5c85a4,editor),(0x5caea0,selected),(0x540c70,1),(0x59a8b0,revision)):
            # 선택 후처리가 실제 읽는 현재 전역만 제공한다.
            self.mu.mem_write(address,struct.pack('<I',value))
        self.mu.reg_write(UC_X86_REG_ESP,STACK)
        self.mu.mem_write(STACK+0x14,struct.pack('<I',0x10000000 if control else 0))
        self.mu.emu_start(0x4da2e8,STOP,count=3000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==0x4da477
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK
        values=[struct.unpack('<I',self.mu.mem_read(a,4))[0] for a in (0x59a8b0,0x5c85a4,0x5caea0)]
        values.append(self.mu.mem_read(POOL+SID*50+34,1)[0])
        return ';'.join(self.trace) or '-',','.join(map(str,values))


class CycleOracle(BridgeOracle):
    """422640과4ac200을 기존 실제 다리 하위 허용 범위에 추가한다."""
    def __init__(self):
        """기존 원문 하위·외부 경계를 그대로 사용한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as source:
            facts={int(row['entry'],16):row for row in csv.DictReader(source,delimiter='\t')}
        for address in (0x422640,0x4ac200):
            # 내보내기의 모든 함수 조각을 읽되 끝 주소는 inclusive다.
            for part in facts[address]['ranges'].split(';'):
                first,last=(int(x,16) for x in part.split('-'))
                self.allowed.append((first,last+1))
            self.function_entries.add(address)
        self.entries['Cycle']=0x422640


def save(name, lines, verify):
    """기존 fixture를 덮어쓰지 않고 새 관측 또는 바이트 재현 일치만 기록한다."""
    path=ROOT/'cpppj/tests/fixtures'/name
    encoded=('\n'.join(lines)+'\n').encode('utf-8')
    if verify:
        assert path.read_bytes()==encoded
    else:
        assert not path.exists();path.write_bytes(encoded)
    return dict(path=path.relative_to(ROOT).as_posix(),rows=len(lines)-1,sha256=hashlib.sha256(encoded).hexdigest())


def main():
    """원문 두 경로를 별도 관측하며 게임/창/통신을 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    args=parser.parse_args()
    primary=PrimaryOracle()
    primary_body=bytes(primary.mu.mem_read(0x4da2e8,0x4da358-0x4da2e8))  #편집 후처리의 실제 PE 기본블록 바이트다.
    lines=['# editor control selected owner flags mutation revision trace state(revision,editor,selected,ownerByte)']
    for case in itertools.product((0,1),(0,1),(0,SID),(0,1,2,255),(0,4),(0,3),(0,0xffffffff)):
        # 제어키·중립/자기/적/상위BYTE·가상효과 뒤 전역 변경·감김을 교차한다.
        trace,state=primary.primary(*case)
        lines.append('\t'.join([*(str(x) for x in case),trace,state]))
    first=save('editor-primary-selection-x86.tsv',lines,args.verify)
    cycle=CycleOracle();code_before=cycle.code_evidence();lines=['# frame flags seed pattern slots trace random']
    for frame,flags,seed,pattern in itertools.product(range(10),(0,4),(1,0x12345678,0xffffffff),(0,0x5a78)):
        # 첫20/40 중첩, 후보 부재와 각 방향/난수/WORD를 원문 함수에 직접 넣는다.
        cycle.prepare(seed,pattern)
        cycle.mu.mem_write(SCRATCH+BRIDGE*500+0x114,struct.pack('<I',len(CYCLE_CODES)//4))
        cycle.mu.mem_write(SCRATCH+0x17000,CYCLE_CODES)
        cycle.mu.mem_write(SCRATCH+BRIDGE*500+0xec,struct.pack('<I',flags))
        cycle.mu.mem_write(BRIDGE_POOL+5*50+36,struct.pack('<I',frame))
        cycle.call('Cycle',[5])
        assert not cycle.heap_live and cycle.allocations==cycle.frees
        lines.append('\t'.join(map(str,(frame,flags,seed,pattern,cycle.observe(),';'.join(cycle.trace) or '-',struct.unpack('<I',cycle.mu.mem_read(0x532710,4))[0]))))
    second=save('editor-bridge-cycle-x86.tsv',lines,args.verify)
    assert code_before==cycle.code_evidence() and primary_body==bytes(primary.mu.mem_read(0x4da2e8,len(primary_body)))
    record=dict(host=socket.gethostname(),lastFullDecompile='HJOW-Athlon / 2026-10-23',originalGameExecuted=False,verifiedExistingFixtures=args.verify,primary=first,primaryCode=dict(range='004da2e8-004da357',bytes=len(primary_body),sha256=hashlib.sha256(primary_body).hexdigest()),cycle=second,cycleCodesHex=CYCLE_CODES.hex(),cycleCode=code_before,codeImageUnchanged=True,originalSha256=cycle.sha256,replaced=dict(primary=['virtual+74','CoreSelect','Flags2','CycleBridge'],cycle=['operator_new(1024)','free','virtual+88']),scope='선택 후처리 기본블록256행과 실제 다리 순환120행. 입력 부모/Pick/Select/게임GUI 전체의 관측은 아님')
    (ROOT/'extracted/cpp-editor-primary-selection-748/native.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({key:record[key] for key in ('host','primary','cycle','verifiedExistingFixtures')},ensure_ascii=False))


if __name__=='__main__':
    main()  # 기존 원본과 관측 fixture를 수정하지 않는다.
