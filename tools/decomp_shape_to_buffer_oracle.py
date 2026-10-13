#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 ShapeToBuffer와 보호 Dude SHP의 메모리 RLE 출력만 두 PE에서 관찰한다."""
import argparse
import csv
import json
import struct
import itertools
from pathlib import Path
from decomp_dudespawn_oracle import DudeSpawnOracle
from decomp_postpop_oracle import ARENA
from decomp_creation_oracle import TYPES
from decomp_sid_oracle import POOL,SidOracle
from decomp_oracle import ROOT
from decomp_owner_oracle import digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_GDTR,UC_X86_REG_CS,UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,UC_X86_REG_FS,UC_X86_REG_GS

# 캡처 진입/heap/문맥/배율/추가핫스폿 전역의 실제 주소다.
SPEC={
    'originals':dict(entry=0x4a7db0,new=0x4e4391,context=0x5b5ddc,scale=0x59a918,extra=0x542414,hot=0x1dc),
    'originalCD':dict(entry=0x45b100,new=0x4f1650,context=0x55bb18,scale=0x565c30,extra=0x5207b0,hot=0x1bc),
}
# 이전 population/player와 겹치지 않는 현재 출력버퍼/네 정수 인자다.
BUFFER=ARENA+0x6000
OUT=ARENA+0x9000
GDT=ARENA+0xa000 # VFX의 실제 push ds/pop es 뒤에도32비트 평면 스택을 유지하는 격리 descriptor다.
FIXTURE=ROOT/'cpppj/tests/fixtures/shape-to-buffer-x86.tsv' # 전체 실제 픽셀과 메타데이터를 저장한다.
REPORT=ROOT/'cpppj/shape-to-buffer.json' # 기존 생성/입자/Sources 격자와 별도 출처다.


class ShapeToBufferOracle(DudeSpawnOracle):
    """실제 TYPE/header/RLE를 실행하며 요청한 픽셀heap만 격리 버퍼로 공급한다."""
    def __init__(self,edition):
        """기존 실제 원본 환경과 새 세 단계 helper 범위를 연결한다."""
        super().__init__(edition);self.c=SPEC[edition];self.captureActive=False;self.heap=[]
        self.flat_segments()
        # 원본 wrapper와 VFX raster의 불연속 실제 범위를 각각 보존한다.
        for name in ('burning-frame-helper','burning-frame-paint-helper','burning-frame-raster-helper'):
            paths=[ROOT/f'extracted/{name}/{edition}/{part}' for part in ('creation.c','functions.tsv')];self.exports.extend(paths)
            with paths[1].open(encoding='utf-8') as fp:
                # 실제 함수 사이의 바이트는 허용하지 않는다.
                for row in csv.DictReader(fp,delimiter='\t'):
                    for part in row['ranges'].split(';'):
                        low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def flat_segments(self):
        """현재 입력 준비가 지운 격리 GDT도 복원해 실제32비트 평면 raster 환경을 공급한다."""
        self.mu.mem_write(GDT,struct.pack('<3Q',0,0x00cf93000000ffff,0x00cf9b000000ffff));self.mu.reg_write(UC_X86_REG_GDTR,(0,GDT,23,0));self.mu.reg_write(UC_X86_REG_CS,16)
        # 실제 raster가 DS를 ES로 복사해도 같은4GB/32비트 descriptor를 사용한다.
        for register in (UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,UC_X86_REG_FS,UC_X86_REG_GS):self.mu.reg_write(register,8)

    def on_instruction(self,mu,address,size,data):
        """heap만 현재 크기/순서로 기록하며 캡처/실제 그리기를 대체하지 않는다."""
        if hasattr(self,'c') and self.captureActive and address==self.c['new']:
            esp=mu.reg_read(UC_X86_REG_ESP);count=self.read(esp+4);ret=self.read(esp)
            if count>0x2000:raise RuntimeError('ShapeToBuffer 격리heap 범위 오류')
            self.heap.append(count);mu.reg_write(UC_X86_REG_EAX,BUFFER);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        SidOracle.on_instruction(self,mu,address,size,data)

    def run_capture(self,frame,scale=65536,extra=0,state=0,vtable=None):
        """명시 현재 raw/frame/배율만 공급하고 실제 전체 메모리 출력/문맥 복원을 관찰한다."""
        self.captureActive=False;self.prepare(dict(capacity=0,count=0,blocked=0,x=0,y=0,owner=0,repeat=0));self.flat_segments();mu=self.mu;patch=self.edition=='originals';typ=TYPES+70*self.creation['type_stride'];raw=bytearray(self.stride);struct.pack_into('<I',raw,0,self.p['vtable'] if vtable is None else vtable);raw[10]=70;raw[11]=state
        if patch:struct.pack_into('<i',raw,36,frame)
        else:raw[34]=frame&255
        mu.mem_write(POOL+50*self.stride,bytes(raw));mu.mem_write(self.c['scale'],struct.pack('<I',scale));mu.mem_write(self.c['extra'],struct.pack('<I',extra));mu.mem_write(typ+self.c['hot'],struct.pack('<ii',-2,3));mu.mem_write(BUFFER,bytes([0x5a])*0x2000);mu.mem_write(OUT,bytes(16));self.heap=[]
        context=bytes(mu.mem_read(self.c['context'],20));self.write_ranges.extend([(self.c['context'],self.c['context']+20),(BUFFER,BUFFER+0x2000),(OUT,OUT+16)]);mu.reg_write(UC_X86_REG_FPCW,0x027f);mu.reg_write(UC_X86_REG_FPSW,0);self.captureActive=True
        self.invoke(self.c['entry'],[50,OUT,OUT+4,OUT+8,OUT+12]);self.captureActive=False
        values=struct.unpack('<4i',mu.mem_read(OUT,16));width,height=values[:2];pointer=mu.reg_read(UC_X86_REG_EAX)
        if pointer!=BUFFER or width<0 or height<0 or width*height>0x2000 or bytes(mu.mem_read(self.c['context'],20))!=context:raise RuntimeError('ShapeToBuffer 반환/전체 문맥 복원 오류')
        return dict(frame=frame,scale=scale,extra=extra,state=state,metadata=values,pixels=bytes(mu.mem_read(BUFFER,width*height)).hex(),heap=self.heap,context_restored=True)


def smoke():
    """두 실제 보호 프레임의 정상 반환을 조사하며 full fixture/report를 생성하지 않는다."""
    # 같은 입력을 원본 주소가 다른 두 판본에서 각각 직접 실행한다.
    for edition in SPEC:
        oracle=ShapeToBufferOracle(edition)
        for frame in (0,64):
            result=oracle.run_capture(frame);print(json.dumps(dict(edition=edition,**result),ensure_ascii=False),flush=True)


def generate():
    """같은 현재 입력의 정상 반환/실제 픽셀과 모든 원본 범위·보호 자산 SHA를 기록한다."""
    rows=[];editions={};files={Path(__file__)}
    # 서로 다른 원본 주소 공간에서 각 입력을 한 번씩 실행한다.
    for edition in SPEC:
        oracle=ShapeToBufferOracle(edition);cases=[(frame,scale,extra,state,oracle.p['vtable']) for frame,scale,extra,state in itertools.product((0,1,7,32,64),(32768,65536,98304,131072),(0,1),(0,2))]
        if edition=='originalCD':cases.extend([(0,65536,0,1,oracle.p['vtable']),(0,65536,0,0,0),(0,65536,0,0,0xdddddddd),(255,65536,0,0,oracle.p['vtable'])])
        # 메타데이터나 픽셀 기대값은 oracle 출력으로만 얻는다.
        for frame,scale,extra,state,vtable in cases:
            try:result=oracle.run_capture(frame,scale,extra,state,vtable)
            except Exception as error:raise RuntimeError(f'{edition} {(frame,scale,extra,state,vtable)}: {error}') from error
            if len(result['heap'])!=1:raise RuntimeError('ShapeToBuffer 실제 heap 호출 수 오류')
            rows.append([edition,frame,scale,extra,state,vtable,','.join(map(str,result['metadata'])),result['pixels'] or '-',result['heap'][0],1])
        editions[edition]=dict(returns=len(cases),assertions=0,heap_calls=len(cases),capture_replaced=0,rle_replaced=0);files.update(oracle.exports);files.add(oracle.shapeFile);files.add(ROOT/edition/('Netstorm.exe' if edition=='originals' else 'NETSTORM.EXE'));files.add(ROOT/edition/'netstorm.tarc');print(edition,editions[edition],flush=True)
    # 실제 보호 SHP를 소비한 C++ 몸체/기존RLE/새검사와 도구/내보내기 출처를 함께 기록한다.
    for name in ('cpppj/src/client/RawShapeToBuffer.h','cpppj/src/client/RawShapeToBuffer.cpp','cpppj/src/client/VFXDraw.h','cpppj/src/client/VFXDraw.cpp','cpppj/src/client/SquidRenderer.cpp','cpppj/tests/ShapeToBufferTests.cpp','tools/decomp_shape_to_buffer_oracle.py','tools/decomp_dudespawn_oracle.py','tools/decomp_postpop_oracle.py','tools/decomp_display_oracle.py','tools/decomp_creation_oracle.py','tools/decomp_sid_oracle.py','tools/decomp_oracle.py','tools/decomp_owner_oracle.py','tools/ghidra/burning-frame-helper-functions.json','tools/ghidra/burning-frame-paint-helper-functions.json','tools/ghidra/burning-frame-raster-helper-functions.json'):files.add(ROOT/name)
    FIXTURE.write_text('# 실제 ShapeToBuffer/보호Dude main·빈I64/SHP/plain RLE. 현재 raw/배율/기준점 입력, heap만 명시경계.\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8');files.add(FIXTURE)
    REPORT.write_text(json.dumps(dict(schema=1,host='HJOW-Athlon',primary_target='10.78',cd_target='10.72',total=len(rows),editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(files)},cpp_tests_completed=0,os_calls=0,limitations=['실제 현재TYPE/header→heap→투명초기화→plain RLE→전체render문맥20바이트복원→TYPE추가hotspot을 직접 실행한다.','heap은 요청크기 관찰과 격리반환경계이며 capture/RLE는대체하지않는다.','보호Dude70/frame0·1·7·32·64와정상/dead·양수fixed16배율·owner0를 명시입력으로 사용한다.','CD의free/가상표0·해제표식/frame255은8×8/4×4투명fallback으로정상반환한다.','실제 VFX의 pushDS/popES에는4GB/32bit 평면GDT 입력을공급한다. nullselector의에뮬레이터16bit스택실패는완료관찰로세지않는다.','mapped owner색표/추가가상그림·원본BurningFrame전체/Draw/첫팔레트/Host삭제·캠페인월드는후속이다.'],),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def verify():
    """정상164고유입력과 당시 전체 원문 SHA 및 실제 픽셀 버퍼를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[row for row in csv.reader(FIXTURE.open(encoding='utf-8'),delimiter='\t') if row and not row[0].startswith('#')]
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('ShapeToBuffer SHA 불일치 '+name)
    if len(rows)!=report['total'] or report['os_calls']:raise RuntimeError('ShapeToBuffer 정상/OS 분류 오류')
    # 각 판본의 현재 입력5열과 전체 픽셀 길이는 저장된 메타데이터/heap 요청과 일치해야 한다.
    for edition,data in report['editions'].items():
        selected=[row for row in rows if row[0]==edition]
        if len(selected)!=data['returns'] or len({tuple(row[1:6]) for row in selected})!=len(selected) or data['assertions'] or data['capture_replaced'] or data['rle_replaced']:raise RuntimeError('ShapeToBuffer 고유/정상 반환 오류')
        for row in selected:
            width,height,_,_=map(int,row[6].split(','));pixels=bytes.fromhex(row[7]) if row[7]!='-' else b''
            if len(pixels)!=width*height or int(row[8])!=len(pixels) or row[9]!='1':raise RuntimeError('ShapeToBuffer 전체 픽셀/문맥 오류')
    print(json.dumps(dict(verified=True,total=len(rows),cpp_completed=False),ensure_ascii=False))


# 명시 CLI에서 새 격자를 작성하거나 현재 관찰 SHA를 검증한다.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args();verify() if args.verify else smoke() if args.smoke else generate()
