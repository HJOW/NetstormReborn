#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 Missile Frame과 클라우드 플로터 회피/공유 PRNG의 최초 원문을 격리 관측한다."""
import argparse
import csv
import json
import socket
import struct
from pathlib import Path
from decomp_thunder_missile import Oracle as BaseOracle, sample, KEYS as BASE_KEYS
from decomp_gunmissile_oracle import OBJECT
from decomp_owner_oracle import ROOT, TYPES, digest
from unicorn.x86_const import UC_X86_REG_ESP

KEYS = (*BASE_KEYS, 'targetType', 'floaterType', 'cutoff', 'seed', 'sourceType', 'displayType')  # 출력과 독립인 원문 타입/예산/시드 입력이다.
FIXTURE = ROOT/'cpppj/tests/fixtures/missile-floater-x86.tsv'  # 기존 투사체 기대값을 변경하지 않는 새 최초 관측이다.


def case(**changes):
    """생성자를 대신 실행하지 않고 독립2초 비행 이미지의 입력만 만든다."""
    result = sample(kind=1, endx=0x42480000, endy=0x42b40000, fraction=4, previous=1, impactSound=0)
    result.update(targetType=146, floaterType=146, cutoff=800, seed=0xfed, sourceType=131, displayType=132)
    result.update(changes);return result


def cases():
    """확률 경계/미선택 충돌/현재 타입과 다섯 실제 발사자의 공유 Frame을 독립 입력한다."""
    result = []
    inverse = pow(0x10003, -1, 1 << 32)  # 희망 상위WORD를 만드는 시드 입력만 구하며 회피 결과는 계산하지 않는다.
    seeds = (0, 1, 0xfed, 0xffffffff, 0x12345678)+tuple((((word << 16)-3)*inverse)&0xffffffff for word in (0,1,32767,32768,32769,52428,52429,65535))
    for source, display in ((71,83),(140,83),(128,83),(131,132),(129,130)):  # group0/썬/썬더/아이스의 같은 실제 Frame이다.
        for seed in seeds:  # 상태0의 대체 시드와 임계값 양쪽/동률을 포함한다.
            for cutoff in (-1,0,1,500,800,1000,1001):  # signed DWORD800과 경계 밖 값도 실제 FILD로 읽는다.
                result.append(case(sourceType=source,displayType=display,seed=seed,cutoff=cutoff))
    for fraction in (0,1,3,4,5,8,12):  # 충돌 밖/범위 만료와 회피가 겹치는 경우를 구별한다.
        for state in range(4):  # free/dead의 원문 관문이다.
            for corner in (0,1):  # 코너 충돌에도 같은 회피를 적용하는 원문 순서다.
                result.append(case(fraction=fraction,targetstate=state,corner=corner))
    for extra, passed, previous in ((16,0,1),(0,1,1),(0,0,0)):  # 충돌 억제 분기는 난수를 소비하지 않는다.
        result.append(case(extra=extra,passed=passed,previous=previous))
    for target, current in ((71,146),(146,71),(71,71)):  # 하드코딩146이 아닌 현재 전역과 현재 TYPE를 비교한다.
        result.append(case(targetType=target,floaterType=current))
    return list({tuple(value[key] for key in KEYS):value for value in result}.values())


class Oracle(BaseOracle):
    """기존 전체 Frame 관측에 실제455930 난수/공유 DWORD만 더한다."""
    def __init__(self):
        """현재 PC의 정밀 범위와 보호 PE를 사용하고 추가 난수 몸체를 승인한다."""
        super().__init__(0x027f)
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        self.entries.add(0x455930)
        for part in facts[0x455930]['ranges'].split(';'):  # 실제 난수의 모든 함수 조각만 추가한다.
            low,high = (int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))

    def run_body(self, value):
        """실제 첫 명령 이전의 입력 설치와 최초 반환/전체 상태를 구별한다."""
        self.primed = False
        output = super().run_body(value)
        return [*output,str(self.read(0x532710))]

    def on_instruction(self, mu, address, size, data):
        """명시 raw 입력만 바꾸고 Frame/난수/회피/이후 효과 판단은 실제 명령으로 실행한다."""
        if not self.primed:
            self.primed = True
            target,source,display = (self.case[key] for key in ('targetType','sourceType','displayType'))
            source_bytes = bytes(mu.mem_read(TYPES+131*self.type_stride,self.type_stride))
            display_bytes = bytes(mu.mem_read(TYPES+132*self.type_stride,self.type_stride))
            mu.mem_write(TYPES+source*self.type_stride,source_bytes);mu.mem_write(TYPES+display*self.type_stride,display_bytes)
            mu.mem_write(TYPES+source*self.type_stride+0x9c,struct.pack('<I',0 if source in (71,140) else 1))
            mu.mem_write(self.slot(50)+10,bytes([source]));mu.mem_write(self.slot(60)+10,bytes([target]));mu.mem_write(self.slot(90)+10,bytes([display]))
            mu.mem_write(0x5412a0,struct.pack('<I',self.case['floaterType']));mu.mem_write(0x540bbc,struct.pack('<i',self.case['cutoff']));mu.mem_write(0x532710,struct.pack('<I',self.case['seed']))
            self.write_ranges.append((0x532710,0x532714))
        if address == 0x455930:
            args = (self.read(mu.reg_read(UC_X86_REG_ESP)+4),self.read(mu.reg_read(UC_X86_REG_ESP)+8))
            assert tuple(args)==(0,0x447a0000);self.events.append('N:0:1148846080')
        if address == 0x482190:  # 큰 반사 생성은 기존 초기 아이스 관측과 같은 명시 효과 경계다.
            esp = mu.reg_read(UC_X86_REG_ESP);target = self.read(esp);self.events.append('R:90:60')
            from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP
            mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,target);return
        super().on_instruction(mu,address,size,data)


def main():
    """독립 두 관측을 먼저 보존하고 명시 요청에서만 새 fixture를 생성/검증한다."""
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True);parser.add_argument('--fixture',action='store_true');parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    output = ROOT/args.output;output.mkdir(parents=True,exist_ok=False)
    values = cases();first,replay = Oracle(),Oracle()
    observed = [[str(value[key]) for key in KEYS]+first.run_body(value) for value in values]
    repeated = [[str(value[key]) for key in KEYS]+replay.run_body(value) for value in values]
    for name,result in (('first.json',observed),('replay.json',repeated)):  # 각 최초 관측은 별도 파일에 남긴다.
        (output/name).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
    assert observed==repeated
    payload = '# '+'/'.join(KEYS)+'/실제원문출력5열\n'+''.join('\t'.join(row)+'\n' for row in observed)
    if args.verify: assert FIXTURE.read_bytes()==payload.encode('utf-8')
    elif args.fixture:
        with FIXTURE.open('xb') as stream: stream.write(payload.encode('utf-8'))
    paths = ['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','extracted/refined/originals/Netstorm.c','tools/decomp_thunder_missile.py','tools/decomp_gunmissile_oracle.py','tools/decomp_owner_oracle.py','tools/decomp_missile_floater.py']
    if args.fixture or args.verify: paths.append(str(FIXTURE.relative_to(ROOT)).replace('\\','/'))
    report = {'unit':611,'host':socket.gethostname(),'cases':len(values),'observations':2*len(values),'differences':[],'fixtureWritten':args.fixture,'verificationOnly':args.verify,'sourceSha256':{name:digest(ROOT/name) for name in paths},'evidenceSha256':{path.name:digest(path) for path in output.iterdir() if path.is_file()},'scope':'실제00482340 전체Frame/00455930 PRNG와 현재00540bbc signed절삭없는비교. 자산 이동/표적Bounds/피해/표시·음향/반사 생성은 기존 명시 경계. 새 게임/GUI/특정적공격/포획/희생/승리/신규MP 없음.'}
    destination = ROOT/'cpppj/missile-floater-native-20261030.json' if args.fixture or args.verify else output/'report.json'
    destination.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'cases':len(values),'observations':2*len(values),'differences':[]}))


if __name__=='__main__':
    main()
