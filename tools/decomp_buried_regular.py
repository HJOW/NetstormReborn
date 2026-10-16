#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""오벨리스크 Handle004622f0과 직접 QA/TYPE/난수/기본 Handle를 제한 x86으로 관찰한다."""
import csv
import json
import socket
import struct
import sys
from pathlib import Path
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, PLAYERS, TABLE, IMAGE, STACK, STOP, digest
from unicorn import UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 입력은 별도 raw 슬롯/TYPE/반환 float 공간이며 보호 게임 메모리를 사용하지 않는다.
RAW=PLAYERS+50*50
FIXTURE=ROOT/'cpppj/tests/fixtures/buried-regular-x86.tsv'
REPORT=ROOT/'cpppj/buried-regular.json'


class Oracle(BaseOracle):
    """서명된 idiv/한 번 난수 소비는 실제 명령이고 프레임 지정만 명시 효과 경계다."""
    def __init__(self):
        """현재 정밀 표의 여섯 실제 함수만 실행을 허용한다."""
        super().__init__();self.allowed=[];self.returns=0  # 반개방 실행 범위와 정상 반환 누적이다.
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x4622f0,0x4ac550,0x4558f0,0x4acd10,0x49a840,0x496e70):  # 선택된 실제 부모와 직접 helper다.
            for part in functions[address]['ranges'].split(';'):  # 실제 불연속 몸체도 빠짐없이 허용한다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        self.mu.hook_add(UC_HOOK_MEM_WRITE,self.guard)
        self.mu.mem_write(STOP+0x100,b'\xd9\x1d'+struct.pack('<I',IMAGE)+b'\xe9'+struct.pack('<i',STOP-(STOP+0x100+11)))

    def guard(self,mu,access,address,size,value,data):
        """raw 부모/공유 난수/반환 float/스택 이외의 원본 쓰기를 거부한다."""
        ranges=((RAW,RAW+50),(0x532710,0x532714),(IMAGE,IMAGE+4),(STACK-4096,STACK+128))
        if not any(low<=address and address+size<=high for low,high in ranges):raise RuntimeError(f'오벨리스크 선택 밖 쓰기 {address:x}/{size}')

    def instruction(self,mu,address,size,data):
        """실제 Frame 호출의 앞/뒤 표시를 기록하고 명시한 후속 변경 입력만 적용한다."""
        if address in (STOP+0x100,STOP+0x106):return
        if address==0x4acee0:
            esp=mu.reg_read(UC_X86_REG_ESP);frame,flags=self.words(esp+4,2);assert mu.reg_read(UC_X86_REG_ECX)==RAW and flags==0
            self.trace.append('U:'+str(self.words(RAW+36)[0]));self.put(RAW+36,frame);self.trace.append('U:'+str(frame))
            if self.mutation:self.put(0x532710,0x87654321);self.put(RAW+12,0xbeef,2)
            self.ret(purge=8);return
        if not any(low<=address and address+size<=high for low,high in self.allowed):raise RuntimeError(f'오벨리스크 미검토 명령 {address:x}')

    def run(self,values,control):
        """고유 입력만 설치하고 원본 전체 raw/반환/난수/표시 순서를 반환한다."""
        event,count,frame,firstA,firstB,size,rng,payload,qa,self.mutation=values;self.trace=[]
        raw=bytearray(b'\xa5'*50);struct.pack_into('<IHHHBBHff',raw,0,0x50a748,0,0,0,112,0,qa,50.25,52.5);raw[33]=1;struct.pack_into('<I',raw,36,frame);self.mu.mem_write(RAW,bytes(raw))
        self.put(0x5c8464,PLAYERS);self.put(0x59ab20,TABLE);self.put(0x532710,rng);self.put(TABLE+112*500+0x130,firstA);self.put(TABLE+112*500+0x138,firstB);self.put(TABLE+112*500+0x178,size)
        self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 구별되는 비휘발 입력이다.
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,RAW);self.mu.mem_write(STACK,struct.pack('<4I',STOP+0x100,event,count,payload));self.mu.emu_start(0x4622f0,STOP,count=10000)
        assert self.mu.reg_read(UC_X86_REG_ESP)==STACK+16 and self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==0
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 호출 규약을 매번 확인한다.
        self.returns+=1
        return [bytes(self.mu.mem_read(RAW,50)).hex(),self.words(IMAGE)[0],self.words(0x532710)[0],';'.join(self.trace) or '-']


def cases():
    """B구간0/1/8과 음수 signed 횟수·난수·실제 효과 후 현재 상태를 교차한다."""
    for count in (0,1,2,8,9,17,0x80000000,0xffffffff):  # 양수/음수 signed idiv 경계다.
        for size in (0,1,8):  # 없는 B구간도 divisor1로 실제 A분기를 실행한다.
            for rng in (0,1,0xffffffff):  #0은 원본 초기 씨드 치환도 포함한다.
                for frame in (0,17):  # 기존 프레임이 같아도 실제 두 번 표시 갱신한다.
                    for mutation in (0,1):yield (0,count,frame,0,8 if size else 0xffffffff,size,rng,0,114,mutation)  # Frame 뒤 변경한 난수를 다시 읽는다.
    for event in (1,2,6699,6700):  # 기본 Handle이며 Damageable 사건으로 잘못 보내지 않는다.
        for count in (0,0xffffffff):  # event0 외에는 횟수를 해석하지 않는다.
            for qa in (0,114,65535):  # event1의 QA 주소 계산은 반환값을 버린다.
                for payload in (0,0x80000000,0xbf800000,0x7fc12345):yield (event,count,17,0,8,8,0x12345678,payload,qa,0)  # 원본 float 반환 비트를 관찰한다.


def generate():
    """두 x87 정밀도에서 정상 반환 결과가 같은 고유 입력을 fixture/출처와 저장한다."""
    oracle=Oracle();rows=[]
    for values in cases():  # 같은 입력의 반복은 새 고유 입력으로 세지 않는다.
        first=oracle.run(values,0x027f);second=oracle.run(values,0x037f)
        if first!=second:raise RuntimeError('오벨리스크 x87 정밀도 차이')
        rows.append([','.join(map(str,values)),*first])
    FIXTURE.write_text('# inputs raw return rng trace\n'+'\n'.join('\t'.join(map(str,row)) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files=[Path(__file__),ROOT/'tools/decomp_local_mission_transport.py',ROOT/'originals/Netstorm.exe',ROOT/'extracted/refined/originals/functions.tsv',FIXTURE,ROOT/'cpppj/src/o/RawBuriedRegular.h',ROOT/'cpppj/src/o/RawBuriedRegular.cpp',ROOT/'cpppj/tests/BuriedRegularTests.cpp',ROOT/'cpppj/src/client/RawCampaignSession.cpp',ROOT/'cpppj/src/o/CMakeLists.txt',ROOT/'cpppj/tests/CMakeLists.txt']
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),last_full_decompile_host='VM-W11-Codex',last_full_decompile_date='2026-10-16',total=len(rows),normal_returns=oracle.returns,os_calls=0,sha256={path.relative_to(ROOT).as_posix():digest(path) for path in files},limits=['원본 Frame 지정은 두 표시/현재 프레임 쓰기의 명시 경계다. C++ 검사는 실제 SquidFrame을 조합한다.','직접TYPE/QA/기본Handle/난수/서명된idiv는 실제10.78 명령이다.','Frame 후 외부 난수/QA 변경은 명시 입력이며 원본 재조회 순서를 관찰한다.','CD판/오벨리스크 보상 추첨/GUI/자연 완주는 이번 범위가 아니다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('오벨리스크 Handle',len(rows),'고유 입력',oracle.returns,'정상 반환')


def verify():
    """고정 출처와 중복 없는 입력/두 정밀도 정상 반환 수를 감사한다."""
    report=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in report['sha256'].items():  # 원본/제품/검사/도구의 전체 바이트를 비교한다.
        if digest(ROOT/name)!=value:raise RuntimeError('SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==report['total'] and len({row[0] for row in rows})==len(rows) and report['normal_returns']==len(rows)*2 and report['os_calls']==0
    print('오벨리스크 Handle',len(rows),'개 SHA/반환/고유 입력 감사 통과')


# 게임/창을 실행하지 않고 명시 요청에서만 보호 PE의 제한 명령을 관찰한다.
if __name__=='__main__':verify() if '--verify' in sys.argv else generate()
