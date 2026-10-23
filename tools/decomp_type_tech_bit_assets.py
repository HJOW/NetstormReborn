#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호 TYPE116개의 실제 techBit 속성을 현재 PE 번호에 공급해188필드만 추가 관측한다."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import socket
import struct

from decomp_type_tech_bit import TechBitOracle,ROOT,CONTROLS,digest
from decomp_archertype_oracle import TYPES,FRAMES,SHAPE
from shp import load_type_texts

# 최초27합성 관측을 변경하지 않는 별도 원본 자산 필드 관측이다.
FIXTURE=ROOT/'cpppj/tests/fixtures/type-tech-bit-assets-x86.tsv'
REPORT=ROOT/'extracted/cpp-type-tech-bit-assets-20261024-native.json'


def current_name(oracle,address):
    """원본 PE 로딩 표의 NUL 문자열을 직접 읽고 이름 정규화로 순서를 바꾸지 않는다."""
    result=bytearray()
    while True:  # 보호 문자열의 실제 첫 NUL까지 읽는다.
        value=oracle.read(address,1)
        if not value:
            return result.decode('ascii')
        result.append(value);address+=1
        if len(result)>128:
            raise RuntimeError('PE 타입 이름 문자열 범위')


def observe(control):
    """기존 실제 초기화와각실제번호의전체callback을실행하되다른속성/기하완료는주장하지않는다."""
    oracle=TechBitOracle();oracle.control=control;oracle.initialize()
    texts=load_type_texts(str(ROOT/'originals'))
    rows=[]
    for index in range(116):  # 기존 원문 TypeLoadOrder의 PE116포인터를 그대로 읽는다.
        name=current_name(oracle,oracle.read(0x540dd0+index*4))
        text=texts[name.lower()]
        lines=[line for line in text.splitlines() if 'techbit' in line.lower()]
        pairs=[]
        for line in lines:  # 보호 파일의 현재 techBit 줄이 숫자 대입 하나임을 명시 검사한다.
            match=re.fullmatch(r'\s*(techBit)\s*=\s*(-?\d+)\s*;\s*',line,re.IGNORECASE)
            if not match:
                raise RuntimeError('현재 보호 techBit 문법 경계 밖: '+name+':'+line)
            bits=int.from_bytes(struct.pack('<f',float(match[2])),'little')
            pairs.append((match[1],bits))
        oracle.pairs=pairs;oracle.trace=[]
        # 같은 현재 TYPE 저장소의 실제 번호에 callback이 쓰며초기값을제품기대에서계산하지않는다.
        oracle.mu.mem_write(oracle.m['index'],struct.pack('<I',index+1))
        oracle.mu.mem_write(oracle.m['shape'],struct.pack('<I',SHAPE))
        oracle.mu.mem_write(oracle.m['shapeCursor'],struct.pack('<I',oracle.sunBlock))
        oracle.mu.mem_write(oracle.m['astBase'],bytes(4));oracle.mu.mem_write(oracle.m['astEnd'],bytes(4));oracle.mu.mem_write(FRAMES,bytes(0x1000))
        properties,frames=oracle.ast('-',0)
        oracle.invoke(oracle.m['callback'],(0,0,properties,frames))
        number=70+index
        rows.append([str(number),name,hashlib.sha256(text.encode('latin-1')).hexdigest(),json.dumps(pairs,separators=(',',':')),
                     bytes(oracle.mu.mem_read(TYPES+number*500+0x1ec,4)).hex(),';'.join(oracle.trace) or '-'])
    raw=b''.join(bytes(oracle.mu.mem_read(TYPES+number*500+0x1ec,4)) for number in range(188))
    return oracle,rows,raw.hex()


def generate():
    """사본 필드 구현 전 최초 자산대조를 기록하고 두정밀도전체188필드가같은지확인한다."""
    if FIXTURE.exists() or REPORT.exists() or (ROOT/'extracted/cpp-type-tech-bit-20261024-proposed.patch').exists():
        raise RuntimeError('최초 자산 Native 또는 제품제안이 이미 존재합니다')
    first,rows,raw=observe(CONTROLS[0]);second,other,other_raw=observe(CONTROLS[1])
    if rows!=other or raw!=other_raw:
        raise RuntimeError('실제 techBit 자산 두 정밀도 차이')
    FIXTURE.write_text('#188필드\t'+raw+'\n#실제번호/PE이름/보호type본문SHA/techBit AST pair/원문필드4BYTE/원문store\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    sources=[Path(__file__),ROOT/'tools/decomp_type_tech_bit.py',ROOT/'tools/decomp_archertype_oracle.py',ROOT/'tools/decomp_owner_oracle.py',ROOT/'tools/shp.py',
             ROOT/'originals/Netstorm.exe',ROOT/'originals/netstorm.tarc',ROOT/'originals/d/_shapes.shp',FIXTURE,*first.exports]
    report=dict(host=socket.gethostname(),lastDecompileHost='HJOW-Athlon / 2026-10-23',sourceAbsentBeforeObservation=True,
                initialPrefixes=2,normalCallbacks=first.normal+second.normal,assetTypes=116,fieldCount=188,selectedProperties=sum(bool(json.loads(row[3])) for row in rows),
                controls=list(CONTROLS),precisionDifferences=0,instructions=first.instructions+second.instructions,gameOrOsExecuted=False,
                sha256={path.relative_to(ROOT).as_posix():digest(path) for path in sorted(set(sources))},
                limits=['로딩순서는실제PE116포인터다.각번호의현재TYPE+1ec를전체callback의실제대입에서읽는다.',
                        '속성AST는현재보호type본문의techBit숫자줄만공급한다.본문SHA와입력pair를모두보존한다.',
                        '기하/클러스터는기존보호sunArcher35클러스터를모든callback에공급하는미선택필드경계다.다른각타입의실제geometry나전체TYPE복원을주장하지않는다.',
                        '전체188techBit의실제초기-1→해당type실제속성대입만대조한다.lexer/다른속성/전체게임설치는범위밖이다.'])
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('type-tech-bit assets',len(rows),report['selectedProperties'],report['normalCallbacks'],report['instructions'])


def verify():
    """원본을재실행하지않고첫자산대조출처와188DWORD/116행폭만확인한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['sha256'].items():  # 최초 자산과관측기를덮지않는정적감사다.
        if digest(ROOT/name)!=value:
            raise RuntimeError('자산 techBit 최초SHA차이: '+name)
    lines=FIXTURE.read_text(encoding='utf-8').splitlines()
    rows=[line.split('\t') for line in lines if line and not line.startswith('#')]
    if len(bytes.fromhex(lines[0].split('\t')[1]))!=188*4 or len(rows)!=116 or any(len(row)!=6 or len(bytes.fromhex(row[4]))!=4 for row in rows):
        raise RuntimeError('자산 techBit 관측물리폭/행수차이')
    print('type-tech-bit assets verify',len(rows),'0')


if __name__=='__main__':
    parser=argparse.ArgumentParser()  # 첫자산선관측과정적검증을분리한다.
    parser.add_argument('--verify',action='store_true')
    arguments=parser.parse_args()
    verify() if arguments.verify else generate()
