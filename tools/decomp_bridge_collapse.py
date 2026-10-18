#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""10.78의 다리 낙하 wrapper와 보호60프레임의 실제 소유자 캡처를 별도 동결한다."""
import argparse
import csv
import itertools
import json
import socket
import struct
from decomp_owner_capture import Oracle as CaptureOracle,ROOT,STACK,STOP,digest,POOL,TYPES,BUFFER,OUT,SHAPE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/bridge-collapse-x86.tsv'  # C++ 연결 전에 저장하는 전체 원문이다.
REPORT=ROOT/'cpppj/bridge-collapse.json'  # 과거 파편/캡처/다리 자료는 바꾸지 않는다.
WRAPPER=0x460600  # Bridge postDestroy가 직접 호출하는 실제 낙하 파편 wrapper다.


class Oracle(CaptureOracle):
    """실제 색표·캡처·RLE는 실행하고 wrapper의 생성 효과만 명시 경계로 관찰한다."""
    def __init__(self):
        """기존 캡처 환경에 실제16바이트 wrapper 범위만 더한다."""
        super().__init__();self.wrapper=False;self.arguments=[];self.instructions=0
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            row=next(row for row in csv.DictReader(stream,delimiter='\t') if int(row['entry'],16)==WRAPPER)
        # 함수 사이의 바이트는 실행 허용 범위에 포함하지 않는다.
        for part in row['ranges'].split(';'):
            low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def instruction(self,mu,address,size,data):
        """원본 wrapper의 실제 두 인자와 ABI를 기록하고 나머지 계산은 기존 제한을 유지한다."""
        self.instructions+=1
        if self.wrapper and address==0x4604a0:
            esp=mu.reg_read(UC_X86_REG_ESP);ret,source,variant=struct.unpack('<3I',mu.mem_read(esp,12));self.arguments.append((source,variant))
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if address==WRAPPER:self.calls[f'{address:08x}']+=1
        super().instruction(mu,address,size,data)

    def wrapper_call(self,source,precision):
        """실제460600이 source와 kind2를 전달하고 정상 cdecl 반환하는지 관찰한다."""
        self.wrapper=True;self.arguments=[]
        try:self.call(WRAPPER,0,struct.pack('<I',source),0,precision)
        finally:self.wrapper=False
        self.returns+=1;assert len(self.arguments)==1 and self.arguments[0][0]==source
        return self.arguments[0][1]

    def bridge_capture(self,frame,owner,color,precision):
        """실제 다리 VT/보호 헤더/기준점0과 현재 owner 색으로 전체 메모리 픽셀을 읽는다."""
        mu=self.mu;self.maps(82,82,self.candidates,precision);self.mode=1;self.allocations=[];block=self.blocks[12]
        record=bytearray(500);struct.pack_into('<I',record,0,20);struct.pack_into('<I',record,0xdc,SHAPE+block.offset);struct.pack_into('<I',record,0x114,60);struct.pack_into('<2i',record,0x1dc,0,0);mu.mem_write(TYPES+82*500,bytes(record))
        raw=bytearray(50);struct.pack_into('<I',raw,0,0x5034c8);raw[10]=82;raw[34]=owner;struct.pack_into('<i',raw,36,frame);mu.mem_write(POOL+2500,bytes(raw))
        mu.mem_write(0x5c8464,struct.pack('<I',POOL));mu.mem_write(0x59ab20,struct.pack('<I',TYPES));mu.mem_write(0x59a918,struct.pack('<I',65536));mu.mem_write(0x542414,struct.pack('<I',1));mu.mem_write(0x531b08+owner*4,struct.pack('<I',color))
        context=struct.pack('<5I',0x12345678,11,22,33,44);mu.mem_write(0x5b5ddc,context);mu.mem_write(BUFFER,b'\xa5'*0x10000);mu.mem_write(OUT,bytes(16));before=bytes(mu.mem_read(POOL+2500,50))
        self.call(0x4a7db0,0,struct.pack('<5I',50,OUT,OUT+4,OUT+8,OUT+12),0,precision,result=BUFFER);self.returns+=1
        metadata=bytes(mu.mem_read(OUT,16));width,height=struct.unpack('<2i',metadata[:8]);assert self.allocations==[width*height]
        assert bytes(mu.mem_read(0x5b5ddc,20))==context and bytes(mu.mem_read(POOL+2500,50))==before
        return metadata.hex(),bytes(mu.mem_read(BUFFER,width*height)).hex()


def inputs(smoke=False):
    """모든60프레임과 owner0 우회/9개 실제 색 번호를 교차한다."""
    return [(frame,owner,color) for frame,(owner,color) in itertools.product((0,59) if smoke else range(60),[(0,8),*((1,color) for color in range(9))])]


def main():
    """원본 출력의 새 생성과 기존 SHA/입력 행 감사 중 하나를 실행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        # 원본 기대값을 다시 계산하거나 기존 자료를 수정하지 않는다.
        for name,wanted in report['sha256'].items():assert digest(ROOT/name)==wanted,name
        rows=[row.split('\t') for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')]
        assert len(rows)==603 and sum(row[0]=='C' for row in rows)==600 and sum(row[0]=='W' for row in rows)==3
        assert [(int(row[1]),int(row[2]),int(row[3])) for row in rows if row[0]=='C']==inputs()
        print('다리 wrapper/보호60프레임 캡처 SHA 감사:603행');return
    oracles=[Oracle(),Oracle()];rows=[]
    # wrapper 출력은 생성 효과를 대체하는 명시 경계이며 캡처와 합쳐진 전체 수명 증명은 아니다.
    for source in (0,50,65535):
        values=[oracle.wrapper_call(source,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert values[0]==values[1];rows.append(('W',str(source),str(values[0])))
    # 원본 반환값과 픽셀만 저장하며 C++ 식으로 기대값을 만들지 않는다.
    for frame,owner,color in inputs(args.smoke):
        outputs=[oracle.bridge_capture(frame,owner,color,precision) for oracle,precision in zip(oracles,(0x027f,0x037f))];assert outputs[0]==outputs[1];rows.append(('C',str(frame),str(owner),str(color),*outputs[0]))
    if args.smoke:print('다리 wrapper/양 끝 프레임·9색 실제 캡처 smoke 정상 반환');return
    FIXTURE.write_text('# W source variant / C frame owner color metadata16 pixels\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    paths=('originals/Netstorm.exe','originals/d/_shapes.shp','originals/netstorm.tarc','extracted/refined/originals/functions.tsv','tools/decomp_particle_gradient.py','tools/decomp_owner_capture.py','tools/shp.py','tools/decomp_bridge_collapse.py',FIXTURE.relative_to(ROOT).as_posix())
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',unit=180,rows=len(rows),captureInputs=600,wrapperInputs=3,normalReturns=sum(oracle.returns for oracle in oracles),instructions=sum(oracle.instructions for oracle in oracles),precisionDifferences=0,nativeCalls=dict(oracles[0].calls),sha256={name:digest(ROOT/name) for name in paths},limits=['실제460600의 source/kind2 전달과 cdecl 반환을 관찰한다.4604a0 생성 효과는 여기서 명시 경계이며 기존 파편 수명648행과 구별한다.','실제 일반 색표/현재 owner 선택/보호60프레임/캡처/RLE의600입력 전체 픽셀이다. new만 격리 heap이며 양정밀도 차이0이다.','현재 TYPE/기준점0/단위 배율/소유자 색 입력이다. 전체 초기화/추가 가상 그림/CD판/전체 다리 파괴 수명의 새 원본 대조로 확대하지 않는다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('다리 파괴 독립 원본',len(rows),report['normalReturns'],report['instructions'])


if __name__=='__main__':main()
