#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Smackw32.dll의 SmackWait fallback을 실제 SDK 기계어로 독립 관측한다."""
import argparse
import hashlib
import json
import socket
from decomp_local_mission_transport import Oracle as Base, ROOT, IMAGE, STACK, STOP
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
import pefile

FIXTURE=ROOT/'cpppj/tests/fixtures/movie-timer-x86.tsv'  # SDK 최초 관측이며 기존 게임 fixture와 별개다.
REPORT=ROOT/'cpppj/movie-timer-native-20261030.json'  # DLL/도구/범위와 명시 장치 경계를 기록한다.


class Oracle(Base):
    """SDK PE를 메모리에 읽고 오디오 없는 실제 SmackWait만 실행한다."""
    def __init__(self):
        """기존 격리 주소 공간의PE만 SDK로 바꾸고 게임/OS를 실행하지 않는다."""
        super().__init__();image=pefile.PE(str(ROOT/'originals/Smackw32.dll')).get_memory_mapped_image();self.mu.mem_write(0x400000,image);self.put(0x40f93c,0);self.put(0x4120cc,STOP+0x100)

    def instruction(self,mu,address,size,data):
        """시계와 대기 서비스만 명시 반환하고 SDK 허용 범위 밖 실행을 거부한다."""
        if address==STOP+0x100:self.trace.append('time');self.ret(self.now);return
        if address==0x406160:self.trace.append('yield');self.ret();return
        if not 0x403000<=address<=0x403137: raise RuntimeError(f'SDK 시계 범위 밖 {address:08x}')

    def sample(self,fast,ready,due,interval,now):
        """각 관측은 서로 독립인 현재 SDK 타이머 물리를 사용한다."""
        self.now=now;self.trace=[];self.mu.mem_write(IMAGE,b'\0'*0x500)
        for offset,value in ((0x39c,fast),(0x408,ready),(0x40c,due),(0x410,interval),(0x430,0xffffffff)):
            self.put(IMAGE+offset,value)  # soundtrack=-1로 오디오 시계 가지를 선택하지 않는다.
        self.put(STACK,STOP);self.put(STACK+4,IMAGE);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.emu_start(0x403000,STOP,count=10000)
        if self.mu.reg_read(UC_X86_REG_EIP)!=STOP or self.mu.reg_read(UC_X86_REG_ESP)!=STACK+8: raise RuntimeError('SDK SmackWait ret/스택 불일치')
        return '\t'.join(map(str,(fast,ready,due,interval,now,self.mu.reg_read(UC_X86_REG_EAX),self.words(IMAGE+0x408)[0],self.words(IMAGE+0x40c)[0],';'.join(self.trace) or '-')))


def main():
    """최초 SDK 관측을 보존하며 재관측 시 전체 바이트/근거 SHA를 대조한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if not args.verify and (FIXTURE.exists() or REPORT.exists()): raise RuntimeError('SDK 시계 최초 관측 덮어쓰기 금지')
    cases=[(0,0,0xffffffff,6666,0),(0,0,0xffffffff,6666,1000),(0,0,6666,6666,66),(0,0,6666,6666,67),(0,0,6666,6666,133),(0,0,6666,6666,134),(0,0,6666,6666,1000),(0,1,6666,6666,1000),(1,0,6666,6666,1000),(0,0,0xffffffff,0,0),(0,0,0,0,0),(0,0,1,0,1),(0,0,0xffffff00,6666,0x028f5c28),(0,0,0xfffffff0,6666,0x028f5c29)]
    oracle=Oracle();content='# fast ready due interval nowMs return readyOut dueOut trace\n'+'\n'.join(oracle.sample(*case) for case in cases)+'\n';paths=('originals/Smackw32.dll','tools/decomp_local_mission_transport.py','tools/decomp_movie_timer.py');hashes={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    if args.verify:
        if content!=FIXTURE.read_text(encoding='utf-8') or hashes!=json.loads(REPORT.read_text(encoding='utf-8'))['sha256']: raise RuntimeError('SDK 시계 재관측 차이')
    else:
        FIXTURE.write_text(content,encoding='utf-8',newline='\n');REPORT.write_text(json.dumps({'host':socket.gethostname(),'lastFullDecompileHost':'HJOW-Athlon','lastFullDecompileDate':'2026-10-23','observations':len(cases),'sha256':hashes,'fixtureSha256':hashlib.sha256(content.encode()).hexdigest(),'scope':'SDK00403000..00403137의 soundtrack=-1 fallback 실제CPU. timeGetTime/406160은 명시 장치. DLL/게임/창/오디오 장치 실제 로드/실행 없음.'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'observations':len(cases),'verified':args.verify,'differences':0}))


if __name__=='__main__': main()
