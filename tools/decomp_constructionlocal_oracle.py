#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""로컬 건설 배치 전체를 세 PE의 실제 x86으로 실행한다. 게임/OS/창은 실행하지 않는다.

실제 몸체: 로컬 배치 전체, CanonDecoder/프레임 검색, spot 좌표 절삭, 요청 메시지 초기화/길이 쓰기, CD 0시각 생성.
명시 경계: Form 가상 조회/삭제, 생산 칸 조각 반환, SID 생성, 예측 배치, 사제 이동, 요청 전송.
--smoke는 무저장 표본, --verify는 저장 입력/출력과 SHA·진입/반환·ABI·경계 사건을 감사한다.
"""
import argparse
import collections
import csv
import itertools
import json
import struct
from pathlib import Path
from decomp_owner_oracle import OwnerOracle, ROOT, SPECS, TYPES, POOL, STOP, digest
from decomp_constructionplace_oracle import PlaceOracle, VTABLE, PLAIN, BRIDGE, ISLAND, NOISLAND
from decomp_damageablepredestroy_oracle import CONTROLS, bits
from unicorn.x86_const import UC_X86_REG_ECX

# 로컬 배치와 판본별 명시 효과/전역 주소다. 가상 경계는 분석 전용 주소다.
SOURCE, DESTROY, SPOTS, PREVIEW, BORN = STOP+0x400, STOP+0x500, 0x14500000, 50, 100
SPEC = {
    'originals': dict(entry=0x4433b0, piece=0x4468b0, create=0x4af530, place=0x442c80,
        builder=0x443000, send=0x4e2dc0, init=0x40a7c0, local=0x540c70, spots=0x5c7c44, quality=30),
    'originalCD': dict(entry=0x4d14a0, piece=0x43e930, create=0x4ab390, place=0x4d0ca0,
        builder=0x4d1120, send=0x4c0380, init=0x40a470, local=0x50f6c8, spots=0x52fe48, quality=26),
}
SPEC['original1037'] = dict(SPEC['originalCD'])
# 고정 입력 열 순서와 실제 실행에서만 얻는 두 조각 수다.
KEYS = ('type','x','y','argument','direction','local','flags','quality','source','flags1','spot',
    'islandX','islandY','frames','default','toggle','sourceMutate','count','baseCount')
FIXTURE = ROOT/'cpppj/tests/fixtures/constructionlocal-x86.tsv'
REPORT = ROOT/'cpppj/constructionlocal.json'


def local_case(**changes):
    """합성 타입/미리보기의 기본 입력을 만들며 기대 결과는 계산하지 않는다."""
    case = dict(type=PLAIN,x=bits(20.5),y=bits(21.25),argument=7,direction=0,local=3,flags=0,
        quality=0,source=77,flags1=0,spot=2,islandX=3,islandY=3,frames=0,default=3,toggle=0,sourceMutate=0,count=0,baseCount=0)
    case.update(changes)
    return case


def inputs(edition):
    """반환 플래그/품질, 본/받침 할당·배치 순서, 사제·인자 폭, 모든 다리 패턴과 좌표 경계를 교차한다."""
    cases = [local_case(direction=8)]; directions = range(8) if edition == 'originals' else (0,2,4,6)
    # 미리보기 조회 뒤 플래그/품질 변경과 삭제 때 raw 변경을 포함해 캡처 시점을 검사한다.
    for flags, quality, source, mutate in itertools.product((0,8,0x100,0x108,0xffff), (0,1,3,0xff), (0,77,0xffffff00), (0,1)):
        cases.append(local_case(flags=flags,quality=quality,source=source,sourceMutate=mutate,argument=0x12345678,local=0xffffff03))
    # 받침은 시작 spot 한 칸으로 결정한다. 8은 시작 칸 하나만 bit 2이며 다른 칸은 0이다.
    for flags1, spot, foot, toggle in itertools.product((0,0x400,0x8000,0x8400,0x7bff), (0,2,4,8), ((1,1),(3,2),(10,10)), (0,0x8000)):
        cases.append(local_case(flags1=flags1,spot=spot,islandX=foot[0],islandY=foot[1],toggle=toggle,quality=1,flags=8))
    # 유효 칸이 없어도 Place/사제 이동/전송은 남는다. 첫 SID는 초기화된 0이다.
    for default, flags1, local in itertools.product((0xffffffff,0,3), (0,0x8000,0x8400), (0,3,0x103)):
        cases.append(local_case(default=default,flags1=flags1,local=local,spot=0))
    # 모든 다리 패턴/방향, 섬/noIsland의 다칸 decoder와 서로 다른 프레임 표를 실행한다.
    for index, (pattern,direction) in enumerate(itertools.product(range(26),directions)):
        cases.append(local_case(type=BRIDGE,argument=pattern,direction=direction,frames=index%4,flags1=0x8400 if index%2 else 0x8000,
            quality=index%4,flags=8,spot=0 if index%3 else 2))
    for typ,direction in itertools.product((ISLAND,NOISLAND),directions):
        cases.append(local_case(type=typ,argument=direction%2 if typ==ISLAND else 0,direction=direction,flags1=0x8000,quality=3))
    # 거의 올림(+0.9999)의 양쪽과 지도 끝을 확인한다. 시작 spot은 모두 지도 안에 있다.
    for (x,y),spot,foot in itertools.product(((0.0,0.0),(0.00001,0.00011),(20.00001,21.00011),
            (20.00011,21.00001),(254.0,254.0),(254.00001,254.00011)),(0,2,8),((3,2),(10,10))):
        cases.append(local_case(x=bits(x),y=bits(y),spot=spot,flags1=0x8400,islandX=foot[0],islandY=foot[1]))
    # 받침이 필요 없는 좌표는 지도 밖이어도 그대로 요청에 실린다. WORD/BYTE 절단은 전송에서만 한다.
    for argument,direction,position in itertools.product((0,0x100,0xffff,0x12345678,0xffffffff),(0,0x100,0xfffffffe),
            ((-3.5,300.25),(255.99998,-0.0))):
        cases.append(local_case(argument=argument,direction=direction,x=bits(position[0]),y=bits(position[1]),quality=0xff))
    return cases


def spot_bytes(case):
    """spot 입력만 만든다. 프로필 8은 거의 올림한 시작 칸 하나만 bit 2다."""
    if case['spot'] != 8: return bytes([case['spot']])*65536
    raw = bytearray(65536)
    x,y = (struct.unpack('<f',struct.pack('<I',case[key]))[0] for key in ('x','y'))
    cx,cy = int(x+0.9998999834060669),int(y+0.9998999834060669)
    if 0<=cx<256 and 0<=cy<256: raw[cy*256+cx] = 2
    return bytes(raw)


class LocalOracle(PlaceOracle):
    """새 허용 몸체만 실행하고 명시 효과의 인자·순서를 관찰한다."""
    def __init__(self,edition):
        """실제 PE의 함수 범위와 합성 Form/spot을 준비한다."""
        super().__init__(edition); self.l = SPEC[edition]; mu = self.mu; mu.mem_map(SPOTS,65536)
        self.exports = [ROOT/f'extracted/constructionlocal/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed = []; self.entries = set()
        with self.exports[1].open(encoding='utf-8') as fp:
            # 내보낸 몸체의 불연속 범위만 실행을 허용한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entries.add(int(row['entry'],16))
                # 포함 끝 주소를 반열린 범위로 바꾼다.
                for part in row['ranges'].split(';'):
                    low,high = (int(value,16) for value in part.split('-')); self.allowed.append((low,high+1))
        mu.mem_write(VTABLE,struct.pack('<I',SOURCE)); mu.mem_write(VTABLE+0x10,struct.pack('<I',DESTROY))
        mu.mem_write(self.l['spots'],struct.pack('<I',SPOTS))
        self.stubs = {SOURCE:('Q',self.stub_source), DESTROY:('D',self.stub_destroy),
            self.l['piece']:('R',self.stub_piece),self.l['create']:('C',self.stub_create),
            self.l['place']:('P',self.stub_place),self.l['builder']:('B',self.stub_builder),self.l['send']:('S',self.stub_send)}

    def stub_source(self):
        """미리보기 가상 조회의 DWORD 결과를 공급하고 지정된 raw 플래그/품질 변경을 수행한다."""
        if self.mu.reg_read(UC_X86_REG_ECX)!=self.slot(PREVIEW): raise RuntimeError('미리보기 조회 this 오류')
        self.events.append(f'Q:{PREVIEW}:{self.case["source"]}')
        if self.case['sourceMutate']:
            self.mu.mem_write(self.slot(PREVIEW)+20,struct.pack('<H',self.case['flags']^0x108))
            self.mu.mem_write(self.slot(PREVIEW)+self.l['quality'],bytes([self.case['quality']^3]))
        self.ret(self.case['source'])

    def stub_destroy(self):
        """미리보기 삭제를 기록하고 raw 내용을 지워 값이 미리 캡처되었는지 검사한다."""
        if self.mu.reg_read(UC_X86_REG_ECX)!=self.slot(PREVIEW): raise RuntimeError('미리보기 삭제 this 오류')
        flags, = self.args(1); self.events.append(f'D:{PREVIEW}:{flags}')
        self.mu.mem_write(self.slot(PREVIEW),bytes(self.stride)); self.ret(purge=4)

    def stub_piece(self):
        """생산 칸 조각 반환의 플래그/연관 번호/품질을 그대로 기록한다."""
        self.events.append('R:'+':'.join(map(str,self.args(3)))); self.ret()

    def stub_create(self):
        """클라이언트 SID 생성 입력을 기록하고 순서대로 실제 슬롯 주소를 돌려준다."""
        typ,flags = self.args(2); sid = BORN+self.born; self.born += 1
        if self.born>33: raise RuntimeError('클라이언트 조각 수 초과')
        self.events.append(f'C:{typ}:{flags}:{sid}'); self.ret(self.slot(sid))

    def stub_place(self):
        """배치 인자와 원본이 쓴 SID 목록을 기록하고 본 배치 뒤 타입 플래그 변경을 공급한다."""
        values = self.args(10); count,pointer = values[6:8]
        if count>18: raise RuntimeError('로컬 SID 배열 개수 오류')
        sids = struct.unpack(f'<{count}H',self.mu.mem_read(pointer,count*2)) if count else ()
        self.events.append('P:'+':'.join(map(str,(*values[:7],*values[8:])))+':'+(','.join(map(str,sids)) or '-'))
        if values[0]==self.case['type']:
            address = TYPES+values[0]*self.type_stride+0xe8
            old = struct.unpack('<I',self.mu.mem_read(address,4))[0]
            self.mu.mem_write(address,struct.pack('<I',old^self.case['toggle']))
        self.ret()

    def stub_builder(self):
        """숨은 결과 포인터를 제외한 플레이어/첫 SID/시각 두 DWORD를 기록한다."""
        self.events.append('B:'+':'.join(map(str,self.args(5)[1:]))); self.ret()

    def stub_send(self):
        """요청 전송 대상과 실제 본문 21바이트(0시각 자리 포함)를 기록한다."""
        target,message = self.args(2); payload = bytes(self.mu.mem_read(message+4,21)).hex()
        self.events.append(f'S:{target}:{payload}'); self.ret()

    def on_instruction(self,mu,address,size,data):
        """명시 경계만 대체하고 메시지 초기화/decoder를 포함한 나머지 몸체는 실제 실행한다."""
        stub = self.stubs.get(address)
        if stub is None: OwnerOracle.on_instruction(self,mu,address,size,data); return
        letter,handler = stub; handler(); self.stub_calls[letter] += 1

    def local(self,case,control):
        """상태를 준비하고 전체 로컬 배치를 정상 반환까지 실행한다. 입력 메모리 쓰기는 경계에만 있다."""
        mu = self.mu; self.setup(case); self.born = 0
        # 받침 타입의 프레임 표와 섬의 크기는 본 타입과 별도로 공급한다.
        if case['type']!=NOISLAND:
            record = TYPES+NOISLAND*self.type_stride; mu.mem_write(record,bytes(self.type_stride))
            mu.mem_write(record+0x114,bytes(mu.mem_read(TYPES+case['type']*self.type_stride+0x114,4))+struct.pack('<i',3))
            mu.mem_write(record+0x124,bytes(mu.mem_read(TYPES+case['type']*self.type_stride+0x124,4)))
            mu.mem_write(record+self.s['foot'],struct.pack('<2i',1,1))
        mu.mem_write(TYPES+ISLAND*self.type_stride+self.s['foot'],struct.pack('<2i',case['islandX'],case['islandY']))
        mu.mem_write(self.l['local'],struct.pack('<I',case['local'])); mu.mem_write(SPOTS,spot_bytes(case))
        raw = bytearray(self.stride); struct.pack_into('<I',raw,0,VTABLE)
        struct.pack_into('<2H',raw,18,case['type'],case['flags']); raw[self.l['quality']] = case['quality']
        mu.mem_write(self.slot(PREVIEW),bytes(raw)); self.case = case; self.write_ranges = []
        self.invoke(self.l['entry'],[PREVIEW,*[case[key] for key in ('x','y','argument','direction')]],control)
        # 관찰된 Place의 개수만 세며 받침 필요 조건을 Python으로 계산하지 않는다.
        placements = [event.split(':') for event in self.events if event.startswith('P:')]
        counts = (int(placements[-1][7]),int(placements[0][7]) if len(placements)>1 else 0)
        return [';'.join(self.events),*counts]


def generate(smoke=False):
    """세 PE에서 같은 입력을 두 x87 제어값으로 실행해 독립 관찰과 SHA 근거를 저장한다."""
    rows = []; editions = {}; paths = {Path(__file__),FIXTURE,ROOT/'tools/ghidra/constructionlocal-functions.json',
        *[ROOT/f'tools/decomp_{name}_oracle.py' for name in ('owner','priestgeometry','constructionplace','damageablepredestroy')]}
    # 각 PE는 독립된 가상 메모리에서 실행한다.
    for edition in SPECS:
        oracle = LocalOracle(edition); cases = inputs(edition)
        if smoke: cases = cases[::29]
        # 기대 사건/개수는 두 실제 실행의 결과로만 얻는다.
        for case in cases:
            results = [oracle.local(case,control) for control in CONTROLS]
            if results[0]!=results[1]: raise RuntimeError(f'로컬 배치 x87 정밀도 차이: {edition} {case}')
            events,case['count'],case['baseCount'] = results[0]
            rows.append([edition,','.join(str(case[key]) for key in KEYS),events])
        editions[edition] = dict(cases=len(cases),returns=dict(oracle.returns),native_calls=dict(oracle.native_calls),
            substitutions=dict(oracle.stub_calls),assertions=oracle.assertions)
        paths.update(oracle.exports); paths.add(ROOT/oracle.spec['binary']); print(f'{edition}: constructionlocal {len(cases)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 로컬 건설 배치 전체 실제 PE 관찰. 명시 경계 Q/D/R/C/P/B/S.\n# 입력: '+' '.join(KEYS)+'\n'+
        '\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',decompile_date='2026-10-11',
        controls=list(CONTROLS),os_calls=0,editions=editions,files={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)},
        limits=['실제 로컬 배치 전체/decoder/프레임 검색/spot 절삭/메시지 초기화·길이 쓰기/CD 시각 생성/정상 반환 ABI·x87',
            'Form 가상 조회/삭제, 생산 칸 반환, SID 생성, 예측 배치, 사제 이동, 메시지 전송은 명시 경계',
            '합성 타입/프레임/Form 입력; 유한 좌표, 받침 spot은 지도 안, 지역 SID 배열은 최대 18개',
            '조회 경계가 raw 플래그/품질을 바꾸고 삭제 경계가 raw를 지우며 본 배치 경계가 타입 플래그를 바꾸는 입력 포함',
            '서버/기존 GUI/커서 내려놓기/실제 Pop 공간 효과/사제 이동/게임/OS 미실행']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(f'constructionlocal 전체 {len(rows)}개')


def verify():
    """저장된 입력/출력·SHA·정상 진입/반환·경계 사건 수와 요청 필드 폭을 감사한다."""
    report = json.loads(REPORT.read_text(encoding='utf-8'))
    rows = [line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 함수 내보내기/실행기/원본/fixture의 변화를 검출한다.
    for name,expected in report['files'].items():
        if digest(ROOT/name)!=expected: raise RuntimeError('SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['os_calls']: raise RuntimeError('로컬 배치 근거 오류')
    # 생성 순서, 관찰 사건과 실제 집계가 판본별로 일치해야 한다.
    for edition,item in report['editions'].items():
        selected = [row for row in rows if row[0]==edition]; cases = inputs(edition); spec = SPEC[edition]
        if len(selected)!=len(cases) or item['cases']!=len(cases) or item['assertions']: raise RuntimeError('로컬 배치 행/assert 오류')
        # 마지막 두 개수는 실제 관찰값이라 기대값을 재계산하지 않는다.
        for row,case in zip(selected,cases,strict=True):
            values = row[1].split(',')
            if len(row)!=3 or len(values)!=len(KEYS) or values[:-2]!=[str(case[key]) for key in KEYS[:-2]]: raise RuntimeError('로컬 배치 입력/열 오류')
            events = row[2].split(';'); sends = [event.split(':') for event in events if event.startswith('S:')]
            if len(sends)!=1 or sends[0][1]!='9' or len(bytes.fromhex(sends[0][2]))!=21 or bytes.fromhex(sends[0][2])[-8:]!=bytes(8):
                raise RuntimeError('로컬 요청 대상/크기/시각 오류')
            placements = [event.split(':') for event in events if event.startswith('P:')]
            observed_counts = (int(placements[-1][7]),int(placements[0][7]) if len(placements)>1 else 0)
            if observed_counts!=tuple(map(int,values[-2:])): raise RuntimeError('로컬 조각 수 관찰 오류')
        if item['returns'].get(f'{spec["entry"]:08x}',0)!=2*len(selected) or item['native_calls'].get(f'{spec["entry"]:08x}',0)!=2*len(selected):
            raise RuntimeError('로컬 배치 진입/정상 반환 수 오류')
        if item['native_calls'].get(f'{spec["init"]:08x}',0)!=2*len(selected): raise RuntimeError('실제 메시지 초기화 진입 수 오류')
        all_events = [event for row in selected for event in row[2].split(';')]
        # 대체 경계와 사건은 일대일이며 같은 경계의 원본 몸체는 실행하지 않는다.
        for letter in 'QDRCPBS':
            if item['substitutions'].get(letter,0)!=2*sum(event.startswith(letter+':') for event in all_events): raise RuntimeError('로컬 경계 사건 수 오류: '+letter)
        if any(item['native_calls'].get(f'{spec[key]:08x}',0) for key in ('piece','create','place','builder','send')): raise RuntimeError('로컬 경계/몸체 혼합')
    print(f'constructionlocal 검증 통과: {len(rows)}개')


def main():
    """전체 기록 생성, 무저장 표본, 저장 근거 감사를 선택한다."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--smoke',action='store_true'); parser.add_argument('--verify',action='store_true')
    args = parser.parse_args()
    if args.verify: verify()
    else: generate(args.smoke)


if __name__ == '__main__': main()
