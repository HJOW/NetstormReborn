#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 10.78의 비행39 생성자/Init/frame을 게임 실행 없이 x86으로 관측한다."""
import csv
import json
import argparse
import bisect
import struct
import pefile
from decomp_pathruntime_oracle import PathOracle, ROOT, PATH, POOL, PARENT, dbits, bits, fnv, digest
from decomp_pathruntime_oracle import RewardOracle, SPEC, ARENA, STACK, HALT, ARRIVAL, UNPOP, POP, REPOP

# 새 관측물은 기존23/54의 fixture를 덮어쓰지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/flypath-x86.tsv'
REPORT=ROOT/'cpppj/flypath.json'
TABLE=ROOT/'cpppj/tests/fixtures/construction-path-vtables.tsv' # 실제 부모/경로 VT의 독립 DWORD 관측이다.

def vtable_text():
    """보호 PE의 정확한 부모/경로 VT를 직접 읽고 함수 본문 추측을 넣지 않는다."""
    image=pefile.PE(str(ROOT/'originals/Netstorm.exe')).get_memory_mapped_image();rows=['# 부모와 이동의 실제 보호 PE 가상 표; 기대값 재계산 없이 DWORD를 저장한다.','type\tvtable\toffset\tbody']
    # 실제 생성자로 확인한 네 가상 표다.
    for typ,vt in ((134,0x5070c0),(105,0x509810),(23,0x50e284),(39,0x50e2ec)):
        # 부모 수명/Pop/표시와 경로 Init/프레임/도착/Attach를 구분한다.
        for offset in ((0x10,0x14,0x18,0x24,0x38,0x3c,0x40,0x44,0x48,0x7c,0x80,0x84,0x88,0x8c) if typ>70 else (0x18,0x20,0x24,0x28,0x2c,0x30)):
            rows.append(f'{typ}\t{vt:08x}\t{offset:02x}\t{struct.unpack_from("<I",image,vt-0x400000+offset)[0]:08x}')
    return '\n'.join(rows)+'\n'
class FlyOracle(PathOracle):
    """실제 비행 가상 표를 유지하며 새 Init과20ms 예약 몸체를 허용한다."""
    def __init__(self):
        """같은 PC의 정밀 내보내기 범위에서 주소를 확인한다."""
        RewardOracle.__init__(self,'originals');self.m=SPEC['originals'];self.allowed=[];self.entry_names={};self.mu.mem_map(0,0x1000)
        self.mu.mem_map(ARENA+0x40000,0x20000)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            # 현재 정밀 내보내기에 있는 실제 몸체만 허용하고 실행한 명령 주소는 따로 센다.
            for row in csv.DictReader(stream,delimiter='\t'):
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.external={self.m['malloc'],self.m['sound'],self.m['assertion'],self.m['kill'],self.m['freeze'],HALT,ARRIVAL,UNPOP,POP,REPOP}
        self.writes=[(ARENA,ARENA+0x60000),(STACK-0xf000,STACK+0x1000),(0,4),(self.m['now'],self.m['now']+8),(self.m['observe'][0],self.m['observe'][0]+4),(self.m['observe'][1],self.m['observe'][1]+8)]
        self.events=[];self.return_code=1002;self.halt_code=10;self.deleted=False;self.frozen=False
        self.ranges=sorted(self.allowed);self.starts=[low for low,high in self.ranges];self.instructions=0

    def on_instruction(self,mu,address,size,data):
        """수천 범위를 명령마다 선형 탐색하지 않고 같은 실제 범위를 이진 검색한다."""
        index=bisect.bisect_right(self.starts,address)-1
        self.instructions+=1
        self.allowed=[self.ranges[index]] if index>=0 else []
        return PathOracle.on_instruction(self,mu,address,size,data)

    def reset_fly(self,fill=0):
        """부모/Host 외부 입력은 기존 관측기와 같고 실제 생성자만39로 교체한다."""
        self.reset(0,fill)
        self.mu.mem_write(PATH,bytes([fill])*self.m['size']);self.invoke(0x48bea0)

def generate():
    """기대값을 재계산하지 않고 원본 반환 후 메모리/효과를 저장한다."""
    oracle=FlyOracle();rows=[]
    # 원본 생성자의 미초기화 heap 보존 범위를 전체 이미지로 관찰한다.
    for fill in (0,0x5a,0xff):
        oracle.reset_fly(fill);rows.append(['ctor',fill,fnv(oracle.mu.mem_read(PATH,oracle.m['size']))])
    # 기존 count를 지우지 않는 Init과 축/동률 방향을 교차한다.
    for count in (0,1,3):
        for x,y in ((10,20),(13.5,20),(10,24.75),(6.25,20),(10,15.5),(14,24),(6,16),(12.25,22.25)):
            oracle.reset_fly();oracle.put(PATH+0x10,count);oracle.put(PATH+oracle.m['current'],2)
            oracle.invoke(0x48af00,[PARENT,bits(x),bits(y),0x12345678,0xabcdef01],PATH)
            rows.append(['init',count,bits(x),bits(y),fnv(oracle.mu.mem_read(PATH,oracle.m['size']))])
    # 예약 직전/같음/직후와 도착 재시도를 같은 실제 frame 몸체로 실행한다.
    for current in (0,1,2):
        for now in (0.0,0.01,0.02,0.5,1.0,2.0,3.0):
            for next_frame in (0.0,0.02,1.0):
                oracle.reset_fly();oracle.put(PATH+4,7);oracle.put(PATH+8,PARENT)
                oracle.path([(10,20),(15,20),(17,22)],[0,0,2],[0,1,3]);oracle.put(PATH+oracle.m['current'],current)
                oracle.put(PATH+0x186b8,dbits(next_frame),8);oracle.put(oracle.m['now'],dbits(now),8)
                oracle.return_code=1002;oracle.halt_code=10;oracle.frozen=False;oracle.events=[]
                oracle.invoke(0x48af90,this=PATH);fields,events=oracle.observed()
                rows.append(['frame',current,dbits(now),dbits(next_frame),fields,events,oracle.u32(PATH+0x186b8),oracle.u32(PATH+0x186bc)])
    FIXTURE.write_text('# kind input... observed\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    TABLE.write_text(vtable_text(),encoding='utf-8',newline='\n')
    files=['tools/decomp_flypath_oracle.py','tools/decomp_pathruntime_oracle.py','cpppj/src/o/RawPathRuntime.cpp','cpppj/src/o/RawPathRuntime.h','cpppj/src/o/RawPathProcess.cpp','cpppj/src/o/RawPathProcess.h','cpppj/tests/RawPathRuntimeTests.cpp','extracted/refined/originals/functions.tsv','originals/Netstorm.exe',str(FIXTURE.relative_to(ROOT)).replace('\\','/')]
    files.append(TABLE.relative_to(ROOT).as_posix())
    REPORT.write_text(json.dumps({'host':'HJOW-Athlon','decompiled_host':'HJOW-Athlon / 2026-10-16','rows':len(rows),'source_sha256':{p:digest(ROOT/p) for p in files},'instructions':oracle.instructions,'external_calls':dict(oracle.stub_hits),'scope':'실제39 생성자/Init/frame; x87 53비트; 부모 가상 효과/Host 삭제/공간 장치는 기존 관측기의 명시 경계; 게임/GUI 실행 없음'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'비행39 원본 관측 {len(rows)}행, {oracle.instructions}명령')

def verify():
    """관측 이후 소스/원본/fixture가 바뀌었으면 조용히 통과시키지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for key in ('source_sha256','currentSources','protectedFileSha256'):
        for path,sha in report.get(key,{}).items():
            if digest(ROOT/path)!=sha:raise SystemExit('SHA 불일치: '+path)
    rows=[row for row in FIXTURE.read_text(encoding='utf-8').splitlines() if row and not row.startswith('#')]
    assert len(rows)==report['rows']==90 and report['instructions']>0
    assert TABLE.read_text(encoding='utf-8')==vtable_text()
    print('비행39 관측 SHA 확인')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else generate()
