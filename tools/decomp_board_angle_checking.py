#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 실제 Boardcoord fmod/끝각 부모를 별도 double 전역/현재 checking 입력으로 관측한다."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from decomp_oracle import Oracle,ROOT,SCRATCH
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

# 실제 두 함수와 검토한 순수 CRT fmod 하위다. CRT를 입력 모델로 대체하지 않는다.
PAIRS=[{'name':'Angle','originals':'0041cdb0','originalCD':'0043f4e0'}]
HELPERS={'originals':[0x4e5e8a,0x4eded0,0x4ee1b7,0x4edf9a],
 'originalCD':[0x4f28fa,0x4f7dc0,0x4f7e8a,0x4f8097]}
# Native ST0 반환을 호스트 OS 없이 double로 저장하는 격리 캡처 코드/출력이다.
CAPTURE,OUTPUT=SCRATCH+0x18000,SCRATCH+0x18100


class AngleOracle(Oracle):
    """실제 fmod/부모를 실행하고 원문 assertion UI만 cdecl 반환한다."""
    def __init__(self,edition):
        """별도 double modulus 전역을 실제 fld 명령에서 확인한다."""
        super().__init__(edition,PAIRS,HELPERS[edition]);self.events=[];self.action=0
        if edition=='originals':self.allowed.extend(((0x4ee254,0x4ee25b),(0x4e5e94,0x4e5eb2)))  # 정밀 함수 밖 CRT 짧은 꼬리는 실제 명령을 따로 읽었다.
        if edition=='originalCD':self.allowed.extend(((0x4f8134,0x4f813b),(0x4f2904,0x4f2922)))  # 실제 두 pop/fldz/ret 꼬리만 별도 검토했다.
        self.checking=0x5e4794 if edition=='originals' else SCRATCH+0x18200
        instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(bytes(self.mu.mem_read(self.entries['Angle'],50)),self.entries['Angle']))
        found=[re.search(r'qword ptr \[0x([0-9a-f]+)\]',i.op_str) for i in instructions if i.mnemonic=='fld']
        self.modulus=next(int(m.group(1),16) for m in found if m)
        # 반환값을 계산하지 않고 Native fstp qword 명령으로 저장한다.
        code=b'\xdd\x1d'+struct.pack('<I',OUTPUT)+b'\xc3';self.mu.mem_write(CAPTURE,code)
        self.allowed.append((CAPTURE,CAPTURE+len(code)));self.entries['Capture']=CAPTURE

    def on_instruction(self,mu,address,size,data):
        """현재 진단의 식/파일/행을 기록하고 checking 변경 후 원래 부모로 돌아온다."""
        if self.stubs.get(address)!='assert':return super().on_instruction(mu,address,size,data)
        esp=mu.reg_read(UC_X86_REG_ESP);ret,expression,file,line=struct.unpack('<IIII',mu.mem_read(esp,16))
        self.events.append(f'{line}:{self.read_string(expression).decode()}:{self.read_string(file).decode()}')
        if self.action:mu.mem_write(self.checking,bytes(4))
        mu.reg_write(UC_X86_REG_EAX,0);mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)

    def observe(self,bits,modulus,checking,action):
        """분리된 CRT modulus만 입력하며 >=361 진단을 결과 주입 없이 실제 fmod로 도달한다."""
        self.events=[];self.action=action;self.mu.mem_write(self.checking,struct.pack('<I',checking));self.mu.mem_write(self.modulus,struct.pack('<d',modulus))
        self.mu.reg_write(UC_X86_REG_FPCW,0x027f);self.call('Angle',[bits]);self.call('Capture',[])
        return [bytes(self.mu.mem_read(OUTPUT,8)).hex(),str(struct.unpack('<I',self.mu.mem_read(self.checking,4))[0]),'|'.join(self.events) or '-']


def main():
    """새 관측은 기존 파일을 덮지 않으며 --verify는 읽기 재관측만 한다."""
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');parser.add_argument('--fixture',type=Path,default=ROOT/'extracted/board-angle-unicorn-x86.tsv');parser.add_argument('--report',type=Path,default=ROOT/'extracted/board-angle-unicorn-native.json');args=parser.parse_args()
    fixture=args.fixture;report=args.report
    values=[0.0,-0.0,1.0,-1.0,90,180,270,359,360,360.5,361,450,719,720,721,-360,-361,-720,-721,1e-6,-1e-6,1e-14,-1e-14,1e-30,-1e-30,3.4e38,-3.4e38]
    bits=list(dict.fromkeys(struct.unpack('<I',struct.pack('<f',v))[0] for v in values))
    bits += [0x43b3ffff,0x43b40001,0x43b47fff,0x43b48000,0x43b48001,1,0x80000001]
    rows=[];binaries={}
    # 양판본 실제 CRT와 부모에 같은 입력을 주며 결과를 Python으로 정규화하지 않는다.
    for edition in HELPERS:
        oracle=AngleOracle(edition);binaries[edition]={'sha256':oracle.sha256,'modulusAddress':hex(oracle.modulus)}
        # 일반360과 별도 전역720은 부모의 float360/361 경계와 독립이다.
        for modulus in (360.0,720.0):
            # 끝각 주변 binary32 인자를 실제 호출 스택에 넣는다.
            for value in bits:
                # 전체 현재 DWORD의 비영 관문을 관측한다.
                for checking in (0,1,0x80000000,0xffffffff):
                    # 원문 진단 뒤 checking0 변경도 같은 호출 결과로 기록한다.
                    for action in (0,1):rows.append([edition,str(value),str(int(modulus)),str(checking),str(action)]+oracle.observe(value,modulus,checking,action))
    output=('\n'.join('\t'.join(row) for row in rows)+'\n').encode();digest=hashlib.sha256(output).hexdigest()
    if args.verify:assert fixture.read_bytes()==output,'각도 부모 원문 재관측 불일치'
    else:
        if fixture.exists() or report.exists():raise FileExistsError('기존 증거에는 --verify를 사용하십시오')
        fixture.parent.mkdir(parents=True,exist_ok=True);report.parent.mkdir(parents=True,exist_ok=True)
        fixture.write_bytes(output);report.write_text(json.dumps({'host':'HJOW-Athlon','lastFullDecompile':'HJOW-Athlon / 2026-11-03','binaries':binaries,'rows':len(rows),'fixtureSha256':digest,
         'floatingControl':'0x027f / x87 53비트','boundary':'실제 PE 부모/CRT fmod·별도 double modulus 입력·진단 UI만 반환·게임/GUI/OS/소켓0'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'rows':len(rows),'sha256':digest,'verify':args.verify}))


if __name__=='__main__':main()
