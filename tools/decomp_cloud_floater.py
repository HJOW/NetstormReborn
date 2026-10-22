#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""클라우드 플로터의 전용 최초 후처리와 event3 Regular를 원본 명령으로 관찰한다."""
import csv
import json
import socket
import struct
import sys
from decomp_local_mission_transport import Oracle as BaseOracle, ROOT, HEAP, PLAYERS, TABLE, STACK, STOP, digest
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_FPTAG, UC_X86_REG_FPSW

# 원본 주소와 구별되는 대상 슬롯/반환 보관/반환float 하위 경계다.
RAW, RESULT, RETURN_FLOAT, CAPTURE_FLOAT = PLAYERS+50*50, HEAP+0x800, STOP+0x100, STOP+0x200


class Oracle(BaseOracle):
    """판단은 실제42aed0/42af60이며 큰 할당·공통 몸체·프레임 효과만 분리한다."""
    def __init__(self,precision):
        """두 원문 함수와 명시 float 반환 보조 코드만 실행한다."""
        super().__init__();self.precision=precision;self.allowed=[];self.count=0
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        for entry in (0x42aed0,0x42af60):  # 전용 두 함수의 불연속 범위도 포함한다.
            for part in facts[entry]['ranges'].split(';'):  # 개별 코드 조각 끝을 포함한다.
                lo,hi=(int(v,16) for v in part.split('-'));self.allowed.append((lo,hi+1))
        self.mu.mem_write(RETURN_FLOAT,b'\xd9\x05'+struct.pack('<I',TABLE)+b'\xc2\x0c\x00')
        self.mu.mem_write(CAPTURE_FLOAT,b'\xd9\x1d'+struct.pack('<I',RESULT)+b'\xe9'+struct.pack('<i',STOP-(CAPTURE_FLOAT+11)))
        self.mu.mem_write(TABLE,struct.pack('<f',-3.5))

    def instruction(self,mu,address,size,data):
        """외부 부수효과는 원래 호출 규약과 실제 선택 순서만 기록한다."""
        args=self.words(mu.reg_read(UC_X86_REG_ESP)+4,4)
        if address==0x4e4391:self.events.append('Allocate,'+str(args[0]));self.ret(HEAP if self.allocated else 0);return
        if address==0x496e80:self.events.append(f'Regular,{args[0]},{args[1]},{args[2]}');self.ret(HEAP,purge=12);return
        if address==0x42ac90:self.events.append('Balloon,'+str(args[0]));self.ret(purge=4);return
        if address==0x4afc90:self.events.append(f'Advance,{args[0]},{args[1]}');self.ret(purge=8);return
        if address==0x42a750:self.events.append(f'Carrier,{args[0]},{args[1]},{args[2]}');mu.reg_write(UC_X86_REG_EIP,RETURN_FLOAT);return
        if RETURN_FLOAT<=address<RETURN_FLOAT+9 or CAPTURE_FLOAT<=address<CAPTURE_FLOAT+11:return
        if not any(lo<=address and address+size<=hi for lo,hi in self.allowed):raise RuntimeError(f'미검토 클라우드 명령 {address:x}')
        self.count+=1

    def run(self,case):
        """정상 반환/비휘발/스택과 원본 호출 순서·실제float 반환 비트를 보존한다."""
        kind,values=case;self.events=[];self.allocated=1;self.put(0x5c8464,PLAYERS);self.mu.mem_write(RAW,bytes(50));self.put(RAW,0x504198);self.put(RAW+10,146,1);self.put(RESULT,0xa5a5a5a5)
        if kind=='P':
            flags,self.allocated,server=values;self.put(0x540bc0,server);entry,args,purge=0x42aed0,[flags],4;target=STOP
        else:entry,args,purge=0x42af60,list(values),12;target=CAPTURE_FLOAT
        self.mu.reg_write(UC_X86_REG_FPCW,self.precision);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  # 호출마다 다른 비휘발 sentinel이다.
        self.mu.reg_write(UC_X86_REG_ECX,RAW);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),target,*args));self.mu.emu_start(entry,STOP)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4+purge
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  # 실제 원문의 보존 규약이다.
        return [kind,','.join(map(str,values)),';'.join(self.events) or '-',str(self.words(RESULT)[0])]


def cases():
    """서버와 무관한 최초 비트/할당 NULL 및 event3/위임을 구별한다."""
    for flags in (0,1,2,17,0x100000,0xffffffff):  # 최초 비트와 무관 비트다.
        for allocated in (0,1):  # 실패해도 기본 벌룬 후처리는 호출한다.
            for server in (0,1):yield 'P',(flags,allocated,server)  # 이 전용 주기는 서버로 제한하지 않는다.
    for event in (0,3,0x493,0xffffffff):  # 정확3과 기본 위임을 구별한다.
        for count in (0,1,80,0xffffffff):  # 무시하는 횟수도 보존한다.
            for payload in (0,0x3dcccccd,0xc0000000,0x40500000):yield 'R',(event,count,payload)  # 반환float의 원문 비트다.


def main():
    """기존 벌룬 원문은 유지하고 새 두 전용 함수의 자료만 작성한다."""
    fixture=ROOT/'cpppj/tests/fixtures/cloud-floater-x86.tsv';report=ROOT/'cpppj/cloud-floater-20261022.json'
    if '--verify' in sys.argv:
        d=json.loads(report.read_text(encoding='utf-8'))
        for p,h in d['sha256'].items():assert digest(ROOT/p)==h,p  # 현재 원본/제품/검사 근거를 확인한다.
        print('Cloud Floater SHA verified:',d['observations']);return
    first,second=Oracle(0x027f),Oracle(0x037f);rows=[first.run(c) for c in cases()];assert rows==[second.run(c) for c in cases()]
    with fixture.open('w',encoding='utf-8',newline='') as f:
        writer=csv.writer(f,delimiter='\t',lineterminator='\n');writer.writerow(['kind','input','trace','result']);writer.writerows(rows)
    paths=['originals/Netstorm.exe','originals/d/TEST02.fort','originals/d/TEST02.english','extracted/refined/originals/functions.tsv','tools/decomp_cloud_floater.py','tools/decomp_local_mission_transport.py','cpppj/src/o/RawBalloonBody.h','cpppj/src/o/RawBalloonBody.cpp','cpppj/src/o/RawFrameAdvance.cpp','cpppj/src/client/RawCampaignSession.cpp','cpppj/src/client/RawCampaignPathBindings.cpp','cpppj/tests/CloudFloaterTests.cpp','cpppj/tests/CampaignSessionTests.cpp','cpppj/tests/fixtures/cloud-floater-x86.tsv']
    d={'host':socket.gethostname(),'last_full_refined_host':'VM-W11-Codex','last_full_refined_date':'2026-10-22','observations':len(rows),'normal_returns':len(rows)*2,'precision_replays':[0x027f,0x037f],'instructions':first.count+second.count,'boundary':'실제42aed0/42af60의 최초 비트/할당 NULL/순서/event3/float 반환을 실행한다. 힙/Regular 생성/기존42ac90/프레임 진행/기본42a750은 명시 큰 경계이며 기존 독립 벌룬·Carrier·FrameAdvance 원문과 별도로 보존한다. GUI 플레이 검증은 아니다.','sha256':{p:digest(ROOT/p) for p in paths if (ROOT/p).is_file()}}
    report.write_bytes((json.dumps(d,ensure_ascii=False,indent=2)+'\n').encode('utf-8'));print('Cloud Floater native:',len(rows))


if __name__=='__main__':main()
