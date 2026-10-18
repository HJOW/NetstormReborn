#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 세 템플 기본 프레임과 전용Draw를 실제 x86으로 관측한다."""
import argparse,csv,hashlib,json,socket,struct
from pathlib import Path
from decomp_rain_draw import Oracle as RainOracle,ROOT,TYPES,RAW,CODES,STACK,STOP
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP
from cpp_menu_smoke import source
from taff import TaffArchive
from shp import read_clusters

# 실제 세 템플의 원본 타입·가상표·전용Draw다. 기본 조회는 같은4acfe0이다.
SPECS={'windVortex':(123,0x519860,0x4dcf10),'rainVortex':(124,0x5196f0,0x4dcd00),'thunderVortex':(125,0x519918,0x4dd0f0)}
FIXTURE=ROOT/'cpppj/tests/fixtures/temple-draw-x86.tsv' # 제품 전에 동결하는 실제 호출 순서다.
REPORT=ROOT/'cpppj/temple-draw.json' # 실행 범위와 외부 제출 경계를 구별한다.


def digest(path):
    """파일 원문 SHA256이다."""
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Oracle(RainOracle):
    """그림/소리 제출만 관찰하고 실제 기본 프레임 조회·TYPE·방향 helper를 실행한다."""
    def __init__(self,name,codes,base):
        """기존 격리 주소/레지스터 진단에 실제 세 전용 함수 범위만 추가한다."""
        self.number,self.table,self.entry=SPECS[name];self.draw,self.sound=0x497280,0x4a9cb0
        self.mu=Uc(UC_ARCH_X86,UC_MODE_32);pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));data=pe.get_memory_mapped_image();self.mu.mem_map(0x400000,(len(data)+4095)&~4095);self.mu.mem_write(0x400000,data)
        # 입력 표/객체/코드/스택/반환 위치는 실제 PE와 겹치지 않는다.
        for address,size in ((TYPES,0x20000),(RAW,0x1000),(CODES,0x1000),(STACK-0xf000,0x10000),(STOP,0x1000)):self.mu.mem_map(address,size)
        self.mu.mem_write(0x59ab20,struct.pack('<I',TYPES));type_base=TYPES+self.number*500
        self.mu.mem_write(type_base+0xdc,struct.pack('<I',0x12345678));self.mu.mem_write(type_base+0x124,struct.pack('<I',CODES));self.mu.mem_write(type_base+0x128,struct.pack('<i',base));self.mu.mem_write(CODES,b''.join(bytes((ord(side),0,0,0)) for side in codes))
        # 현재 보호 .type의 같은 순서로 첫번호/개수를 공급한다.
        for index in range(16):
            letter=chr(65+index);self.mu.mem_write(type_base+0x130+index*4,struct.pack('<i',codes.find(letter)));self.mu.mem_write(type_base+0x170+index*4,struct.pack('<i',codes.count(letter)))
        facts={int(row['entry'],16):row for row in csv.DictReader((ROOT/'extracted/refined/originals/functions.tsv').open(encoding='utf-8'),delimiter='\t')}
        self.allowed=[]
        # 가상Draw와 실제 TYPE/방향 helper의 Ghidra 불연속 범위다.
        for address in (self.entry,0x4ac550,0x49a840,0x4ad710,0x49ace0):
            for part in facts[address]['ranges'].split(';'):
                low,high=(int(value,16) for value in part.split('-'));self.allowed.append((low,high+1))
        expected=bytes.fromhex('0fb6410a50e856d8feff8b802801000083c404c3')
        assert bytes(self.mu.mem_read(0x4acfe0,len(expected)))==expected
        self.allowed.append((0x4acfe0,0x4acfe0+len(expected)))
        assert struct.unpack('<I',self.mu.mem_read(self.table+0x78,4))[0]==0x4acfe0
        assert struct.unpack('<I',self.mu.mem_read(self.table+0xa4,4))[0]==self.entry
        self.events,self.calls,self.checking,self.returns=[],{},False,0
        self.mu.hook_add(UC_HOOK_CODE,self.instruction);self.mu.hook_add(UC_HOOK_MEM_WRITE,self.write)

    def call(self,entry,args):
        """현재 raw를 유지하고 하나의 실제 호출이 정상 반환했는지 검사한다."""
        saved={UC_X86_REG_EBX:0x13579,UC_X86_REG_ESI:0x24680,UC_X86_REG_EDI:0x56789,UC_X86_REG_EBP:0x67890}
        # 각 원본 호출에서 비휘발 레지스터를 독립 초기화한다.
        for register,value in saved.items():self.mu.reg_write(register,value)
        self.mu.reg_write(UC_X86_REG_ECX,RAW);self.mu.reg_write(UC_X86_REG_ESP,STACK);self.mu.mem_write(STACK,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.checking=True;self.mu.emu_start(entry,STOP,count=10000);self.checking=False
        assert self.mu.reg_read(UC_X86_REG_EIP)==STOP and self.mu.reg_read(UC_X86_REG_ESP)==STACK+4*(len(args)+1)
        assert all(self.mu.reg_read(register)==value for register,value in saved.items())
        self.returns+=1;return self.mu.reg_read(UC_X86_REG_EAX)

    def run(self,frame,word):
        """같은 실제 슬롯의 기본 반환값과 추가Draw 순서를 따로 보존한다."""
        raw=bytearray([0xa5]*50);struct.pack_into('<I',raw,0,self.table);raw[10]=self.number;struct.pack_into('<H',raw,12,word);struct.pack_into('<I',raw,36,frame)
        self.mu.mem_write(RAW,bytes(raw));base=self.call(0x4acfe0,());self.events=[];self.call(self.entry,(311,233));assert bytes(self.mu.mem_read(RAW,50))==raw
        return str(base),','.join(self.events) or '-',raw.hex()


def main():
    """현재 원본 해시 검증 또는 새 C++ 전의 실제264입력을 동결한다."""
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    if args.verify:
        report=json.loads(REPORT.read_text('utf-8'))
        for path,wanted in report['sha256'].items():assert digest(ROOT/path)==wanted,path
        assert len([row for row in FIXTURE.read_text('utf-8').splitlines() if row and not row.startswith('#')])==report['rows']
        print('템플 원본264입력/현재 해시 일치');return
    archive=TaffArchive(ROOT/'originals/netstorm.tarc');rows=[];reports={}
    # 실제 .type의 모든 정상 프레임과 원본 WORD 상태 네 가지를 관찰한다.
    for name in SPECS:
        text=source(ROOT/'originals',archive,'d/'+name+'.type').decode('cp1252');clusters=read_clusters(text);codes=''.join(cluster[0][0] for cluster in clusters);base=next((index for index,cluster in enumerate(clusters) if 'baseframe' in cluster[1].lower()),-1);oracle=Oracle(name,codes,base)
        for frame in range(len(codes)):
            for word in (0,1,2,65535):rows.append('\t'.join((name,str(frame),str(word),*oracle.run(frame,word))))
        reports[name]=dict(type=SPECS[name][0],table=hex(oracle.table),draw=hex(oracle.entry),baseFrame=base,codes=codes,rows=len(codes)*4,returns=oracle.returns,instructions=sum(oracle.calls.values()))
    FIXTURE.write_text('# 실제10.78 템플 기본 반환·전용Draw. 그림/소리 제출은 관찰 경계이며 픽셀 출력은 별도다.\n'+'\n'.join(rows)+'\n','utf-8',newline='\n')
    paths=['originals/Netstorm.exe','originals/netstorm.tarc','extracted/refined/originals/functions.tsv','tools/decomp_rain_draw.py','tools/decomp_temple_draw.py',FIXTURE.relative_to(ROOT).as_posix()]
    report=dict(unit=189,date='2026-10-18',host=socket.gethostname(),lastDecompileHost='HJOW-Athlon',lastDecompileDate='2026-10-16',rows=len(rows),normalReturns=sum(row['returns'] for row in reports.values()),instructions=sum(row['instructions'] for row in reports.values()),editions={'10.78':reports,'10.72':'이번 전용 경로 미검증. 기존 레인232원문은 보존한다.'},boundaries='실제 MainFrame/전용Draw/TYPE/방향 helper. 그림과 음향 제출만 관찰 경계이며 SpriteDraw 수집/픽셀/실제 음향·미션 완주 증거는 아니다.',sha256={path:digest(ROOT/path) for path in paths})
    REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8',newline='\n');print('템플',len(rows),'입력',report['normalReturns'],'정상 반환',report['instructions'],'명령')


if __name__=='__main__':main()
