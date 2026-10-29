#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""원본 Enter 부모와 입력줄 조회/전환/프레임말 요청을 독립 x86으로 관측한다."""
import itertools
import json
import socket
import struct
from pathlib import Path
import decomp_production_shortcuts as gate
import decomp_hud_chat_geometry as geometry
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_FPCW

ROOT=Path(__file__).resolve().parents[1]  # 보호 입력과 새 fixture의 작업 루트다.
OUT=ROOT/'extracted/cpp-chat-input-573-20261029'  # 기존 산출물을 덮어쓰지 않는 단위 폴더다.
CURRENT,REQUEST=0x548740,0x54db18  # 현재 Chat과 프레임말 소비 DWORD의 원본 주소다.
KEY_SCHEMA=(*gate.SCHEMA[:9],'visible',*gate.SCHEMA[9:])  # 부모 장치 입력에 실제 조회 반환만 추가한다.
TOGGLE_SCHEMA=('name','operation','present','flagsA','flagsB','swap','request','completed','flagsAout','flagsBout','currentout','requestout','result','trace','stackdelta','nativeinstructions')  # 앞7열은 기대값 없는 입력이다.


class KeyOracle(gate.Oracle):
    """Guide/Gump/뗌/모달/정지 순서는 실제 일반 입력 본체를 실행한다."""
    def instruction(self,mu,address,size,data):
        """조회/표시/부모 Bubble만 명시 장치로 기록한다. 부모 분기는 원문이다."""
        if address==0x42f310:
            self.event('visible');self.returned(self.case['visible']);return
        if address==0x42f270:
            self.event('show',(self.number(mu.reg_read(UC_X86_REG_ESP)+4),));self.returned();return
        if address==0x42e410:
            self.event('open');self.returned();return
        super().instruction(mu,address,size,data)


class ToggleOracle(geometry.Oracle):
    """세 원본 몸체와 Push 뒤 현재 root/자식 재조회를 실제 실행한다."""
    def guard(self,mu,access,address,size,value,data):
        """프레임말 요청 DWORD 외에는 기존 노드/스택 쓰기만 허용한다."""
        if REQUEST<=address and address+size<=REQUEST+4:return
        super().guard(mu,access,address,size,value,data)

    def instruction(self,mu,address,size,data):
        """가상 Push만 현재 포인터 교체를 제공하고 Attach 인자를 기록한다."""
        if address==0x439ba9:self.completed=True;mu.emu_stop();return
        if address==geometry.PUSH:
            self.trace.append('push:'+str(self.role(mu.reg_read(UC_X86_REG_ECX))))
            if self.case['swap']==1:self.number(CURRENT,geometry.NODES[2])
            elif self.case['swap']==2:self.number(geometry.NODES[0]+0xa0,geometry.NODES[3])
            self.finish(0);return
        super().instruction(mu,address,size,data)

    def run(self,case,control):
        """root 둘/Console 둘을 준비하고 정상 RET 또는 실제 tail 끝까지 읽는다."""
        self.case=case;self.trace=[];self.instructions=0;self.completed=False
        self.mu.mem_write(geometry.HEAP,bytes(0x3000))
        for index,node in enumerate(geometry.NODES):  # 원문 크기의 같은 노드와 가상 장치다.
            table=geometry.HEAP+0x1000+index*0x100
            self.number(node,table);self.number(table+0x18,geometry.PUSH);self.number(table+0x20,geometry.ATTACH)
            self.number(node+4,case['flagsA'] if index==1 else case['flagsB'] if index==3 else 0xe01)
            self.number(node+0x4c,geometry.bits(11+index));self.number(node+0x50,geometry.bits(-13-index))
        self.number(geometry.NODES[0]+0xa0,geometry.NODES[1]);self.number(geometry.NODES[2]+0xa0,geometry.NODES[3])
        self.number(CURRENT,geometry.NODES[0] if case['present'] else 0);self.number(REQUEST,case['request'])
        self.mu.mem_write(geometry.STACK-0x1000,bytes(0x2000));self.number(geometry.STACK,geometry.STOP)
        self.mu.reg_write(UC_X86_REG_ESP,geometry.STACK);self.mu.reg_write(UC_X86_REG_EBX,0);self.mu.reg_write(UC_X86_REG_EAX,0x12345678);self.mu.reg_write(UC_X86_REG_FPCW,control)
        entry={'toggle':0x42f210,'visible':0x42f310,'frame':0x439b96}[case['operation']]
        self.mu.emu_start(entry,geometry.STOP+1,count=1000)
        assert self.completed and self.mu.reg_read(UC_X86_REG_FPCW)==control
        expected_stack=geometry.STACK if case['operation']=='frame' else geometry.STACK+4
        result=dict(completed=1,flagsAout=self.number(geometry.NODES[1]+4),flagsBout=self.number(geometry.NODES[3]+4),currentout=self.role(self.number(CURRENT)),requestout=self.number(REQUEST),result=self.mu.reg_read(UC_X86_REG_EAX) if case['operation']=='visible' else 0,trace=';'.join(self.trace) or '-',stackdelta=self.mu.reg_read(UC_X86_REG_ESP)-expected_stack,nativeinstructions=self.instructions)
        for low,expected in self.original.items():  # 전역 장치 쓰기가 원문 명령 바이트를 바꾸지 않았다.
            assert bytes(self.mu.mem_read(low,len(expected)))==expected
        return result


def observe(path,schema,rows,oracle):
    """두 정밀도 결과가 같을 때만 최초 fixture를 쓰고 이후에는 재대조한다."""
    output=[]
    for row in rows:  # 결과는 제품 코드/기대식 없이 원문에서만 얻는다.
        first=oracle.run(row,0x27f);second=oracle.run(row,0x37f);assert first==second,row
        output.append({**row,**first})
    data=('\t'.join(schema)+'\n'+'\n'.join('\t'.join(str(row[key]) for key in schema) for row in output)+'\n').encode('utf-8')
    if path.exists():assert path.read_bytes()==data
    else:path.write_bytes(data)
    return {'rows':len(rows),'observations':len(rows)*2,'precisionDifferences':0,'fixtureSha256':gate.digest(path)}


def main():
    """보호 PE를 확인하고 Enter/가상 전환의 새 독립 자료를 만든다."""
    assert gate.digest(gate.PE_PATH)==gate.PE_SHA;OUT.mkdir(exist_ok=True)
    gate.RANGES=(*gate.RANGES,(0x4d53f0,0x4d542e))  # 가상 Enter의 하위0BYTE가 도달하는 실제 비대상 번역도 원문으로 실행한다.
    keys=[]
    for key,flags,pause,modal,visible in itertools.product((13,0xd0000,ord('v')),(0,0x08000000,0x10000000,0x20000000,0x38000000,0x80000000),(0,3),(0,1),(0,1)):  # 문자/가상키·수정키·뗌·모달을 교차한다.
        keys.append(dict(name=f'key-{len(keys)}',events=f'{key|flags}:71:83',guideResult=0,gumpResult=0,pauseDepth=pause,console=modal,command=0x020000f4,read=0,filled=0,visible=visible))
    for guide,gump,visible in itertools.product((0,1,0xffffffff),(0,1,0xffffffff),(0,1)):  # 선행 소비가 표시 조회까지 막는지 실제 부모로 읽는다.
        keys.append(dict(name=f'gate-{len(keys)}',events='13:71:83',guideResult=guide,gumpResult=gump,pauseDepth=3,console=0,command=0x020000f4,read=0,filled=0,visible=visible))
    geometry.RANGES=(*geometry.RANGES,(0x42f210,0x42f24c),(0x42f310,0x42f329),(0x439b96,0x439ba9))
    toggles=[]
    for op,present,a,b,swap,request in itertools.product(('toggle','visible','frame'),(0,1),(0,1,0x600,0xffffffff),(0,1,0x601),(0,1,2),(0,1,0xffffffff)):  # 결과 기대 없이 원시 입력만 교차한다.
        if (not present and swap) or (op!='frame' and request) or (op=='visible' and swap):continue
        toggles.append(dict(name=f'{op}-{len(toggles)}',operation=op,present=present,flagsA=a,flagsB=b,swap=swap,request=request))
    key_result=observe(ROOT/'cpppj/tests/fixtures/chat-input-key-x86.tsv',KEY_SCHEMA,keys,KeyOracle())
    toggle_result=observe(ROOT/'cpppj/tests/fixtures/chat-input-toggle-x86.tsv',TOGGLE_SCHEMA,toggles,ToggleOracle())
    report={'host':socket.gethostname(),'peSha256':gate.PE_SHA,'key':key_result,'toggle':toggle_result,'nativeBodiesUnchanged':True,'externalDevices':['Guide virtual+24','Gump Route','key: visible/show/open calls','toggle: virtual Push with pointer replacement, Attach'],'scope':'Enter의 조회/표시/열기는 외부 장치이며 원문 부모 관문을 실행한다. 별도 toggle/visible/frame은 실제 몸체와 현재 재조회까지 실행한다. 송신/통신 없음.'}
    target=ROOT/'cpppj/chat-input-native-20261029.json';text=json.dumps(report,ensure_ascii=False,indent=2)+'\n'
    if target.exists():assert target.read_text(encoding='utf-8')==text
    else:target.write_text(text,encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':main()
