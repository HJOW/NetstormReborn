#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""세 실제 PE의 지면/다리 행별 묶음과 해시/CRT 보조를 격리 실행한다. 응답 대체/게임/OS 호출은 없다."""
import argparse
import collections
import csv
import json
import random
import struct
from decomp_drawsort_oracle import REGISTERS
from decomp_process_oracle import (ProcessOracle, ROOT, BINARIES, CONTROLS, STACK, STOP, digest, fbits,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW)

# 격리 출력/전체 해시/슬롯/배경 결과 주소와 실제 관찰 용량이다. 입력 계획은 이 용량 안에서만 실행한다.
HEADERS, ARRAY, GRID, POOL, COVERED = 0x15000000, 0x15001000, 0x15010000, 0x15030000, 0x15040000
CAPACITY, HEADER_CAPACITY, SEED = 96, 64, 0x43341021
FIXTURE = ROOT/'cpppj/tests/fixtures/tilegroup-x86.tsv'
REPORT = ROOT/'cpppj/tilegroup.json'
# 수집/해시 선택/크기/월드 크기/풀/화면 높이의 실제 주소다. 전부 실제 명령으로 실행한다.
SPECS = {
    'originals': dict(collect=0x433410, hash=0x542508, scales=0x5424f8, world=0x531928, pool=0x5c8464, height=0x531864, stride=50),
    'originalCD': dict(collect=0x4884a0, hash=0x5670c0, scales=0x52d598, world=0x52e9a8, pool=0x5395dc, height=None, stride=36),
}
SPECS['original1037'] = dict(SPECS['originalCD'])


def plan():
    """경계/빈 칸/행 순서/첫 SID/부호 깊이/큰 절삭/행 제한에 대한 완성 입력만 만든다."""
    cases = []
    def add(view=(2, 2, 6, 6), cells=(), limit=80, height=768, selected=3):
        """입력 float 비트와 각 칸의 첫 SID를 기록한다. 기대 목록/헤더는 계산하지 않는다."""
        cases.append(dict(view=[fbits(v) for v in view], cells=sorted(dict(cells).items()), limit=limit, height=height, selected=selected))
    # 전체 빈 창, 같은 칸의 반복 SID, 중간 빈 행, 경계 한 칸 확장과 row-major 순서다.
    layouts = [[], [(y*256+x, 5+(x+y)%7) for y in range(1, 8) for x in range(1, 8)],
        [(y*256+x, 5) for y in (1, 3, 7) for x in (1, 4, 7)], [(3*256+3, 11)]]
    # 한 칸의 fractional 경계와 음수의 0 방향 절삭을 독립적으로 바꾼다.
    for layout in layouts:
        for view in ((2,2,6,6), (2.99,2.01,6.99,6.01), (3,3,3,3), (6,6,2,2),
            (4,2,2,6), (2,6,6,2), (-.9,-.9,0,0), (0,0,255,255)):
            add(view, layout)
    # 경계 밖에서도 좌표를 독립적으로 클램프하며 빈 방문 영역은 covered=1로 정상 반환한다.
    for value in (-float('inf'), float('inf'), float('nan'), -2**63, 2**63, -2**31, 2**31,
        2**32, 2**32+512, 2**32-512, -2**32+512, -2**32-512, -100, -.9, 0, .9, 1, 254.99, 255, 256, 10000):
        for axis in range(4):
            view=[2,2,6,6];view[axis]=value;add(view, layouts[-1])
    # 같은 SID라도 여러 칸/행에서 그대로 중복 제출하고 next/type/state를 검사하지 않는다.
    for height in (0, 11, 66, 480, 768, 1080):
        for selected in range(4): add((10,10,30,12), [(y*256+x,5) for y in range(10,13) for x in range(11,31)], height=height, selected=selected)
    # 각 행은 2개<3을 만족하지만 총 6개>3이다. 빈 칸만 있으면 음수 limit도 assert에 도달하지 않는다.
    add((2,2,3,4), [(y*256+x, 5+x) for y in range(2,5) for x in (2,3)], limit=3)
    for limit in (-1,0,1,2): add(cells=(), limit=limit)
    add((3,3,3,3),[(3*256+3,11)],limit=2)
    # 좌표 보조의 +0.5와 절삭 사이 float 저장 부재를 반올림 경계 양쪽에서 관찰한다.
    for value in (.4999999701976776,.5,.5000000596046448,1.4999998807907104,1.5,1.5000001192092896,254.5,255.5):
        add((value,0,value,0),[(x,5+x%7) for x in range(4)]+[(254,9),(255,10)])
        add((0,value,0,value),[(y*256,5+y%7) for y in range(4)]+[(254*256,9),(255*256,10)])
    # CD의 최대 유효 48개 헤더와 10.78의 동적 행 상한을 동시에 지나지 않는 입력이다.
    add((0,0,255,255), [(y*256+5,5+y%7) for y in range(48)],height=768)
    # 네 모서리와 모든 칸이 찬 3x3 창에서 배경 덮음 true/false를 관찰한다.
    for cx,cy in ((0,0),(255,0),(0,255),(255,255),(15,17)):
        cells=[(y*256+x,5+(x+y)%7) for y in range(max(0,cy-1),min(256,cy+2)) for x in range(max(0,cx-1),min(256,cx+2))]
        add((cx,cy,cx,cy),cells);add((cx,cy,cx,cy),cells[1:])
    rng=random.Random(SEED)
    # 고정 시드의 작은 창/불규칙 머리로 경계와 중복/빈 행 조합을 넓힌다.
    for _ in range(180):
        x,y=rng.randrange(240),rng.randrange(240);width,height=rng.randrange(7),rng.randrange(7)
        cells=[((y+dy)*256+x+dx,rng.randrange(5,12)) for dy in range(8) for dx in range(8) if rng.randrange(3)]
        view=(x+rng.choice((-.9,0,.9)), y+rng.choice((-.9,0,.9)), x+width+rng.choice((0,.9)), y+height+rng.choice((0,.9)))
        add(view,cells,height=rng.choice((128,480,768,1080)),selected=rng.randrange(4))
    return cases


def slots(edition):
    """판본별 슬롯의 모든 바이트를 입력으로 만든다. 깊이/좌표/next/type/state는 기대 결과가 아니다."""
    stride=SPECS[edition]['stride'];result=[]
    # 부호 깊이의 최솟값/최댓값/양음수와 단일 SID 반복 입력에 필요한 7개 슬롯이다.
    for sid,depth in enumerate((128,255,0,1,127,200,19),5):
        value=bytearray(((i*29+sid*17)&255 for i in range(stride)))
        struct.pack_into('<H',value,4,5 if sid==5 else sid-1)
        value[10]=0 if sid==5 else sid;value[11]=255 if sid==6 else 0
        struct.pack_into('<II',value,14,fbits(100.5-sid*3.25),fbits(-20.75+sid*1.5))
        value[35 if stride==50 else 33]=depth;result.append((sid,bytes(value)))
    return result


def encoded(case,edition):
    """C++ 재생용 config/희소 해시 입력/완성 슬롯 바이트를 직렬화한다."""
    config=','.join(map(str,(*case['view'],case['limit'],case['height'],case['selected'])))
    cells=';'.join(f'{index},{sid}' for index,sid in case['cells']) or '-'
    raw=';'.join(f'{sid}={value.hex()}' for sid,value in slots(edition))
    return config,cells,raw


class TileGroupOracle(ProcessOracle):
    """내보낸 수집기/해시/좌표/CRT 실제 몸체만 실행하고 모든 반환/쓰기 범위를 감사한다."""
    def __init__(self,edition):
        """독립 PE와 불연속 실행 허용 범위 및 격리 데이터 저장소를 준비한다."""
        super().__init__(edition);self.d=SPECS[edition]
        self.extra=[ROOT/f'extracted/tilegroup/{edition}/{name}' for name in ('creation.c','functions.tsv')]
        self.allowed,self.entry_names=[],{}
        with self.extra[1].open(encoding='utf-8') as fp:
            # 함수 사이 코드를 허용하지 않고 각 실제 몸체만 등록한다.
            for row in csv.DictReader(fp,delimiter='\t'):
                self.entry_names[int(row['entry'],16)]=row['entry']
                # 불연속 범위를 각각 등록한다.
                for part in row['ranges'].split(';'):
                    low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.mu.mem_map(HEADERS-0x1000,0x52000);self.returns=collections.Counter()

    def on_instruction(self,mu,address,size,data):
        """응답 대체 없이 실제 명령만 실행하며 assert/OS/누락 함수에 도달하면 실패한다."""
        if not any(low<=address and address+size<=high for low,high in self.allowed):
            raise RuntimeError(f'Tile group 허용 밖 실행: {self.binary} {address:08x}')
        if address in self.entry_names: self.hits[self.entry_names[address]]+=1;self.entries[self.entry_names[address]]+=1

    def run(self,case,control):
        """전체 멤버/헤더/배경/현재 해시와 ABI/x87/보호 바이트를 원본 명령에서 관찰한다."""
        mu,d=self.mu,self.d;self.entries=collections.Counter();config,cells,raw=encoded(case,self.binary)
        grid=bytearray(256*256*2)
        # 기대 수집을 계산하지 않고 완성 버킷 입력만 쓴다.
        for index,sid in case['cells']: struct.pack_into('<H',grid,index*2,sid)
        mu.mem_write(GRID,bytes(grid));mu.mem_write(POOL,b'\0'*0x1000)
        # 슬롯 전부를 입력 값으로 설정한다.
        for sid,value in slots(self.binary): mu.mem_write(POOL+sid*d['stride'],value)
        mu.mem_write(HEADERS-16,b'\xa5'*(HEADER_CAPACITY*8+32));mu.mem_write(ARRAY-16,b'\xa5'*(CAPACITY*16+32));mu.mem_write(COVERED-16,b'\xa5'*36)
        self.put(d['pool'],POOL);self.put(d['world'],256)
        if d['height']: self.put(d['height'],case['height'])
        mu.mem_write(d['scales'],struct.pack('<4I',1,2,4,16))
        mu.mem_write(d['hash'],struct.pack('<7I',case['selected'],99,7,GRID,GRID,GRID,GRID))
        self.writes=[(0,4),(STACK-0xf000,STACK+0x1000),(HEADERS,HEADERS+HEADER_CAPACITY*8),
            (ARRAY,ARRAY+CAPACITY*16),(COVERED,COVERED+4),(d['hash'],d['hash']+12)]
        # 비영 보존 레지스터/SEH/x87을 설정해 실제 cdecl 정상 반환을 확인한다.
        for reg,value in REGISTERS.items(): mu.reg_write(reg,value)
        # 호출자 임시 레지스터는 0에서 시작한다.
        for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX): mu.reg_write(reg,0)
        mu.reg_write(UC_X86_REG_ESP,STACK);mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,control);mu.reg_write(UC_X86_REG_FPSW,0);self.put(0,0xffffffff)
        args=[*case['view'],HEADERS,ARRAY,case['limit'],COVERED]
        mu.mem_write(STACK,struct.pack('<9I',STOP,*(v&0xffffffff for v in args)));mu.emu_start(d['collect'],STOP,timeout=30000000,count=5000000)
        if mu.reg_read(UC_X86_REG_EIP)!=STOP or mu.reg_read(UC_X86_REG_ESP)!=STACK+4 or any(mu.reg_read(r)!=v for r,v in REGISTERS.items()):
            raise RuntimeError('Tile group 정상 반환/스택/보존 레지스터 오류')
        if mu.reg_read(UC_X86_REG_FPSW)&0x3800 or mu.reg_read(UC_X86_REG_FPCW)!=control or self.u32(0)!=0xffffffff: raise RuntimeError('Tile group x87/SEH 오류')
        self.returns[f'{d["collect"]:08x}']+=1;count=mu.reg_read(UC_X86_REG_EAX)
        if not 0<=count<HEADER_CAPACITY: raise RuntimeError('Tile group 헤더 범위 오류')
        headers=bytes(mu.mem_read(HEADERS,HEADER_CAPACITY*8));members=bytes(mu.mem_read(ARRAY,CAPACITY*16));used=0
        # 관찰 헤더의 실제 연속 포인터/양수 개수와 미사용 출력 보존을 확인한다.
        for index in range(count):
            pointer,length=struct.unpack_from('<2I',headers,index*8)
            if pointer!=ARRAY+used*16 or not 0<length<case['limit']: raise RuntimeError('Tile group 관찰 헤더 계약 오류')
            used+=length
        if used>CAPACITY or headers[count*8:]!=b'\xa5'*(HEADER_CAPACITY-count)*8 or members[used*16:]!=b'\xa5'*(CAPACITY-used)*16: raise RuntimeError('Tile group 미사용 출력 변경')
        guard=bytes(mu.mem_read(HEADERS-16,16))+bytes(mu.mem_read(HEADERS+HEADER_CAPACITY*8,16))+bytes(mu.mem_read(ARRAY-16,16))+bytes(mu.mem_read(ARRAY+CAPACITY*16,16))+bytes(mu.mem_read(COVERED-16,16))+bytes(mu.mem_read(COVERED+4,16))
        if guard!=b'\xa5'*96 or bytes(mu.mem_read(GRID,len(grid)))!=grid: raise RuntimeError('Tile group 보호/해시 입력 변경')
        # 슬롯의 미사용 바이트까지 모두 보존한다.
        for sid,value in slots(self.binary):
            if bytes(mu.mem_read(POOL+sid*d['stride'],len(value)))!=value: raise RuntimeError('Tile group 슬롯 변경')
        state=','.join(map(str,struct.unpack('<3I',mu.mem_read(d['hash'],12))))
        if state!='2,4,64' or self.u32(COVERED) not in (0,1): raise RuntimeError('Tile group 정상 종료 상태 오류')
        return [config,cells,raw,str(count),str(self.u32(COVERED)),members.hex(),headers.hex(),state,
            json.dumps(dict(self.entries),sort_keys=True,separators=(',',':')),guard.hex()]


def generate(smoke):
    """두 x87 정밀도에서 같은 관찰을 요구하며 실제 PE의 완성 결과만 fixture에 저장한다."""
    rows,editions=[],{};files={ROOT/'tools/decomp_tilegroup_oracle.py',ROOT/'tools/decomp_process_oracle.py',
        ROOT/'tools/decomp_drawsort_oracle.py',ROOT/'tools/ghidra/tilegroup-functions.json',FIXTURE}
    # 세 독립 판본의 PE를 같은 완성 입력에서 실행한다.
    for edition in BINARIES:
        oracle=TileGroupOracle(edition);indexed=list(enumerate(plan()))
        if smoke: indexed=indexed[::37]+indexed[-2:]
        # 53/64비트 정밀도 루트를 각각 실행해 전체 출력과 진입 집계가 같은지 확인한다.
        for index,case in indexed:
            observed=[oracle.run(case,control) for control in CONTROLS]
            if observed[0]!=observed[1]: raise RuntimeError(f'Tile group x87 차이: {edition} {index}')
            rows.append([edition,str(index),*observed[0]])
        editions[edition]=dict(cases=len(indexed),entries=dict(oracle.hits),returns=dict(oracle.returns))
        files.update(oracle.exports+oracle.extra);files.add(ROOT/BINARIES[edition]);print(f'{edition}: {len(indexed)}개 통과',flush=True)
    if smoke: return
    FIXTURE.write_text('# 실제 행별 지면/다리 묶음/헤더/배경/해시 관찰. 모든 호출은 실제 몸체다.\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    REPORT.write_text(json.dumps(dict(total=len(rows),host='HJOW-Athlon',date='2026-10-11',controls=list(CONTROLS),os_calls=0,assertions=0,
        boundaries=[],editions=editions,files={p.relative_to(ROOT).as_posix():digest(p) for p in sorted(files)},
        limits=['전체 수집/해시 주소/좌표 절삭/CRT 실제 몸체, 응답 대체 없음',
            '희소 칸과 완성 raw 슬롯 입력, 전체 출력/미사용/보호/입력 보존, 진입/반환/ABI/x87/SEH/쓰기/SHA 감사',
            '원본 assert를 지나지 않는 입력만 PE 관찰; 손상 토큰/행 한계는 C++ 계약 검사',
            '카메라 투영/실제 SHP 그리기/Client 자동 프레임은 후속']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def verify():
    """계획/입력/폭/보호/SHA와 실제 함수 진입/정상 반환 집계를 감사한다. 기대 수집은 계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'));rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 도구/원본/읽기 전용 내보내기/fixture의 정확한 바이트를 대조한다.
    for name,sha in report['files'].items():
        if digest(ROOT/name)!=sha: raise RuntimeError('Tile group SHA 불일치: '+name)
    if report['total']!=len(rows) or report['controls']!=list(CONTROLS) or report['boundaries'] or report['os_calls'] or report['assertions'] or set(report['editions'])!=set(BINARIES): raise RuntimeError('Tile group 총계/경계 오류')
    # 판본별 순서/중복과 진입/반환을 완성 입력 계획에 맞춰 따로 감사한다.
    for edition,item in report['editions'].items():
        chosen=[row for row in rows if row[0]==edition];entries=collections.Counter()
        if len(chosen)!=len(plan()) or item['cases']!=len(chosen): raise RuntimeError('Tile group 행 수 오류')
        # 입력과 전체 결과 폭/가드만 확인하고 기대 목록을 구현하지 않는다.
        for index,(row,case) in enumerate(zip(chosen,plan(),strict=True)):
            if len(row)!=12 or row[1]!=str(index) or tuple(row[2:5])!=encoded(case,edition): raise RuntimeError('Tile group 입력/폭 오류')
            if len(bytes.fromhex(row[7]))!=CAPACITY*16 or len(bytes.fromhex(row[8]))!=HEADER_CAPACITY*8 or row[9]!='2,4,64' or row[11]!=(b'\xa5'*96).hex(): raise RuntimeError('Tile group 출력/보호 오류')
            # 저장된 실제 진입 집계는 두 정밀도 실행을 합친다.
            for address,count in json.loads(row[10]).items(): entries[address]+=count*2
        if dict(entries)!=item['entries'] or item['returns']!={f'{SPECS[edition]["collect"]:08x}':len(chosen)*2}: raise RuntimeError('Tile group 실행 집계 오류')
    print(f'tilegroup 검증 통과: {len(rows)}개, 두 정밀도 정상 반환 {len(rows)*2}회, 응답/OS/assert 0')


def main():
    """전체 관찰/무저장 표본/저장 감사 중 하나를 수행한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true')
    args=parser.parse_args();verify() if args.verify else generate(args.smoke)


if __name__=='__main__': main()
