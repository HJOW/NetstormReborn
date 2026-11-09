"""보호 PE의 하드너·프레임 후보 검색·게임 난수를 격리 x86 메모리에서 관찰한다."""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  # 읽기 전용 원본/새 근거 자료의 저장소 루트다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL,STACK,STOP,TYPE,CODES,HEAP=0x2000000,0x3000000,0xf00000,0x2100000,0x2101000,0x2104000  # 실제 게임/OS와 분리한 명시 입력 주소다.

def sample(pe,mode,genus,seed,qa,current,codes):
    """실제 부모 함수와 후보 검색·난수는 원본 명령을 실행하고 장치 경계만 기록한다."""
    image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase
    emu=Uc(UC_ARCH_X86,UC_MODE_32);emu.mem_map(base,(len(image)+4095)&~4095);emu.mem_write(base,image)
    emu.mem_map(0,0x1000);emu.mem_map(POOL,0x20000);emu.mem_map(TYPE,0x10000);emu.mem_map(STACK,0x10000);emu.mem_map(STOP,0x1000)
    parent,target=POOL+50*50,POOL+60*50;trace=[];allocations=[]
    def word(address,value=None):
        """비정렬 DWORD의 실제 저장 비트만 읽거나 쓴다."""
        if value is None:return int.from_bytes(emu.mem_read(address,4),'little')
        emu.mem_write(address,struct.pack('<I',value&0xffffffff))
    def ret(value=None,pop=0):
        """각 실제 하위의 호출 규약에 맞춰 반환 주소/스택을 복구한다."""
        esp=emu.reg_read(UC_X86_REG_ESP);dest=word(esp)
        if value is not None:emu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        emu.reg_write(UC_X86_REG_ESP,esp+4+pop);emu.reg_write(UC_X86_REG_EIP,dest)
    def hook(uc,address,size,unused):
        """자산 조회/할당/장치만 가로채며 후보 선택과 난수 결과를 계산하지 않는다."""
        esp=uc.reg_read(UC_X86_REG_ESP)
        if address==0x4ac200:ret(genus)
        elif address in (0x4ac550,0x49a840):ret(TYPE)
        elif address==0x4e4391:
            assert word(esp+4)==1024;allocations.append(HEAP);ret(HEAP)
        elif address==0x4e4354:
            assert word(esp+4)==allocations.pop();ret()
        elif address==0x4acee0:
            assert not allocations and uc.reg_read(UC_X86_REG_ECX)==target and word(esp+8)==0
            trace.append('S');word(target+36,word(esp+4));ret(pop=8)
        elif address==0x45a6c0:
            assert not allocations and [word(esp+4+i*4) for i in range(3)]==[60,1,2]
            trace.append('F');ret()
    word(0x5c8464,POOL);word(0x532710,seed);word(TYPE+0x114,len(codes));word(TYPE+0x124,CODES)
    emu.mem_write(CODES,bytes(value for code in codes for value in code));emu.mem_write(target+10,b'\x5e');word(target+36,current)
    emu.mem_write(parent+12,struct.pack('<H',60 if qa else 0));emu.hook_add(UC_HOOK_CODE,hook)
    esp=STACK+0x8000;word(esp,STOP);word(esp+4,60);emu.reg_write(UC_X86_REG_ESP,esp);emu.reg_write(UC_X86_REG_ECX,parent)
    emu.emu_start(0x41fe90 if mode=='E' else 0x41feb0,STOP,count=50000)
    assert emu.reg_read(UC_X86_REG_EIP)==STOP and not allocations
    return [str(emu.reg_read(UC_X86_REG_EAX)) if mode=='E' else '-',str(word(target+36)),str(word(0x532710)),''.join(trace) or '-']

def main():
    """새 독립 입력/출력을 저장하고 게임·MP 실행 없이 정적 근거를 남긴다."""
    out=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'extracted/cpp-resume-1227'
    out.mkdir(parents=True,exist_ok=True);pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'))
    inputs=[];normal=[(65,80,1,0),(65,80,2,64),(66,80,3,64),(65,65,4,64),(65,80,5,192),(65,80,6,32)]
    for genus in (0,4,0x200000,0xffffffff,0x80000000):  # 자격 반환의 전체 비트와 무관 비트를 교차한다.
        inputs.append(('E',genus,55,0,0,normal))
    for codes in (normal,[(65,80,1,0),(65,65,2,64)],[(128,80,1,0),(128,80,2,64)]):  # 일치/마스크 실패/음수 방향을 실제 PE에 준다.
        for seed in (0,1,55,0xffffffff):  #0시드 대체와 1후보여도 난수 소비를 관찰한다.
            inputs.append(('A',4,seed,0,0,codes))
    codes=[(65,80,index&255,64) for index in range(300)]
    for seed in (0,0x12345678,0xffffffff):  #256개 후보 제한과 표의 순서를 독립 관찰한다.
        inputs.append(('A',4,seed,0,0,codes))
    inputs.append(('A',4,55,1,0xffffffff,normal))  # QA 대상이면 잘못된 현재 프레임도 조회 전에 반환한다.
    rows=[]
    for mode,genus,seed,qa,current,codes in inputs:  # 기대값을 C++ 알고리즘으로 계산하지 않고 원본 관찰만 저장한다.
        result=sample(pe,mode,genus,seed,qa,current,codes)
        rows.append([mode,str(genus),str(seed),str(qa),str(current),bytes(v for c in codes for v in c).hex(),*result])
    fixture=ROOT/'cpppj/tests/fixtures/bomb-hardener-x86.tsv'
    fixture.write_text('# mode genus seed qa current codes result frame state effects\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8')
    report=dict(passed=True,cases=len(rows),fixture=str(fixture.relative_to(ROOT)),fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),originalPeSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),originalExecutions=0,multiplayerExecutions=0,emulatedFunctions=['0041fe90','0041feb0','004ad680','0049aa90','004558c0'],deviceBoundaries=['TYPE lookup','operator_new/free','SquidFrame.Set','RawFlare.Spawn'])
    (out/'oracle.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report))

if __name__=='__main__':
    main()
