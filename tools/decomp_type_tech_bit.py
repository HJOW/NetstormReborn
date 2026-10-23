#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""기존 TYPE 초기화/전체 callback 관측기를 재사용해 techBit 실제 DWORD를 먼저 고정한다."""
from pathlib import Path
import argparse
import hashlib
import json
import socket
import struct

from decomp_archertype_oracle import ArcherTypeOracle, ROOT, TYPES, AST, FRAMES, SHAPE, META, float_bits
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW

# 최초 Native 자료는 기존 도구/fixture/제품과 분리하고 이후 덮어쓰지 않는다.
FIXTURE=ROOT/'cpppj/tests/fixtures/type-tech-bit-x86.tsv'
REPORT=ROOT/'extracted/cpp-type-tech-bit-20261024-native.json'
CONTROLS=(0x027f,0x037f)  # 두 실제 x87 정밀도에서 동일한 숫자 속성을 읽는다.


def digest(path):
    """원본/도구/첫 출력의 실제 바이트 SHA를 기록한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cases():
    """순서·대소문자·float32와 signed64 절삭 하위DWORD의 입력만 지정한다."""
    values=(-1,-0.75,0,0.75,1,1.75,31,31.75,32,255,256,-2,-32,-32.75,2147483520,2147483648,
            -2147483648,-2147483904,4294967296,4294967808,1e20,-1e20)
    result=[[],*[[('techBit',float_bits(value))] for value in values]]
    result.extend([[('TECHBIT',float_bits(3.75))], [('TeChBiT',float_bits(-1)),('techBit',float_bits(7.75))],
                   [('techBit',float_bits(31)),('techBit',float_bits(-1))], [('techBit',float_bits(3)),('techBit',float_bits(3))]])
    return result


class TechBitOracle(ArcherTypeOracle):
    """이전 전체 callback/CRT/후처리를 그대로 실행하고 선택 필드 쓰기만 추가 수집한다."""
    def __init__(self):
        super().__init__('originals')
        self.control=CONTROLS[0]  # 현재 호출에 공급하는 외부 FPCW다.
        self.trace=[]  # 실제 +1ec 쓰기의 주소/폭/값 순서다.
        self.pairs=[]  # 현재 AST 속성 입력이다.
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.observe_tech_store)

    def on_instruction(self,mu,address,size,data):
        """기존 호출기가 공급하는 CW를 진입 때 명시 정밀도 입력으로만 바꾼다."""
        if address in (self.m['init'],self.m['callback'],self.m['post']):
            mu.reg_write(UC_X86_REG_FPCW,self.control)
        super().on_instruction(mu,address,size,data)

    def observe_tech_store(self,mu,access,address,size,value,data):
        """실제188TYPE의 techBit DWORD와 겹치는 쓰기를 결과에서 관찰한다."""
        relative=address-TYPES
        if 0<=relative<188*500 and relative%500==0x1ec:
            self.trace.append(f'{relative//500}:{size}:{value&((1<<(size*8))-1):x}')

    def ast(self,orientation,layout):
        """보호35클러스터 입력은 재사용하고 경제/다른 속성 대신 명시 techBit 숫자 AST만 만든다."""
        _,frames=super().ast('-',0)
        properties=[]  # 원문 linked-list를 읽는 기존 전체 callback에 공급할 입력 순서다.
        for name,bits in self.pairs:  # 원본은 같은 속성의 뒤 대입까지 실제 실행한다.
            properties.append(self.node(9,0,self.node(1,0,self.string(name)),self.node(2,0,bits)))
        return self.linked(properties,7),frames

    def observe(self,pairs,control):
        """실제 초기화 접두와 전체 callback/후처리를 구별해 같은 TYPE 필드와 store를 수집한다."""
        self.control=control;self.trace=[];self.pairs=pairs
        self.initialize()
        initial=bytes(self.mu.mem_read(TYPES,188*500))
        initial_bits=b''.join(initial[index*500+0x1ec:index*500+0x1f0] for index in range(188)).hex()
        initial_trace=';'.join(self.trace)
        self.trace=[]
        self.run('-',0)
        raw=bytes(self.mu.mem_read(TYPES+71*500,500))
        bits=raw[0x1ec:0x1f0].hex()
        callback_trace=';'.join(self.trace) or '-'
        if self.mu.reg_read(UC_X86_REG_FPCW)!=control or self.mu.reg_read(UC_X86_REG_FPSW)&0x3800:
            raise RuntimeError('techBit x87 제어/스택 차이')
        return [initial_bits,initial_trace,bits,callback_trace,raw.hex()]


def generate():
    """제품 techBit 사본 이전에만 최초 자료를 생성하고 같은 명시 입력 두 정밀도를 대조한다."""
    proposal=ROOT/'extracted/cpp-type-tech-bit-20261024-proposed.patch'
    if FIXTURE.exists() or REPORT.exists() or proposal.exists():
        raise RuntimeError('최초 Native/제품 제안이 이미 존재하여 덮어쓰지 않습니다')
    oracle=TechBitOracle();rows=[]
    for pairs in cases():  # 기대 필드값이나 타입번호 뺄셈을 계산하지 않고 실제 PE에서만 읽는다.
        observations=[oracle.observe(pairs,control) for control in CONTROLS]
        if observations[0]!=observations[1]:
            raise RuntimeError('techBit 정밀도별 원시 출력 차이')
        rows.append([json.dumps(pairs,separators=(',',':')),*observations[0]])
    FIXTURE.write_text('# 입력 AST pair(name,float32bits) / 초기188DWORD / 초기실제store / callback techBit4BYTE / callback실제store / 전체TYPE500BYTE\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    sources=[Path(__file__),ROOT/'tools/decomp_archertype_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/shp.py',
             ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'originals/d/_shapes.shp',ROOT/'extracted/refined/originals/Netstorm.c',
             ROOT/'extracted/refined/originals/functions.tsv',FIXTURE,*oracle.exports]
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon / 2026-10-23',sourceAbsentBeforeObservation=True,
                uniqueInputs=len(rows),controls=list(CONTROLS),observations=len(rows)*2,initialPrefixes=oracle.prefix,normalCalls=oracle.normal,
                instructions=oracle.instructions,assertions=oracle.assertions,nativeCalls=dict(oracle.native_calls),boundaries=dict(oracle.boundaries),
                selectedField='TYPE+1ec/techBitIndex',actualTypeNumber=71,gameOrOsExecuted=False,
                sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(set(sources))},
                limits=['0049ebb0는전체188TYPE초기화후실제parser진입에서명시정지하는접두다.전체게임로딩완료가아니다.',
                        '0049c3b0전체callback과0049b0d0전체후처리는보호sunArcher35클러스터+명시techBit숫자AST로정상반환한다.',
                        '전체500BYTE는증거보존이며C++대조범위는선택techBit필드/초기188값/속성순서뿐이다.',
                        '실제파일lexer/문법parser/고장난AST/필수Child에서의명시중단은이번추가관측범위밖이다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('type-tech-bit',len(rows),oracle.prefix,oracle.normal,oracle.instructions)


def verify():
    """최초 Native를 재실행하거나 덮지 않고 출처/행수/물리폭만 확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['sha256'].items():  # 원본과 기존 관측기까지 실제 SHA로 확인한다.
        if digest(ROOT/name)!=value:
            raise RuntimeError('techBit 최초 SHA 차이: '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    if len(rows)!=len(cases()) or report['observations']!=2*len(rows) or report['initialPrefixes']!=2*len(rows) or report['normalCalls']!=4*len(rows):
        raise RuntimeError('techBit 최초 관측 수 차이')
    for row,pairs in zip(rows,cases()):  # 원문 결과값을 재계산하지 않고 입력 계획과 BYTE 폭만 감사한다.
        if len(row)!=6 or json.loads(row[0])!=[list(pair) for pair in pairs] or len(bytes.fromhex(row[1]))!=188*4 or len(bytes.fromhex(row[3]))!=4 or len(bytes.fromhex(row[5]))!=500:
            raise RuntimeError('techBit 원시 입력/물리폭 차이')
    print('type-tech-bit verify',len(rows),'0')


if __name__=='__main__':
    parser=argparse.ArgumentParser()  # 최초 선관측과 정적 재검증을 구별한다.
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate()
