"""일반 Sprite4791b0/4794b0과 기본 Form finder를 보호 PE의 격리 명령으로 관찰한다."""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]  #원본/CD와 기존 fixture를 수정하지 않는다.
sys.path.insert(0,str(ROOT/'extracted/oracle-python'))
import pefile
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP

POOL,STACK,STOP,TYPE,GUMP,VT=0x2000000,0x3000000,0xf00000,0x2100000,0x2200000,0x2201000  #실제 프로세스/장치와 분리한 명시 주소다.

def sample(pe,mode,left,top,right,bottom,high,low,pattern,mutate,excluded,end,current,divisor,payload):
    """부모·현재 프레임 개수·Form finder는 원문 전체를 실행하고 화면/Gump 경계만 기록한다."""
    image=pe.get_memory_mapped_image();base=pe.OPTIONAL_HEADER.ImageBase;emu=Uc(UC_ARCH_X86,UC_MODE_32);emu.mem_map(base,(len(image)+4095)&~4095);emu.mem_write(base,image)
    for address,size in [(POOL,0x20000),(STACK,0x10000),(STOP,0x1000),(TYPE,0x1000),(GUMP,0x2000)]:  #겹치지 않는 입력/스택/장치 응답만 확보한다.
        emu.mem_map(address,size)
    trace=[];first=True;target=POOL+50*50
    def put(address,value,width=4):
        """원문 물리 폭의 낮은 비트만 쓴다."""
        emu.mem_write(address,(value&((1<<(8*width))-1)).to_bytes(width,'little'))
    def read(address,width=4):
        """현재 비정렬 little endian 필드를 읽는다."""
        return int.from_bytes(emu.mem_read(address,width),'little')
    def signed(value):
        """DWORD를 signed32 로그 인자로 해석한다."""
        return value if value<0x80000000 else value-0x100000000
    def ret(value=None,pop=0):
        """선택 장치의 cdecl/thiscall 반환 스택을 정확히 복구한다."""
        esp=emu.reg_read(UC_X86_REG_ESP);dest=read(esp)
        if value is not None:emu.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        emu.reg_write(UC_X86_REG_ESP,esp+4+pop);emu.reg_write(UC_X86_REG_EIP,dest)
    def hook(uc,address,size,unused):
        """원문 분기/좌표를 재계산하지 않고 큰 장치 인자와 명시 효과만 관찰한다."""
        nonlocal first
        esp=uc.reg_read(UC_X86_REG_ESP)
        if address==0x498ff0:
            trace.append('B');assert read(esp+8)==161 and read(esp+12)==(current&0xffffffff)
            for index,value in enumerate((left,top,right,bottom)):  #현재 bounds 경계가 반환하는 네 DWORD 입력만 공급한다.
                put(read(esp+4)+index*4,value)
            if mutate==1:put(target+26,2)
            ret()
        elif address==0x499310:
            assert read(esp+20)==0;trace.append('D:'+':'.join(str(signed(read(esp+4+i*4))) for i in range(4)))
            if mutate==2:put(target+35,0x89,1);put(target+6,61,2)
            ret()
        elif address==0x463ff0:
            ident=read(esp+4);trace.append(f'Q:{ident}');ret(0 if ident%2 else GUMP+(ident-1000)*8)
        elif address==STOP+0x100:
            ident=(uc.reg_read(UC_X86_REG_ECX)-GUMP)//8+1000;assert read(esp+4)==0;trace.append(f'G:{ident}:0')
            if first and mutate==3:put(POOL+60*50+4,0,2);put(POOL+61*50+10,69,1)
            if first and mutate==4:put(0x541074,3);put(0x54116c,70)
            first=False;ret(pop=4)
        elif address==0x4e0620:trace.append(f'A:{read(esp+12)}');ret()
        elif address==0x49a840:assert read(esp+4)==161;ret(TYPE)
        elif address==0x4acee0:
            assert uc.reg_read(UC_X86_REG_ECX)==target and read(esp+8)==0;trace.append(f'S:{signed(read(esp+4))}');put(target+36,read(esp+4));ret(pop=8)
    put(0x5c8464,POOL);put(0x5e4794,1);put(0x594fc8,0);put(0x540bc4,0);put(0x541074,excluded);put(0x54116c,end);put(TYPE+0x114,divisor)
    put(target,0x50cc08);put(target+10,161,1);put(target+11,3,1);put(target+26,high);put(target+8,low,2);put(target+35,0x77,1);put(target+36,current)
    kinds=(2,3,6,9,10,46,69,70);put(target+6,60 if pattern else 0,2)
    for index,kind in enumerate(kinds):  #다양한 상태의 Form과 자산을 포함하되 실제 finder가 필터하게 한다.
        sid=60+index;put(POOL+sid*50+10,kind,1);put(POOL+sid*50+11,(0,1,2,8,3,255,4,0)[index],1);put(POOL+sid*50+4,sid+1 if index<7 else 0,2);put(POOL+sid*50+18,1000+sid)
        put(GUMP+sid*8,VT)
    put(VT+12,STOP+0x100);emu.hook_add(UC_HOOK_CODE,hook)
    retstub=STOP+0x500;out=STOP+0x800;code=b'\xd9\x1d'+struct.pack('<I',out)+b'\xe9'+struct.pack('<i',STOP-(retstub+11));emu.mem_write(retstub,code)
    esp=STACK+0x8000;put(esp,STOP if mode=='D' else retstub);put(esp+4,0x12345678);put(esp+8,0x87654321);put(esp+12,payload);emu.reg_write(UC_X86_REG_ESP,esp);emu.reg_write(UC_X86_REG_ECX,target);error=False
    try:emu.emu_start(0x4791b0 if mode=='D' else 0x4794b0,STOP,count=50000)
    except UcError:
        if mode!='R' or emu.reg_read(UC_X86_REG_EIP)!=0x4794c4:raise
        error=True
    if not error:assert emu.reg_read(UC_X86_REG_EIP)==STOP
    return [str(int(error)),str(read(target+35,1)),str(read(target+36)),str(read(out)) if mode=='R' and not error else '-',';'.join(trace) or '-']

def main():
    """일반 Sprite 전용 입력만 관찰하고 메타/MP/게임은 실행하지 않는다."""
    out=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'extracted/cpp-resume-1233';out.mkdir(parents=True,exist_ok=True);pe=pefile.PE(str(ROOT/'originals/Netstorm.exe'));inputs=[]
    rects=[(10,20,30,40),(-11,-21,2,4),(0,0,1,1),(100,200,99,198),(2147483637,-2147483640,2147483647,-2147483630),(-50000,-30000,50000,30000)]
    scales=[(0,0),(0,65535),(1,0),(1,1),(2,0),(0x3fff,65535),(0x4000,0),(0x80000002,17)]
    for rect in rects:  #일반/음수/홀수 폭/역방향 사각형과 DWORD 경계를 명시 입력으로 준다.
        for high,low in scales:  #낮은 WORD와 상위14bit의 배율 경계를 원문에 입력한다.
            inputs.append(('D',*rect,high,low,1,0,1,47,0,8,0))
    for pattern in (0,1):  #빈/혼합 Form 체인의 두 입력이다.
        for mutate in range(5):  #bounds/dirty/Redraw 뒤 현재 자료 재조회와 선저장next를 관찰한다.
            for excluded,end in [(1,47),(0x10002,70),(3,0x80000000)]:  #전체DWORD 비교와 signed process 상한을 교차한다.
                inputs.append(('D',10,20,31,45,1,0,pattern,mutate,excluded,end,7,8,0))
    for current in (0,1,7,0xfffffffe,0xffffffff,0x7fffffff,0x80000000):  #프레임 감김/음수 나머지와 IDIV overflow를 교차한다.
        for divisor in (1,3,8,0,-1,-3,-2147483648):  #0분모와 음수 제수도 원문에서 직접 관찰한다.
            for payload in (0x3e000000,0x80000000,0x7fc12345):  #일반/음수0/quiet NaN의 실제 반환 저장 비트다.
                inputs.append(('R',0,0,0,0,0,0,0,0,1,47,current,divisor,payload))
    rows=[[*map(str,args),*sample(pe,*args)] for args in inputs]  #기대값은 제품 알고리즘 대신 원문 명령의 관찰만 쓴다.
    fixture=ROOT/'cpppj/tests/fixtures/sprite-refresh-x86.tsv';fixture.write_text('# mode left top right bottom high low pattern mutate excluded end current divisor payload error byte35 frame resultPayload trace\n'+'\n'.join('\t'.join(row) for row in rows)+'\n',encoding='utf-8',newline='\n')
    report=dict(passed=True,cases=len(rows),displayCases=sum(row[0]=='D' for row in rows),regularCases=sum(row[0]=='R' for row in rows),fixture=str(fixture.relative_to(ROOT)),fixtureSha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),originalPeSha256=hashlib.sha256((ROOT/'originals/Netstorm.exe').read_bytes()).hexdigest(),originalExecutions=0,multiplayerExecutions=0,dedicatedAirExecutions=0,emulatedFunctions=['004791b0','004794b0','004ad6c0','004b22d0','004b1b80','004b1bd0','004b1bf0'],deviceBoundaries=['498ff0 supplied bounds','499310 actual dirty arguments','FindGump and virtual Redraw(false)','Frame.Set','TYPE lookup'])
    (out/'oracle.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(report))

if __name__=='__main__':
    main()
