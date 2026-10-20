#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 미션 techAllowed의 항목 재조회·이름 검색·허용 표 쓰기를 제품 변경 전에 관측한다."""
import argparse
import itertools
import json
import socket
import struct
from decomp_mission_abort import Oracle as BaseOracle, ROOT, POOL, TEXT, TYPES, STACK, digest
from decomp_cursor_position import ranges
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP
from shp import TYPE_LOAD_ORDER
from cpp_goal_stage_entry_smoke import MISSIONS, ARCHIVE
from cpp_menu_smoke import source
from cpp_fort_smoke import script_value

MISSION,BUFFER,KEY=POOL+0x88000,TEXT+0x6000,TEXT+0x7000  # 미션·512바이트 목록·설정 키의 격리 주소다.
TABLE=0x5ca0d0  # 원본 허용 표의 첫 DWORD이며 앞 한 칸도 잘못된 타입 관측에만 보존한다.
FIXTURE=ROOT/'cpppj/tests/fixtures/mission-technology-x86.tsv'  # 새 독립 원문이며 기존 TotalMade fixture를 바꾸지 않는다.
REPORT=ROOT/'cpppj/mission-technology-native.json'  # 실제 몸체와 명시 Config/임시 버퍼 경계를 구별한다.


def inputs():
    """빈 항목·대소문자·전환·알 수 없는 이름·현재 설정 재조회 및 보호11개 목록을 준비한다."""
    texts=['','deny','allow','all','DENY;ALL;ALLOW;windVortex','deny;all;allow;sunArcher;windVortex','deny;windVortex;allow;windVortex','deny;;all;allow;sunArcher',';deny;all','allow;all;deny;sunArcher','deny;all;allow;sunArcher;','deny;all;allow;missingType','deny;all; allow;windVortex','deny;all;allow;sunArcher\0;deny;all']
    rows=[]
    # 같은 목록을 서로 다른 초기 표에서 관측해 유지되는 칸도 구별한다.
    for text,seed in itertools.product(texts,(0,3,0xffffffff)):rows.append((text,'',seed))
    # 같은 인덱스 규칙이 매회 현재 Config 목록을 다시 읽는지 구별한다.
    for text in ('deny;all;allow;windVortex','allow;all;deny;sunArcher'):
        for changed in ('allow;windVortex;;sunArcher','deny;all;allow;sunArcher',''):
            rows.append((text,changed+'\0',7))
    # 보호 스크립트의 실제 초기 목록은 별도 이름 기대값 없이 같은 실제 PE에 공급한다.
    for mission in MISSIONS:
        text=source(ROOT/'originals',ARCHIVE,f'd/{mission}.english').decode('cp1252');rows.append((script_value(text,'techAllowed') or '','',17))
    return list(dict.fromkeys(rows))


class Oracle(BaseOracle):
    """482eb0/482d50·실제 이름 검색·두 허용 쓰기와 CRT만 실행한다."""
    def __init__(self):
        """격리 PE와 정확한 함수 범위 및 독립 타입 이름 입력을 연결한다."""
        super().__init__();ranges(self,(0x482eb0,0x482d50,0x49a860,0x4c23c0,0x4c23e0));self.returns=0;self.lookups=0;self.events=[]
        self.put(0x59ab20,TYPES);self.put(0x541348,188);self.mu.mem_write(TYPES,bytes(188*500))
        # 원본 자산 로딩 순서의 번호70부터 같은20바이트 이름 필드를 공급한다.
        for number,name in enumerate(TYPE_LOAD_ORDER,70):self.mu.mem_write(TYPES+number*500+4,name.encode('ascii')+b'\0')

    def guard(self,mu,access,address,size,value,data):
        """실제 쓰기는 허용 표와 앞 한 칸 또는 기존 격리 버퍼/스택만 허용한다."""
        if TABLE-4<=address and address+size<=TABLE+256*4:return
        super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """Config의 현재 문자열과 임시 버퍼/서식만 공급하며 목록 분할/분기는 실제 명령에 맡긴다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,4)
        if address==0x4b9320:
            assert args[0]==512;self.ret(BUFFER);return
        if address==0x4a89c0:
            assert self.string(args[1])==b'mission.%s' and self.string(args[2])==b'techAllowed'
            self.mu.mem_write(args[0],b'mission.techAllowed\0');self.ret(args[0]);return
        if address==0x4409d0:
            assert self.string(args[0])==b'mission.techAllowed' and args[1]==BUFFER
            value=self.changed if self.change and self.lookups else self.text;self.lookups+=1
            self.mu.mem_write(BUFFER,value.split(b'\0',1)[0]+b'\0');self.ret(1,8);return
        if address==0x482d50:self.events.append('G,'+str(args[1]))
        if address==0x4c23c0:self.events.append('A,'+str(args[0]))
        if address==0x4c23e0:self.events.append('T,'+str(args[0])+','+str(args[1]))
        super().instruction(mu,address,size,data)

    def sample(self,row):
        """원본 전체257DWORD와 조회/쓰기 순서를 정상 반환 뒤 그대로 읽는다."""
        text,changed,seed=row;self.operation='T';self.text=text.encode('cp1252');self.changed=changed.encode('cp1252');self.change=bool(changed);self.lookups=0;self.events=[]
        self.mu.mem_write(MISSION,bytes(208));self.mu.mem_write(TABLE-4,struct.pack('<257I',*[((index*7)^seed)&0xffffffff for index in range(257)]))
        self.call(0x482eb0,[],MISSION);self.returns+=1
        return bytes(self.mu.mem_read(TABLE-4,257*4)).hex(),'|'.join(self.events)


def main():
    """최초 원문을 한 번 저장하거나 보호 해시만 확인하며 게임/OS/제품 검사는 실행하지 않는다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        record=json.loads(REPORT.read_text('utf-8'))
        # 보존된 첫 관측의 모든 입력 파일을 읽기 전용으로 확인한다.
        for name,expected in record['sha256'].items():assert digest(ROOT/name)==expected,name
        print('미션 기술 허용 최초 PE/목록/이름/표 관측 보존 확인');return
    if FIXTURE.exists() or REPORT.exists():raise RuntimeError('최초 관측 덮어쓰기 금지')
    oracle=Oracle();rows=inputs();lines=['# 현재 목록 재조회/실제 이름 검색/255칸 쓰기와256번째·앞 한 칸 보존이다.']
    # 입력 목록과 표 결과는 제품이 생기기 전 실제 PE에서만 수집한다.
    for row in rows:
        table,trace=oracle.sample(row);lines.append('\t'.join((row[0].encode('cp1252').hex() or '-',row[1].encode('cp1252').hex() or '-',str(row[2]),table,trace)))
    FIXTURE.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    paths=['originals/Netstorm.exe','originalCD/NETSTORM.EXE','originals/netstorm.tarc','extracted/refined/originals/Netstorm.c','extracted/refined/originals/functions.tsv','tools/decomp_mission_technology.py','tools/shp.py','cpppj/tests/fixtures/mission-technology-x86.tsv']
    record={'host':socket.gethostname(),'lastDecompileHost':'HJOW-Athlon / 2026-10-20','inputs':len(rows),'normalReturns':oracle.returns,'instructions':oracle.instructions,'scope':'실제482eb0/482d50/49a860/4c23c0/4c23e0와CRT. Config 조회/512임시버퍼/키서식/TLS는 명시 경계다. 잘못된 이름은 실제 표 앞 한 칸 쓰기를 관측하며 제품 메모리 안전 동작과 구별한다.','gameExecuted':False,'osExecuted':False,'sha256':{name:digest(ROOT/name) for name in paths}}
    REPORT.write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({key:record[key] for key in ('inputs','normalReturns','instructions')}))


if __name__=='__main__':
    main()
