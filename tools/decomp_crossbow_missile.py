#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""TYPE140 크로스보우의 실제 포구·arrow81 생성/비행을 10.78 x86으로 독립 관찰한다."""
import argparse
import csv
import hashlib
import itertools
import json
import socket
import struct
from collections import Counter
from pathlib import Path
from decomp_gunmissile_oracle import MissileOracle, SPEC, OBJECT, VTABLE, HEADER, OWNER, POP, UNPOP, DESTROY, bits
from decomp_owner_oracle import OwnerOracle, ROOT, TYPES, STACK, STOP, digest
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

# 독립 입력 열과 새 관찰 파일이다. 기존 Missile 기대값은 변경하지 않는다.
KEYS = ('kind', 'x', 'y', 'hotx', 'hoty', 'typehotx', 'typehoty', 'endx', 'endy',
        'fraction', 'targetstate', 'previous', 'corner', 'range', 'firingSound',
        'displaySound', 'impactSound', 'directions', 'frames', 'extra', 'passed')
FIXTURE = ROOT / 'cpppj/tests/fixtures/crossbow-missile-x86.tsv'
REPORT = ROOT / 'cpppj/crossbow-missile.json'
OUTPUT = HEADER + 0x800  # 실제 포구 반환값을 받는 격리 메모리다.
# 최초699행의 기대값이 경계 확장 전후로 그대로인지 검사하는 동결 근거다.
LEGACY_FIXTURE_SHA = 'e99c617f827fab16dd0d8ea0496f29a4b6ff991394b6ed3b15fa49f67286a25a'
FIXTURE_HEADER = '# 실제10.78 TYPE140 포구/arrow81 Missile 생성·프레임. 자산/공간/표적 치수/피해/사운드는 명시 경계다.\n'


def sample(**values):
    """원본 기대값 계산 없이 정상 raw 입력의 기본값만 구성한다."""
    result = dict(zip(KEYS, (0, bits(50), bits(50), 0, 0, 0, 0, bits(100), bits(90),
                            0, 0, 0, 0, bits(40), 123, 0, 456, 16, 16, 0, 0)))
    result.update(values)
    return result


def inputs():
    """포구 보정·방향·사운드 우선순위와 일반/코너 충돌·만료를 독립 교차한다."""
    rows = []
    # 생성자는 서로 다른 보정 부호와 발사/표시 사운드 우선순위를 방문한다.
    for point, hot, typehot, corner, sound, directions in itertools.product(
            ((50,50), (80.25,64.75), (1.25,1.25)), ((0,0), (16,11), (-8,-5)),
            ((0,0), (16,11)), (0,1), ((0,0), (0,789), (123,789)), (0,16)):
        rows.append(sample(x=bits(point[0]), y=bits(point[1]), hotx=hot[0], hoty=hot[1],
                           typehotx=typehot[0], typehoty=typehot[1], corner=corner,
                           firingSound=sound[0], displaySound=sound[1], directions=directions))
    # 방향 getter의 전체 사분면과 동일 지점은 실제 명령으로 처리한다.
    for target in ((50,10),(90,10),(90,50),(90,90),(50,90),(10,90),(10,50),(10,10),(51,51.5)):
        rows.append(sample(endx=bits(target[0]), endy=bits(target[1])))
    # frame은 생성자 결과를 재사용하지 않는 독립 2초 비행 이미지다.
    for fraction, targetstate, previous, corner, distance, impact in itertools.product(
            (0,1,3,4,5,8,12), range(4), (0,1), (0,1), (9,40), (0,456)):
        rows.append(sample(kind=1, endx=bits(50), endy=bits(90), fraction=fraction,
                           targetstate=targetstate, previous=previous, corner=corner,
                           range=bits(distance), impactSound=impact))
    # 통과 표식과 충돌 금지 bit10은 정상 충돌 경로와 따로 방문한다.
    for fraction, corner, extra, passed in itertools.product((3,4,5), (0,1), (0,16), (0,1)):
        rows.append(sample(kind=1, endx=bits(50), endy=bits(90), fraction=fraction,
                           previous=1, corner=corner, extra=extra, passed=passed))
    # TYPE140의 실제 가상 포구는 기본/current 프레임과 SHP에 관계없이 raw 좌표를 복사한다.
    for point, extra in itertools.product(((0.5,10.25),(94.125,108.375),(-10000,-10000),(255.75,255.5)), (0,1)):
        rows.append(sample(kind=2, x=bits(point[0]), y=bits(point[1]), extra=extra))
    # 기존699행의 C++ 통과 후 방향당3프레임인 별도 경계를 추가한다. 기존 기대값은 그대로다.
    for target in ((10,10),(90,10),(90,90),(10,90)):
        rows.append(sample(endx=bits(target[0]), endy=bits(target[1]), directions=8, frames=24))
    # 앞의 기본 격자와 겹친 입력은 한 고유 관찰로만 센다.
    return list({tuple(row[key] for key in KEYS):row for row in rows}.values())


class Oracle(MissileOracle):
    """공통 Missile 몸체와 실제 TYPE140 포구/방향 helper를 새 입력으로 실행한다."""
    def __init__(self, control):
        """현재 PC의 전체 정밀 자료에서 필요한 함수 범위만 허용한다."""
        OwnerOracle.__init__(self, 'originals')
        self.p = SPEC['originals'];self.control = control;self.allowed = [];self.events = [];self.boundaries = Counter();self.returns = 0
        self.mu.mem_map(0, 0x1000)
        self.exports += [ROOT/'extracted/refined/originals/functions.tsv', ROOT/'extracted/refined/originals/Netstorm.c']
        with self.exports[-2].open(encoding='utf-8') as stream:
            # 정밀 함수 행을 실제 진입 주소로 색인해 허용 범위 조회에 사용한다.
            facts = {int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        # 생성/비행/기하/절삭과 이들의 실제 CRT 보조 함수만 실행한다.
        entries = (0x481a50,0x482340,0x40e800,0x40e880,0x40e8f0,0x41cfc0,0x41d0d0,
                   0x41d180,0x41d6e0,0x468cc0,0x431f90,0x43fa50,0x4e49c0)
        self.entries.update(entries)
        # 검토한 각 함수의 불연속 실행 구간을 원문 자료에서 가져온다.
        for entry in entries:
            # 같은 함수에 속한 각 구간의 양 끝을 실행 허용 목록에 등록한다.
            for segment in facts[entry]['ranges'].split(';'):
                # 원문 범위의 두 16진수 주소만 정수로 변환한다.
                low, high = (int(value,16) for value in segment.split('-'))
                self.allowed.append((low, high+1))
        # PE 직접 검토로 확인한 미소유 함수: raw 좌표 DWORD 두 개 복사, RET4다.
        self.allowed.append((0x41c110,0x41c122));self.entries.add(0x41c110)
        assert self.read(0x50b6e8+0xb4)==0x41c110 and self.read(0x50d630+0x18)==0x482340

    def on_instruction(self, mu, address, size, data):
        """TYPE140 자료와 외부 자산/사운드 경계만 공급하고 수치 계산은 실제 CPU에 맡긴다."""
        esp=mu.reg_read(UC_X86_REG_ESP);ret=self.read(esp)
        if address==self.p['group']:
            self.boundaries[f'{address:08x}']+=1;mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret);return
        if address==0x41d6e0:
            # 실제 방향 helper의 다섯 스택 인자를 원본 폭으로 읽는다.
            args=[self.read(esp+4+i*4) for i in range(5)]
            self.events.append('F:'+':'.join(map(str,args[:4]))+':81')
        super().on_instruction(mu,address,size,data)

    def run_body(self, case):
        """전체 객체·세 슬롯·효과 순서와 ABI/x87/SEH/쓰기 범위를 관찰한다."""
        self.case=case;self.events=[];mu=self.mu;p=self.p;kind=case['kind']
        mu.mem_write(0,bytes(4));mu.mem_write(OUTPUT,bytes(8))
        # 전체128BYTE를 결정적 패턴으로 채워 원본이 보존한 padding도 관찰한다.
        fields=bytearray((i*13+7)&255 for i in range(128));mu.mem_write(OBJECT,bytes(fields));mu.mem_write(VTABLE,bytes(0x400))
        # 실제 포구와 명시 외부 자산 효과를 해당 가상 슬롯에 연결한다.
        for offset,entry in ((0xb4,0x41c110),(0x74,OWNER),(0x90,POP),(0x48,UNPOP),(0x10,DESTROY)):
            mu.mem_write(VTABLE+offset,struct.pack('<I',entry))
        # source/target/display 세 슬롯을 현재 독립 입력으로 다시 구성한다.
        for sid,typeid in ((50,140),(60,71),(90,81)):
            raw=bytearray(50);struct.pack_into('<I',raw,0,VTABLE);raw[10]=typeid;raw[11]=case['targetstate'] if sid==60 else 0;raw[34]=1;raw[40]=case['extra'] if sid==60 else 0
            struct.pack_into('<II',raw,14,case['x'] if sid==50 else bits(50),case['y'] if sid==50 else bits(50));mu.mem_write(self.slot(sid),bytes(raw));mu.mem_write(TYPES+typeid*self.type_stride,bytes(self.type_stride))
        source=TYPES+140*self.type_stride;display=TYPES+81*self.type_stride
        mu.mem_write(source+0x44,struct.pack('<I',case['range']));mu.mem_write(source+0x70,struct.pack('<I',case['firingSound']));mu.mem_write(source+0x1dc,struct.pack('<ii',case['typehotx'],case['typehoty']))
        mu.mem_write(display+0x70,struct.pack('<II',case['displaySound'],case['impactSound']));mu.mem_write(display+0x110,struct.pack('<ii',case['directions'],case['frames']))
        mu.mem_write(TYPES+136*self.type_stride+0x74,struct.pack('<I',case['impactSound']));mu.mem_write(HEADER+0x620,struct.pack('<hh',case['hotx'],case['hoty']))
        # 동일 판본의 실제 전역 번호와 진단 스위치를 입력 상태로 준비한다.
        for address,value in ((p['capacity'],188),(p['checking'],0),(p['sun'],128),(p['disc'],83),(p['direct'],71),(p['process'],28),(p['impactType'],136),(0x541268,132),(0x541260,130),(0x5412a0,146),(0x54daa4,0)):
            mu.mem_write(address,struct.pack('<I',value))
        mu.mem_write(p['scaleX'],struct.pack('<ff',16,11))
        if kind==0:
            mu.mem_write(p['now'],struct.pack('<d',10.25));corner=(case['endx'],case['endy']) if case['corner'] else (bits(-10000),bits(-10000))
            args=[50,60,bits(35),81,0,case['endx'],case['endy'],*corner,0,bits(-10000),bits(-10000)];entry=p['ctor'];this=OBJECT
        elif kind==1:
            struct.pack_into('<IIIIIIII',fields,0,p['vtable'],81,90,0,case['passed'],50,60,case['corner'])
            struct.pack_into('<IIII',fields,32,0,0,0,0);struct.pack_into('<ff',fields,48,35,0)
            struct.pack_into('<II',fields,56,case['x'],case['y']);struct.pack_into('<II',fields,64,case['endx'],case['endy']);struct.pack_into('<II',fields,72,case['endx'],case['endy'])
            struct.pack_into('<ffff',fields,88,-10000,-10000,-10000,-10000);struct.pack_into('<dd',fields,104,12,10);struct.pack_into('<ff',fields,120,50,89 if case['previous'] else -10000)
            mu.mem_write(OBJECT,bytes(fields));mu.mem_write(p['now'],struct.pack('<d',10+case['fraction']*0.5));args=[];entry=p['frame'];this=OBJECT
        else:
            args=[OUTPUT];entry=self.read(0x50b6e8+0xb4);this=self.slot(50)
        # 관찰 대상 세 슬롯의 전체 폭만 추가 쓰기 허용 구간으로 만든다.
        self.write_ranges=[(0,4),(OBJECT,OBJECT+128),(OUTPUT,OUTPUT+8)]+[(self.slot(sid),self.slot(sid)+50) for sid in (50,60,90)]
        # 비휘발 레지스터마다 다른 값을 넣어 원본 호출의 보존 여부를 구별한다.
        saved={register:0x12340000+index for index,register in enumerate((UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP))}
        # 비휘발 레지스터의 독립 초기값을 실제 CPU 상태에 쓴다.
        for register,value in saved.items():mu.reg_write(register,value)
        # 나머지 일반 반환 레지스터를 초기화해 이전 입력의 영향을 제거한다.
        for register in (UC_X86_REG_EAX,UC_X86_REG_EDX):mu.reg_write(register,0)
        mu.reg_write(UC_X86_REG_EFLAGS,2);mu.reg_write(UC_X86_REG_FPCW,self.control);mu.reg_write(UC_X86_REG_FPTAG,0xffff);mu.reg_write(UC_X86_REG_FPSW,0);mu.reg_write(UC_X86_REG_ECX,this);mu.reg_write(UC_X86_REG_ESP,STACK);mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        mu.emu_start(entry,STOP,count=100000)
        assert mu.reg_read(UC_X86_REG_EIP)==STOP and mu.reg_read(UC_X86_REG_ESP)==STACK+4+len(args)*4,(case,'return/stack')
        # 각 비휘발 레지스터를 호출 전 독립 값과 대조한다.
        assert all(mu.reg_read(register)==value for register,value in saved.items()),(case,'nonvolatile')
        assert not(mu.reg_read(UC_X86_REG_FPSW)&0x3800) and mu.reg_read(UC_X86_REG_FPCW)==self.control and self.read(0)==0,(case,'x87/SEH')
        if kind==0:assert mu.reg_read(UC_X86_REG_EAX)==OBJECT
        if kind==2:assert mu.reg_read(UC_X86_REG_EAX)==OUTPUT
        self.returns+=1
        # 같은 source/target/display 순서로 전체150BYTE를 직렬화한다.
        slots=b''.join(bytes(mu.mem_read(self.slot(sid),50)) for sid in (50,60,90)).hex()
        return [';'.join(self.events),bytes(mu.mem_read(OBJECT,128)).hex(),slots,bytes(mu.mem_read(OUTPUT,8)).hex()]


def generate(smoke):
    """기본027f 기대값을 먼저 고정하고 두 x87 정밀도 차이도 원문 그대로 기록한다."""
    cases=inputs();cases=cases[::113]+[sample(kind=2)] if smoke else cases
    # 각 입력을 전체 KEYS 순서로 묶어 중복 관찰이 없음을 검사한다.
    assert len({tuple(case[key] for key in KEYS) for case in cases})==len(cases)
    oracles=[Oracle(0x027f),Oracle(0x037f)];lines=[];precision_differences=[]
    # 모든 독립 입력을 두 정밀도에서 실행해 기본 결과와 차이 기록을 보존한다.
    for index,case in enumerate(cases):
        # 동일한 입력 사본을 각 CPU에 공급해 실행기 사이의 공유 변경을 막는다.
        try:outputs=[oracle.run_body(dict(case)) for oracle in oracles]
        except Exception as error:raise RuntimeError(f'크로스보우 Missile 입력{index} {case}: {error}') from error
        if outputs[0]!=outputs[1]:precision_differences.append(dict(index=index,input=case,secondary_result=outputs[1]))
        # 원본 입력 열을 지정 순서로 붙이고 기본027f의 실제 결과를 저장한다.
        lines.append('\t'.join([str(case[key]) for key in KEYS]+outputs[0]))
    if smoke:print('크로스보우 Missile 원본 smoke 통과',len(lines));return
    # 추가한 방향당3프레임 입력만 제외해 최초699행의 보존 증거를 복원한다.
    legacy_lines=[line for case,line in zip(cases,lines) if not(case['directions']==8 and case['frames']==24)]
    assert len(legacy_lines)==699 and hashlib.sha256((FIXTURE_HEADER+'\n'.join(legacy_lines)+'\n').encode('utf-8')).hexdigest()==LEGACY_FIXTURE_SHA
    FIXTURE.write_text(FIXTURE_HEADER+'\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=set(oracles[0].exports+[Path(__file__),FIXTURE,ROOT/'originals/Netstorm.exe',ROOT/'tools/decomp_gunmissile_oracle.py',ROOT/'tools/decomp_owner_oracle.py'])
    report=dict(schema=1,host=socket.gethostname(),date='2026-10-16',last_full_refined_host='VM-W11-Codex',unique_inputs=len(cases),normal_returns=len(cases)*2,
                # 생성/비행/독립 포구 종류마다 실제 고유 입력 수를 센다.
                precision_controls=[0x027f,0x037f],fixture_control=0x027f,precision_differences=len(precision_differences),precision_difference_records=precision_differences,kinds={str(kind):sum(case['kind']==kind for case in cases) for kind in range(3)},
                coverage_expansion=dict(previous_fixture_sha256=LEGACY_FIXTURE_SHA,previous_inputs=699,unchanged_inputs=len(legacy_lines),added_inputs=len(cases)-len(legacy_lines),purpose='기존699행 C++ 세 검사가 통과한 뒤 dirs8/frames24의 네 사분면 경계를 추가했다. 실패에 맞춰 기대값을 수정한 것이 아니다.'),
                # 최초 입력들 중 실제 원점/종점이 같아지는 생성자 행을 찾는다.
                zero_distance_case=dict(input_index=next(index for index,case in enumerate(cases) if case['kind']==0 and case['endx']==bits(51) and case['endy']==bits(51.5)),target_position=[51,51.5],target_dimensions=[2,3],native_endpoint=[50,50],native_origin=[50,50],scope='최초699행부터 들어 있던 실제 생성자 원점/종점 일치 입력이다. frame의0기간 처리는 별도다.'),
                metadata=dict(firingType=140,group=0,displayType=81,speed=35,effectType=0,virtualMuzzle='0050b6e8+b4 -> 0041c110'),
                # 각 정밀도 실행기의 반환/함수/경계/명령 집계를 개별 기록한다.
                executions=[dict(control=oracle.control,returns=oracle.returns,assertions=oracle.assertions,native_calls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),instructions=oracle.instructions) for oracle in oracles],
                game_gui_runs=0,limitations=['실제 포구·Missile 생성/비행·방향 helper와 하위 수치 명령을 실행한다.','합성 TYPE/SHP/표적 치수·bounds, 자산 생성/소유자/공간/사운드/피해/부착/삭제는 명시 경계다.','실제 게임/GUI·캠페인 완주·CD10.72 대조는 하지 않는다. 반사/빗나감 및 원본 assert 입력은 별도다.','표적은 TYPE71이다. TYPE146의 확률 회피와 free 발사자 직접 피해 하위는 포함하지 않는다.','fixture는 x87 027f의 원문 기대값이다. 037f 결과가 다른 행은 제외하지 않고 별도 원문 결과와 함께 기록한다.','전체128BYTE/세 슬롯150BYTE/효과 순서·반환/스택/비휘발/x87/SEH/쓰기guard를 대조한다.'],
                # 원문/도구/fixture를 경로순으로 고정해 재검증에 사용한다.
                sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(paths)})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print('크로스보우 Missile 고유 입력',len(cases),'정상 반환',len(cases)*2)


def verify():
    """원문 SHA와 고유 입력·정상 반환·분기 수를 검사하며 기대값을 재계산하지 않는다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    # 저장된 각 입력 파일의 현재 바이트가 원본 관찰 당시와 같은지 검사한다.
    for name,expected in report['sha256'].items():
        if digest(ROOT/name)!=expected:raise RuntimeError('크로스보우 Missile SHA 불일치 '+name)
    # 설명 줄을 제외한 모든 TSV 행을 같은 열 구분으로 읽는다.
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    # 전체 입력 열 조합으로 고유성을 확인한다.
    assert len(rows)==report['unique_inputs'] and len({tuple(row[:len(KEYS)]) for row in rows})==len(rows)
    # 각 정밀도에서 모든 입력이 정상 반환하고 assert에 닿지 않았음을 확인한다.
    assert report['normal_returns']==len(rows)*2 and all(item['returns']==len(rows) and item['assertions']==0 for item in report['executions'])
    # 모든 몸체 종류에 대해 실제 TSV 행 수와 보고서 집계를 대조한다.
    assert all(sum(row[0]==str(kind) for row in rows)==report['kinds'][str(kind)] for kind in range(3))
    assert report['precision_differences']==len(report['precision_difference_records'])
    # 추가된 방향당3프레임 입력을 제외해 최초699행만 선택한다.
    legacy_rows=[row for row in rows if not(row[17]=='8' and row[18]=='24')]
    assert report['coverage_expansion']['unchanged_inputs']==len(legacy_rows)==699
    assert report['coverage_expansion']['added_inputs']==len(rows)-len(legacy_rows)==4
    # 최초 행들을 같은 바이트로 직렬화해 기존 fixture 전체 SHA를 보존했음을 검사한다.
    assert hashlib.sha256((FIXTURE_HEADER+'\n'.join('\t'.join(row) for row in legacy_rows)+'\n').encode('utf-8')).hexdigest()==LEGACY_FIXTURE_SHA==report['coverage_expansion']['previous_fixture_sha256']
    print('크로스보우 Missile 원본 입력/SHA 검증 통과',len(rows))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--smoke',action='store_true');parser.add_argument('--verify',action='store_true');arguments=parser.parse_args()
    verify() if arguments.verify else generate(arguments.smoke)
