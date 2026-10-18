#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제71/128의 캡처용+78/+a4를 원본 실행으로 관측한다. 그림/프레임헤더는 선언 경계다."""
import argparse,csv,json,struct
from decomp_temple_draw import Oracle as TempleOracle,ROOT,TYPES,RAW,CODES,STACK,STOP,digest
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
FIXTURE=ROOT/'cpppj/tests/fixtures/burning-archer-capture-x86.tsv' # 새 실제 가상 경로의 입력/출력이다.
REPORT=ROOT/'cpppj/burning-archer-capture.json' # 파생 캡처 픽셀/전투 완주와 구별한다.

class Oracle(TempleOracle):
    """기존 정밀 TYPE 조회와 정상 반환 진단에 실제 썬 디스크 발사기를 추가한다."""
    def __init__(self):
        """원본 두VT와 실제 함수 범위를 확인한다."""
        super().__init__('windVortex','ABCP',13);self.allowed.append((0x41c100,0x41c103))
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in (0x46a6e0,0x49a840):
            for part in facts[entry]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        for table,draw in ((0x50b888,0x46a6e0),(0x50b9b8,0x41c100)):
            assert struct.unpack('<I',self.mu.mem_read(table+0x78,4))[0]==0x4acfe0 and struct.unpack('<I',self.mu.mem_read(table+0xa4,4))[0]==draw
        self.mu.mem_write(0x5411a4,struct.pack('<I',83));self.mu.mem_write(TYPES+83*500+0xdc,struct.pack('<I',0x12340083))
    def instruction(self,mu,address,size,data):
        """현재 header 조회와 그림 요청만 관찰 경계로 처리한다."""
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x419850:
            args=struct.unpack('<3I',mu.mem_read(esp,12));assert args[1:]==(0,self.frame) and mu.reg_read(UC_X86_REG_ECX)==TYPES+71*500;value=CODES+512;purge=8
        elif address==0x497280:
            x,y,shape,frame=struct.unpack('<4I',mu.mem_read(esp+4,16));assert mu.reg_read(UC_X86_REG_ECX)==RAW
            if shape==0:shape,frame=self.number,self.frame
            else:assert shape==0x12340083;shape=83
            self.events.append(','.join(map(str,(shape,frame,x,y))));value=0;purge=16
        else:super().instruction(mu,address,size,data);return
        mu.reg_write(UC_X86_REG_EAX,value);mu.reg_write(UC_X86_REG_ESP,esp+4+purge);mu.reg_write(UC_X86_REG_EIP,struct.unpack('<I',mu.mem_read(esp,4))[0])
    def run_case(self,number,base,frame,word,extra):
        """실제 raw/헤더 입력과 기본 반환/추가 순서를 읽는다."""
        self.number,self.frame=number,frame;raw=bytearray(50);struct.pack_into('<I',raw,0,0x50b888 if number==71 else 0x50b9b8);raw[10]=number;struct.pack_into('<H',raw,12,word);struct.pack_into('<I',raw,36,frame);raw[40]=extra;self.mu.mem_write(RAW,bytes(raw));self.mu.mem_write(TYPES+number*500+0x128,struct.pack('<i',base));self.mu.mem_write(CODES+512+0x20,struct.pack('<hh',-7,11));self.events=[];actual=self.call(0x4acfe0,());self.call(0x46a6e0 if number==71 else 0x41c100,(311,233));assert bytes(self.mu.mem_read(RAW,50))==raw;return str(actual)+'|'+';'.join(self.events)
def main():
    """새 연결 전에88원문을 동결하거나 현재 원본 핀만 검사한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        print('연소 캡처 가상88원문/해시 일치');return
    inputs=[(71,34,frame,word,extra) for frame in (0,1,17,33,34) for word in (0,1,2,65535) for extra in (0,2,3,10)]+[(128,-1,frame,word,extra) for frame in (0,8) for word in (0,1) for extra in (2,3)]
    oracle=Oracle();rows=[','.join(map(str,v))+'\t'+oracle.run_case(*v) for v in inputs];FIXTURE.write_text('# 실제71/128의+78/+a4. header 포구(-7,11)/그림 제출은 선언 경계다.\n'+'\n'.join(rows)+'\n','utf-8',newline='\n');paths=['originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_burning_archer_capture.py','tools/decomp_temple_draw.py',FIXTURE.relative_to(ROOT).as_posix()];REPORT.write_text(json.dumps(dict(unit=190,rows=len(rows),returns=oracle.returns,instructions=sum(oracle.calls.values()),boundary='실제+78/+a4/TYPE조회. 현재 header 반환과 그림 제출만 선언 경계. 전체 캡처/픽셀/전투는 별도.',sha256={path:digest(ROOT/path) for path in paths}),ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print(len(rows),oracle.returns,sum(oracle.calls.values()))
if __name__=='__main__':main()
