#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""영역 Remove 진단 중 후보가 free/다른 타입이 된 뒤 실제 setter가 계속되는 경계를 관측한다."""
import argparse
import csv
import hashlib
import json
import struct
from decomp_graph_detach_live import DetachOracle, SPECS, IDS, TABLE, STACK
from decomp_surface_oracle import ROOT, POOL, SPOTS
from unicorn.x86_const import UC_X86_REG_ESP

# 일반 영역 생성과 기존 실제 setter까지 검토한 양판본 진입점이다.
REGION={'originals':(0x462d40,[0x41def0,0x41d770,0x41d7a0,0x44daa0,0x462a50]),'originalCD':(0x45c210,[0x440a50,0x440a80,0x40f180,0x45b530])}


class RegionOracle(DetachOracle):
    """영역 부모와 실제 일반 finder/Remove/setter를 실행하며 진단만 반환한다."""
    def __init__(self,edition):
        """검토한 추가 몸체만 최신 정밀 범위로 허용한다."""
        super().__init__(edition);entry,helpers=REGION[edition];self.entries['Region']=entry
        with (ROOT/f'extracted/refined/{edition}/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 함수 사이의 미검토 바이트는 허용하지 않는다.
        for address in [entry]+helpers:
            # 불연속 몸체의 inclusive 양 끝만 변환한다.
            for part in facts[address]['ranges'].split(';'):
                lo,hi=(int(value,16) for value in part.split('-'));self.allowed.append((lo,hi+1))

    def on_instruction(self,mu,address,size,data):
        """Remove의 첫 assertion에서 같은 raw 후보를 free 또는 비표면으로 바꾼다."""
        if self.stubs.get(address)=='assert':
            esp=mu.reg_read(UC_X86_REG_ESP);line=struct.unpack('<I',mu.mem_read(esp+12,4))[0]
            if line==0x59 and self.action in (9,10):
                mu.mem_write(POOL+IDS[0]*self.squid_stride+11,b'\x01')
                if self.action==10:mu.mem_write(POOL+IDS[0]*self.squid_stride+10,b'R')  # TYPE82는 표면이 아니다.
        return super().on_instruction(mu,address,size,data)

    def observe_region(self,checking,action):
        """초기 Disabled Detach는 자연 반환만 하며 그 뒤 영역의 독립 입력을 준비한다."""
        self.observe_detach('Disabled',checking,0,0);self.action=action;self.events=[]
        self.mu.mem_write(SPOTS+20*256+20,b'\x08');self.mu.mem_write(TABLE+3*6,bytes(4))
        self.mu.mem_write(self.detach['enabled'],struct.pack('<I',1));self.call('Region',[IDS[0]])
        slots=b''.join(bytes(self.mu.mem_read(POOL+sid*self.squid_stride,self.squid_stride)) for sid in IDS)
        return ['-',bytes(self.mu.mem_read(TABLE,255*6)).hex(),bytes(self.mu.mem_read(STACK,4096*4)).hex(),slots.hex(),
            str(struct.unpack('<I',self.mu.mem_read(self.checking,4))[0]),'|'.join(self.events) or '-','1','0']


def main():
    """기존 증거를 덮지 않고 독립 원문 관측을 기록한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    fixture=ROOT/'cpppj/tests/fixtures/graph-region-free-x86.tsv';report=ROOT/'cpppj/graph-region-free-native-20261103.json';rows=[];binaries={}
    # checking0이면 원문은 callback을 호출하지 않아 같은 입력이 그대로 유지된다.
    for edition in SPECS:
        oracle=RegionOracle(edition);binaries[edition]=oracle.sha256
        # 높은 비트의 비영 관문도 실제 DWORD 그대로 관측한다.
        for checking in (0,0x80000000):
            # 9는free만,10은free+비표면 타입으로 바꾸는 입력이다.
            for action in (9,10):rows.append([edition,'Region:0',str(checking),str(action)]+oracle.observe_region(checking,action))
    output=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(output).hexdigest()
    if args.verify:assert fixture.read_bytes()==output,'영역 free 복귀의 원문 재관측 불일치'
    else:
        if fixture.exists() or report.exists():raise FileExistsError('기존 증거에는 --verify를 사용하십시오')
        fixture.write_bytes(output);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','binaries':binaries,
            'rows':len(rows),'fixtureSha256':digest,'boundary':'실제 영역/일반 finder/Remove/setter·진단만 명시 반환·게임/GUI/OS/소켓0'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify}))


if __name__=='__main__':main()
