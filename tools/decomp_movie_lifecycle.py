#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 MovieGump setup/end/draw와 실제 CRT 절삭을 게임/장치 없이 독립 관측한다."""
import argparse
import csv
import hashlib
import json
import socket
import struct
from decomp_local_mission_transport import Oracle as Base, ROOT, TEXT, IMAGE, HEAP, TABLE, STACK, STOP
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

FIXTURE=ROOT/'cpppj/tests/fixtures/movie-lifecycle-x86.tsv'  # 정식 최초 원문 관측의 저장 경로다.
REPORT=ROOT/'cpppj/movie-lifecycle-native-20261030.json'  # 원본/범위/도구와 명시 장치의 근거다.


class Oracle(Base):
    """setup/end/draw의 실제 판단과 ftol을 실행하고 화면/소리 장치만 대체한다."""
    def __init__(self):
        """정밀 함수표의 네 원문 범위만 허용하고 SDK 버퍼 연결을 격리한다."""
        super().__init__()
        with (ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8') as stream:
            facts={int(r['entry'],16):r for r in csv.DictReader(stream,delimiter='\t')}
        self.allowed=[]
        for entry in (0x483680,0x4837f0,0x483bf0,0x4e49c0):
            for part in facts[entry]['ranges'].split(';'):  # 불연속 범위 밖 명령은 실행하지 않는다.
                low,high=(int(v,16) for v in part.split('-'));self.allowed.append((low,high+1))
        self.put(TABLE+0x44,STOP+0x100);self.put(TABLE+8,STOP+0x110);self.put(0x500430,STOP+0x120)

    def event(self,text):
        """실제 장치 진입 순서와 선택 중단 직전의 상태를 남긴다."""
        self.events.append(text)
        if self.stop==text: self.status='stop';self.mu.emu_stop();return False
        return True

    def instruction(self,mu,address,size,data):
        """원문의 인자/호출 규약을 검사한 후 명시 장치 효과만 반환한다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,8)
        if address==STOP+0x100: self.ret(1);return
        if address==STOP+0x110:
            assert args[0]==0
            if self.event('redraw:0'):
                if self.mutate: self.put(TEXT+0xa8,0 if self.words(TEXT+0xa8)[0] else 1)
                self.ret(purge=4)
            return
        if address==STOP+0x120:
            assert args==(IMAGE,0,0,self.vw,self.vh,HEAP if self.buffer else 0,0,args[7])
            if self.event('bind'): self.ret(purge=28)
            return
        if address==0x4a2800:
            assert args[:3]==(0,256,TABLE+0x2000)
            if self.event('capture'): mu.mem_write(args[2],bytes((i*19+2)&255 for i in range(1024)));self.ret()
            return
        if address==0x4648e0:
            if self.event('bounds'): mu.mem_write(args[0],struct.pack('<4i',*self.bounds));self.ret(purge=4)
            return
        if address in (0x4a4790,0x4a2060):
            values=[v if v<0x80000000 else v-0x100000000 for v in args[:4]]
            if self.event(('clear:' if address==0x4a4790 else 'fill:')+','.join(map(str,values))+','+str(args[4])): self.ret()
            return
        if address==0x4a2640:
            assert args[:2]==(0,256) and args[3]==1
            if self.event('apply'): self.applied=bytes(mu.mem_read(args[2],1024)).hex();self.ret()
            return
        if address==0x4a1fe0:
            assert args[2]==HEAP and args[3:]==(self.vw,0,0,self.vw,self.vh)
            signed=[v if v<0x80000000 else v-0x100000000 for v in args[:2]]
            if self.event('blit:'+','.join(map(str,(*signed,self.vw,self.vh)))): self.ret()
            return
        if address==0x465330:
            assert args[0] in (0,1)
            if self.event('idle:'+str(args[0])):
                if self.mutate and args[0]: self.put(TEXT+0xa8,0 if self.words(TEXT+0xa8)[0] else 1)
                self.ret(purge=4)
            return
        if address in (0x435dc0,0x4a40d0,0x4aa600):
            name={0x435dc0:'updates',0x4a40d0:'cursor',0x4aa600:'sound'}[address]
            if self.event(name+':'+str(args[0])): self.ret()
            return
        if address in (0x4a8ef0,0x4aa900,0x4aa970,0x460df0,0x460e30,0x4a4fc0):
            name={0x4a8ef0:'shutdown',0x4aa900:'mute',0x4aa970:'unmute',0x460df0:'pause:1',0x460e30:'pause:0',0x4a4fc0:'refresh'}[address]
            if self.event(name): self.ret()
            return
        if address==0x4e0620:
            if self.event('assert:'+str(args[2])): self.ret()
            return
        if not any(low<=address<high for low,high in self.allowed): raise RuntimeError(f'영상 수명 허용 범위 밖 {address:08x}')

    def sample(self,kind,w,h,vw,vh,freeze,pending,buffer,sound,clip,mutate,stop):
        """독립 현재 노드/팔레트/clip/버퍼와 float32 Gump 치수를 설치한다."""
        self.events=[];self.status='normal';self.applied='-';self.stop=stop;self.mutate=mutate;self.vw=vw;self.vh=vh;self.buffer=buffer
        self.bounds=(-5,7,int(w)-5,int(h)+7);clips=((0,0,1024,768),(12,30,20,40),(-1000,-1000,2000,2000),(0,0,0,0));self.mu.mem_write(0x5c7a44,struct.pack('<4i',*clips[clip]));self.put(0x5a4058,9);self.put(0x54daa0,sound)
        self.mu.mem_write(TEXT,b'\0'*0x4c8);self.mu.mem_write(IMAGE,b'\0'*0x500);self.mu.mem_write(TABLE+0x2000,b'\xa5'*1024);self.mu.mem_write(TEXT+0xb8,b'\x5a'*1024)
        for offset,value in ((0,TABLE),(0xa0,IMAGE),(0xa8,freeze),(0xac,pending),(0xb0,HEAP if buffer else 0),(0xb4,TABLE+0x2000)):
            self.put(TEXT+offset,value)  # 포인터는 호스트 별칭이며 실제 원문은 바꾸지 않는다.
        self.mu.mem_write(TEXT+0x44,struct.pack('<ff',w,h));self.mu.mem_write(TEXT+0x4b8,struct.pack('<4i',11,22,33,44));self.put(IMAGE+4,vw);self.put(IMAGE+8,vh)
        self.put(STACK,STOP);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,TEXT)
        self.mu.emu_start({'begin':0x483680,'end':0x4837f0,'draw':0x483bf0}[kind],STOP,count=100000)
        if self.status=='normal' and (self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+4): raise RuntimeError('영상 수명 ret/스택 불일치')
        rect=','.join(str(v) for v in struct.unpack('<4i',self.mu.mem_read(TEXT+0x4b8,16)))
        output=(self.status,rect,self.words(TEXT+0xa8)[0],self.words(TEXT+0xac)[0],bytes(self.mu.mem_read(TABLE+0x2000,1024)).hex(),self.applied,';'.join(self.events) or '-')
        return '\t'.join(map(str,(kind,w,h,vw,vh,freeze,pending,buffer,sound,clip,mutate,stop,*output)))


def main():
    """최초 기준은 원본 CPU만으로 생성하고 재관측 시 전체 바이트와 근거 해시를 비교한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('영상 수명 최초 관측 덮어쓰기 금지')
    cases=[]
    for w,h in ((1024,768),(1280,720),(1280,800),(640,480),(639.5,319.5),(641,321)):
        for freeze in (0,1): cases.append(('begin',w,h,640,320,freeze,3,1,1,0,0,'-'))  # 화면비/홀수/음수 차이의 실제 CRT 절삭을 비교한다.
    for freeze in (0,1):
        for sound in (0,1): cases.append(('end',1024,768,640,320,freeze,3,1,sound,0,0,'-'))
    for clip in range(4):
        for pending,buffer in ((0,0),(1,0),(0,1),(1,1)): cases.append(('draw',1024,768,640,320,1,pending,buffer,1,clip,0,'-'))  # 내부/외부/빈clip과 팔레트/버퍼 관문이다.
    cases += [('draw',639,319,640,320,0,1,1,0,0,0,'-')]
    cases += [(kind,1024,768,640,320,freeze,1,1,1,0,1,'-') for kind in ('begin','end') for freeze in (0,1)]
    cases += [('begin',1024,768,640,320,1,1,1,1,0,0,stop) for stop in ('capture','shutdown','mute','bind','idle:1','updates:1','pause:1','cursor:0')]
    cases += [('end',1024,768,640,320,1,1,1,1,0,0,stop) for stop in ('bounds','clear:-5,7,1019,775,0','apply','sound:3','unmute','idle:0','redraw:0','updates:0','pause:0','refresh')]
    cases += [('draw',1024,768,640,320,1,1,1,1,0,0,stop) for stop in ('bounds','fill:0,0,1024,768,9','apply','blit:187,231,640,320')]
    oracle=Oracle();content='# kind w h vw vh freeze pending buffer sound clip mutate stop status image freezeOut pendingOut savedHex appliedHex trace\n'+'\n'.join(oracle.sample(*case) for case in cases)+'\n'
    paths=('originals/Netstorm.exe','extracted/refined/originals/functions.tsv','tools/decomp_local_mission_transport.py','tools/decomp_movie_lifecycle.py');hashes={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    if args.verify:
        if content!=FIXTURE.read_text(encoding='utf-8') or hashes!=json.loads(REPORT.read_text(encoding='utf-8'))['sha256']: raise RuntimeError('영상 수명 원문 재관측 차이')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n');REPORT.write_text(json.dumps({'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','observations':len(cases),'sha256':hashes,'fixtureSha256':hashlib.sha256(content.encode()).hexdigest(),'scope':'00483680/004837f0/00483bf0/004e49c0 실제CPU. 화면/음향/SDK/가상함수는 명시 장치. 게임/창/오디오/네트워크 실행 없음.'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'observations':len(cases),'verified':args.verify,'differences':0}))


if __name__=='__main__': main()
