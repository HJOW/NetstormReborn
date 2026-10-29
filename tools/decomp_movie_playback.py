#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""게임/창/SDK 없이 원본 MovieGump의 옵션·입력·팔레트·idle 기계어를 관측한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, IMAGE, TABLE, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

FIXTURE = ROOT / 'cpppj/tests/fixtures/movie-playback-x86.tsv'  # 최초 원문 관측이며 제품 결과로 다시 만들지 않는다.
REPORT = ROOT / 'cpppj/movie-playback-native-20261030.json'  # 같은 PC/PE/도구와 명시 SDK 대체 경계를 기록한다.


class Oracle(Base):
    """판단·반복·팔레트 쓰기는 실제 PE로 실행하고 가상/SDK 장치만 입력으로 둔다."""
    def __init__(self):
        """검토한 세 원문 몸체와 실제 palette 몸체만 실행 허용한다."""
        super().__init__()
        with (ROOT / 'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts = {int(row['entry'], 16): row for row in csv.DictReader(stream, delimiter='\t')}
        self.allowed = []
        for address in (0x483970, 0x4839c0, 0x483a30, 0x483a90):  # 원문의 불연속 함수 범위도 그대로 보존한다.
            for part in facts[address]['ranges'].split(';'):  # 범위 사이 padding을 허용하지 않는다.
                low, high = (int(value, 16) for value in part.split('-'));self.allowed.append((low, high + 1))
        for offset, target in ((0x54, STOP+0x100), (0x4c, STOP+0x110)):
            self.put(TABLE+offset, target)  # atEnd/end의 명시 가상 경계다.
        for address, target in ((0x500448, STOP+0x120), (0x500444, STOP+0x130), (0x500440, STOP+0x140), (0x50043c, STOP+0x150)):
            self.put(address, target)  # SmackWait/DoFrame/ToBufferRect/NextFrame stdcall을 명시 장치에 연결한다.

    def event(self, text):
        """도달 순서와 중단 직전의 부분효과를 보존한다."""
        self.events.append(text)
        if self.stop == text:
            self.status='stop';self.mu.emu_stop();return False
        return True

    def instruction(self, mu, address, size, data):
        """외부 호출의 인자/규약을 확인하고 원문 영역 밖 실행은 거부한다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,5)
        if address==STOP+0x100:
            result=self.ends[min(self.queries,1)];self.queries+=1
            if self.event('atEnd:'+str(result)): self.ret(result)
            return
        if address==STOP+0x110:
            if self.event('end'):
                if self.mutate: self.put(TEXT+0xa4, 0 if self.words(TEXT+0xa4)[0] else 1)
                self.ret()
            return
        if address==STOP+0x120:
            assert args[0]==IMAGE
            if self.event('wait:'+str(self.wait)): self.ret(self.wait,4)
            return
        if address==STOP+0x130:
            assert args[0]==IMAGE
            if self.event('decode'): self.ret(purge=4)
            return
        if address==STOP+0x140:
            assert args[:2]==(IMAGE,0)
            index=self.rectIndex;self.rectIndex+=1
            if not self.event('rect:'+str(index)): return
            if index<self.rects:
                values=((1,2,3,4),(-5,8,9,10))[index]
                mu.mem_write(IMAGE+0x380, struct.pack('<4I',*[v&0xffffffff for v in values]));self.ret(1,8)
            else: self.ret(0,8)
            return
        if address==STOP+0x150:
            assert args[0]==IMAGE
            if self.event('next'): self.ret(purge=4)
            return
        if address==0x464740:
            if self.event('origin'): mu.mem_write(args[0],struct.pack('<ii',-3,7));self.ret(purge=4)
            return
        if address==0x499370:
            signed=[v if v<0x80000000 else v-0x100000000 for v in args[:4]]
            assert args[4]==4 and self.words(0x59a8d0)[0]==1
            if self.event('dirty:'+','.join(map(str,signed))): self.ret()
            return
        if address==0x466480:
            assert mu.reg_read(UC_X86_REG_ECX)==TEXT
            if self.event('kill'): self.ret()
            return
        if address==0x4c2630:
            message=self.string(args[1])
            text='palette' if message==b'SmackerMovieGump::setNewPalette()\n' else 'invalid:'+str(args[2])
            if self.event(text): self.ret()
            return
        if not any(low<=address<high for low,high in self.allowed): raise RuntimeError(f'영상 실행 허용 범위 밖 {address:08x}')

    def sample(self,kind,arg,auto,freeze,end0,end1,wait,palette,rects,mutate,stop):
        """각 입력은 새 전체 노드/팔레트/장치 현재값을 사용한다."""
        self.events=[];self.status='normal';self.ends=(end0,end1);self.queries=0;self.wait=wait;self.rects=rects;self.rectIndex=0;self.mutate=mutate;self.stop=stop
        self.mu.mem_write(TEXT,b'\xa5'*0x4c8);self.mu.mem_write(IMAGE,b'\0'*0x500)
        for offset,value in ((0,TABLE),(0xa0,IMAGE),(0xa4,auto),(0xa8,freeze),(0xac,0),(0x4b8,10),(0x4bc,20)):
            self.put(TEXT+offset,value)  # 오프셋별 현재값과 callback이 바꿀 수 있는 필드다.
        self.put(IMAGE+0x68,palette);self.mu.mem_write(IMAGE+0x6c,bytes((i*37+11)&255 for i in range(768)));self.put(0x59a8d0,7)
        address={'input':0x4839c0,'idle':0x483a90,'palette':0x483a30,'option':0x483970}[kind]
        values=[STOP]+([arg,1,0] if kind in ('input','option') else [])
        self.mu.mem_write(STACK,struct.pack('<'+'I'*len(values),*values));self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,TEXT);self.mu.reg_write(UC_X86_REG_EAX,0)
        self.mu.emu_start(address,STOP,count=100000)
        if self.status=='normal' and (self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+len(values)*4): raise RuntimeError('영상 ret/스택 불일치')
        result=str(self.mu.reg_read(UC_X86_REG_EAX)) if kind=='input' and self.status=='normal' else '-'
        tail=[self.status,result,','.join(map(str,self.words(TEXT+0xa4,3))),str(self.words(0x59a8d0)[0]),bytes(self.mu.mem_read(TEXT+0xb8,1024)).hex(),';'.join(self.events) or '-']
        return '\t'.join(map(str,(kind,arg,auto,freeze,end0,end1,wait,palette,rects,mutate,stop,*tail)))


def main():
    """최초 관측과 재관측을 구별하고 fixture/보고서 덮어쓰기를 방지한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('영상 최초 관측 덮어쓰기 금지')
    cases=[]
    for event in (0,27,32,65,0x10000,0x20000,0x30000,0x200001b,0x2000020,0x2010000,0x2020000,0x400001b,0x8000020,0xffffffff):
        for auto in (0,1): cases.append(('input',event,auto,1,0,0,0,0,0,0,'-'))  # modifier/버튼 뗌/미등록 입력도 원문으로 판정한다.
    for auto in (0,1):
        for ends,wait,palette,rects in (((1,0),0,0,0),((0,0),1,1,2),((0,0),0,0,0),((0,0),0,1,2),((0,1),0,1,1)):
            cases.append(('idle',0,auto,1,*ends,wait,palette,rects,0,'-'))  # 초기 종료·대기·zero/multi dirty·해제후 종료를 구별한다.
    cases += [('palette',0,0,0,0,0,0,0,0,0,'-')]+[('option',option,3,4,0,0,0,0,0,0,'-') for option in (0,1,2,3,0xffffffff)]
    cases += [('input',27,auto,1,0,0,0,0,0,1,'-') for auto in (0,1)]
    cases += [('idle',0,auto,1,0,1,0,1,2,1,'-') for auto in (0,1)]
    cases += [('idle',0,1,1,0,1,0,1,2,0,stop) for stop in ('atEnd:0','wait:0','palette','decode','origin','rect:0','dirty:8,29,3,4','rect:1','atEnd:1','end','kill')]
    oracle=Oracle();content='# kind arg auto freeze end0 end1 wait palette rects mutate stop status result fields dirty paletteHex trace\n'+'\n'.join(oracle.sample(*case) for case in cases)+'\n'
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_transport.py','tools/decomp_movie_playback.py')
    hashes={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    if args.verify:
        if content!=FIXTURE.read_text(encoding='utf-8') or hashes!=json.loads(REPORT.read_text(encoding='utf-8'))['sha256']: raise RuntimeError('영상 원문 재관측 차이')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n');REPORT.write_text(json.dumps({'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','observations':len(cases),'sha256':hashes,'fixtureSha256':hashlib.sha256(content.encode()).hexdigest(),'scope':'00483970/004839c0/00483a30/00483a90 실제 CPU. SDK·가상end/atEnd·원점·dirty·kill·로그는 명시 장치. 게임/창/SMK/오디오/네트워크 실행 없음.'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'observations':len(cases),'verified':args.verify,'differences':0}))


if __name__=='__main__': main()
