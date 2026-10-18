#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Kernel의 표시 프레임 진입 gate487940을 관측한다."""
import argparse,json,struct
from decomp_burningprocess_oracle import BurningOracle,ROOT,OBJECT,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/render-frame-gate-x86.tsv' # 새 실제 gate 출력이며 파생Frame 결과가 아니다.
REPORT=ROOT/'cpppj/render-frame-gate.json' # 현재 전역 프레임과 등록slot의 원본 판정을 기록한다.


class Oracle(BurningOracle):
    """gate 명령은 실행하며 파생Frame 진입만 관찰한다."""
    def __init__(self):
        """정확한24바이트와 Burning 가상표의 두 슬롯을 확인한다."""
        super().__init__('originals');expected=bytes.fromhex('8b411c83f8077c0a3305a8b45500a80375058b01ff6024c3');assert bytes(self.mu.mem_read(0x487940,len(expected)))==expected;self.allowed.append((0x487940,0x487940+len(expected)));self.hit=0
        assert self.read(0x503730+0x18)==0x487940 and self.read(0x503730+0x24)==0x424530

    def on_instruction(self,mu,address,size,data):
        """실제 가상+24가 선택됐을 때만 호출을 기록한다."""
        if address==0x424530:
            self.hit+=1;esp=mu.reg_read(UC_X86_REG_ESP);mu.reg_write(UC_X86_REG_EIP,self.read(esp));mu.reg_write(UC_X86_REG_ESP,esp+4);return
        super().on_instruction(mu,address,size,data)

    def run(self,slot,frame):
        """정상반환과 실제 선택 여부만 읽는다."""
        self.hit=0;self.mu.mem_write(OBJECT,struct.pack('<I',0x503730));self.mu.mem_write(OBJECT+28,struct.pack('<i',slot));self.mu.mem_write(0x55b4a8,struct.pack('<I',frame));self.mu.reg_write(UC_X86_REG_ECX,OBJECT);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<I',STOP));self.mu.emu_start(0x487940,STOP,count=100)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4;return self.hit


def main():
    """128개의 실제 slot/프레임 교차 결과를 동결 또는 검증한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        print('표시 프레임gate128 원문/해시 일치');return
    oracle=Oracle();rows=[f'{slot}\t{frame}\t{oracle.run(slot,frame)}' for slot in (-1,0,6,7,8,9,15,1023) for frame in range(16)];FIXTURE.write_text('# 실제487940 표시 slot/현재frame의 파생+24 선택. 파생Frame 자체는 관찰 경계다.\n'+'\n'.join(rows)+'\n','utf-8',newline='\n')
    paths=['originals/Netstorm.exe','tools/decomp_render_frame_gate.py','tools/decomp_burningprocess_oracle.py',FIXTURE.relative_to(ROOT).as_posix()];REPORT.write_text(json.dumps(dict(unit=190,rows=len(rows),instructions=oracle.instructions,sha256={path:digest(ROOT/path) for path in paths}),indent=2)+'\n','utf-8',newline='\n');print('표시 gate128 정상 반환')


if __name__=='__main__':main()
