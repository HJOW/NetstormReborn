"""그라비테이션 정기의 진행/역방향/예약·삭제를 보호 PE 명령에서 독립 관찰한다."""
from pathlib import Path
import hashlib
import itertools
import json
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  #읽기 전용 원본과 새 독립 기대값의 루트다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW

POOL,TYPE,STACK,STOP,VTABLE,HEAP=0x2000000,0x2100000,0x3000000,0xf00000,0xf01000,0x2101000  #실제 프로세스/장치와 분리한 입력 메모리다.

def sample(pe,total,count,frame,payload):
    """4ac550/할당/가상 장치만 경계로 두고 정기/기본 반환은 실제 명령을 실행한다."""
    image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase;emu=Uc(UC_ARCH_X86,UC_MODE_32)
    emu.mem_map(base,(len(image)+4095)&~4095);emu.mem_write(base,image)
    emu.mem_map(0,0x1000);emu.mem_map(POOL,0x10000);emu.mem_map(TYPE,0x10000);emu.mem_map(STACK,0x10000);emu.mem_map(STOP,0x3000)
    parent=POOL+60*50;trace=[]
    def word(address,value=None):
        """명시 DWORD 비트만 읽거나 쓴다."""
        if value is None:return int.from_bytes(emu.mem_read(address,4),'little')
        emu.mem_write(address,struct.pack('<I',value&0xffffffff))
    def ret(value=None,pop=0):
        """실제 하위의 스택 인자 폭으로 정상 반환한다."""
        esp=emu.reg_read(UC_X86_REG_ESP);dest=word(esp)
        if value is not None:emu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        emu.reg_write(UC_X86_REG_ESP,esp+4+pop);emu.reg_write(UC_X86_REG_EIP,dest)
    def hook(uc,address,size,unused):
        """원문이 선택한 효과만 관찰하며 프레임/조건·반환은 계산하지 않는다."""
        esp=uc.reg_read(UC_X86_REG_ESP)
        if address==0x4ac550:ret(TYPE)
        elif address==0x4e4391:assert word(esp+4)==40;ret(HEAP)
        elif address==0x496e80:
            assert uc.reg_read(UC_X86_REG_ECX)==HEAP and [word(esp+4+i*4) for i in range(3)]==[2,60,0x3d4ccccd]
            trace.append('N');ret(HEAP,pop=12)
        elif address==VTABLE+0x200:assert word(esp+4)==0x100048;trace.append('D');ret(pop=4)
        elif address==0x4acee0:
            assert uc.reg_read(UC_X86_REG_ECX)==parent and word(esp+8)==0
            word(parent+36,word(esp+4));trace.append('S');ret(pop=8)
        elif address==VTABLE+0x210:trace.append('R');ret()
    word(0x5c8464,POOL);word(parent,VTABLE);word(parent+36,frame);word(TYPE+0x170,total)
    word(VTABLE+0x10,VTABLE+0x200);word(VTABLE+0x88,VTABLE+0x210)
    emu.mem_write(STOP,b'\xd9\x1d'+struct.pack('<I',HEAP+0x200))  #원본 ST0 반환을 float 비트로 저장하고 즉시 멈춘다.
    esp=STACK+0x8000;word(esp,STOP);word(esp+4,0x12345678);word(esp+8,count);word(esp+12,payload)
    emu.reg_write(UC_X86_REG_FPCW,0x027f);emu.reg_write(UC_X86_REG_ESP,esp);emu.reg_write(UC_X86_REG_ECX,parent);emu.hook_add(UC_HOOK_CODE,hook)
    emu.emu_start(0x41f760,STOP+6,count=5000);assert emu.reg_read(UC_X86_REG_EIP)==STOP+6
    return [str(word(parent+36)),str(word(HEAP+0x200)),''.join(trace)]

def main():
    """signed 횟수·프레임 감김·NaN의 독립 결과를 새 fixture로 보존한다."""
    out=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'extracted/cpp-resume-1229';out.mkdir(parents=True,exist_ok=True)
    pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
    #행렬은 입력 조합만 생성하며 예상 프레임/반환을 계산하지 않는다.
    inputs=[(*row,0x3e800000) for row in itertools.product((0,1,2,5,8),(0,2,5,0x7fffffff,0xffffffff,0x80000000),(0,1,7,0xffffffff))]
    inputs += [(5,0,1,0x7fc12345),(5,8,0,0x7fc12345)]
    rows=[]
    for row in inputs:  #기대 분기/프레임을 Python으로 계산하지 않고 실제 PE의 관찰만 저장한다.
        rows.append([*map(str,row),*sample(pe,*row)])
    fixture=ROOT/'cpppj/tests/fixtures/bomb-gravity-regular-x86.tsv';fixture.write_text('# total count frame payload expectedFrame result trace\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(passed=True,cases=len(rows),fixture=fixture.relative_to(ROOT).as_posix(),fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),originalPeSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),emulatedFunctions=['0041f760','00496e70'],gameExecutions=0,multiplayerExecutions=0)
    (out/'oracle.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report))

if __name__=='__main__':
    main()
