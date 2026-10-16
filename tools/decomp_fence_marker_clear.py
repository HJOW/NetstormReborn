#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""fencemark 기존 표식 삭제004566d0을 실제 중심/Rect/CRT/finder/태그로 관찰한다."""
import csv
import json
import socket
import struct
import sys
from pathlib import Path
from decomp_rawfinder_oracle import FinderOracle, ROOT, POOL, VTABLE, bits, sha
from decomp_bridgedecay_oracle import STACK_BASE, STACK, STOP, FPU_CONTROLS
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

# 여섯 원본 입력 슬롯과 결과 파일은 다른 펜스 관찰과 독립이다.
IDS=(50,51,60,61,62,63)
FIXTURE=ROOT/'cpppj/tests/fixtures/fence-marker-clear-x86.tsv'
REPORT=ROOT/'cpppj/fence-marker-clear.json'


class Oracle(FinderOracle):
    """실제 삭제 선택/중심/검색은 실행하고 가상 삭제 하위 효과만 명시 입력으로 둔다."""
    def __init__(self):
        """현재 정밀 범위만 추가하며 기존 실제 finder의 입력/실행 계약을 유지한다."""
        super().__init__('originals');self.returns=0  #정상 thiscall 반환 누계다.
        self.put(VTABLE+0x10,STOP+0x450)
        self.body_paths.append(ROOT/'extracted/refined/originals/functions.tsv')
        with self.body_paths[-1].open(encoding='utf-8') as stream:
            functions={int(row['entry'],16):row for row in csv.DictReader(stream,delimiter='\t')}
        for address in (0x4566d0,0x4adea0,0x49a900,0x4ac550,0x49a840,0x40e880,0x40e8f0,0x41def0,0x4ad390,0x41d0d0,0x40e800,0x41d770,0x41d7a0):  #직접 중심/Rect/정수화/태그의 실제 함수다.
            for part in functions[address]['ranges'].split(';'):  #원본 불연속 범위를 바꾸지 않는다.
                low,high=(int(value,16) for value in part.split('-'));self.allowed[low-self.base:high+1-self.base]=b'\x01'*(high+1-low)
        self.stubs={}  #기존 다른 삭제 시나리오의 대체를 이번 부모에서 사용하지 않는다.

    def put(self,address,value,width=4):
        """원본 입력/명시 효과의 정확한 정수 폭만 쓴다."""
        self.mu.mem_write(address,(value&((1<<(width*8))-1)).to_bytes(width,'little'))

    def read(self,address,width=4):
        """현재 little endian 메모리 비트를 읽는다."""
        return int.from_bytes(self.mu.mem_read(address,width),'little')

    def on_write(self,mu,access,address,size,value,data):
        """실제 명령은 local finder/스택/FS만 쓸 수 있다."""
        if STACK_BASE<=address and address+size<=STACK_BASE+0x40000:return
        if address==0 and size==4:return
        raise RuntimeError(f'fencemark 삭제의 선택 밖 쓰기 {address:x}/{size}')

    def on_instruction(self,mu,address,size,data):
        """실제 Begin 사각형과 각 삭제 인수/명시 변경만 관찰한다."""
        sp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x4b16d0:self.area=','.join(str(self.read(sp+4+4*i)) for i in range(4))
        if address==STOP+0x450:
            sid=(mu.reg_read(UC_X86_REG_ECX)-POOL)//50;flags=self.read(sp+4);assert flags==64;self.events.append(f'D:{sid}:{flags}');self.deletions+=1
            if self.mutation==1:self.put(self.slot(sid)+12,77,2)
            if self.mutation==2 and self.deletions==1:self.put(self.slot(61)+12,50,2)
            if self.mutation==3 and self.deletions==1:self.put(self.slot(sid)+4,0,2)
            mu.reg_write(UC_X86_REG_EIP,self.read(sp));mu.reg_write(UC_X86_REG_ESP,sp+8);return
        super().on_instruction(mu,address,size,data)

    def run(self,values,control):
        """입력 버킷만 공급하고 실제 선택/순서/전체raw/호출 규약을 관찰한다."""
        scene,qaMask,packedMask,self.mutation=values;self.events=[];self.area='';self.deletions=0
        first=(50.25,50.5);second=(56.25,50.5) if scene==0 else (50.25,56.5) if scene==1 else (56.25,56.5) if scene==2 else (44.25,50.5)
        objects=[]
        for sid in IDS:  #부모/다른 끝점/세mark/다른타입을 같은 실제 해시에 등록한다.
            x,y=first if sid==50 else second if sid==51 else ((first[0]+second[0])*.5+(sid-61)*.25,(first[1]+second[1])*.5)
            objects.append((sid,136 if sid<60 else 91 if sid==63 else 90,0,0,bits(x),bits(y),1,1,0,1))
        self.setup(list(reversed(objects)));self.put(0x5411c0,90);self.put(0x541288,140);self.put(0x5c847c,128);self.put(0x531928,256);self.put(0,0x12345678)
        for sid in IDS:  #현재 태그와 저장 단계 외의 바이트는 기존 finder 입력 그대로다.
            self.put(self.slot(sid)+33,1,1);self.put(self.slot(sid)+12,50 if sid==63 or sid>=60 and qaMask&(1<<(sid-60)) else 77,2)
            self.put(self.slot(sid)+26,0 if sid>=60 and packedMask&(1<<(sid-60)) else 1);self.put(self.slot(sid)+30,50 if sid>=60 and packedMask&(1<<(sid-60)) else 77,1)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):self.mu.reg_write(reg,0x12340000+reg)  #비휘발 입력을 구별한다.
        self.mu.reg_write(UC_X86_REG_EFLAGS,2);self.mu.reg_write(UC_X86_REG_FPCW,control);self.mu.reg_write(UC_X86_REG_FPTAG,0xffff);self.mu.reg_write(UC_X86_REG_FPSW,0)
        self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.reg_write(UC_X86_REG_ECX,self.slot(50));self.mu.mem_write(STACK,struct.pack('<II',STOP,51));self.mu.emu_start(0x4566d0,STOP,count=100000)
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+8 and self.mu.reg_read(UC_X86_REG_FPCW)==control and self.mu.reg_read(UC_X86_REG_FPSW)&0x3800==0
        for reg in (UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):assert self.mu.reg_read(reg)==0x12340000+reg  #격리 명령의 정상 비휘발 보존 규약을 확인한다.
        self.returns+=1
        return [','.join(map(str,values)),self.area,';'.join(self.events) or '-',b''.join(bytes(self.mu.mem_read(self.slot(sid),50)) for sid in IDS).hex()]


def cases():
    """수평/수직/대각/역방향과 두 형식 태그의 모든 조합을 준비한다."""
    for scene in range(4):  #양 축/대각/역방향의 실제 bounding rect다.
        for qa in range(8):  #세 후보의 QA 일치 여부다.
            for packed in range(8):yield (scene,qa,packed,0)  #다른 끝점의 packed DWORD/BYTE 일치도 독립이다.
    for mutation in (1,2,3):yield (0,5,0,mutation)  #삭제 효과 뒤의 현재 후보/미리 보관한 next를 구별한다.


def generate():
    """고유 입력259개의 두 x87 정상 결과를 저장하고 명시 경계를 기록한다."""
    oracle=Oracle();rows=[]
    for values in cases():  #중복 재생을 새 관찰로 세지 않는다.
        first=oracle.run(values,FPU_CONTROLS[0]);second=oracle.run(values,FPU_CONTROLS[1])
        if first!=second:raise RuntimeError('fencemark 삭제 x87 정밀도 차이')
        rows.append(first)
    FIXTURE.write_text('# inputs area trace objects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    files=[Path(__file__),ROOT/'tools/decomp_rawfinder_oracle.py',ROOT/'tools/decomp_bridgeeffects_oracle.py',ROOT/'tools/decomp_bridgedecay_oracle.py',ROOT/'originals/Netstorm.exe',FIXTURE,ROOT/'cpppj/src/o/RawFenceMarkerClear.h',ROOT/'cpppj/src/o/RawFenceMarkerClear.cpp',ROOT/'cpppj/tests/FenceMarkerClearTests.cpp',*oracle.body_paths]
    REPORT.write_text(json.dumps(dict(host=socket.gethostname(),last_full_decompile_host='VM-W11-Codex',last_full_decompile_date='2026-10-16',total=len(rows),normal_returns=oracle.returns,os_calls=0,sha256={path.relative_to(ROOT).as_posix():sha(path) for path in dict.fromkeys(files)},limits=['선택/중심/Rect/floor·ceil/실제 일반 finder/태그 getter는 실제 원본 명령이다.','가상 삭제는 인수 관찰과 명시 raw변경 경계이며 전체 Damageable/목록/Graph 삭제의 새 관찰이 아니다.','입력은 현재 버킷/발자국이며 C++도 실제 같은 형태의 hash/finder를 사용한다.','포스필드170 삭제/재생성/전체 미션 완주를 증명하지 않는다.']),ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('fencemark 삭제',len(rows),oracle.returns,'정상 반환')


def verify():
    """현재 출처/정상 반환/고유 입력을 감사한다."""
    record=json.loads(REPORT.read_text(encoding='utf-8'))
    for name,value in record['sha256'].items():  #모든 입력/제품/도구의 현재 바이트를 비교한다.
        if sha(ROOT/name)!=value:raise RuntimeError('SHA 불일치 '+name)
    rows=[line.split('\t') for line in FIXTURE.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    assert len(rows)==record['total'] and len({row[0] for row in rows})==len(rows) and record['normal_returns']==len(rows)*2
    print('fencemark 삭제',len(rows),'개 SHA/반환/고유 입력 감사 통과')


# 명시 명령에서만 원본 PE의 제한 범위를 실행하며 게임/창은 실행하지 않는다.
if __name__=='__main__':verify() if '--verify' in sys.argv else generate()
